dateutil_tz=True

import sys
import subprocess
import threading
from threading import Timer
import time
from collections import deque
import datetime
try:
	import dateutil.tz
except ImportError:
	print "no timezone support, time will be expressed only in local time"
	dateutil_tz=False

import os
import os.path
import json
import re
import requests
import libSMS

#------------------------------------------------------------
#low-level data prefix
#------------------------------------------------------------
LL_PREFIX_1='\xFF'
LL_PREFIX_LORA='\xFE'

#list here other radio type
LORA_RADIO=1

#will be dynamically determined according to the second data prefix
radio_type=LORA_RADIO

#------------------------------------------------------------
#LoRa header packet information
#------------------------------------------------------------
PKT_TYPE_DATA=0x10
PKT_TYPE_ACK=0x20

#------------------------------------------------------------
#last pkt information
#------------------------------------------------------------
pdata="0,0,0,0,0,0,0,0"
rdata="0,0,0,0"
tdata="N/A"
tmst="N/A"

short_info_1="N/A"
short_info_2="N/A"
short_info_3="N/A"

#use same variable name than Semtech Packet Forwarder
rxnb=0 #Number of radio packets received (unsigned integer)
rxok=0 #Number of radio packets received with a valid PHY CRC
rxfw=0 #Number of radio packets forwarded (unsigned integer)
ackr=0 #Percentage of upstream datagrams that were acknowledged
dwnb=0 #Number of downlink datagrams received (unsigned integer)
txnb=0 #Number of packets emitted (unsigned integer)

t_rxnb=0 #this one will not be reset

dst=0
ptype=0
ptypestr="N/A"
src=0
seq=0
datalen=0
SNR=0
RSSI=0
bw=0
cr=0
sf=0
rfq=0

_hasRadioData=False
#------------------------------------------------------------
#to display non printable characters
replchars = re.compile(r'[\x00-\x1f]')

def replchars_to_hex(match):
	return r'\x{0:02x}'.format(ord(match.group()))

#------------------------------------------------------------
#open gateway_conf.json file 
#------------------------------------------------------------
f = open(os.path.expanduser("gateway_conf.json"),"r")
lines = f.readlines()
f.close()
array = ""
#get all the lines in a string
for line in lines :
	array += line

#change it into a python array
json_array = json.loads(array)

#------------------------------------------------------------
#get gateway ID
#------------------------------------------------------------
_gwid = json_array["gateway_conf"]["gateway_ID"]

#------------------------------------------------------------
#for doing periodic status/tasks
#------------------------------------------------------------
try:
	_gw_status = json_array["gateway_conf"]["status"]
except KeyError:
	_gw_status = 0		

if _gw_status < 0:
	_gw_status = 0 		
					
def status_target():
	while True:
		print datetime.datetime.now()
		print 'post status: gw ON'
		print 'post status: executing periodic tasks'
		sys.stdout.flush()

		global rxnb
		global rxok
		global rxfw
		global ackr
		global dwnb
		global txnb
		
		#build the stat string: 
		stats_str=""
		stats_str+=str(rxnb)+","
		stats_str+=str(rxok)+","
		stats_str+=str(rxfw)+","
		stats_str+=str(ackr)+","
		stats_str+=str(dwnb)+","
		stats_str+=str(txnb)
						
		try:
			cmd_arg='python post_status_processing_gw.py'+" \""+stats_str+"\""+" 2> /dev/null" 
			os.system(cmd_arg)
			
			#reset statistics for next period
			rxnb=0 #Number of radio packets received (unsigned integer)
			rxok=0 #Number of radio packets received with a valid PHY CRC
			rxfw=0 #Number of radio packets forwarded (unsigned integer)
			ackr=0 #Percentage of upstream datagrams that were acknowledged
			dwnb=0 #Number of downlink datagrams received (unsigned integer)
			txnb=0 #Number of packets emitted (unsigned integer)		
				
		except:
			print "Error when executing post_status_processing_gw.py"			
		global _gw_status
		time.sleep(_gw_status)

#-------------------------------------------------------------------------------------------
#for doing fast statistics or display, periodicity is fixed by ["status_conf"]["fast_stats"]
#-------------------------------------------------------------------------------------------
		
try:
	_fast_stats = json_array["status_conf"]["fast_stats"]
except KeyError:
	_fast_stats = 0	
	
if _fast_stats < 0:
	_fast_stats = 0	
	
def statistics_target():
	while True:
		#avoid any print output here to not overload log file		
		try:
			cmd_arg='python sensors_in_raspi/stats.py'+" \""+short_info_1+"\""+" \""+short_info_2+"\""+" \""+short_info_3+"\""+" 2> /dev/null" 
			os.system(cmd_arg)
		except:
			print "Error when executing sensors_in_raspi/stats.py"
		global _fast_stats					
		time.sleep(_fast_stats)
		
#------------------------------------------------------------
#check Internet connectivity
#------------------------------------------------------------

def checkNet():
    print "post_processing_gw.py checks Internet connecitivity with www.google.com"
    try:
        response = requests.get("http://www.google.com")
        print "response code: " + str(response.status_code)
        return True
    except requests.ConnectionError:
        print "No Internet"
        return False

#------------------------------------------------------------
#for handling images
#------------------------------------------------------------
#list of active nodes
nodeL = deque([])
#association to get the file handler
fileH = {}
#association to get the image filename
imageFilenameA = {}
#association to get the image SN
imgsnA= {} 
#association to get the image quality factor
qualityA = {}
#association to get the cam id
camidA = {}
#global image seq number
imgSN=0

def image_timeout():
	#get the node which timer has expired first
	#i.e. the one that received image packet earlier
	node_id=nodeL.popleft()
	print "close image file for node %d" % node_id
	f=fileH[node_id]
	f.close()
	del fileH[node_id]
	print "decoding image "+os.path.expanduser(imageFilenameA[node_id])
	sys.stdout.flush()
	
	cmd = '/home/pi/lora_gateway/ucam-images/decode_to_bmp -received '+os.path.expanduser(imageFilenameA[node_id])+\
		' -SN '+str(imgsnA[node_id])+\
		' -src '+str(node_id)+\
		' -camid '+str(camidA[node_id])+\
		' -Q '+str(qualityA[node_id])+\
		' -vflip'+\
		' /home/pi/lora_gateway/ucam-images/128x128-test.bmp'
	
	print "decoding with command"
	print cmd
	args = cmd.split()

	out = 'error'
		
	try:
		out = subprocess.check_output(args, stderr=None, shell=False)
		
		if (out=='error'):
			print "decoding error"
		else:
        	# leave enough time for the decoding program to terminate
			time.sleep(3)
			out = out.replace('\r','')
			out = out.replace('\n','')
			print "producing file " + out
			print "creating if needed the uploads/node_"+str(node_id)+" folder"
			try:
				os.mkdir(os.path.expanduser(_web_folder_path+"images/uploads/node_"+str(node_id)))
			except OSError:
				print "folder already exist"				 	 
			print "moving decoded image file into " + os.path.expanduser(_web_folder_path+"images/uploads/node_"+str(node_id))
			os.rename(os.path.expanduser("./"+out), os.path.expanduser(_web_folder_path+"images/uploads/node_"+str(node_id)+"/"+out))
			print "done"	

	except subprocess.CalledProcessError:
		print "launching image decoding failed!"
		
	sys.stdout.flush()
	
#------------------------------------------------------------
#for managing the input data when we can have aes encryption
#------------------------------------------------------------
_linebuf="the line buffer"
_linebuf_idx=0
_has_linebuf=0

def getSingleChar():
	global _has_linebuf
	#if we have a valid _linebuf then read from _linebuf
	if _has_linebuf==1:
		global _linebuf_idx
		global _linebuf
		if _linebuf_idx < len(_linebuf):
			_linebuf_idx = _linebuf_idx + 1
			return _linebuf[_linebuf_idx-1]
		else:
			#no more character from _linebuf, so read from stdin
			_has_linebuf = 0
			return sys.stdin.read(1)
	else:
		return sys.stdin.read(1)	
	
def getAllLine():
	global _linebuf_idx
	p=_linebuf_idx
	_linebuf_idx = 0
	global _has_linebuf
	_has_linebuf = 0	
	global _linebuf
	#return the remaining of the string and clear the _linebuf
	return _linebuf[p:]	
	
def fillLinebuf(n):
	global _linebuf_idx
	_linebuf_idx = 0
	global _has_linebuf
	_has_linebuf = 1
	global _linebuf
	#fill in our _linebuf from stdin
	_linebuf=sys.stdin.read(n)

#////////////////////////////////////////////////////////////
# CHANGE HERE THE VARIOUS PATHS FOR YOUR LOG FILES
#////////////////////////////////////////////////////////////
_folder_path = "/home/pi/Dropbox/LoRa-test/"
_web_folder_path = "/var/www/html/"
_telemetrylog_filename = _folder_path+"telemetry_"+str(_gwid)+".log"
_imagelog_filename = _folder_path+"image_"+str(_gwid)+".log"

# END
#////////////////////////////////////////////////////////////
#------------------------------------------------------------
#open clouds.json file to get enabled clouds
#------------------------------------------------------------
from clouds_parser import retrieve_enabled_clouds

#get a copy of the list of enabled clouds
_enabled_clouds=retrieve_enabled_clouds()

print "post_processing_gw.py got cloud list: "
print _enabled_clouds

#------------------------------------------------------------
#start various threads
#------------------------------------------------------------
#status feature
if (_gw_status):
	print "Starting thread to perform periodic gw status/tasks"
	sys.stdout.flush()
	t_status = threading.Thread(target=status_target)
	t_status.daemon = True
	t_status.start()
	time.sleep(1)	
	
#fast statistics tasks
if (_fast_stats):
	print "Starting thread to perform fast statistics tasks"
	sys.stdout.flush()
	t_status = threading.Thread(target=statistics_target)
	t_status.daemon = True
	t_status.start()
	time.sleep(1)	

print ''	
print "Current working directory: "+os.getcwd()

#------------------------------------------------------------
#main loop
#------------------------------------------------------------

while True:

	sys.stdout.flush()
	ch = getSingleChar()

	if (ch=='^'):
		now = datetime.datetime.now()
		ch=sys.stdin.read(1)
		
		if (ch=='p'):		
			pdata = sys.stdin.readline()
			print now.isoformat()
			print "rcv ctrl pkt info (^p): "+pdata,
			arr = map(int,pdata.split(','))
			print "splitted in: ",
			print arr
			dst=arr[0]
			ptype=arr[1]
			ptypestr="N/A"
			if ((ptype & 0xF0)==PKT_TYPE_DATA):
				ptypestr="DATA"														
			if ((ptype & 0xF0)==PKT_TYPE_ACK):
				ptypestr="ACK"
			src=arr[2]
			seq=arr[3]
			datalen=arr[4]
			SNR=arr[5]
			RSSI=arr[6]

			info_str="(dst=%d type=0x%.2X(%s) src=%d seq=%d len=%d SNR=%d RSSI=%d)" % (dst,ptype,ptypestr,src,seq,datalen,SNR,RSSI)	
			print info_str

			rxnb=rxnb+1
			t_rxnb=t_rxnb+1
			rxok=rxnb
			
			short_info_1="src=%d seq=%d #pk=%d" % (src,seq,t_rxnb)
			short_info_2="SNR=%d RSSI=%d" % (SNR,RSSI)

		if (ch=='r'):		
			rdata = sys.stdin.readline()
			print "rcv ctrl radio info (^r): "+rdata,
			arr = map(int,rdata.split(','))
			print "splitted in: ",
			print arr
			bw=arr[0]
			cr=arr[1]
			sf=arr[2]
			rfq=arr[3]
			info_str="(BW=%d CR=%d SF=%d)" % (bw,cr,sf)
			print info_str

		if (ch=='t'):
			tdata = sys.stdin.readline()
			print "rcv timestamp (^t): "+tdata
			tdata = tdata.replace('\n','')
			
			tmst=tdata.count('*')
			
			if (tmst != 0):
				tdata_tmp=tdata.split('*')[0]
				tmst=tdata.split('*')[1]
				tmst='*'+tmst
				tdata=tdata_tmp
			else:
				tmst=''
			
			tdata = tdata.split('+')[0]
			short_info_3=tdata.split('+')[0]
			
			if (dateutil_tz==True):
				print "adding time zone info"
				#adding time zone info to tdata to get time zone information when uploading to clouds
				localdt = datetime.datetime.now(dateutil.tz.tzlocal())
				i=localdt.isoformat().find("+")
				#get only the timezone data and put it back to the original timestamp from low-level gateway
				tdata = tdata+localdt.isoformat()[i:]
				print "new rcv timestamp (^t): %s" % tdata
			else:
				tdata = tdata+"+00:00"
									
		if (ch=='l'):
			#TODO: LAS service	
			print "not implemented yet"
			
		if (ch=='$'):
			
			data = sys.stdin.readline()
			print data,
			
			#when the low-level gateway program reset the radio module then it is will send "^$Resetting the radio module"
			if 'Resetting' in data:
			
				if _use_mail_alert:
					print "post_processing_gw.py sends mail indicating that gateway has reset radio module..."
					if checkNet():
						send_alert_mail("Gateway "+_gwid+" has reset its radio module")
						print "Sending mail done"
				
				if _use_sms_alert:
					print "post_processing_gw.py sends SMS indicating that gateway has reset radio module..."
					success = libSMS.send_sms(sm, "Gateway "+_gwid+" has reset its radio module", contact_sms)
					if (success):
						print "Sending SMS done"
						
		continue

#------------------------------------------------------------
# '\' is reserved for message logging service
#------------------------------------------------------------

	if (ch=='\\' and _hasRadioData==True):
	
		_hasRadioData=False
		now = datetime.datetime.now()		
		ch=getSingleChar()			
				
		if (ch=='$'): #log in a file
			
			data = getAllLine()
			
			print "rcv msg to log (\$) in log file: "+data,
			f=open(os.path.expanduser(_telemetrylog_filename),"a")
			f.write(info_str+' ')	
			f.write(now.isoformat()+'> ')
			f.write(data)
			f.close()	

		#up dữ liệu lên đám mây
		elif (ch=='!'): 
			ldata = getAllLine()
			print "number of enabled clouds is %d" % len(_enabled_clouds)	
			for cloud_index in range(0,len(_enabled_clouds)):
				try:
					print "--> cloud[%d]" % cloud_index
					cloud_script=_enabled_clouds[cloud_index]
					print "uploading with "+cloud_script
					sys.stdout.flush()
					cmd_arg=cloud_script+" \""+ldata.replace('\n','').replace('\0','')+"\""+" \""+pdata.replace('\n','')+"\""+" \""+rdata.replace('\n','')+"\""+" \""+tdata.replace('\n','')+"\""+" \""+_gwid.replace('\n','')+"\""
				except UnicodeDecodeError, ude:
					print ude
				else:
					print cmd_arg
					sys.stdout.flush()
					try:
						os.system(cmd_arg)
					except:
						print "Error when uploading data to the cloud"	

			print "--> cloud end"			
														
		else:
			print "unrecognized data logging prefix: discard data"
			getAllLine() 

		continue
	
	#handle low-level gateway data
	if (ch == LL_PREFIX_1):
	
		print "got first framing byte"
		ch=getSingleChar()	
		
		#data from low-level LoRa gateway?		
		if (ch == LL_PREFIX_LORA):
			#the data prefix is inserted by the gateway
			#do not modify, unless you know what you are doing and that you modify lora_gateway (comment WITH_DATA_PREFIX)
			print "--> got LoRa data prefix"
			radio_type=LORA_RADIO
							
			_hasRadioData=True	

			#if we have raw output from gw, then try to determine which kind of packet it is	
			print "+++ rxlora[%d]. dst=%d type=0x%.2X src=%d seq=%d len=%d SNR=%d RSSIpkt=%d BW=%d CR=4/%d SF=%d" % (rfq,dst,ptype,src,seq,datalen,SNR,RSSI,bw,cr,sf)								
			#now we read datalen bytes in our line buffer
			fillLinebuf(datalen)					
			continue	
						
		if (ch >= '\x50' and ch <= '\x54'):
			print "--> got image packet"
			
			cam_id=ord(ch)-0x50;
			src_addr_msb = ord(getSingleChar())
			src_addr_lsb = ord(getSingleChar())
			src_addr = src_addr_msb*256+src_addr_lsb
					
			seq_num = ord(getSingleChar())
	
			Q = ord(getSingleChar())
	
			data_len = ord(getSingleChar())
	
			if (src_addr in nodeL):
				#already in list
				#get the file handler
				theFile=fileH[src_addr]
				#TODO
				#start some timer to remove the node from nodeL
			else:
				#new image packet from this node
				nodeL.append(src_addr)
				filename =(_folder_path+"images/ucam_%d-node_%.4d-cam_%d-Q%d.dat" % (imgSN,src_addr,cam_id,Q))
				print "first pkt from node %d" % src_addr
				print "creating file %s" % filename
				theFile=open(os.path.expanduser(filename),"w")
				#associates the file handler to this node
				fileH.update({src_addr:theFile})
				#and associates imageFilename, imagSN,Q and cam_id
				imageFilenameA.update({src_addr:filename})
				imgsnA.update({src_addr:imgSN})
				qualityA.update({src_addr:Q})
				camidA.update({src_addr:cam_id})
				imgSN=imgSN+1
				t = Timer(90, image_timeout)
				t.start()
				#log only the first packet and the filename
				f=open(os.path.expanduser(_imagelog_filename),"a")
				f.write(info_str+' ')	
				now = datetime.datetime.now()
				f.write(now.isoformat()+'> ')
				f.write(filename+'\n')
				f.close()				
							
			print "pkt %d from node %d data size is %d" % (seq_num,src_addr,data_len)
			print "write to file"
			
			theFile.write(format(data_len, '04X')+' ')
	
			for i in range(1, data_len):
				ch=getSingleChar()
				#sys.stdout.write(hex(ord(ch)))
				#sys.stdout.buffer.write(ch)
				print (hex(ord(ch))),
				theFile.write(format(ord(ch), '02X')+' ')
				
			print "End"
			sys.stdout.flush()
			theFile.flush()
			continue
			
	if (ch == '?'):
		sys.stdin.readline()
		continue
	
	sys.stdout.write(ch)
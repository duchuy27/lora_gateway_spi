#include <SX127XLT.h>                                          
#include "SX127X_lora_gateway.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef GETOPT_ISSUE
int getopt (int argc, char * const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
#endif
#include <getopt.h>
#include <termios.h> 
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#define PRINTLN                   printf("\n")
#define PRINT_CSTSTR(param)       printf(param)
#define PRINTLN_CSTSTR(param)			do {printf(param);printf("\n");} while(0)
#define PRINT_STR(fmt,param)      printf(fmt,param)
#define PRINTLN_STR(fmt,param)		{printf(fmt,param);printf("\n");}
#define PRINT_VALUE(fmt,param)    printf(fmt,param)
#define PRINTLN_VALUE(fmt,param)	do {printf(fmt,param);printf("\n");} while(0)
#define PRINT_HEX(fmt,param)      printf(fmt,param)
#define PRINTLN_HEX(fmt,param)		do {printf(fmt,param);printf("\n");} while(0)
#define FLUSHOUTPUT               fflush(stdout)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ADDITIONAL FEATURES
//cấu hình lại LoRa sau 100 pkt(gói) nhận được
#define PERIODIC_RESET100

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// FOR DOWNLINK FEATURES
//comment if you want to disable downlink at compilation
#define DOWNLINK
#if defined DOWNLINK

//use same definitions from LMIC, but in ms
enum { DELAY_JACC1       =  5000 }; // in millisecs
enum { DELAY_DNW1        =  1000 }; // in millisecs down window #1
enum { DELAY_EXTDNW2     =  1000 }; // in millisecs
enum { DELAY_JACC2       =  DELAY_JACC1+(int)DELAY_EXTDNW2 }; // in millisecs
enum { DELAY_DNW2        =  DELAY_DNW1 +(int)DELAY_EXTDNW2 }; // in millisecs down window #1

// need to give some time for the post-processing stage to generate a downlink.txt file if any
// for LoRaWAN, need to give some time to lorawan_downlink.py to get PULL_RESP and generate downlink.txt
// as the receive window 1 is at 1s after packet transmission at device side, it does not give much margin
enum { DELAY_DNWFILE     =  900 }; // in millisecs
enum { MARGIN_DNW        =  20 }; // in millisecs

#include "base64.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

using namespace rapidjson;
using namespace std;

bool enableDownlinkCheck=true;
bool checkForLateDownlinkRX2=false;
bool checkForLateDownlinkJACC1=false;
bool checkForLateDownlinkJACC2=false;

int xtoi(const char *hexstring);
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//BAND868 by default
#if not defined BAND433
#define BAND433
#endif 

///////////////////////////////////////////////////////////////////
// BAND433
#define LORAWAN_UPFQ 433175000
#define LORAWAN_D2FQ 434665000
#define LORAWAN_D2SF 12

#define MAX_NB_CHANNEL 4
#define STARTING_CHANNEL 0
#define ENDING_CHANNEL 3
uint32_t loraChannelArray[MAX_NB_CHANNEL]={CH_00_433,CH_01_433,CH_02_433,CH_03_433};                                

// will use 0xFF0xFE to prefix data received from LoRa, so that post-processing stage can differenciate
// data received from radio
#define WITH_DATA_PREFIX

#ifdef WITH_DATA_PREFIX
#define DATA_PREFIX_0 0xFF
#define DATA_PREFIX_1 0xFE
#endif

///////////////////////////////////////////////////////////////////
// CONFIGURATION VARIABLES
//
#define LORAWAN_MODE 11
uint8_t loraMode=-1;
const uint16_t MAX_TIMEOUT = 10000;	
uint16_t rcv_timeout=MAX_TIMEOUT;
int status_counter=0;
uint8_t SIFS_cad_number;
// set to 0 to disable carrier sense based on CAD
uint8_t cad_number=3;

uint16_t optBW=0; 
uint8_t optSF=0;
uint8_t optCR=0;
uint8_t optCH=0;
bool  optRAW=false;
double optFQ=-1.0;
uint8_t optSW=0x12;
bool  optHEX=false;
bool optIIQ=false;

///////////////////////////////////////////////////////////////////
// TIMER-RELATED VARIABLES
//
uint32_t rcv_time_ms;
uint32_t rcv_time_tmst;
char time_str[100];
///////////////////////////////////////////////////////////////////

//create a library class instance called LT
SX127XLT LT;

//create the buffer that received packets are copied into
uint8_t RXBUFFER[RXBUFFER_SIZE];

uint32_t RXpacketCount;
uint32_t RXpacketCountReset=0;
uint32_t errors;

//stores length of packet received
uint8_t RXPacketL;                     
//stores RSSI of received packet          
int8_t  PacketRSSI;      
//stores signal to noise ratio (SNR) of received packet                        
int8_t  PacketSNR;                               
                                   
///////////////////////////////////////////////////////////////////
void loraConfig() {

  // has customized LoRa settings   
  if (optBW!=0 || optCR!=0 || optSF!=0) {
		if (optCR!=0) {
			PRINT_CSTSTR("^$LoRa CR4/");
			if (optCR<5)
				optCR=optCR+4;
			PRINTLN_VALUE("%d", optCR);    
			if (optCR==5) CodeRate = LORA_CR_4_5;
			else if (optCR==6) CodeRate = LORA_CR_4_6;		
			else if (optCR==7) CodeRate = LORA_CR_4_7;				
			else if (optCR==8) CodeRate = LORA_CR_4_8;
			else {
				PRINT_CSTSTR("^$Unknown LoRa CR, taking CR4/5\n");
				CodeRate = LORA_CR_4_5;
			}	
		}
			
		if (optSF!=0) {
			PRINT_CSTSTR("^$LoRa SF ");
			PRINTLN_VALUE("%d", optSF);		
			if (optSF==6)
				SpreadingFactor = LORA_SF6;
			else if (optSF==7)
				SpreadingFactor = LORA_SF7;
			else if (optSF==8)
				SpreadingFactor = LORA_SF8;
			else if (optSF==9)
				SpreadingFactor = LORA_SF9;
			else if (optSF==10)
				SpreadingFactor = LORA_SF10;
			else if (optSF==11)
				SpreadingFactor = LORA_SF11;
			else if (optSF==12)
				SpreadingFactor = LORA_SF12;				
			else {
				PRINT_CSTSTR("^$Unknown LoRa SF, taking SF12\n");
				SpreadingFactor = LORA_SF12;
			}																	
		}

		if (optBW!=0) {    
			PRINT_CSTSTR("^$LoRa BW ");
			PRINTLN_VALUE("%ld", optBW*1000);			
			if (optBW==500)
				Bandwidth = LORA_BW_500;			
			else if (optBW==250)
				Bandwidth = LORA_BW_250;
			else if (optBW==125)
				Bandwidth = LORA_BW_125;							    
			else if (optBW==62)
				Bandwidth = LORA_BW_062; //actual  62500hz 			
			else if (optBW==41)
				Bandwidth = LORA_BW_041; //actual  41670hz
			else if (optBW==31)
				Bandwidth = LORA_BW_031; //actual  31250hz
			else if (optBW==20)
				Bandwidth = LORA_BW_020; //actual  20830hz			
			else if (optBW==15)
				Bandwidth = LORA_BW_015; //actual  15630hz
			else if (optBW==10)
				Bandwidth = LORA_BW_010; //actual  10420hz
			else if (optBW==7)
				Bandwidth = LORA_BW_007; //actual   7810hz	
			else {
				PRINT_CSTSTR("^$Unknown LoRa BW, taking BW125\n");
				Bandwidth = LORA_BW_125;
			}															
		}

		if (optSF<10)
			SIFS_cad_number=6;
		else 
			SIFS_cad_number=3;
  }
  
  // Select frequency channel
	if (optFQ>0.0)
		DEFAULT_CHANNEL=optFQ;
		  
  // LoRaWAN
	if (loraMode==LORAWAN_MODE) {
		PRINT_CSTSTR("^$Configuring for LoRaWAN\n");
    	
		if (optFQ<0.0) {
			DEFAULT_CHANNEL=optFQ=LORAWAN_UPFQ;
		}
		//set raw mode for LoRaWAN
		//overriding existing configuration
		optRAW=true;		
	}

#ifdef MY_FREQUENCY
	DEFAULT_CHANNEL=MY_FREQUENCY;
#endif
	
	//otherwise DEFAULT_CHANNEL is set in SX12XX_lora_gateway.h
	PRINT_CSTSTR("^$Frequency ");
	PRINTLN_VALUE("%lu", DEFAULT_CHANNEL);

	PRINT_CSTSTR("^$LoRa addr ");
	PRINTLN_VALUE("%d", GW_ADDR);
	
  if (optRAW) {
		PRINT_CSTSTR("^$Raw format, not assuming any header in reception\n");  
		// when operating in raw format, the gateway will not decode the packet header but will pass all the payload to stdout
		// note that in this case, the gateway may process packet that are not addressed explicitly to it as the dst field is not checked at all
		// this would be similar to a promiscuous sniffer, but most of real LoRa gateway works this way 
  } 

  //The function call list below shows the complete setup for the LoRa device using the information defined in the
  //Settings.h file.
  //The 'Setup Lora device' list below can be replaced with a single function call;
  //LT.setupLoRa(Frequency, Offset, SpreadingFactor, Bandwidth, CodeRate, Optimisation);

  //***************************************************************************************************
  //Setup Lora device
  //***************************************************************************************************

  LT.setMode(MODE_STDBY_RC);
  //set for LoRa transmissions                              
  LT.setPacketType(PACKET_TYPE_LORA);
  //set the operating frequency                 
  LT.setRfFrequency(DEFAULT_CHANNEL, Offset);                   
//run calibration after setting frequency
  LT.calibrateImage(0);
  //set LoRa modem parameters
  LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);                                  
  //where in the SX buffer packets start, TX and RX
  LT.setBufferBaseAddress(0x00, 0x00);
  //set packet parameters                    
  LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  //syncword, LORA_MAC_PRIVATE_SYNCWORD = 0x12, or LORA_MAC_PUBLIC_SYNCWORD = 0x34
  //TODO check for sync word when SX128X 
	if (loraMode==LORAWAN_MODE) LT.setSyncWord(LORA_MAC_PUBLIC_SYNCWORD);              
	else LT.setSyncWord(LORA_MAC_PRIVATE_SYNCWORD);
  //set for highest sensitivity at expense of slightly higher LNA current
  LT.setHighSensitivity();  //set for maximum gain
  //set for IRQ on RX done
  LT.setDioIrqParams(IRQ_RADIO_ALL, IRQ_RX_DONE, 0, 0);
#ifdef PABOOST
  LT.setPA_BOOST(true);
  PRINTLN_CSTSTR("^$Use PA_BOOST amplifier line");
#endif
  
  //***************************************************************************************************

	//the gateway needs to know its address to verify that a packet requesting an ack is for him
	//this is done in the SX12XX library
	//
	LT.setDevAddr(GW_ADDR);
	//TX power and ramp time
	//this is required because the gateway can have to transmit ACK messages prior to send any downlink packet
	//so txpower needs to be set before hand.
	PRINT_CSTSTR("^$Set LoRa txpower dBm to ");
	PRINTLN_VALUE("%d",(uint8_t)MAX_DBM); 
	LT.setTxParams(MAX_DBM, RADIO_RAMP_DEFAULT);            
	//as the "normal" behavior is to invert I/Q on TX, the option here is only to set invert I/Q on RX
	//to test inversion with reception when a device has been set to invert I/Q on TX
	//of course this is only valid if proposed by the radio module which is the case for SX127X
  if (optIIQ) {
		PRINT_CSTSTR("^$Invert I/Q on RX\n");  
		LT.invertIQ(true);
  }

  // get preamble length
  PRINT_CSTSTR("^$Preamble Length: ");
  PRINTLN_VALUE("%d", LT.getPreamble());

    while (LT.getPreamble() != 8) {
      PRINTLN_CSTSTR("^$Bad Preamble Length: set back to 8");
  		LT.setPacketParams(8, LORA_PACKET_VARIABLE_LENGTH, 255, LORA_CRC_ON, LORA_IQ_NORMAL);
  }    
 
  PRINT_CSTSTR("^$Sync word: 0x"); 
  PRINTLN_HEX("%X", LT.getSyncWord());  

  //reads and prints the configured LoRa settings, useful check
  PRINT_CSTSTR("^$");
  LT.printModemSettings();                                     
  PRINTLN;
  //reads and prints the configured operting settings, useful check
  PRINT_CSTSTR("^$");
  LT.printOperatingSettings();                                
  PRINTLN; 

  PRINT_CSTSTR("^$SX127X ");    
  // Print a success message
  PRINTLN_CSTSTR("configured as LR-BS. Waiting RF input for transparent RF-serial bridge");
#ifdef PERIODIC_RESET100
	PRINTLN_CSTSTR("^$Will safely reset radio module every 100 packet received");
#endif   
}

uint32_t getGwDateTime()
{
  char time_buffer[30];
  struct tm* tm_info;
  struct timeval tv;
  
  gettimeofday(&tv, NULL);
  tm_info = localtime(&tv.tv_sec);
  strftime(time_buffer, 30, "%Y-%m-%dT%H:%M:%S", tm_info);
  sprintf(time_str, "%s.%06ld", time_buffer, tv.tv_usec);
  
  return (tv.tv_usec+tv.tv_sec*1000000UL);
}

void setup()
{
  srand (time(NULL));
  
  SPI.begin();

  //setup hardware pins used by device, then check if device is found
//   if (LT.begin(NSS, NRESET, DIO0, DIO1, DIO2, LORA_DEVICE)) PRINTLN_CSTSTR("^$**********Power ON");
//   else{
//     PRINTLN_CSTSTR("^$No device responding");
//     while (1){}
//   }

  loraConfig(); 

#if defined DOWNLINK
  PRINTLN_CSTSTR("^$Low-level gateway has downlink support");
  if (!enableDownlinkCheck)
  	PRINTLN_CSTSTR("^$But downlink is disabled");
#endif

  getGwDateTime();
  PRINT_STR("%s\n", time_str);
  
  FLUSHOUTPUT;
  delay(500);
}

void packet_is_Error()
{
  uint16_t IRQStatus;
  IRQStatus = LT.readIrqStatus();                   //read the LoRa device IRQ status register

  if (IRQStatus & IRQ_RX_TIMEOUT)                   //check for an RX timeout
  {
  	//here if we don't receive anything, we normally print nothing
    //PRINT_CSTSTR("RXTimeout");
  }
  else
  {
    errors++;
    PRINT_CSTSTR("^$Receive error");
    PRINT_CSTSTR(",RSSI,");
    PRINT_VALUE("%d",PacketRSSI);
    PRINT_CSTSTR("dBm,SNR,");
    PRINT_VALUE("%d",PacketSNR);
    PRINT_CSTSTR("dB,Length,");
    PRINT_VALUE("%d",LT.readRXPacketL());               //get the device packet length
    PRINT_CSTSTR(",Packets,");
    PRINT_VALUE("%d",RXpacketCount);
    PRINT_CSTSTR(",Errors,");
    PRINT_VALUE("%d",errors);
    PRINT_CSTSTR(",IRQreg,");
    PRINT_HEX("%X",IRQStatus);
    LT.printIrqStatus();                            //print the names of the IRQ registers set
    PRINTLN;
  }

  FLUSHOUTPUT;
}
  
void loop(void) 
{ 
	int i=0;
	int RXPacketL=0;
	char print_buf[100];
	uint32_t current_time_tmst;

	/////////////////////////////////////////////////////////////////// 
	// START OF PERIODIC TASKS

	bool receivedFromLoRa=false;

	/////////////////////////////////////////////////////////////////// 
	// THE MAIN PACKET RECEPTION LOOP
	//
	if (!checkForLateDownlinkRX2 && !checkForLateDownlinkJACC2) {
	
		if (status_counter==60 || status_counter==0) {
			PRINT_CSTSTR("^$Low-level gw status ON");
			PRINTLN;
			FLUSHOUTPUT; 
			status_counter=0;
		} 
		
		if (optRAW)
			RXPacketL = LT.receive(RXBUFFER, RXBUFFER_SIZE, 10000, WAIT_RX);
		else {	
			RXPacketL = LT.receiveAddressed(RXBUFFER, RXBUFFER_SIZE, 10000, WAIT_RX);
			
			//for packet with our header, destination must be GW_ADDR (i.e. 1) otherwise we reject it
			if (LT.readRXDestination()!=GW_ADDR)
				RXPacketL=-1;
		}
		
		if (RXPacketL>0) {
			//get timestamps only when we actually receive something
			rcv_time_tmst=getGwDateTime();
			rcv_time_ms=millis();
			
			//we actually received something when we were "waiting" for a JACC1 downlink 
			//we give priority to the latest reception
			if (rcv_timeout!=MAX_TIMEOUT) {
				//remove any downlink.txt file that may have been generated late because it was not for the new packet reception
				remove("downlink/downlink.txt");
				//set back to normal reception mode
				checkForLateDownlinkJACC1=false;
				rcv_timeout=MAX_TIMEOUT;
			}
		}
		
		status_counter++;
	
		if (RXPacketL==0) {
			packet_is_Error();

			if (errors>10) {
				errors=0;
				PRINTLN_CSTSTR("^$Resetting radio module, too many errors");
				LT.resetDevice();
				loraConfig();
				// to start over
				status_counter=0;
			}
		}
		
		FLUSHOUTPUT;
	
		/////////////////////////////////////////////////////////////////// 
		// LoRa reception       
	
		if (RXPacketL>0) {                 
		
#ifdef DEBUG_DOWNLINK_TIMING
		 	//this is the time (millis()) at which the packet has been completely received at the radio module
		 	PRINT_CSTSTR("^$rcv_time_ms:"); 		 
      PRINT_VALUE("%lu",rcv_time_ms);
      PRINTLN;
#endif
		
			RXpacketCount++;
			
			receivedFromLoRa=true;       

			//remove any downlink.txt file that may have been generated late and will therefore be out-dated
			remove("downlink/downlink.txt");

			PacketRSSI = LT.readPacketRSSI();              //read the recived RSSI value
			PacketSNR = LT.readPacketSNR();                //read the received SNR value

			sprintf(print_buf, "--- rxlora[%lu]. dst=%d type=0x%02X src=%d seq=%d", 
				RXpacketCount,
				LT.readRXDestination(),
				LT.readRXPacketType(), 
				LT.readRXSource(),
				LT.readRXSeqNo());
	 
			PRINT_STR("%s", print_buf);

			sprintf(print_buf, " len=%d SNR=%d RSSIpkt=%d BW=%d CR=4/%d SF=%d\n", 
				RXPacketL, 
				PacketSNR,
				PacketRSSI,
				LT.returnBandwidth()/1000,				
				LT.getLoRaCodingRate(),
				LT.getLoRaSF());     

			PRINT_STR("%s", print_buf);              

			// provide a short output for external program to have information about the received packet
			// ^psrc_id,seq,len,SNR,RSSI
			sprintf(print_buf, "^p%d,%d,%d,%d,",
				LT.readRXDestination(),
				LT.readRXPacketType(), 
				LT.readRXSource(),
				LT.readRXSeqNo());
	 
			PRINT_STR("%s", print_buf);       

			sprintf(print_buf, "%d,%d,%d\n",
				RXPacketL, 
				PacketSNR,
				PacketRSSI);

			PRINT_STR("%s", print_buf); 

			// ^rbw,cr,sf,fq
			sprintf(print_buf, "^r%d,%d,%d,%ld\n", 
				LT.returnBandwidth()/1000,			
				LT.getLoRaCodingRate(),	
				LT.getLoRaSF(),
				(uint32_t)(DEFAULT_CHANNEL/1000.0));

			PRINT_STR("%s", print_buf);

			//print the reception date and time
			sprintf(print_buf, "^t%s*%lu\n", time_str, rcv_time_tmst);
			PRINT_STR("%s", print_buf);

	#if defined WITH_DATA_PREFIX
			PRINT_STR("%c",(char)DATA_PREFIX_0);        
			PRINT_STR("%c",(char)DATA_PREFIX_1);
	#endif

			// print to stdout the content of the packet
			//
			FLUSHOUTPUT;
	 
			for (int a=0; a<RXPacketL; a++) {
		
				if (optHEX) {
					if ((uint8_t)RXBUFFER[a]<16)
						PRINT_CSTSTR("0");
					PRINT_HEX("%X",	(uint8_t)RXBUFFER[a]);
					PRINT_CSTSTR(" ");	
				}
				else
					PRINT_STR("%c",(char)RXBUFFER[a]);
				}
		 
			PRINTLN;
			FLUSHOUTPUT;  
		}
	}	

  if (receivedFromLoRa || checkForLateDownlinkRX2 || checkForLateDownlinkJACC1 || checkForLateDownlinkJACC2) {

#ifdef DOWNLINK
    // handle downlink request
		bool hasDownlinkEntry=false;    	
		char* downlink_json_entry=NULL;
		uint8_t TXPacketL;
		
#ifdef DEBUG_DOWNLINK_TIMING
		//this is the time (millis()) at which we start waiting for checking downlink requests
		PRINT_CSTSTR("^$downlink wait: ");
		PRINTLN_VALUE("%lu", millis());
#endif
    		    	
		if (enableDownlinkCheck)  {	

			//wait until it is the (approximate) right time to check for downlink requests
			//because the generation of downlink.txt file takes some time
			//only for RX1, after that, we simply pass
			while (millis()-rcv_time_ms < DELAY_DNWFILE)
				;

#ifdef DEBUG_DOWNLINK_TIMING
			//this is the time (millis()) at which we start checking downlink requests
		PRINT_CSTSTR("^$downlink check: ");
		PRINTLN_VALUE("%lu", millis());
#endif
			//to set time_str 
			current_time_tmst=getGwDateTime();   	
	
			// use rapidjson to parse all lines
			// if there is a downlink.txt file then read all lines in memory for
			// further processing
			if (checkForLateDownlinkRX2)
				PRINT_CSTSTR("^$----delayRX2-----------------------------------------\n");
			else if (checkForLateDownlinkJACC1)	
				PRINT_CSTSTR("^$----retryJACC1---------------------------------------\n");    			
			else if (checkForLateDownlinkJACC2)	
				PRINT_CSTSTR("^$----retryJACC2---------------------------------------\n");
			else	
				PRINT_CSTSTR("^$-----------------------------------------------------\n");
				
			PRINT_CSTSTR("^$Check for downlink requests ");
			PRINT_STR("%s", time_str);
			PRINTLN_VALUE("%lu", current_time_tmst);
			
			FILE *fp = fopen("downlink/downlink.txt","r");
			
			if (fp) {
				
				fclose(fp);
				
				// remove any \r characters introduced by various OS and/or tools
				system("sed -i 's/\r//g' downlink/downlink.txt");
				// remove empty lines
				system("sed -i '/^$/d' downlink/downlink.txt");
				
				fp = fopen("downlink/downlink.txt","r");
				
				size_t len=0;
				
				ssize_t dl_line_size=getline(&downlink_json_entry, &len, fp);

				PRINT_CSTSTR("^$Read downlink: ");
				PRINTLN_VALUE("%lu", dl_line_size);
							
				if (dl_line_size>0) {
					hasDownlinkEntry=true;
					//PRINT_CSTSTR("^$Downlink entry: ");
					//PRINTLN_STR("%d", downlink_json_entry);
				}
				
				fclose(fp);
    			
#ifdef KEEP_DOWNLINK_BACKUP_FILE 
				char tmp_c[100];   			
				sprintf(tmp_c, "mv downlink/downlink.txt downlink/downlink-backup-%s.txt", time_str);
				system(tmp_c);
#else
				remove("downlink/downlink.txt");
#endif

				//got a downlink, clear all indicators and back to normal reception for next time
				checkForLateDownlinkRX2=false;
				checkForLateDownlinkJACC1=false;
				checkForLateDownlinkJACC2=false;
				rcv_timeout=MAX_TIMEOUT;					
			}
			else {
				hasDownlinkEntry=false;
				PRINT_CSTSTR("^$NO NEW DOWNLINK ENTRY\n");

				if (checkForLateDownlinkRX2) {
					checkForLateDownlinkRX2=false;
					// retry later, last retry
					checkForLateDownlinkJACC1=true;
					//will however wait for new incoming messages up to rcv_time_ms+DELAY_JACC1-(DELAY_DNW1-DELAY_DNWFILE)
					rcv_timeout=rcv_time_ms+DELAY_JACC1-millis()-(DELAY_DNW1-DELAY_DNWFILE);    			
    		}
				else
					//it was already the last retry for late downlink
					if (checkForLateDownlinkJACC2) {
						checkForLateDownlinkJACC2=false;
						rcv_timeout=MAX_TIMEOUT;
					}   
					else
					if (checkForLateDownlinkJACC1) { 
						// retry later, for RX2 (join)
						delay(DELAY_EXTDNW2);
						checkForLateDownlinkJACC1=false;
						checkForLateDownlinkJACC2=true;
					}				 			
					else {
						//we initiate the entire downlink check only for LoRaWAN, i.e. when raw mode is enabled
						if (optRAW) { 
							// retry later, for RX2 (data)
							delay(DELAY_EXTDNW2);
							checkForLateDownlinkRX2=true;
						}
					}    			

				FLUSHOUTPUT;
    	}	   		
    }

#ifdef DEBUG_DOWNLINK_TIMING
		//this is the time (millis()) at which we start processing the downlink requests
		PRINT_CSTSTR("^$downlink process: ");
		PRINTLN_VALUE("%lu", millis());
#endif
        
    if (hasDownlinkEntry) {
			Document document;
		  
			bool is_lorawan_downlink=false;
		
			PRINT_CSTSTR("^$Process downlink request\n");
			//PRINT_CSTSTR("^$Process downlink requests: ");
			//PRINTLN_STR("%s", downlink_json_entry);
		
			document.Parse(downlink_json_entry);
					
			if (document.IsObject()) {
				if (document.HasMember("txpk"))								
					if (document["txpk"].HasMember("modu"))
						if (document["txpk"]["modu"].IsString())
							if (document["txpk"]["modu"]=="LORA") {
								is_lorawan_downlink=true;    
								//PRINT_CSTSTR("^$data = ");
								//PRINTLN_STR("%s", document["txpk"]["data"].GetString());
							}
						
				if (is_lorawan_downlink) {		
					PRINT_CSTSTR("^$LoRaWAN downlink request\n");				

					//use the tmst field provided by the Network Server in the downlink message
					uint32_t send_time_tmst=document["txpk"]["tmst"].GetUint();
					//to get the waiting delay
					uint32_t rx_wait_delay=send_time_tmst-rcv_time_tmst;

					//uncomment to test re-scheduling for RX2
					//delay(500);	
					
					bool go_for_downlink=false;
									
					// just in case
					if (send_time_tmst < rcv_time_tmst)
						PRINT_CSTSTR("^$WARNING: downlink tmst in the past, abort\n");	
					else 
					if (send_time_tmst - rcv_time_tmst > ((uint32_t)DELAY_JACC2+(uint32_t)DELAY_EXTDNW2)*1000)
						PRINT_CSTSTR("^$WARNING: downlink tmst too much in future, abort\n");	
					else
						{
							uint32_t wmargin;
							
							//determine if we have some margin to re-schedule
							if (rx_wait_delay/1000 == (uint32_t)DELAY_DNW1 || rx_wait_delay/1000 == (uint32_t)DELAY_JACC1)
								wmargin=(uint32_t)DELAY_EXTDNW2;
							else
								wmargin=0;
							
							//are we behind scheduled 	
							if (millis() > rcv_time_ms + rx_wait_delay/1000 - MARGIN_DNW) {
								//
								if (wmargin) {
									rx_wait_delay+=(uint32_t)wmargin*1000;
									go_for_downlink=true;
									PRINT_CSTSTR("^$WARNING: too late for RX1, try RX2\n");	
								}
								else
									PRINT_CSTSTR("^$WARNING: too late to send downlink, abort\n");
							}
							else
								go_for_downlink=true;				
						}
								
					if (go_for_downlink) {												
						uint8_t downlink_payload[256];										
				
						//convert base64 data into binary buffer
						int downlink_size = b64_to_bin(document["txpk"]["data"].GetString(), document["txpk"]["data"].GetStringLength(), downlink_payload, sizeof downlink_payload);
				
						if (downlink_size != document["txpk"]["size"].GetInt()) {
							PRINT_CSTSTR("^$WARNING: mismatch between .size and .data size once converter to binary\n");
						}							
						//uncomment to test for RX2
						//rx_wait_delay+=(uint32_t)DELAY_EXTDNW2*1000;

						bool useRX2=false;
				
						if (rx_wait_delay/1000 == (uint32_t)DELAY_DNW2 || rx_wait_delay/1000 == (uint32_t)DELAY_JACC2)
							useRX2=true;
				
						// should wait for RX2
						if (useRX2) {           

							PRINT_CSTSTR("^$Target RX2\n");
							//set frequency according to RX2 window
							LT.setRfFrequency(LORAWAN_D2FQ, Offset);
							//change to SF according to RX2 window
  							LT.setModulationParams(LORAWAN_D2SF, Bandwidth, CodeRate, Optimisation);									
						}
						else PRINT_CSTSTR("^$Target RX1\n");
					
						//wait until it is the (approximate) right time to send the downlink data
						//downlink data can use DELAY_DNW1 or DELAY_DNW2
						//join-request-accept can use DELAY_JACC1 or DELAY_JACC2
						while (millis()-rcv_time_ms < (rx_wait_delay/1000-MARGIN_DNW /* take a margin*/))
							;
			
						//	send the packet
#ifdef DEBUG_DOWNLINK_TIMING						
						PRINT_CSTSTR("^$downlink send: ");
						PRINTLN_VALUE("%lu", millis());						
#endif
						//LoRaWAN downlink so no header in communication lib								
						TXPacketL=LT.transmit((uint8_t*)downlink_payload, downlink_size, 10000, MAX_DBM, WAIT_TX);    

						if (TXPacketL>0)
							PRINTLN_CSTSTR("^$Packet sent");
						else	
							PRINT_CSTSTR("^$Send error\n");

						if (useRX2) {					
							//set back to the reception frequency
							LT.setRfFrequency(DEFAULT_CHANNEL, Offset);
							PRINT_CSTSTR("^$Set back frequency\n");	
							//set back the SF
  							LT.setModulationParams(SpreadingFactor, Bandwidth, CodeRate, Optimisation);
							PRINT_CSTSTR("^$Set back SF\n");
						}
					}
				} 
				else {			
					// non-LoRaWAN downlink		    
					if (document["dst"].IsInt()) {
						PRINT_CSTSTR("^$dst = ");
						PRINTLN_VALUE("%d", document["dst"].GetInt());
					}
							
					// check if it is a valid send request
					if (document["status"]=="send_request" && document["dst"].IsInt()) {
						
						//TODO CarrierSense?
						if (1) {
				
							uint8_t p_type=PKT_TYPE_DATA | PKT_FLAG_DATA_DOWNLINK;
						
							uint8_t l=document["data"].GetStringLength();
							uint8_t* pkt_payload=(uint8_t*)document["data"].GetString();
							uint8_t downlink_message[256];
						 
#ifdef INCLUDE_MIC_IN_DOWNLINK    				
							// we test if we have MIC data in the json entry
							if (document["MIC0"].IsString() && document["MIC0"]!="") {
						
								// indicate a downlink packet with a 4-byte MIC after the payload
								p_type = p_type | PKT_FLAG_DATA_ENCRYPTED;
					
								memcpy(downlink_message, (uint8_t*)document["data"].GetString(), l);
					
								// set the 4-byte MIC after the payload
								downlink_message[l]=(uint8_t)xtoi(document["MIC0"].GetString());
								downlink_message[l+1]=(uint8_t)xtoi(document["MIC1"].GetString());
								downlink_message[l+2]=(uint8_t)xtoi(document["MIC2"].GetString());
								downlink_message[l+3]=(uint8_t)xtoi(document["MIC3"].GetString());
							
								// here we sent the downlink packet with the 4-byte MIC
								// at the device, the expected behavior is
								// - to test for the packet type, then remove 4 bytes from the payload length to get the real payload
								// - use AES encryption on the clear payload to compute the MIC and compare with the MIC sent in the downlink packet
								// - if both MIC are equal, then accept the downlink packet as a valid downlink packet
								l += 4;
								pkt_payload=downlink_message;
							}			
#endif

							//wait until it is the (approximate) right time to send the downlink date
							//the end-device approximately waits for 1s
							while (millis()-rcv_time_ms < (DELAY_DNW1-MARGIN_DNW) /* take a margin*/)
								;
						
#ifdef DEBUG_DOWNLINK_TIMING						
							PRINT_CSTSTR("^$downlink send: ");
							PRINTLN_VALUE("%lu", millis());			
#endif									
							TXPacketL=LT.transmitAddressed(pkt_payload, l, p_type, document["dst"].GetInt(), GW_ADDR, 10000, MAX_DBM, WAIT_TX);
							
							if (TXPacketL>0)
								PRINT_CSTSTR("^$Packet sent\n");
							else	
								PRINT_CSTSTR("^$Send error\n");	
						}
						else {
							PRINT_CSTSTR("^$DELAYED: busy channel\n");	
							// here we will retry later because of a busy channel
						}
					}
					else {
						PRINT_CSTSTR("^$DISCARDING: not valid send_request\n");   	
					}
				}							

#ifdef KEEP_DOWNLINK_SENT_FILE

				if (document.HasMember("status")) {

					StringBuffer json_record_buffer;
					Writer<StringBuffer> writer(json_record_buffer);
						
					getGwDateTime();

					//transmission successful		
					if (TXPacketL>0)
						document["status"].SetString("sent", document.GetAllocator());
					else
						document["status"].SetString("sent_fail", document.GetAllocator());

					document.Accept(writer);
					PRINT_CSTSTR("^$JSON record: ");
					PRINTLN_STR("%s", json_record_buffer.GetString());

					FILE *fp = fopen("downlink/downlink-sent.txt","a");
		
					if (fp) {
						fprintf(fp, "%s %s\n", time_str, json_record_buffer.GetString());
						fclose(fp);
					}					
				}  		
#endif			

				if (downlink_json_entry)
					free(downlink_json_entry);
				
				PRINT_CSTSTR("^$-----------------------------------------------------\n");
			}
			else { 
				PRINT_CSTSTR("^$WARNING: not valid JSON format, abort\n");
			}			
			FLUSHOUTPUT;
		}	
		
		//remove any downlink.txt file that may have been generated late and will therefore be out-dated
		if (!checkForLateDownlinkRX2 && !checkForLateDownlinkJACC2)
			remove("downlink/downlink.txt");
#endif
	} 

#ifdef PERIODIC_RESET100	
	//test periodic reconfiguration
	if ( RXpacketCount!=0 && !(RXpacketCount % 100) && RXpacketCount!=RXpacketCountReset)
	{	
		PRINTLN_CSTSTR("^$****Periodic reset");
		RXpacketCountReset=RXpacketCount;
		LT.resetDevice();
		loraConfig();	
	}	
#endif	
}

int xtoi(const char *hexstring) {
	int	i = 0;
	
	if ((*hexstring == '0') && (*(hexstring+1) == 'x'))
		  hexstring += 2;
	while (*hexstring)
	{
		char c = toupper(*hexstring++);
		if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A')))
			break;
		c -= '0';
		if (c > 9)
			c -= 7;
		i = (i << 4) + c;
	}
	return i;
}

//######################################
uint8_t *temp = NULL;
//######################################

int main (int argc, char *argv[]){

  int opt=0;
  
  //Specifying the expected options
  static struct option long_options[] = {
      {"mode", required_argument, 0,  'a' },
      {"bw", required_argument, 0,    'b' },
      {"cr", required_argument, 0,    'c' },
      {"sf", required_argument, 0,    'd' }, 
      {"raw", no_argument, 0,         'e' },
      {"freq", required_argument, 0,  'f' },
      {"ch", required_argument, 0,    'g' },       
      {"sw", required_argument, 0,    'h' },
#ifdef DOWNLINK      
      {"ndl", no_argument, 0,    'i' },       
#endif                            
      {"hex", no_argument, 0,    'j' },
      {"iiq", no_argument, 0,    'k' },      
      {0, 0, 0,  0}
  };
  
  int long_index=0;
  
  while ((opt = getopt_long(argc, argv,"a:b:c:d:ef:g:h:ij", 
                 long_options, &long_index )) != -1) {
      switch (opt) {
           case 'a' : loraMode = atoi(unistd::optarg); 
           						// from 1 to 10 (see https://github.com/CongducPham/LowCostLoRaGw#annexa-lora-mode-and-predefined-channels)
           						// for SX128X, BW 125, 250 and 500 are replaced respectively by BW 203, 406 and 812
           						// keep loraMode == 11 == LoRaWAN for the moment
               break;
           case 'b' : optBW = atoi(unistd::optarg); 
                      // 7, 10, 15, 20, 31, 41, 62, 125, 250 or 500 for SX126X and SX127X_lora_gateway
                      // 125/200/203, 250/400/406, 500/800/812 or 1600/1625 for SX128X
               break;
           case 'c' : optCR = atoi(unistd::optarg);
                      // 5, 6, 7 or 8
               break;
           case 'd' : optSF = atoi(unistd::optarg);
                      // 5, 6, 7, 8, 9, 10, 11 or 12
               break;
           case 'e' : optRAW=true;
           						// no header
           						// required for LoRaWAN
               break;               
           case 'f' : optFQ=atof(unistd::optarg)*1000000.0;
                      // optFQ in MHz e.g. 868.1
               break;     
           case 'g' : optCH=atoi(unistd::optarg);
                      if (optCH < STARTING_CHANNEL || optCH > ENDING_CHANNEL)
                        optCH=STARTING_CHANNEL;
                      optCH=optCH-STARTING_CHANNEL;  
                      DEFAULT_CHANNEL=loraChannelArray[optCH]; 
                      // pre-defined channels (see https://github.com/CongducPham/LowCostLoRaGw#annexa-lora-mode-and-predefined-channels)
               break;      
           case 'h' : { 
           							uint8_t sw=atoi(unistd::optarg);
                      	// assume that sw is expressed in hex value
                      	optSW = (sw / 10)*16 + (sw % 10);
                      	// optSW is no longer used, it is not taken into account anymore
                      	// we only differentiate between LoRaWAN_mode and non-LoRaWAN mode
                      }
               break;
#ifdef DOWNLINK                       
					 case 'i' : enableDownlinkCheck=false;
               break;    
#endif
           case 'j' : optHEX=true;
               break;
           case 'k' : optIIQ=true;
               break;                                                                  
           //default: print_usage(); 
           //    exit(EXIT_FAILURE);
      }
  }

  	//######################################
	uint8_t *bridge_map = NULL;

	int fd = 0;
	int result = 0;

	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd < 0) {
		perror("Couldn't open /dev/mem\n");
		return -2;
	}

	bridge_map = (uint8_t *)mmap(NULL, BRIDGE_SPAN, PROT_READ | PROT_WRITE,
								MAP_SHARED, fd, BRIDGE);

	if (bridge_map == MAP_FAILED) {
		perror("mmap failed.");
		unistd::close(fd);
		return -3;
	}

	temp = bridge_map + DATA;
	//######################################

  setup();
  
  while(1){
    loop();
  }

  	//######################################
	result = munmap(bridge_map, BRIDGE_SPAN);

	if (result < 0) {
		perror("Couldnt unmap bridge.");
		unistd::close(fd);
		return -4;
	}
	unistd::close(fd);
	//######################################
  
  return (0);
}

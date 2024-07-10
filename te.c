#
TARGET = lora_gateway

#
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib


CROSS_COMPILE = arm-linux-gnueabihf-

CFLAGS = -g -Wall   -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY)   -I$(HWLIBS_ROOT)/include/
LDFLAGS =  -g -Wall 
CC = $(CROSS_COMPILE)g++ -std=c++11
ARCH= arm

build: $(TARGET)
$(TARGET): SX1272.o lora_gateway.o base64.o AES_128_V10.o Encrypt_V31.o local_lorawan.o support_function.o ReadWriteJson.o
	$(CC) $(LDFLAGS) -lm  $^ -o $@  

SX1272.o: SX1272.cpp SX1272.h
	$(CC) $(CFLAGS) -c SX1272.cpp -o SX1272.o

lora_gateway.o: lora_gateway.cpp radio.makefile
	$(CC) $(CFLAGS) -c lora_gateway.cpp -o lora_gateway.o

base64.o: base64.c base64.h
	$(CC) $(CFLAGS) -c base64.c -o base64.o	

AES_128_V10.o: AES-128_V10.cpp AES-128_V10.h
	$(CC) $(CFLAGS) -c AES-128_V10.cpp -o AES_128_V10.o
	
Encrypt_V31.o: Encrypt_V31.cpp Encrypt_V31.h
	$(CC) $(CFLAGS) -c Encrypt_V31.cpp -o Encrypt_V31.o
	
local_lorawan.o: local_lorawan.cpp local_lorawan.h
	$(CC) $(CFLAGS) -c local_lorawan.cpp -o local_lorawan.o
	
support_function.o: support_function.cpp support_function.h
	$(CC) $(CFLAGS) -c support_function.cpp -o support_function.o

ReadWriteJson.o: ReadWriteJson.cpp ReadWriteJson.h
	$(CC) $(CFLAGS) -c ReadWriteJson.cpp -o ReadWriteJson.o
	
.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~ 
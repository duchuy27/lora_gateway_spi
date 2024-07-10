include radio.makefile
-include /boot/radio.makefile

ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib

CROSS_COMPILE =/home/qikay/de10nano-wd/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-

CFLAGS = -g -Wall -D$(ALT_DEVICE_FAMILY) \
	-I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) \
	-I$(HWLIBS_ROOT)/include/
LDFLAGS = -g -Wall
CC = $(CROSS_COMPILE)gcc
ARCH = arm

# --------------> ALL RPI using wiringPi
########################################
lora_gateway: SX127X_lora_gateway_wiring

#SX12XX lora gateway 
################## $(CC) -lrt -lpthread -lwiringPi SX127X_lora_gateway_wiring.o arduinoPi.o SX12XX/SX127XLT_wiring.o base64.o -o SX127X_lora_gateway_wiring
SX127X_lora_gateway_wiring: SX127XLT_wiring.o SX127X_lora_gateway_wiring.o arduinoPi.o  base64.o
	$(CC) $(LDFLAGS) -lm $^ -o SX127X_lora_gateway_wiring
	rm -f lora_gateway
	ln -s SX127X_lora_gateway_wiring ./lora_gateway	

################################
# rulesCROSS_COMPLIE
################################

SX127XLT_wiring.o: SX127XLT.cpp SX127XLT.h SX127XLT_Definitions.h
	$(CC) $(CFLAGS) -c $< -o $@

SX127X_lora_gateway_wiring.o: SX12XX_lora_gateway.cpp SX127X_lora_gateway.h radio.makefile
	$(CC) $(CFLAGS) -c $< -o $@

arduinoPi.o: arduinoPi.cpp arduinoPi.h
	$(CC) $(CFLAGS) -c $< -o $@	
	
base64.o: base64.c base64.h
	$(CC) $(CFLAGS) -c base64.c -o base64.o

.PHONY: clean

clean:
	cd SX1272; rm -rf *.o; cd ../SX12XX; rm -rf *.o; cd ..
	rm -rf *.o lora_gateway

include radio.makefile
include raspberry.makefile

-include /boot/radio.makefile
-include /boot/raspberry.makefile

# --------------> ALL RPI using wiringPi
########################################
lora_gateway: SX127X_lora_gateway_wiring

#SX12XX lora gateway 
####################
SX127X_lora_gateway_wiring: SX127X_lora_gateway_wiring.o arduinoPi.o SX12XX/SX127XLT_wiring.o base64.o
	g++ -lrt -lpthread -lwiringPi SX127X_lora_gateway_wiring.o arduinoPi.o SX12XX/SX127XLT_wiring.o base64.o -o SX127X_lora_gateway_wiring
	rm -f lora_gateway
	ln -s SX127X_lora_gateway_wiring ./lora_gateway	

################################
# rules
################################
SX127X_lora_gateway_wiring.o: SX12XX_lora_gateway.cpp SX127X_lora_gateway.h radio.makefile raspberry.makefile
	g++ $(CROSS_COMPLIE) -I SX12XX -I . -DSX127X -c SX12XX_lora_gateway.cpp -o SX127X_lora_gateway_wiring.o

arduinoPi.o: arduinoPi.cpp arduinoPi.h
	g++ $(CROSS_COMPLIE) -c arduinoPi.cpp -o arduinoPi.o		

SX12XX/SX127XLT_wiring.o: SX12XX/SX127XLT.cpp SX12XX/SX127XLT.h SX12XX/SX127XLT_Definitions.h
	g++ $(CROSS_COMPLIE) -I SX12XX -I . -c SX12XX/SX127XLT.cpp -o SX12XX/SX127XLT_wiring.o
	
base64.o: base64.c base64.h
	g++ -c base64.c -o base64.o	

clean:
	cd SX1272; rm -rf *.o; cd ../SX12XX; rm -rf *.o; cd ..
	rm -rf *.o lora_gateway

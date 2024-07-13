#ifndef SX127XLT_h
#define SX127XLT_h

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <stdint.h>
	
	#ifdef USE_ARDUPI
  #include "arduPi.h"	
	#else
  #include "arduinoPi.h"
	#endif
	
  #ifndef inttypes_h
    #include <inttypes.h>
  #endif
#endif

#include "SX127XLT_Definitions.h"

class SX127XLT
{
  public:

    SX127XLT();
    void setMode(uint8_t modeconfig);         //same function as setStandby()
    void calibrateImage(uint8_t null);

    //*******************************************************************************
    //lay thong so
    //*******************************************************************************
    void setDevice(uint8_t type);
    void printDevice(); 
    void printOperatingMode();
    uint8_t getOpmode();
    uint8_t getVersion();
    uint8_t getPacketMode();           //LoRa or FSK
    uint8_t getHeaderMode();
    uint8_t getCRCMode();
    uint8_t getAGC();
    uint8_t getLNAgain();
    uint8_t getLNAboostHF();
    uint8_t getLNAboostLF();
    void printOperatingSettings();

    //*******************************************************************************
    //LoRa specific routines
    //*******************************************************************************
    uint32_t getFreqInt();
    uint8_t getLoRaSF();
    uint32_t returnBandwidth(uint8_t BWregvalue); 
    uint8_t getLoRaCodingRate();
    uint8_t getOptimisation();
    uint8_t getSyncWord();
    uint8_t getInvertIQ();
    uint16_t getPreamble();
    void printModemSettings();

    void writeRegister( uint8_t address, uint8_t value );
    uint8_t readRegister( uint8_t address );

    void setTxParams(int8_t txPower, uint8_t rampTime);
    void setPacketParams(uint16_t packetParam1, uint8_t  packetParam2, uint8_t packetParam3, uint8_t packetParam4, uint8_t packetParam5);
    void setModulationParams(uint8_t modParam1, uint8_t modParam2, uint8_t  modParam3, uint8_t  modParam4);
    void setRfFrequency(uint64_t freq64, int32_t offset);

    void setTx(uint32_t timeout);
    void setRx(uint32_t timeout);
    void setHighSensitivity();

    uint8_t readRXPacketL(); 
    int8_t readPacketRSSI();
    int8_t readPacketSNR();
         
    uint8_t readRXPacketType();
    uint8_t readRXDestination();
    uint8_t readRXSource();

    void setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress);
    void setPacketType(uint8_t PacketType);

    void clearIrqStatus( uint16_t irqMask );
    uint16_t readIrqStatus();
    void setDioIrqParams(uint16_t irqMask, uint16_t dio0Mask, uint16_t dio1Mask, uint16_t dio2Mask );
    void printIrqStatus();

    //*******************************************************************************
    //Packet Read and Write Routines
    //*******************************************************************************
    uint8_t receive(uint8_t *rxbuffer, uint8_t size, uint32_t rxtimeout, uint8_t wait);
    uint8_t receiveAddressed(uint8_t *rxbuffer, uint8_t size, uint32_t rxtimeout, uint8_t wait);
    uint8_t transmit(uint8_t *txbuffer, uint8_t size, uint32_t txtimeout, int8_t txPower, uint8_t wait);
    uint8_t transmitAddressed(uint8_t *txbuffer, uint8_t size, char txpackettype, char txdestination, char txsource, uint32_t txtimeout, int8_t txpower, uint8_t wait);
    uint8_t returnOptimisation(uint8_t SpreadingFactor, uint8_t Bandwidth);  //this returns the required optimisation setting
    float calcSymbolTime(float Bandwidth, uint8_t SpreadingFactor);
    void setSyncWord(uint8_t syncword);
    uint32_t returnBandwidth();
    uint8_t invertIQ(bool invert);
    uint8_t readRXSeqNo();
    void setPA_BOOST(bool pa_boost);
    void setDevAddr(uint8_t addr);

    //######################################
    uint8_t spi_read(uint32_t data);
    void spi_write(uint32_t data);
    //######################################
    
  private:

    int8_t _NSS, _NRESET, _DIO0, _DIO1, _DIO2;
    uint8_t _RXPacketL;             //length of packet received
    uint8_t _RXPacketType;          //type number of received packet
    uint8_t _RXDestination;         //destination address of received packet
    uint8_t _RXSource;              //source address of received packet
    int8_t  _PacketRSSI;            //RSSI of received packet
    int8_t  _PacketSNR;             //signal to noise ratio of received packet
    uint8_t _TXPacketL;             //length of transmitted packet
    uint16_t _IRQmsb;               //for setting additional flags
    uint8_t _Device;                //saved device type
    uint8_t _UseCRC;                //when packet parameters set this flag enabled if CRC on packets in use
    uint8_t _PACKET_TYPE;           //used to save the set packet type
    uint8_t _freqregH, _freqregM, _freqregL;  //the registers values for the set frequency   
    uint32_t savedFrequency;
    int32_t savedOffset;
    int16_t  _RSSI;
    uint8_t _TXSeqNo; //sequence number of transmitted packet
    uint8_t _RXSeqNo; //sequence number of received packet
    bool _PA_BOOST;
    uint32_t _RXTimestamp;
    uint32_t _RXDoneTimestamp;
    uint8_t _DevAddr;
    uint8_t _AckStatus;
    int8_t _PacketSNRinACK;
    void (*_lowPowerFctPtr)(unsigned long);
};

//######################################
extern uint8_t *temp;
//######################################

#endif
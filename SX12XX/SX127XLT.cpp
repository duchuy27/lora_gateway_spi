#if defined __SAMD21G18A__ && not defined ARDUINO_SAMD_FEATHER_M0
	#define PRINTLN                   SerialUSB.println("")
	#define PRINT_CSTSTR(param)   		SerialUSB.print(F(param))
	#define PRINTLN_CSTSTR(param)			SerialUSB.println(F(param))
	#define PRINT_STR(fmt,param)      SerialUSB.print(param)
	#define PRINTLN_STR(fmt,param)		SerialUSB.println(param)
	#define PRINT_VALUE(fmt,param)    SerialUSB.print(param)
	#define PRINTLN_VALUE(fmt,param)	SerialUSB.println(param)
	#define PRINT_HEX(fmt,param)      SerialUSB.print(param,HEX)
	#define PRINTLN_HEX(fmt,param)		SerialUSB.println(param,HEX)
	#define FLUSHOUTPUT               SerialUSB.flush()
#elif defined ARDUINO
	#define PRINTLN                   Serial.println("")
	#define PRINT_CSTSTR(param)   		Serial.print(F(param))
	#define PRINTLN_CSTSTR(param)			Serial.println(F(param))
	#define PRINT_STR(fmt,param)      Serial.print(param)
	#define PRINTLN_STR(fmt,param)		Serial.println(param)
	#define PRINT_VALUE(fmt,param)    Serial.print(param)
	#define PRINTLN_VALUE(fmt,param)	Serial.println(param)
	#define PRINT_HEX(fmt,param)      Serial.print(param,HEX)
	#define PRINTLN_HEX(fmt,param)		Serial.println(param,HEX)
	#define FLUSHOUTPUT               Serial.flush()
#else
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
#endif

//! MACROS //
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)  // read a bit
#define bitSet(value, bit) ((value) |= (1UL << (bit)))    // set bit to '1'
#define bitClear(value, bit) ((value) &= ~(1UL << (bit))) // set bit to '0'
#define lowByte(value) ((value) & 0x00FF)
#define highByte(value) (((value) >> 8) & 0x00FF)

#ifdef ARDUINO
#define USE_SPI_TRANSACTION          //this is the standard behaviour of library, use SPI Transaction switching
#else
#include <math.h>
#endif

#include <SX127XLT.h>

//use polling on REG_IRQFLAGS instead of DIO0
#define USE_POLLING
//invert IQ in ack transaction
#define INVERTIQ_ON_ACK

#define LTUNUSED(v) (void) (v)       //add LTUNUSED(variable); in functions to avoid compiler warnings

//#define SX127XDEBUG1               //enable level 1 debug messages
//#define SX127XDEBUG2               //enable level 2 debug messages
//#define SX127XDEBUG3               //enable level 3 debug messages
#define SX127XDEBUG_TEST           //enable test debug messages
#define SX127XDEBUGACK               //enable ack transaction debug messages
#define SX127XDEBUGRTS
//#define SX127XDEBUGCAD
//#define DEBUGPHANTOM               //used to set bebuging for Phantom packets
//#define SX127XDEBUGPINS            //enable pin allocation debug messages
//#define DEBUGFSKRTTY               //enable for FSKRTTY debugging 


SX127XLT::SX127XLT()
{  
  _PA_BOOST = false;
  _TXSeqNo = 0;
  _RXSeqNo = 0;

  _lowPowerFctPtr=NULL;    
}

void SX127XLT::setMode(uint8_t modeconfig)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setMode()");
#endif

  uint8_t regdata;

  regdata = modeconfig + _PACKET_TYPE;
  writeRegister(REG_OPMODE, regdata);
}


void SX127XLT::calibrateImage(uint8_t null)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("calibrateImage()");
#endif

  LTUNUSED(null);

  uint8_t regdata, savedmode;
  savedmode = readRegister(REG_OPMODE);
  writeRegister(REG_OPMODE, MODE_SLEEP);
  writeRegister(REG_OPMODE, MODE_STDBY);
  regdata = (readRegister(REG_IMAGECAL) | 0x40);
  writeRegister(REG_IMAGECAL, regdata);
  writeRegister(REG_OPMODE, MODE_SLEEP);
  writeRegister(REG_OPMODE, savedmode);
  delay(10);                              //calibration time 10mS
}

void SX127XLT::printDevice()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printDevice()");
#endif

  switch (_Device)
  {
    case DEVICE_SX1278:
      PRINT_CSTSTR("SX1278");
      break;
    default:
      PRINT_CSTSTR("Unknown Device");

  }
}

//############################################
uint8_t SX127XLT::spi_read(uint32_t data){
  int t = 0;

  t = *((uint32_t *)temp + 0);

  if((t & 0x01) == 1){
		//nhan data
		*((uint32_t *)temp + 1) = 0;
		*((uint32_t *)temp + 0) = data;
		*((uint32_t *)temp + 2) = 1;
    t = *(temp + 0);

		while((t & 0x01) != 1){
      t = *((uint32_t *)temp + 0);
		}
		*((uint32_t *)temp + 1) = 1;
		//printf("nhan duoc: %x\n", t >> 2);
	}
  uint8_t result = t<<2;
  return result;
}

void SX127XLT::spi_write(uint32_t data){
  int t = 0;

  t = *((uint32_t *)temp + 0);

  if((t & 0x01) == 1){
		//nhan data
		*((uint32_t *)temp + 1) = 0;
		*((uint32_t *)temp + 0) = data;
		*((uint32_t *)temp + 2) = 1;
    t = *(temp + 0);

		while((t & 0x01) != 1){
      t = *((uint32_t *)temp + 0);
		}
		*((uint32_t *)temp + 1) = 1;
		//printf("Da truyen: %x\n", data);
	}
}
//############################################


void SX127XLT::writeRegister(uint8_t address, uint8_t value)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("writeRegister()");
#endif

  uint8_t spibuf[2];
  spibuf[0] = address | 0x80;
  spibuf[1] = value;

  uint32_t data;
  data = spibuf[0] | (spibuf[1] << 8);
  spi_write(data);


#ifdef SX127XDEBUG2
  PRINT_CSTSTR("Write register ");
  printHEXByte0x(address);
  PRINT_CSTSTR(" ");
  printHEXByte0x(value);
  PRINTLN;
  FLUSHOUTPUT;
#endif
}


uint8_t SX127XLT::readRegister(uint8_t address)
{
  uint8_t regdata;
	uint8_t spibuf[2];
	spibuf[0] = address & 0x7F;
	spibuf[1] = 0x00;

  uint32_t data;
  data = spibuf[0] | (spibuf[1] << 8);
  regdata = spi_read(data);

  return regdata;
}


void SX127XLT::printOperatingMode()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printOperatingMode()");
#endif

  uint8_t regdata;

  regdata = getOpmode();

  switch (regdata)
  {
    case 0:
      PRINT_CSTSTR("SLEEP");
      break;

    case 1:
      PRINT_CSTSTR("STDBY");
      break;

    case 2:
      PRINT_CSTSTR("FSTX");
      break;

    case 3:
      PRINT_CSTSTR("TX");
      break;

    case 4:
      PRINT_CSTSTR("FSRX");
      break;

    case 5:
      PRINT_CSTSTR("RXCONTINUOUS");
      break;

    case 6:
      PRINT_CSTSTR("RXSINGLE");
      break;

    case 7:
      PRINT_CSTSTR("CAD");
      break;

    default:
      PRINT_CSTSTR("NOIDEA");
      break;
  }
}


void SX127XLT::printOperatingSettings()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printOperatingSettings()");
#endif

  printDevice();
  PRINT_CSTSTR(",");

  printOperatingMode();

  PRINT_CSTSTR(",Version_");
  PRINT_HEX("%X",getVersion());

  PRINT_CSTSTR(",PacketMode_");

  if (getPacketMode())
  {
    PRINT_CSTSTR("LoRa");
  }
  else
  {
    PRINT_CSTSTR("FSK");
  }

  if (getHeaderMode())
  {
    PRINT_CSTSTR(",Implicit");
  }
  else
  {
    PRINT_CSTSTR(",Explicit");
  }

  PRINT_CSTSTR(",CRC_");
  if (getCRCMode())
  {
    PRINT_CSTSTR("On");
  }
  else
  {
    PRINT_CSTSTR("Off");
  }


  PRINT_CSTSTR(",AGCauto_");
  if (getAGC())
  {
    PRINT_CSTSTR("On");
  }
  else
  {
    PRINT_CSTSTR("Off");
  }

  PRINT_CSTSTR(",LNAgain_");
  PRINT_VALUE("%d",getLNAgain());

  PRINT_CSTSTR(",LNAboostHF_");
  if (getLNAboostHF())
  {
    PRINT_CSTSTR("On");
  }
  else
  {
    PRINT_CSTSTR("Off");
  }

  PRINT_CSTSTR(",LNAboostLF_");
  if (getLNAboostLF())
  {
    PRINT_CSTSTR("On");
  }
  else
  {
    PRINT_CSTSTR("Off");
  }

}

void SX127XLT::setTxParams(int8_t txPower, uint8_t rampTime)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setTxParams()");
#endif

	if (txPower < 0)
		return;

  byte RegPaDacReg=(_Device==DEVICE_SX1272)?0x5A:0x4D;
  
  uint8_t param1, param2;

  writeRegister(REG_PARAMP, rampTime);       //Reg 0x0A this is same for SX1272 and SX1278

  if (txPower == 20) {
    //20dBm needs PA_BOOST
    _PA_BOOST=true;
    //set the high output power config with register REG_PA_DAC
    writeRegister(RegPaDacReg, 0x87);
    param2 = OCP_TRIM_150MA;
  }
  else {
    if (txPower > 14)
      txPower=14;
        
    // disable high power output in all other cases
    writeRegister(RegPaDacReg, 0x84);
    
    if (txPower > 10)
      // set RegOcp for OcpOn and OcpTrim
      // 130mA
      param2 = OCP_TRIM_130MA;
    else
    if (txPower > 5)
      // 100mA
      param2 = OCP_TRIM_100MA;
    else
      param2 = OCP_TRIM_80MA;
  }
  
  if (_Device == DEVICE_SX1272) {
    if (_PA_BOOST) {
      param1 = txPower - 2;
      // we set the PA_BOOST pin
      param1 = param1 | 0b10000000;
    }
    else
      param1 = txPower + 1;

    writeRegister(REG_PACONFIG, param1);	// Setting output power value
  }
  else {
    // for the SX1276
    uint8_t pmax=15;

    // then Pout = Pmax-(15-_power[3:0]) if  PaSelect=0 (RFO pin for +14dBm)
    // so L=3dBm; H=7dBm; M=15dBm (but should be limited to 14dBm by RFO pin)

    // and Pout = 17-(15-_power[3:0]) if  PaSelect=1 (PA_BOOST pin for +14dBm)
    // so x= 14dBm (PA);
    // when p=='X' for 20dBm, value is 0x0F and RegPaDacReg=0x87 so 20dBm is enabled

    if (_PA_BOOST) {
      param1 = txPower - 17 + 15;
      // we set the PA_BOOST pin
      param1 = param1 | 0b10000000;
    }
    else
      param1 = txPower - pmax + 15;

    // set MaxPower to 7 -> Pmax=10.8+0.6*MaxPower [dBm] = 15
    param1 = param1 | 0b01110000;

    writeRegister(REG_PACONFIG, param1);
  }
  
  writeRegister(REG_OCP, param2);
}    

void SX127XLT::setPacketParams(uint16_t packetParam1, uint8_t  packetParam2, uint8_t packetParam3, uint8_t packetParam4, uint8_t packetParam5)
{
  //format is PreambleLength, Fixed\Variable length packets, Packetlength, CRC mode, IQ mode

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("SetPacketParams()");
#endif

  uint8_t preambleMSB, preambleLSB, regdata;

  //*******************************************************
  //These changes are the same for SX1272 and SX127X
  //PreambleLength reg 0x20, 0x21
  preambleMSB = packetParam1 >> 8;
  preambleLSB = packetParam1 & 0xFF;
  writeRegister(REG_PREAMBLEMSB, preambleMSB);
  writeRegister(REG_PREAMBLELSB, preambleLSB);

  //TX Packetlength reg 0x22
  writeRegister(REG_PAYLOADLENGTH, packetParam3);                //when in implicit mode, this is used as receive length also

  if (packetParam5==LORA_IQ_INVERTED)
  	invertIQ(true);
  else	
  	invertIQ(false);

  //CRC mode
  _UseCRC = packetParam4;                                       //save CRC status

  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272
    //Fixed\Variable length packets
    regdata = ( (readRegister(REG_MODEMCONFIG1)) & (~READ_IMPLCIT_AND_X)); //mask off bit 0
    writeRegister(REG_MODEMCONFIG1, (regdata + packetParam2));             //write out with bit 0 set appropriatly

    //CRC on payload
    regdata = ( (readRegister(REG_MODEMCONFIG2)) & (~READ_HASCRC_AND_X));  //mask off all bits bar CRC on - bit 2
    writeRegister(REG_MODEMCONFIG2, (regdata + (packetParam4 << 2)));      //write out with CRC bit 2 set appropriatly
  }
  else
  {
    //for SX1272
    //Fixed\Variable length packets
    regdata = ( (readRegister(REG_MODEMCONFIG1)) & (~READ_IMPLCIT_AND_2)); //mask off bit 2
    writeRegister(REG_MODEMCONFIG1, (regdata + (packetParam2 << 2)));      //write out with bit 2 set appropriatly

    //CRC on payload
    regdata = ( (readRegister(REG_MODEMCONFIG1)) & (~READ_HASCRC_AND_2));  //mask of all bits bar CRC on - bit 1
    writeRegister(REG_MODEMCONFIG1, (regdata + (packetParam4 << 1)));      //write out with CRC bit 1 set appropriatly
  }
}


void SX127XLT::setModulationParams(uint8_t modParam1, uint8_t modParam2, uint8_t  modParam3, uint8_t  modParam4)
{
  //order is SpreadingFactor, Bandwidth, CodeRate, Optimisation

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setModulationParams()");
#endif

  uint8_t regdata, bw;

  //Spreading factor - same for SX1272 and SX127X - reg 0x1D
  regdata = (readRegister(REG_MODEMCONFIG2) & (~READ_SF_AND_X));
  writeRegister(REG_MODEMCONFIG2, (regdata + (modParam1 << 4)));
  
  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272

    //bandwidth
    regdata = (readRegister(REG_MODEMCONFIG1) & (~READ_BW_AND_X));
    writeRegister(REG_MODEMCONFIG1, (regdata + modParam2));

    //Coding rate
    regdata = (readRegister(REG_MODEMCONFIG1) & (~READ_CR_AND_X));
    writeRegister(REG_MODEMCONFIG1, (regdata + (modParam3)));

    //Optimisation
    if (modParam4 == LDRO_AUTO)
    {
      modParam4 = returnOptimisation(modParam2, modParam1);
    }

    regdata = (readRegister(REG_MODEMCONFIG3) & (~READ_LDRO_AND_X));
    writeRegister(REG_MODEMCONFIG3, (regdata + (modParam4 << 3)));

  }
  else
  {
    //for SX1272
    //bandwidth
    regdata = (readRegister(REG_MODEMCONFIG1) & (~READ_BW_AND_2));   //value will be LORA_BW_500 128, LORA_BW_250 64, LORA_BW_125 0

    switch (modParam2)
    {
      case LORA_BW_125:
        bw = 0x00;
        break;

      case LORA_BW_500:
        bw = 0x80;
        break;

      case LORA_BW_250:
        bw = 0x40;
        break;

      default:
        bw = 0x00;                       //defaults to LORA_BW_125
    }

    writeRegister(REG_MODEMCONFIG1, (regdata + bw));

    //Coding rate
    regdata = (readRegister(REG_MODEMCONFIG1) & (~READ_CR_AND_2));
    writeRegister(REG_MODEMCONFIG1, (regdata + (modParam3 << 2)));

    //Optimisation
    if (modParam4 == LDRO_AUTO)
    {
      modParam4 = returnOptimisation(modParam2, modParam1);
    }

    regdata = (readRegister(REG_MODEMCONFIG1) & (~READ_LDRO_AND_2));
    writeRegister(REG_MODEMCONFIG1, (regdata + modParam4));

  }
  
  if ( modParam1 == LORA_SF6)
  {
  writeRegister(REG_DETECTOPTIMIZE, 0x05);
  writeRegister(REG_DETECTIONTHRESHOLD, 0x0C);
  }
  else
  {
  writeRegister(REG_DETECTOPTIMIZE, 0x03);
  writeRegister(REG_DETECTIONTHRESHOLD, 0x0A);
  }
 
}


void SX127XLT::setRfFrequency(uint64_t freq64, int32_t offset)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setRfFrequency()");
#endif

  savedFrequency = freq64;
  savedOffset = offset;
  
  freq64 = freq64 + offset;
  freq64 = ((uint64_t)freq64 << 19) / 32000000;
  _freqregH = freq64 >> 16;
  _freqregM = freq64 >> 8;
  _freqregL = freq64;
    
  writeRegister(REG_FRMSB, _freqregH);
  writeRegister(REG_FRMID, _freqregM);
  writeRegister(REG_FRLSB, _freqregL);
}


uint32_t SX127XLT::getFreqInt()
{
  //get the current set LoRa device frequency, return as long integer
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getFreqInt()");
#endif

  uint8_t Msb, Mid, Lsb;
  uint32_t uinttemp;
  float floattemp;
  Msb = readRegister(REG_FRMSB);
  Mid = readRegister(REG_FRMID);
  Lsb = readRegister(REG_FRLSB);
  floattemp = ((Msb * 0x10000ul) + (Mid * 0x100ul) + Lsb);
  floattemp = ((floattemp * 61.03515625) / 1000000ul);
  uinttemp = (uint32_t)(floattemp * 1000000);
  return uinttemp;
}

int32_t SX127XLT::getFrequencyErrorHz()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getFrequencyErrorHz()");
#endif

  uint16_t msb, mid, lsb;
  int16_t freqerr;
  uint8_t bw;
  float bwconst;

  msb = readRegister(REG_FEIMSB);
  mid = readRegister(REG_FEIMID);
  lsb = readRegister(REG_FEILSB);

  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272
    bw = (readRegister(REG_MODEMCONFIG1)) & (0xF0);

    switch (bw)
    {
      case LORA_BW_125:           //ordered with most commonly used first
        bwconst = 2.097;
        break;

      case LORA_BW_062:
        bwconst = 1.049;
        break;

      case LORA_BW_041:
        bwconst = 0.6996;
        break;

      case LORA_BW_007:
        bwconst = 0.1309;
        break;

      case LORA_BW_010:
        bwconst = 0.1745;
        break;

      case LORA_BW_015:
        bwconst = 0.2617;
        break;

      case LORA_BW_020:
        bwconst = 0.3490;
        break;

      case LORA_BW_031:
        bwconst = 0.5234;
        break;

      case LORA_BW_250:
        bwconst = 4.194;
        break;

      case LORA_BW_500:
        bwconst = 8.389;
        break;

      default:
        bwconst = 0x00;
    }
  }
  else
  {
    //for the SX1272
    bw = (readRegister(REG_MODEMCONFIG1)) & (0xF0);

    switch (bw)
    {
      case 0:                   //this is LORA_BW_125
        bwconst = 2.097;
        break;

      case 64:                  //this is LORA_BW_250
        bwconst = 4.194;
        break;

      case 128:                 //this is LORA_BW_250
        bwconst = 8.389;
        break;

      default:
        bwconst = 0x00;
    }


  }

  freqerr = msb << 12;          //shift lower 4 bits of msb into high 4 bits of freqerr
  mid = (mid << 8) + lsb;
  mid = (mid >> 4);
  freqerr = freqerr + mid;

  freqerr = (int16_t) (freqerr * bwconst);

  return freqerr;
}


void SX127XLT::setTx(uint32_t timeout)
{
  //There is no TX timeout function for SX127X, the value passed is ignored

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setTx()");
#endif

  LTUNUSED(timeout);                                  //unused TX timeout passed for compatibility with SX126x, SX128x

  clearIrqStatus(IRQ_RADIO_ALL);

  writeRegister(REG_OPMODE, (MODE_TX + 0x88));    //TX on LoRa mode
}


void SX127XLT::setRx(uint32_t timeout)
{
  //no timeout in this routine, left in for compatibility
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setRx()");
#endif

  LTUNUSED(timeout);

  clearIrqStatus(IRQ_RADIO_ALL);

  writeRegister(REG_OPMODE, (MODE_RXCONTINUOUS + 0x80));    //RX on LoRa mode
}

void SX127XLT::setHighSensitivity()
{
  //set Boosted LNA for HF mode

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setHighSensitivity()");
#endif

  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272
    writeRegister(REG_LNA, 0x3B ); //at HF 150% LNA current.
  }
  else
  {
    //for SX1272
    writeRegister(REG_LNA, 0x23 ); //at HF 10% LNA current.
  }
}


uint8_t SX127XLT::getAGC()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getAGC()");
#endif

  uint8_t regdata;

  if (_Device != DEVICE_SX1272)
  {
    regdata = readRegister(REG_MODEMCONFIG3);
    regdata = (regdata & READ_AGCAUTO_AND_X);
  }
  else
  {
    regdata = readRegister(REG_MODEMCONFIG2);
    regdata = (regdata & READ_AGCAUTO_AND_2);
  }

  return (regdata >> 2);
}


uint8_t SX127XLT::getLNAgain()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getLNAgain()");
#endif

  uint8_t regdata;
  regdata = readRegister(REG_LNA);
  regdata = (regdata & READ_LNAGAIN_AND_X);
  return (regdata >> 5);
}


uint8_t SX127XLT::getCRCMode()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getCRCMode()");
#endif

  uint8_t regdata;

  regdata = readRegister(REG_MODEMCONFIG2);

  if (_Device != DEVICE_SX1272)
  {
    regdata = readRegister(REG_MODEMCONFIG2);
    regdata = ((regdata & READ_HASCRC_AND_X) >> 2);
  }
  else
  {
    regdata = readRegister(REG_MODEMCONFIG1);
    regdata = ((regdata & READ_HASCRC_AND_2) >> 1);
  }

  return regdata;
}


uint8_t SX127XLT::getHeaderMode()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getHeaderMode()");
#endif

  uint8_t regdata;

  regdata = readRegister(REG_MODEMCONFIG1);

  if (_Device != DEVICE_SX1272)
  {
    regdata = (regdata & READ_IMPLCIT_AND_X);
  }
  else
  {
    regdata = ((regdata & READ_IMPLCIT_AND_2) >> 2);
  }

  return regdata;
}


uint8_t SX127XLT::getLNAboostLF()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getLNAboostLF");
#endif

  uint8_t regdata;

  regdata = readRegister(REG_LNA);

  if (_Device != DEVICE_SX1272)
  {
    regdata = (regdata & READ_LNABOOSTLF_AND_X);
  }
  else
  {
    regdata = (regdata & READ_LNABOOSTLF_AND_X);
  }

  return (regdata >> 3);
}


uint8_t SX127XLT::getLNAboostHF()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getLNAboostHF()");
#endif

  uint8_t regdata;

  regdata = readRegister(REG_LNA);

  if (_Device != DEVICE_SX1272)
  {
    regdata = (regdata & READ_LNABOOSTHF_AND_X);
  }
  else
  {
    regdata = (regdata & READ_LNABOOSTHF_AND_X);
  }

  return regdata;
}


uint8_t SX127XLT::getOpmode()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getOpmode()");
#endif

  uint8_t regdata;

  regdata = (readRegister(REG_OPMODE) & READ_OPMODE_AND_X);

  return regdata;
}


uint8_t SX127XLT::getPacketMode()
{
  //its either LoRa or FSK

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getPacketMode()");
#endif

  uint8_t regdata;

  regdata = (readRegister(REG_OPMODE) & READ_RANGEMODE_AND_X);

  return (regdata >> 7);
}


uint8_t SX127XLT::readRXPacketL()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readRXPacketL()");
#endif

  _RXPacketL = readRegister(REG_RXNBBYTES);
  return _RXPacketL;
}

int8_t SX127XLT::readPacketRSSI()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readPacketRSSI()");
#endif

  _PacketRSSI = readRegister(REG_PKTRSSIVALUE);
  
  readPacketSNR();

  if( _PacketSNR < 0 )
  {
      // added by C. Pham, using Semtech SX1272 rev3 March 2015
      // for SX1272 we use -139, for SX1276, we use -157
      // then for SX1276 when using low-frequency (i.e. 433MHz band) then we use -164
      _PacketRSSI = -(OFFSET_RSSI+(_Device==DEVICE_SX1272?0:18)+(savedFrequency<436000000?7:0)) + (double)_PacketRSSI + (double)_PacketSNR*0.25;
  }
  else
  {
      _PacketRSSI= -(OFFSET_RSSI+(_Device==DEVICE_SX1272?0:18)+(savedFrequency<436000000?7:0)) + (double)_PacketRSSI*16.0/15.0;
  }
  
  return _PacketRSSI;
}

int8_t SX127XLT::readPacketSNR()

{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readPacketSNR()");
#endif

  uint8_t regdata;
  regdata = readRegister(REG_PKTSNRVALUE);

  if (regdata > 127)
  {
    _PacketSNR  =  ((255 - regdata) / 4) * (-1);
  }
  else
  {
    _PacketSNR  = regdata / 4;
  }

  return _PacketSNR;
}


uint8_t SX127XLT::readRXPacketType()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readRXPacketType()");
#endif

  return _RXPacketType;
}


uint8_t SX127XLT::readRXDestination()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readRXDestination()");
#endif
  return _RXDestination;
}


uint8_t SX127XLT::readRXSource()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readRXSource()");
#endif

  return _RXSource;
}


void SX127XLT::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setBufferBaseAddress()");
#endif

  writeRegister(REG_FIFOTXBASEADDR, txBaseAddress);
  writeRegister(REG_FIFORXBASEADDR, rxBaseAddress);
}


void SX127XLT::setPacketType(uint8_t packettype )
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setPacketType()");
#endif
  uint8_t regdata;

  regdata = (readRegister(REG_OPMODE) & 0x7F);           //save all register bits bar the LoRa\FSK bit 7

  if (packettype == PACKET_TYPE_LORA)
  {
    _PACKET_TYPE = PACKET_TYPE_LORA;
    writeRegister(REG_OPMODE, 0x80);                     //REG_OPMODE, need to set to sleep mode before configure for LoRa mode
    writeRegister(REG_OPMODE, (regdata + 0x80));         //back to original standby mode with LoRa set
  }
  else
  {
    _PACKET_TYPE = PACKET_TYPE_GFSK;
    writeRegister(REG_OPMODE, 0x00);                     //REG_OPMODE, need to set to sleep mode before configure for FSK
    writeRegister(REG_OPMODE, regdata);                  //back to original standby mode with LoRa set
  }

}


void SX127XLT::clearIrqStatus(uint16_t irqMask)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("clearIrqStatus()");
#endif

  uint8_t masklsb;
  uint16_t maskmsb;
  _IRQmsb = _IRQmsb & 0xFF00;                                 //make sure _IRQmsb does not have LSB bits set.
  masklsb = (irqMask & 0xFF);
  maskmsb = (irqMask & 0xFF00);
  writeRegister(REG_IRQFLAGS, masklsb);                       //clear standard IRQs
  _IRQmsb = (_IRQmsb & (~maskmsb));                           //only want top bits set.
}


uint16_t SX127XLT::readIrqStatus()
{
#ifdef SX127XDEBUG1
  PRINT_CSTSTR("readIrqStatus()");
#endif

  bool packetHasCRC;

  packetHasCRC = (readRegister(REG_HOPCHANNEL) & 0x40);        //read the packet has CRC bit in RegHopChannel


#ifdef DEBUGPHANTOM
  PRINT_CSTSTR("PacketHasCRC = ");
  PRINTLN_VALUE("%d",packetHasCRC);
  PRINT_CSTSTR("_UseCRC = ");
  PRINTLN_VALUE("%d",_UseCRC);
#endif
  
  if ( !packetHasCRC && _UseCRC )                                //check if packet header indicates no CRC on packet, byt use CRC set
  {
   bitSet(_IRQmsb, 10);                                          //flag the phantom packet, set bit 10
  }

  return (readRegister(REG_IRQFLAGS) + _IRQmsb);
}


void SX127XLT::setDioIrqParams(uint16_t irqMask, uint16_t dio0Mask, uint16_t dio1Mask, uint16_t dio2Mask)
{
  //note the irqmask contains the bit values of the interrupts that are allowed, so for all interrupts value is 0xFFFF

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setDioIrqParams()");
#endif

  uint8_t mask0, mask1, mask2;

  LTUNUSED(dio2Mask);                      //variable left in call for compatibility with other libraries


  switch (dio0Mask)
  {
    case IRQ_RX_DONE:
      mask0 = 0;
      break;

    case IRQ_TX_DONE:
      mask0 = 0x40;
      break;

    case IRQ_CAD_DONE:
      mask0 = 0x80;
      break;

    default:
      mask0 = 0x0C;
  }

  switch (dio1Mask)               //for compatibility with other libraries IRQ_RX_TIMEOUT = IRQ_RX_TX_TIMEOUT
  {
    case IRQ_RX_TIMEOUT:
      mask1 = 0;
      break;

    case IRQ_FSHS_CHANGE_CHANNEL:
      mask1 = 0x10;
      break;

    case IRQ_CAD_ACTIVITY_DETECTED:
      mask1 = 0x20;
      break;

    default:
      mask1 = 0x30;
  }

  mask2 = 0x00;                  //is always IRQ_FSHS_CHANGE_CHANNEL

  writeRegister(REG_IRQFLAGSMASK, ~irqMask);
  writeRegister(REG_DIOMAPPING1, (mask0 + mask1 + mask2));
}


void SX127XLT::printIrqStatus()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printIrqStatus()");
#endif

  uint8_t _IrqStatus;
  _IrqStatus = readIrqStatus();

  //0x01
  if (_IrqStatus & IRQ_CAD_ACTIVITY_DETECTED)
  {
    PRINT_CSTSTR(",IRQ_CAD_ACTIVITY_DETECTED");
  }

  //0x02
  if (_IrqStatus & IRQ_FSHS_CHANGE_CHANNEL)
  {
    PRINT_CSTSTR(",IRQ_FSHS_CHANGE_CHANNEL");
  }

  //0x04
  if (_IrqStatus & IRQ_CAD_DONE)
  {
    PRINT_CSTSTR(",IRQ_CAD_DONE");
  }

  //0x08
  if (_IrqStatus & IRQ_TX_DONE)
  {
    PRINT_CSTSTR(",IRQ_TX_DONE");
  }

  //0x10
  if (_IrqStatus & IRQ_HEADER_VALID)
  {
    PRINT_CSTSTR(",IRQ_HEADER_VALID");
  }

  //0x20
  if (_IrqStatus & IRQ_CRC_ERROR)
  {
    PRINT_CSTSTR(",IRQ_CRC_ERROR");
  }

  //0x40
  if (_IrqStatus & IRQ_RX_DONE)
  {
    PRINT_CSTSTR(",IRQ_RX_DONE");
  }

  //0x80
  if (_IrqStatus & IRQ_RX_TIMEOUT )
  {
    PRINT_CSTSTR(",IRQ_RX_TIMEOUT ");
  }

  if (_IRQmsb & IRQ_TX_TIMEOUT  )
  {
    PRINT_CSTSTR(",TX_TIMEOUT");
  }

  if (_IRQmsb & IRQ_RX_TIMEOUT  )
  {
    PRINT_CSTSTR(",RX_TIMEOUT");
  }

  if (_IRQmsb & IRQ_NO_PACKET_CRC  )
  {
    PRINT_CSTSTR(",NO_PACKET_CRC");
  }

}

void SX127XLT::printHEXByte0x(uint8_t temp)
{
  //print a byte, adding 0x

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printHEXByte0x()");
#endif

  PRINT_CSTSTR("0x");
  if (temp < 0x10)
  {
    PRINT_CSTSTR("0");
  }
  PRINT_HEX("%X",temp);
}


uint8_t SX127XLT::receive(uint8_t *rxbuffer, uint8_t size, uint32_t rxtimeout, uint8_t wait )
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("receive()");
#endif

  uint16_t index;
  uint32_t endtimeoutmS;
  uint8_t regdata;

  setMode(MODE_STDBY_RC);
  regdata = readRegister(REG_FIFORXBASEADDR);                               //retrieve the RXbase address pointer
  writeRegister(REG_FIFOADDRPTR, regdata);                                  //and save in FIFO access ptr

  setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_HEADER_VALID), 0, 0);  //set for IRQ on RX done
  setRx(0);                                                                //no actual RX timeout in this function
  
  if (!wait)
  {
    return 0;                                                              //not wait requested so no packet length to pass
  }

  if (rxtimeout == 0)
  {
#ifdef USE_POLLING
    index = readRegister(REG_IRQFLAGS);

    //poll the irq register for ValidHeader, bit 4
    while ((bitRead(index, 4) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
        // adding this small delay decreases the CPU load of the lora_gateway process to 4~5% instead of nearly 100%
        // suggested by rertini (https://github.com/CongducPham/LowCostLoRaGw/issues/211)
        // tests have shown no significant side effects
        delay(1);        
      }    
    
    //_RXTimestamp start when a valid header has been received
    _RXTimestamp=millis();
    
    //poll the irq register for RXDone, bit 6
    while ((bitRead(index, 6) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }
  else
  {
    endtimeoutmS = (millis() + rxtimeout);
#ifdef USE_POLLING

#ifdef SX127XDEBUG1
  	PRINTLN_CSTSTR("RXDone using polling");
#endif 
    index = readRegister(REG_IRQFLAGS);

    //poll the irq register for ValidHeader, bit 4
    while ((bitRead(index, 4) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
        delay(1);        
      }    

		if (bitRead(index, 4)!=0) {
    	//_RXTimestamp start when a valid header has been received
    	_RXTimestamp=millis();		
#ifdef SX127XDEBUG1		
  		PRINTLN_CSTSTR("Valid header detected");		
#endif

			//update endtimeoutmS to avoid timeout at the middle of a packet reception
			endtimeoutmS = (_RXTimestamp + 10000);
    }
              
    //poll the irq register for RXDone, bit 6
    while ((bitRead(index, 6) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }

  //_RXDoneTimestamp start when the packet been fully received  
  _RXDoneTimestamp=millis();

#ifdef SX127XDEBUG1
  PRINTLN_VALUE("%ld", _RXTimestamp);
	PRINTLN_VALUE("%ld", _RXDoneTimestamp);  		
#endif
    
  setMode(MODE_STDBY_RC);                                            //ensure to stop further packet reception

#ifdef USE_POLLING
  if (bitRead(index, 6) == 0)                                        
#endif  
  {
    _IRQmsb = IRQ_RX_TIMEOUT;
#ifdef SX127XDEBUG1    
    PRINTLN_CSTSTR("RX timeout");
#endif     
    return 0;
  }

  if ( readIrqStatus() != (IRQ_RX_DONE + IRQ_HEADER_VALID) )
  {
    return 0;                                                                //no RX done and header valid only, could be CRC error
  }

  _RXPacketL = readRegister(REG_RXNBBYTES);

  if (_RXPacketL > size)                      //check passed buffer is big enough for packet
  {
    _RXPacketL = size;                        //truncate packet if not enough space in passed buffer
  }

#ifdef USE_SPI_TRANSACTION   //to use SPI_TRANSACTION enable define at beginning of CPP file 
  SPI.beginTransaction(SPISettings(LTspeedMaximum, LTdataOrder, LTdataMode));
#endif

  //digitalWrite(_NSS, LOW);                    //start the burst read
  *((uint32_t *)temp + 1) = 0;

#if defined ARDUINO || defined USE_ARDUPI  
  SPI.transfer(REG_FIFO);
#endif  

  for (index = 0; index < _RXPacketL; index++)
  {
#if defined ARDUINO || defined USE_ARDUPI
    regdata = SPI.transfer(0);
#else  
    regdata = readRegister(REG_FIFO);
#endif    
    rxbuffer[index] = regdata;
  }
  //digitalWrite(_NSS, HIGH);
  *((uint32_t *)temp + 1) = 1;

#ifdef USE_SPI_TRANSACTION
  SPI.endTransaction();
#endif

  return _RXPacketL;                           //so we can check for packet having enough buffer space
}


uint8_t SX127XLT::receiveAddressed(uint8_t *rxbuffer, uint8_t size, uint32_t rxtimeout, uint8_t wait)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("receiveAddressed()");
#endif

  uint16_t index;
  uint32_t endtimeoutmS;
  uint8_t regdata;

  setMode(MODE_STDBY_RC);
  regdata = readRegister(REG_FIFORXBASEADDR);                                //retrieve the RXbase address pointer
  writeRegister(REG_FIFOADDRPTR, regdata);                                   //and save in FIFO access ptr

  setDioIrqParams(IRQ_RADIO_ALL, (IRQ_RX_DONE + IRQ_HEADER_VALID), 0, 0);   //set for IRQ on RX done
  setRx(0);                                                                 //no actual RX timeout in this function
  
  if (!wait)
  {
    return 0;                                                              //not wait requested so no packet length to pass
  }

  if (rxtimeout == 0)
  {
#ifdef USE_POLLING
    index = readRegister(REG_IRQFLAGS);

    //poll the irq register for ValidHeader, bit 4
    while ((bitRead(index, 4) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
        // adding this small delay decreases the CPU load of the lora_gateway process to 4~5% instead of nearly 100%
        // suggested by rertini (https://github.com/CongducPham/LowCostLoRaGw/issues/211)
        // tests have shown no significant side effects
        delay(1);        
      }    

    //_RXTimestamp start when a valid header has been received
    _RXTimestamp=millis();
        
    //poll the irq register for RXDone, bit 6
    while ((bitRead(index, 6) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }
  else
  {
    endtimeoutmS = (millis() + rxtimeout);
#ifdef USE_POLLING

#ifdef SX127XDEBUG1
  	PRINTLN_CSTSTR("RXDone using polling");
#endif 
    index = readRegister(REG_IRQFLAGS);
		
    //poll the irq register for ValidHeader, bit 4
    while ((bitRead(index, 4) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
        delay(1);        
      }    

		if (bitRead(index, 4)!=0) {
    	//_RXTimestamp start when a valid header has been received
    	_RXTimestamp=millis();		
#ifdef SX127XDEBUG1
  		PRINTLN_CSTSTR("Valid header detected"); 		
#endif

			//update endtimeoutmS to avoid timeout at the middle of a packet reception
			endtimeoutmS = (_RXTimestamp + 10000);
    }
              
    //poll the irq register for RXDone, bit 6
    while ((bitRead(index, 6) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }

  //_RXDoneTimestamp start when the packet been fully received  
  _RXDoneTimestamp=millis();

#ifdef SX127XDEBUG1
  PRINTLN_VALUE("%ld", _RXTimestamp);
	PRINTLN_VALUE("%ld", _RXDoneTimestamp);  		
#endif
  
  setMode(MODE_STDBY_RC);                                            //ensure to stop further packet reception

#ifdef USE_POLLING
  if (bitRead(index, 6) == 0)
#endif  
  {
    _IRQmsb = IRQ_RX_TIMEOUT;
#ifdef SX127XDEBUG1    
    PRINTLN_CSTSTR("RX timeout");
#endif    
    return 0;
  }

  if ( readIrqStatus() != (IRQ_RX_DONE + IRQ_HEADER_VALID) )
  {
    return 0;                       //no RX done and header valid only, could be CRC error
  }

  _RXPacketL = readRegister(REG_RXNBBYTES);
  
#ifdef SX127XDEBUG1
  PRINT_CSTSTR("Receive ");
  PRINT_VALUE("%d", _RXPacketL);
  PRINTLN_CSTSTR(" bytes");     		
#endif  

#ifdef USE_SPI_TRANSACTION   //to use SPI_TRANSACTION enable define at beginning of CPP file 
  SPI.beginTransaction(SPISettings(LTspeedMaximum, LTdataOrder, LTdataMode));
#endif

  *((uint32_t *)temp + 1) = 0;                    //start the burst read

#if defined ARDUINO || defined USE_ARDUPI
  SPI.transfer(REG_FIFO);
  // we read our header
  _RXDestination = SPI.transfer(0);
  _RXPacketType = SPI.transfer(0);
  _RXSource = SPI.transfer(0);
  _RXSeqNo = SPI.transfer(0);  
#else  
  // we read our header
  _RXDestination = readRegister(REG_FIFO);
  _RXPacketType = readRegister(REG_FIFO);
  _RXSource = readRegister(REG_FIFO);
  _RXSeqNo = readRegister(REG_FIFO);
#endif  

#ifdef SX127XDEBUG1
	PRINT_CSTSTR("dest: ");
	PRINTLN_VALUE("%d", _RXDestination);
	PRINT_CSTSTR("ptype: ");
	PRINTLN_VALUE("%d", _RXPacketType);		
	PRINT_CSTSTR("src: ");
	PRINTLN_VALUE("%d", _RXSource);
	PRINT_CSTSTR("seq: ");
	PRINTLN_VALUE("%d", _RXSeqNo);				 
#endif	
  
  //the header is not passed to the user
  _RXPacketL=_RXPacketL-HEADER_SIZE;
  
  if (_RXPacketL > size)                      //check passed buffer is big enough for packet
  {
    _RXPacketL = size;                          //truncate packet if not enough space in passed buffer
  }  

  for (index = 0; index < _RXPacketL; index++)
  {
#if defined ARDUINO || defined USE_ARDUPI
    regdata = SPI.transfer(0);
#else  
    regdata = readRegister(REG_FIFO);
#endif
    rxbuffer[index] = regdata;
  }
  *((uint32_t *)temp + 1) = 1;

#ifdef USE_SPI_TRANSACTION
  SPI.endTransaction();
#endif

  //sender request an ACK
	if ( (_RXPacketType & PKT_FLAG_ACK_REQ) && (_RXDestination==_DevAddr) ) {
		uint8_t TXAckPacketL;
		const uint8_t TXBUFFER_SIZE=3;
		uint8_t TXBUFFER[TXBUFFER_SIZE];
		
#ifdef SX127XDEBUGACK
  	PRINT_CSTSTR("ACK requested by ");
  	PRINTLN_VALUE("%d", _RXSource);
  	PRINTLN_CSTSTR("ACK transmission turnaround safe wait...");
#endif
#ifdef INVERTIQ_ON_ACK
#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("invert IQ for ack transmission");
#endif
		invertIQ(true);
#endif			
		delay(100);
		TXBUFFER[0]=2; //length
		TXBUFFER[1]=0; //RFU
		TXBUFFER[2]=readRegister(REG_PKTSNRVALUE); //SNR of received packet
		
		uint8_t saveTXSeqNo=_TXSeqNo;
		_TXSeqNo=_RXSeqNo;
		// use -1 as txpower to use default setting
		TXAckPacketL=transmitAddressed(TXBUFFER, 3, PKT_TYPE_ACK, _RXSource, _DevAddr, 10000, -1, WAIT_TX);	

#ifdef SX127XDEBUGACK
		if (TXAckPacketL)
  		PRINTLN_CSTSTR("ACK sent");
  	else	
  		PRINTLN_CSTSTR("error when sending ACK"); 
#endif
#ifdef INVERTIQ_ON_ACK
#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("set back IQ to normal");
#endif
		invertIQ(false);
#endif		
		_TXSeqNo=saveTXSeqNo;
	}

  return _RXPacketL;                           //so we can check for packet having enough buffer space
}

uint8_t SX127XLT::transmit(uint8_t *txbuffer, uint8_t size, uint32_t txtimeout, int8_t txpower, uint8_t wait)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("transmit()");
#endif

  uint8_t index, ptr;
  uint8_t bufferdata;
  uint32_t endtimeoutmS;

  if (size == 0)
  {
    return false;
  }

  setMode(MODE_STDBY_RC);
  ptr = readRegister(REG_FIFOTXBASEADDR);       //retrieve the TXbase address pointer
  writeRegister(REG_FIFOADDRPTR, ptr);          //and save in FIFO access ptr

#ifdef USE_SPI_TRANSACTION                   //to use SPI_TRANSACTION enable define at beginning of CPP file 
  SPI.beginTransaction(SPISettings(LTspeedMaximum, LTdataOrder, LTdataMode));
#endif

  *((uint32_t *)temp + 1) = 0;
  
#if defined ARDUINO || defined USE_ARDUPI  
  SPI.transfer(WREG_FIFO);
#endif

  for (index = 0; index < size; index++)
  {
    bufferdata = txbuffer[index];
#if defined ARDUINO || defined USE_ARDUPI    
    SPI.transfer(bufferdata);
#else
		writeRegister(WREG_FIFO, bufferdata);
#endif    
  }
  *((uint32_t *)temp + 1) = 1;

#ifdef USE_SPI_TRANSACTION
  SPI.endTransaction();
#endif

  _TXPacketL = size;
  writeRegister(REG_PAYLOADLENGTH, _TXPacketL);

  setTxParams(txpower, RADIO_RAMP_DEFAULT);            //TX power and ramp time

  setDioIrqParams(IRQ_RADIO_ALL, IRQ_TX_DONE, 0, 0);   //set for IRQ on TX done on first DIO pin
  setTx(0);                                            //TX timeout is not handled in setTX()

  // increment packet sequence number
  _TXSeqNo++;

  if (!wait)
  {
    return _TXPacketL;
  }

  if (txtimeout == 0)
  {
#ifdef USE_POLLING
    index = readRegister(REG_IRQFLAGS);
    //poll the irq register for TXDone, bit 3
    while ((bitRead(index, 3) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }
  else
  {
    endtimeoutmS = (millis() + txtimeout);
#ifdef USE_POLLING

#ifdef SX127XDEBUG1
  	PRINTLN_CSTSTR("TXDone using polling");
#endif  
    index = readRegister(REG_IRQFLAGS);
    //poll the irq register for TXDone, bit 3
    while ((bitRead(index, 3) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }

  setMode(MODE_STDBY_RC);                                            //ensure we leave function with TX off

#ifdef USE_POLLING
  if (bitRead(index, 3) == 0)
#endif  
  {
    _IRQmsb = IRQ_TX_TIMEOUT;
    return 0;
  }

  return _TXPacketL;                                                     //no timeout, so TXdone must have been set
}


uint8_t SX127XLT::transmitAddressed(uint8_t *txbuffer, uint8_t size, char txpackettype, char txdestination, char txsource, uint32_t txtimeout, int8_t txpower, uint8_t wait )
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("transmitAddressed()");
#endif

  uint8_t index, ptr;
  uint8_t bufferdata;
  uint32_t endtimeoutmS;

  if (size == 0)
  {
    return false;
  }

  setMode(MODE_STDBY_RC);
  ptr = readRegister(REG_FIFOTXBASEADDR);         //retrieve the TXbase address pointer
  writeRegister(REG_FIFOADDRPTR, ptr);            //and save in FIFO access ptr

#ifdef USE_SPI_TRANSACTION                     //to use SPI_TRANSACTION enable define at beginning of CPP file 
  SPI.beginTransaction(SPISettings(LTspeedMaximum, LTdataOrder, LTdataMode));
#endif

  *((uint32_t *)temp + 1) = 0;

  // we insert our header
#if defined ARDUINO || defined USE_ARDUPI  
  SPI.transfer(WREG_FIFO);
  SPI.transfer(txdestination);                    //Destination node
  SPI.transfer(txpackettype);                     //Write the packet type
  SPI.transfer(txsource);                         //Source node
  SPI.transfer(_TXSeqNo);                           //Sequence number    
#else  
  writeRegister(WREG_FIFO, txdestination);
  writeRegister(WREG_FIFO, txpackettype);
  writeRegister(WREG_FIFO, txsource);
  writeRegister(WREG_FIFO, _TXSeqNo);
#endif  
  
  _TXPacketL = HEADER_SIZE + size;                      //we have added 4 header bytes to size

  for (index = 0; index < size; index++)
  {
    bufferdata = txbuffer[index];
#if defined ARDUINO || defined USE_ARDUPI    
    SPI.transfer(bufferdata);
#else
    writeRegister(WREG_FIFO, bufferdata);
#endif    
  }
  *((uint32_t *)temp + 1) = 1;

#ifdef USE_SPI_TRANSACTION
  SPI.endTransaction();
#endif

  writeRegister(REG_PAYLOADLENGTH, _TXPacketL);

  setTxParams(txpower, RADIO_RAMP_DEFAULT);             //TX power and ramp time

  setDioIrqParams(IRQ_RADIO_ALL, IRQ_TX_DONE, 0, 0);   //set for IRQ on TX done
  setTx(0);                                            //TX timeout is not handled in setTX()  
  
  // increment packet sequence number
  if ((uint8_t)txpackettype!=PKT_TYPE_RTS)
  	_TXSeqNo++;

  if (!wait)
  {
    return _TXPacketL;
  }

  if (txtimeout == 0)
  {
#ifdef USE_POLLING
    index = readRegister(REG_IRQFLAGS);
    //poll the irq register for TXDone, bit 3
    while ((bitRead(index, 3) == 0))
      {
        index = readRegister(REG_IRQFLAGS);
      }         
#endif    
  }
  else
  {
    endtimeoutmS = (millis() + txtimeout);
#ifdef USE_POLLING

#ifdef SX127XDEBUG1
  	PRINTLN_CSTSTR("TXDone using polling");
#endif 
    index = readRegister(REG_IRQFLAGS);
    //poll the irq register for TXDone, bit 3
    while ((bitRead(index, 3) == 0) && (millis() < endtimeoutmS))
      {
        index = readRegister(REG_IRQFLAGS);
      }
#endif    
  }

  setMode(MODE_STDBY_RC);                                            //ensure we leave function with TX off

#ifdef USE_POLLING
  if (bitRead(index, 3) == 0)
#endif  
  {
    _IRQmsb = IRQ_TX_TIMEOUT;
    return 0;
  }

	_AckStatus=0;

	if (txpackettype & PKT_FLAG_ACK_REQ) {
		uint8_t RXAckPacketL;
		const uint8_t RXBUFFER_SIZE=3;
		uint8_t RXBUFFER[RXBUFFER_SIZE];

#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("transmitAddressed is waiting for an ACK");
#endif
#ifdef INVERTIQ_ON_ACK
#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("invert IQ for ack reception");
#endif
		invertIQ(true);
#endif
		delay(10);			
		//try to receive the ack
		RXAckPacketL=receiveAddressed(RXBUFFER, RXBUFFER_SIZE, 2000, WAIT_RX);
		
		if (RXAckPacketL) {
#ifdef SX127XDEBUGACK
			PRINT_CSTSTR("received ");
			PRINT_VALUE("%d", RXAckPacketL);
			PRINTLN_CSTSTR(" bytes");
			PRINT_CSTSTR("dest: ");
			PRINTLN_VALUE("%d", readRXDestination());
			PRINT_CSTSTR("ptype: ");
			PRINTLN_VALUE("%d", readRXPacketType());		
			PRINT_CSTSTR("src: ");
			PRINTLN_VALUE("%d", readRXSource());
			PRINT_CSTSTR("seq: ");
			PRINTLN_VALUE("%d", readRXSeqNo());				 
#endif		
			if ( (readRXSource()==(uint8_t)txdestination) && (readRXDestination()==(uint8_t)txsource) && (readRXPacketType()==PKT_TYPE_ACK) && (readRXSeqNo()==(_TXSeqNo-1)) ) {
				//seems that we got the correct ack
#ifdef SX127XDEBUGACK
				PRINTLN_CSTSTR("ACK is for me!");
#endif		
				_AckStatus=1;
				uint8_t value=RXBUFFER[2];
				
				if (value & 0x80 ) // The SNR sign bit is 1
				{
					// Invert and divide by 4
					value = ( ( ~value + 1 ) & 0xFF ) >> 2;
					_PacketSNRinACK = -value;
				}
				else
				{
					// Divide by 4
					_PacketSNRinACK = ( value & 0xFF ) >> 2;
				}				
			}
			else {
#ifdef SX127XDEBUGACK
				PRINTLN_CSTSTR("not for me!");
#endif			
			}
		}
		else {
#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("received nothing");		 
#endif			
		}
#ifdef INVERTIQ_ON_ACK
#ifdef SX127XDEBUGACK
		PRINTLN_CSTSTR("set back IQ to normal");
#endif
		invertIQ(false);
#endif		
	} 
  return _TXPacketL;                                                     //no timeout, so TXdone must have been set
}


uint8_t SX127XLT::getLoRaSF()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getLoRaSF()");
#endif

  uint8_t regdata;
  regdata = readRegister(REG_MODEMCONFIG2);
  regdata = ((regdata & READ_SF_AND_X) >> 4);       //SX1272 and SX1276 store SF in same place

  return regdata;
}


uint8_t SX127XLT::getLoRaCodingRate()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getLoRaCodingRate");
#endif

  uint8_t regdata;
  regdata = readRegister(REG_MODEMCONFIG1);

  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272
    regdata = (((regdata & READ_CR_AND_X) >> 1) + 4);
  }
  else
  {
    regdata = (((regdata & READ_CR_AND_2) >> 3) + 4);
  }

  return regdata;
}


uint8_t SX127XLT::getOptimisation()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getOptimisation");
#endif

  uint8_t regdata;

  if (_Device != DEVICE_SX1272)
  {
    //for all devices apart from SX1272
    regdata = readRegister(REG_MODEMCONFIG3);
    regdata = ((regdata & READ_LDRO_AND_X) >> 3);
  }
  else
  {
    regdata = readRegister(REG_MODEMCONFIG1);
    regdata = (regdata & READ_LDRO_AND_2);
  }

  return regdata;
}


uint8_t SX127XLT::getSyncWord()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getSyncWord");
#endif

  return readRegister(REG_SYNCWORD);
}


uint8_t SX127XLT::getInvertIQ()
{
  //IQ mode reg 0x33

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getInvertIQ");
#endif

  uint8_t regdata;
  regdata =  ( (readRegister(REG_INVERTIQ)) & 0x40 );
  return regdata;
}


uint8_t SX127XLT::getVersion()
{
  //IQ mode reg 0x33

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getVersion");
#endif

  return readRegister(REG_VERSION);
}


uint16_t SX127XLT::getPreamble()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("getPreamble");
#endif

  uint16_t regdata;
  regdata =  ( (readRegister(REG_PREAMBLEMSB) << 8) + readRegister(REG_PREAMBLELSB) );
  return regdata;
}


uint32_t SX127XLT::returnBandwidth(byte BWregvalue)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("returnBandwidth()");
#endif

  //uint8_t regdata;

  if (_Device == DEVICE_SX1272)
  {
    switch (BWregvalue)
    {
      case 0:
        return 125000;

      case 64:
        return 250000;

      case 128:
        return 500000;

      default:
        return 0xFF;                      //so that a bandwidth invalid entry can be identified ?
    }
  }
  else
  {

    switch (BWregvalue)
    {
      case 0:
        return 7800;

      case 16:
        return 10400;

      case 32:
        return 15600;

      case 48:
        return 20800;

      case 64:
        return 31200;

      case 80:
        return 41700;

      case 96:
        return 62500;

      case 112:
        return 125000;

      case 128:
        return 250000;

      case 144:
        return 500000;

      default:
        return 0xFF;                      //so that a bandwidth invalid entry can be identified ?
    }

  }
}

uint8_t SX127XLT::returnOptimisation(uint8_t Bandwidth, uint8_t SpreadingFactor)
{
  //from the passed bandwidth (bandwidth) and spreading factor this routine
  //calculates whether low data rate optimisation should be on or off

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("returnOptimisation()");
#endif

  uint32_t tempBandwidth;
  float symbolTime;
  tempBandwidth = returnBandwidth(Bandwidth);
  symbolTime = calcSymbolTime(tempBandwidth, SpreadingFactor);

#ifdef SX127XDEBUG1
  PRINT_CSTSTR("Symbol Time ");
  PRINT_VALUE("%.3f",symbolTime);
  PRINTLN_CSTSTR("mS");
#endif

  if (symbolTime > 16)
  {
#ifdef SX127XDEBUG1
    PRINTLN_CSTSTR("LDR Opt on");
#endif
    return LDRO_ON;
  }
  else
  {
#ifdef SX127XDEBUG1
    PRINTLN_CSTSTR("LDR Opt off");
#endif
    return LDRO_OFF;
  }
}


float SX127XLT::calcSymbolTime(float Bandwidth, uint8_t SpreadingFactor)
{
  //calculates symbol time from passed bandwidth (lbandwidth) and Spreading factor (lSF)and returns in mS

#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("calcSymbolTime()");
#endif

  float symbolTimemS;

  symbolTimemS = 1000 / (Bandwidth / ( 1 << SpreadingFactor));
  return symbolTimemS;
}


void SX127XLT::printModemSettings()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("printModemSettings()");
#endif

  uint8_t regdata;

  printDevice();
  PRINT_CSTSTR(",");
  PRINT_VALUE("%lu",getFreqInt());
  PRINT_CSTSTR("hz,SF");
  PRINT_VALUE("%d",getLoRaSF());
  PRINT_CSTSTR(",BW");


  if (_Device == DEVICE_SX1272)
  {
    regdata = (readRegister(REG_MODEMCONFIG1) & READ_BW_AND_2);
  }
  else
  {
    regdata = (readRegister(REG_MODEMCONFIG1) & READ_BW_AND_X);
  }

  PRINT_VALUE("%ld",returnBandwidth(regdata));
  PRINT_CSTSTR(",CR4:");
  PRINT_VALUE("%d",getLoRaCodingRate());
  PRINT_CSTSTR(",LDRO_");

  if (getOptimisation())
  {
    PRINT_CSTSTR("On");
  }
  else
  {
    PRINT_CSTSTR("Off");
  }

  PRINT_CSTSTR(",SyncWord_0x");
  PRINT_HEX("%X",getSyncWord());
  if (getInvertIQ() == LORA_IQ_INVERTED)
  {
    PRINT_CSTSTR(",IQInverted");
  }
  else
  {
    PRINT_CSTSTR(",IQNormal");
  }
  PRINT_CSTSTR(",Preamble_");
  PRINT_VALUE("%d",getPreamble());
}


void SX127XLT::setSyncWord(uint8_t syncword)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setSyncWord()");
#endif

  writeRegister(REG_SYNCWORD, syncword);
}

uint8_t SX127XLT::readBytes(uint8_t *rxbuffer,   uint8_t size)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readBytes()");
#endif

  uint8_t x, index;
  
  for (index = 0; index < size; index++)
  {
#if defined ARDUINO || defined USE_ARDUPI  
  x = SPI.transfer(0);
#else
	x = readRegister(REG_FIFO);
#endif  
  rxbuffer[index] = x;
  }
  
  _RXPacketL = _RXPacketL + size;                      //increment count of bytes read
  return size;
}

void SX127XLT::setTXDirect()
{
  //turns on transmitter, in direct mode for FSK and audio  power level is from 2 to 17
#ifdef SX127XDEBUG1
  PRINT_CSTSTR("setTXDirect()");
#endif
  writeRegister(REG_OPMODE, 0x0B);           //TX on direct mode, low frequency mode
}

uint32_t SX127XLT::returnBandwidth()
{
  uint8_t regdata;

  if (_Device == DEVICE_SX1272)
  {
    regdata = (readRegister(REG_MODEMCONFIG1) & READ_BW_AND_2);
  }
  else
  {
    regdata = (readRegister(REG_MODEMCONFIG1) & READ_BW_AND_X);
  }

  return (returnBandwidth(regdata));
}

#ifdef SX127XDEBUG3

void printDouble(double val, byte precision){
    // prints val with number of decimal places determine by precision
    // precision is a number from 0 to 6 indicating the desired decimial places
    // example: lcdPrintDouble( 3.1415, 2); // prints 3.14 (two decimal places)

    if(val < 0.0){
        PRINT_CSTSTR("-");
        val = -val;
    }

    PRINT_VALUE("%d",int(val));  //prints the int part
    if( precision > 0) {
        PRINT_CSTSTR("."); // print the decimal point
        unsigned long frac;
        unsigned long mult = 1;
        byte padding = precision -1;
        while(precision--)
            mult *=10;

        if(val >= 0)
            frac = (val - int(val)) * mult;
        else
            frac = (int(val)- val ) * mult;
        unsigned long frac1 = frac;
        while( frac1 /= 10 )
            padding--;
        while(  padding--)
            PRINT_CSTSTR("0");
        PRINT_VALUE("%d",frac) ;
    }
}

#endif

// need to set cad_number to a value > 0
// we advise using cad_number=3 for a SIFS and DIFS=3*SIFS
#define DEFAULT_CAD_NUMBER    3

uint8_t	SX127XLT::invertIQ(bool invert)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("invertIQ()");
#endif

  if (invert)
  {
  	writeRegister(REG_INVERTIQ, 0x66);
  	writeRegister(REG_INVERTIQ2, 0x19);
  }
  else
  {
  	writeRegister(REG_INVERTIQ, 0x27);
  	writeRegister(REG_INVERTIQ2, 0x1D);  
  }
  
  return(0);
}


uint8_t SX127XLT::readRXSeqNo()
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("readRXSeqNo()");
#endif
  
  return(_RXSeqNo);
}

void SX127XLT::setPA_BOOST(bool pa_boost)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setPA_BOOST()");
#endif

  _PA_BOOST=pa_boost;
} 

void SX127XLT::setDevAddr(uint8_t addr)
{
#ifdef SX127XDEBUG1
  PRINTLN_CSTSTR("setDevAddr()");
#endif
	
	_DevAddr=addr;
}
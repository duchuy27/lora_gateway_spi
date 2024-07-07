//*******  Setup hardware pin definitions here ! ***************
//pinout is defined for our RFM95 PCB breakout

//will be translated into GPIO8/SPI_CE0 by arduPI
#ifndef NSS
#define NSS 10                                  //select pin on LoRa device
#endif
//will be translated into GPIO4 by arduPI
#ifndef NRESET
#define NRESET 6                                //reset pin on LoRa device
#endif
//will be translated into GPIO18/GEN01 by arduPI. WARNING: our current RFM95 PCB breakout does not connect DIO0 on the RPI header pin
//we do not connect DIO0 as we use polling method
#ifndef DIO0
#define DIO0 2                                  //DIO0 pin on LoRa device, used for RX and TX done 
#endif
#define DIO1 -1                                 //DIO1 pin on LoRa device, normally not used so set to -1 
#define DIO2 -1                                 //DIO2 pin on LoRa device, normally not used so set to -1

#define LORA_DEVICE DEVICE_SX1278            

//*******  Setup LoRa Parameters Here ! ***************
//Cấu hình LoRa Parameters
const uint32_t Offset = 0;                			//tần số offset
uint8_t Bandwidth = LORA_BW_125;          			//băng thông
uint8_t SpreadingFactor = LORA_SF12;      			//hệ số phổ rộng
uint8_t CodeRate = LORA_CR_4_5;           			//tỷ lệ mã hóa
const uint8_t Optimisation = LDRO_AUTO;   			//tối ưu hóa tốc độ dữ liệu thấp

#define RXBUFFER_SIZE 250                  			//Kích thước bộ đệm RX 

// GATEWAY HAS ADDRESS 1
#define GW_ADDR 1

// IMPORTANT
#define FCC_US_REGULATION
//công suất phát tối đa là 14
#define MAX_DBM 14

//FREQUENCY CHANNELS:
const uint32_t CH_00_433 = 433300000;
const uint32_t CH_01_433 = 433600000;
const uint32_t CH_02_433 = 433900000;
const uint32_t CH_03_433 = 434300000;
uint32_t DEFAULT_CHANNEL=CH_00_433;

//Loại gói tin LoRa
#define PKT_TYPE_DATA   0x10
#define PKT_TYPE_ACK    0x20

#define PKT_TYPE_MASK   0xF0
#define PKT_FLAG_MASK   0x0F

#define PKT_FLAG_ACK_REQ            0x08
#define PKT_FLAG_DATA_ENCRYPTED     0x04
#define PKT_FLAG_DATA_WAPPKEY       0x02
#define PKT_FLAG_DATA_DOWNLINK      0x01

//######################################
#define BRIDGE 0xC0000000
#define BRIDGE_SPAN 0x18

#define DATA 0x00
extern uint8_t *temp;
//######################################
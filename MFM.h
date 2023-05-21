/* Library for reading MFM 72/120/220/230/630 Modbus Energy meters.
*  Reading via Hardware or Software Serial library & rs232<->rs485 converter
*  2016-2023 Reaper7 (tested on wemos d1 mini->ESP8266 with Arduino 1.8.10 & 2.5.2 esp8266 core)
*  crc calculation by Jaime García (https://github.com/peninquen/Modbus-Energy-Monitor-Arduino/)
*/
//------------------------------------------------------------------------------
#ifndef MFM_h
#define MFM_h
//------------------------------------------------------------------------------
#include <Arduino.h>
#include <MFM_Config_User.h>

#if defined ( USE_HARDWARESERIAL )
    #include <HardwareSerial.h>
#else
    #include <SoftwareSerial.h>
#endif
//------------------------------------------------------------------------------
//DEFAULT CONFIG (DO NOT CHANGE ANYTHING!!! for changes use MFM_Config_User.h):
//------------------------------------------------------------------------------
#if !defined ( MFM_UART_BAUD )
    #define MFM_UART_BAUD                               9600                      //  default baudrate
#endif

#if !defined ( DERE_PIN )
    #define DERE_PIN                                    NOT_A_PIN                 //  default digital pin for control MAX485 DE/RE lines (connect DE & /RE together to this pin)
#endif

#if defined ( USE_HARDWARESERIAL )

#if !defined ( MFM_UART_CONFIG )
    #define MFM_UART_CONFIG                           SERIAL_8N1                //  default hardware uart config
#endif

#if defined ( ESP8266 ) && !defined ( SWAPHWSERIAL )
    #define SWAPHWSERIAL                              0                         //  (only esp8266) when hwserial used, then swap uart pins from 3/1 to 13/15 (default not swap)
#endif

#if defined ( ESP32 )
    #if !defined ( MFM_RX_PIN )
        #define MFM_RX_PIN                              -1                        //  use default rx pin for selected port
    #endif
    #if !defined ( MFM_TX_PIN )
        #define MFM_TX_PIN                              -1                        //  use default tx pin for selected port
    #endif
#endif

#else

#if defined ( ESP8266 ) || defined ( ESP32 )
    #if !defined ( MFM_UART_CONFIG )
        #define MFM_UART_CONFIG                         SWSERIAL_8N1              //  default softwareware uart config for esp8266/esp32
    #endif
#endif

//  #if !defined ( MFM_RX_PIN ) || !defined ( MFM_TX_PIN )
//    #error "MFM_RX_PIN and MFM_TX_PIN must be defined in MFM_Config_User.h for Software Serial option)"
//  #endif

#if !defined ( MFM_RX_PIN )
    #define MFM_RX_PIN                                -1
#endif
#if !defined ( MFM_TX_PIN )
    #define MFM_TX_PIN                                -1
#endif

#endif

#if !defined ( WAITING_TURNAROUND_DELAY )
    #define WAITING_TURNAROUND_DELAY                    200                       //  time in ms to wait for process current request
#endif

#if !defined ( RESPONSE_TIMEOUT )
    #define RESPONSE_TIMEOUT                            500                       //  time in ms to wait for return response from all devices before next request
#endif

#if !defined ( MFM_MIN_DELAY )
    #define MFM_MIN_DELAY                               20                        //  minimum value (in ms) for WAITING_TURNAROUND_DELAY and RESPONSE_TIMEOUT
#endif

#if !defined ( MFM_MAX_DELAY )
    #define MFM_MAX_DELAY                               5000                      //  maximum value (in ms) for WAITING_TURNAROUND_DELAY and RESPONSE_TIMEOUT
#endif

//------------------------------------------------------------------------------

#define MFM_ERR_NO_ERROR                              0                         //  no error
#define MFM_ERR_CRC_ERROR                             1                         //  crc error
#define MFM_ERR_WRONG_BYTES                           2                         //  bytes b0,b1 or b2 wrong
#define MFM_ERR_NOT_ENOUGHT_BYTES                     3                         //  not enough bytes from MFM
#define MFM_ERR_TIMEOUT                               4                         //  timeout

//------------------------------------------------------------------------------

#define FRAMESIZE                                     9                         //  size of out/in array
#define MFM_REPLY_BYTE_COUNT                          0x04                      //  number of bytes with data

#define MFM_B_01                                      0x01                      //  BYTE 1 -> slave address (default value 1 read from node 1)
#define MFM_B_02                                      0x04                      //  BYTE 2 -> function code (default value 0x04 read from 3X input registers)
#define MFM_B_05                                      0x00                      //  BYTE 5
#define MFM_B_06                                      0x02                      //  BYTE 6
//  BYTES 3 & 4 (BELOW)

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      INPUT REGISTERS LIST FOR MFM DEVICES                                                                                                                                |
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//      REGISTER NAME                                 REGISTER ADDRESS              UNIT          | MFM630  | MFM230  | MFM220  | MFM120CT| MFM120  | MFM72D  | MFM72 V2|
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define MFM_VOLTAGE_V1N                                 0x0000                    //  V           |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_VOLTAGE_V2N                                 0x0002                    //  V           |    1    |         |         |         |         |         |    1    |
#define MFM_VOLTAGE_V3N                                 0x0004                    //  V           |    1    |         |         |         |         |         |    1    |
#define MFM_AVERAGE_VOLTAGE_LN                          0x0006                    //  V           |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_VOLTAGE_V12                                 0x0008                    //  V           |    1    |         |         |         |         |         |    1    |
#define MFM_VOLTAGE_V23                                 0x000A                    //  V           |    1    |         |         |         |         |         |    1    |
#define MFM_VOLTAGE_V31                                 0x000C                    //  V           |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_AVERAGE_VOLTAGE_LL                          0x000E                    //  V           |    1    |         |         |         |         |         |    1    |
#define MFM_CURRENT_I1                                  0x0010                    //  A           |    1    |         |         |         |         |         |    1    |
#define MFM_CURRENT_I2                                  0x0012                    //  A           |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_CURRENT_I3                                  0x0014                    //  A           |    1    |         |         |         |         |         |    1    |
#define MFM_AVERAGE_CURRENT                             0x0016                    //  A           |    1    |         |         |         |         |         |    1    |
#define MFM_KW1                                         0x0018                    //  kW          |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_KW2                                         0x001A                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_KW3                                         0x001C                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_KVA1                                        0x001E                    //  kVA         |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_KVA2                                        0x0020                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_KVA3                                        0x0022                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_KVAR1                                       0x0024                    //  kVAr        |    1    |    1    |    1    |    1    |         |         |         |
#define MFM_KVAR2                                       0x0026                    //  kVAr        |    1    |         |         |         |         |         |         |
#define MFM_KVAR3                                       0x0028                    //  kVAr        |    1    |         |         |         |         |         |         |
#define MFM_TOTAL_KW                                    0x002A                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_TOTAL_KVA                                   0x002C                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_TOTAL_KVAR                                  0x002E                    //  kVAr        |    1    |         |         |         |         |         |    1    |
#define MFM_PF1                                         0x0030                    //  ??          |    1    |         |         |         |         |         |    1    |
#define MFM_PF2                                         0x0032                    //  ??          |    1    |         |         |         |         |         |    1    |
#define MFM_PF3                                         0x0034                    //  ??          |    1    |         |         |         |         |    1    |    1    |
#define MFM_AVERAGE_PF                                  0x0036                    //  ??          |    1    |         |         |         |         |    1    |    1    |
#define MFM_FREQUENCY                                   0x0038                    //  Hz          |    1    |         |         |         |         |         |    1    |
#define MFM_KWH                                         0x003A                    //  kWh         |    1    |         |         |         |         |         |    1    |
#define MFM_KVAH                                        0x003C                    //  kVAh        |    1    |         |         |         |         |         |    1    |
#define MFM_KVARH                                       0x003E                    //  kVAhr       |    1    |         |         |         |         |         |    1    |
#define MFM_KW_MAX_ACTIVE_POWER                         0x0040                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_KW_MIN_ACTIVE_POWER                         0x0042                    //  kW          |    1    |         |         |         |         |         |         |
#define MFM_KVAR_MAX_REACTIVE_POWER                     0x0044                    //  kVAr        |    1    |         |         |         |         |         |         |
#define MFM_KVAR_MIN_REACTIVE_POWER                     0x0046                    //  kVAr        |    1    |    1    |    1    |    1    |    1    |         |    1    |
#define MFM_KVA_MAX_APPARENT_POWER                      0x0048                    //  kVA         |    1    |    1    |    1    |    1    |    1    |    1    |    1    |
#define MFM_NEUTRAL_CURRENT                             0x007A                    //  A           |    1    |    1    |    1    |    1    |    1    |    1    |    1    |
#define MFM_THD_VOLTAGE_V1N                             0x007C                    //  ??          |    1    |    1    |    1    |    1    |    1    |         |         |
#define MFM_THD_VOLTAGE_V2N                             0x007E                    //  ??          |    1    |    1    |    1    |    1    |    1    |         |         |
#define MFM_THD_VOLTAGE_V3N                             0x0080                    //  ??          |    1    |         |         |         |         |         |         |
#define MFM_THD_VOLTAGE_V12                             0x0082                    //  ??          |    1    |         |         |         |         |         |         |
#define MFM_THD_VOLTAGE_V23                             0x0084                    //  ??          |    1    |    1    |         |         |         |         |         |
#define MFM_THD_VOLTAGE_V31                             0x0086                    //  ??          |    1    |    1    |         |         |         |         |         |
#define MFM_THD_CURRENT_I1                              0x0088                    //  ??          |         |    1    |         |         |         |         |         |
#define MFM_THD_CURRENT_I2                              0x008A                    //  ??          |         |    1    |         |         |         |         |         |
#define MFM_THD_CURRENT_I3                              0x008C                    //  ??          |         |    1    |         |         |         |         |         |
#define MFM_SERIAL_NUMBER                               0x02AC                    //  Float       |         |    1    |         |         |         |         |         |
#define MFM_MAX_I1_DEMAND                               0x02B4                    //  A           |    1    |         |         |         |         |         |         |
#define MFM_MAX_I2_DEMAND                               0x02B6                    //  A           |    1    |         |         |         |         |         |         |
#define MFM_MAX_I3_DEMAND                               0x02B8                    //  A           |    1    |         |         |         |         |         |         |
#define MFM_MAX_AVERAGE_I_DEMAND                        0x02BA                    //  A           |    1    |         |         |         |         |         |         |
#define MFM_PHASE_SEQUENCE_INDICATION                   0x02BC                    //  Float       |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KW_MAX_ACTIVE_POWER                0x02BE                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KW_MIN_ACTIVE_POWER                0x02C0                    //  kW          |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVAR_MAX_REACTIVE_POWER            0x02C2                    //  kVAr        |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVAR_MIN_REACTIVE_POWER            0x02C4                    //  kVAr        |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVA_MAX_APPARENT_POWER             0x02C6                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVA_MAX_I1_DEMAND                  0x02C8                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVA_MAX_I2_DEMAND                  0x02CA                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVA_MAX_I3_DEMAND                  0x02CC                    //  kVA         |    1    |         |         |         |         |         |    1    |
#define MFM_EXISTING_KVA_MAX_AVG_1_DEMAND               0x02CE                    //  kVA         |    1    |         |         |         |         |         |    1    |

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
//      TODO HOLDING REGISTERS LIST FOR MFM384                                                            |
//---------------------------------------------------------------------------------------------------------
//      REGISTER NAME                                 REGISTER ADDRESS              UNIT        | DDM18SD |
//---------------------------------------------------------------------------------------------------------
#define DDM_PHASE_1_VOLTAGE                           0x0000                    //  V           |    1    |
#define DDM_PHASE_1_CURRENT                           0x0008                    //  A           |    1    |
#define DDM_PHASE_1_POWER                             0x0012                    //  W           |    1    |
#define DDM_PHASE_1_REACTIVE_POWER                    0x001A                    //  VAr         |    1    |
#define DDM_PHASE_1_POWER_FACTOR                      0x002A                    //              |    1    |
#define DDM_FREQUENCY                                 0x0036                    //  Hz          |    1    |
#define DDM_IMPORT_ACTIVE_ENERGY                      0x0100                    //  kWh         |    1    |
#define DDM_IMPORT_REACTIVE_ENERGY                    0x0400                    //  kVArh       |    1    |
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
//      REGISTERS LIST FOR DEVNAME DEVICE                                                                 |
//---------------------------------------------------------------------------------------------------------
//      REGISTER NAME                                 REGISTER ADDRESS              UNIT        | DEVNAME |
//---------------------------------------------------------------------------------------------------------
//#define DEVNAME_VOLTAGE                             0x0000                    //  V           |    1    |
//#define DEVNAME_CURRENT                             0x0002                    //  A           |    1    |
//#define DEVNAME_POWER                               0x0004                    //  W           |    1    |
//---------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

class MFM {
public:
#if defined ( USE_HARDWARESERIAL )                                              //  hardware serial
#if defined ( ESP8266 )                                                       //  on esp8266
    MFM(HardwareSerial& serial, long baud = MFM_UART_BAUD, int dere_pin = DERE_PIN, int config = MFM_UART_CONFIG, bool swapuart = SWAPHWSERIAL);
#elif defined ( ESP32 )                                                       //  on esp32
    MFM(HardwareSerial& serial, long baud = MFM_UART_BAUD, int dere_pin = DERE_PIN, int config = MFM_UART_CONFIG, int8_t rx_pin = MFM_RX_PIN, int8_t tx_pin = MFM_TX_PIN);
#else                                                                         //  on avr
    MFM(HardwareSerial& serial, long baud = MFM_UART_BAUD, int dere_pin = DERE_PIN, int config = MFM_UART_CONFIG);
#endif
#else                                                                           //  software serial
#if defined ( ESP8266 ) || defined ( ESP32 )                                  //  on esp8266/esp32
    MFM(SoftwareSerial& serial, long baud = MFM_UART_BAUD, int dere_pin = DERE_PIN, int config = MFM_UART_CONFIG, int8_t rx_pin = MFM_RX_PIN, int8_t tx_pin = MFM_TX_PIN);
#else                                                                         //  on avr

    MFM(SoftwareSerial &serial, long baud = MFM_UART_BAUD, int dere_pin = DERE_PIN);

#endif
#endif

    virtual ~MFM();

    void begin(void);

    float readVal(uint16_t reg,
                  uint8_t node = MFM_B_01);                       //  read value from register = reg and from deviceId = node
    uint16_t getErrCode(
            bool _clear = false);                                   //  return last errorcode (optional clear this value, default flase)
    uint32_t getErrCount(
            bool _clear = false);                                  //  return total errors count (optional clear this value, default flase)
    uint32_t getSuccCount(
            bool _clear = false);                                 //  return total success count (optional clear this value, default false)
    void clearErrCode();                                                        //  clear last errorcode
    void clearErrCount();                                                       //  clear total errors count
    void clearSuccCount();                                                      //  clear total success count
    void setMsTurnaround(
            uint16_t _msturnaround = WAITING_TURNAROUND_DELAY);    //  set new value for WAITING_TURNAROUND_DELAY (ms), min=MFM_MIN_DELAY, max=MFM_MAX_DELAY
    void setMsTimeout(
            uint16_t _mstimeout = RESPONSE_TIMEOUT);                  //  set new value for RESPONSE_TIMEOUT (ms), min=MFM_MIN_DELAY, max=MFM_MAX_DELAY
    uint16_t
    getMsTurnaround();                                                 //  get current value of WAITING_TURNAROUND_DELAY (ms)
    uint16_t
    getMsTimeout();                                                    //  get current value of RESPONSE_TIMEOUT (ms)

private:
#if defined ( USE_HARDWARESERIAL )
    HardwareSerial& MFMSer;
#else
    SoftwareSerial &MFMSer;
#endif

#if defined ( USE_HARDWARESERIAL )
    int _config = MFM_UART_CONFIG;
#if defined ( ESP8266 )
    bool _swapuart = SWAPHWSERIAL;
#elif defined ( ESP32 )
    int8_t _rx_pin = -1;
    int8_t _tx_pin = -1;
#endif
#else
#if defined ( ESP8266 ) || defined ( ESP32 )
    int _config = MFM_UART_CONFIG;
#endif
    int8_t _rx_pin = -1;
    int8_t _tx_pin = -1;
#endif
    long _baud = MFM_UART_BAUD;
    int _dere_pin = DERE_PIN;
    uint16_t readingerrcode = MFM_ERR_NO_ERROR;                                 //  4 = timeout; 3 = not enough bytes; 2 = number of bytes OK but bytes b0,b1 or b2 wrong, 1 = crc error
    uint16_t msturnaround = WAITING_TURNAROUND_DELAY;
    uint16_t mstimeout = RESPONSE_TIMEOUT;
    uint32_t readingerrcount = 0;                                               //  total errors counter
    uint32_t readingsuccesscount = 0;                                           //  total success counter
    uint16_t calculateCRC(uint8_t *array, uint8_t len);

    void flush(unsigned long _flushtime = 0);                                   //  read serial if any old data is available or for a given time in ms
    void dereSet(bool _state = LOW);                                            //  for control MAX485 DE/RE pins, LOW receive from MFM, HIGH transmit to MFM
};

#endif // MFM_h

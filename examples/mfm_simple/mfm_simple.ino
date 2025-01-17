/*  WEMOS D1 Mini
                     ______________________________
                    |   L T L T L T L T L T L T    |
                    |                              |
                 RST|                             1|TX HSer
                  A0|                             3|RX HSer
                  D0|16                           5|D1
                  D5|14                           4|D2
                  D6|12                    10kPUP_0|D3
RX SSer/HSer swap D7|13                LED_10kPUP_2|D4
TX SSer/HSer swap D8|15                            |GND
                 3V3|__                            |5V
                       |                           |
                       |___________________________|
*/

//REMEMBER! uncomment #define USE_HARDWARESERIAL
//in MFM_Config_User.h file if you want to use hardware uart

#include <MFM.h>                                                                //import MFM library

#if defined ( USE_HARDWARESERIAL )                                              //for HWSERIAL

#if defined ( ESP8266 )                                                         //for ESP8266
MFM MFM(Serial1, MFM_UART_BAUD, NOT_A_PIN, SERIAL_8N1);                                  //config MFM
#elif defined ( ESP32 )                                                         //for ESP32
MFM MFM(Serial1, MFM_UART_BAUD, NOT_A_PIN, SERIAL_8N1, MFM_RX_PIN, MFM_TX_PIN);          //config MFM
#else                                                                           //for AVR
MFM MFM(Serial1, MFM_UART_BAUD, NOT_A_PIN);                                              //config MFM on Serial1 (if available!)
#endif

#else                                                                           //for SWSERIAL

#include <SoftwareSerial.h>                                                     //import SoftwareSerial library
#if defined ( ESP8266 ) || defined ( ESP32 )                                    //for ESP
SoftwareSerial swSerMFM;                                                        //config SoftwareSerial
MFM MFM(swSerMFM, MFM_UART_BAUD, NOT_A_PIN, SWSERIAL_8N1, MFM_RX_PIN, MFM_TX_PIN);       //config MFM
#else                                                                           //for AVR
SoftwareSerial swSerMFM(MFM_RX_PIN, MFM_TX_PIN);                                //config SoftwareSerial
MFM MFM(swSerMFM, MFM_UART_BAUD, NOT_A_PIN);                                             //config MFM
#endif

#endif

void setup() {
  Serial.begin(115200);                                                         //initialize serial
  MFM.begin();                                                                  //initialize MFM communication
}

void loop() {
  char bufout[10];
  sprintf(bufout, "%c[1;0H", 27);
  Serial.print(bufout);

  Serial.print("Voltage:   ");
  Serial.print(MFM.readVal(MFM_VOLTAGE_V1N), 2);                            //display voltage
  Serial.println("V");

  Serial.print("Current:   ");
  Serial.print(MFM.readVal(MFM_CURRENT_I1), 2);                            //display current
  Serial.println("A");

  Serial.print("Power:     ");
  Serial.print(MFM.readVal(MFM_KW1), 2);                              //display power
  Serial.println("W");

  Serial.print("Frequency: ");
  Serial.print(MFM.readVal(MFM_FREQUENCY), 2);                                  //display frequency
  Serial.println("Hz");

  delay(1000);                                                                  //wait a while before next loop
}

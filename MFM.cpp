/* Library for reading MFM 72/120/220/230/630 Modbus Energy meters.
*  Reading via Hardware or Software Serial library & rs232<->rs485 converter
*  2016-2023 Reaper7 (tested on wemos d1 mini->ESP8266 with Arduino 1.8.10 & 2.5.2 esp8266 core)
*  crc calculation by Jaime García (https://github.com/peninquen/Modbus-Energy-Monitor-Arduino/)
*/
//------------------------------------------------------------------------------
#include "MFM.h"
//------------------------------------------------------------------------------
#if defined ( USE_HARDWARESERIAL )
#if defined ( ESP8266 )
MFM::MFM(HardwareSerial& serial, long baud, int dere_pin, int config, bool swapuart) : MFMSer(serial) {
  this->_baud = baud;
  this->_dere_pin = dere_pin;
  this->_config = config;
  this->_swapuart = swapuart;
}
#elif defined ( ESP32 )
MFM::MFM(HardwareSerial& serial, long baud, int dere_pin, int config, int8_t rx_pin, int8_t tx_pin) : MFMSer(serial) {
  this->_baud = baud;
  this->_dere_pin = dere_pin;
  this->_config = config;
  this->_rx_pin = rx_pin;
  this->_tx_pin = tx_pin;
}
#else
MFM::MFM(HardwareSerial& serial, long baud, int dere_pin, int config) : MFMSer(serial) {
  this->_baud = baud;
  this->_dere_pin = dere_pin;
  this->_config = config;
}
#endif
#else
#if defined ( ESP8266 ) || defined ( ESP32 )
MFM::MFM(SoftwareSerial& serial, long baud, int dere_pin, int config, int8_t rx_pin, int8_t tx_pin) : MFMSer(serial) {
  this->_baud = baud;
  this->_dere_pin = dere_pin;
  this->_config = config;
  this->_rx_pin = rx_pin;
  this->_tx_pin = tx_pin;
}
#else

MFM::MFM(SoftwareSerial &serial, long baud, int dere_pin) : MFMSer(serial) {
    this->_baud = baud;
    this->_dere_pin = dere_pin;
}

#endif
#endif

MFM::~MFM() {
}

void MFM::begin(void) {
#if defined ( USE_HARDWARESERIAL )
#if defined ( ESP8266 )
    MFMSer.begin(_baud, (SerialConfig)_config);
#elif defined ( ESP32 )
    MFMSer.begin(_baud, _config, _rx_pin, _tx_pin);
#else
    MFMSer.begin(_baud, _config);
#endif
#else
#if defined ( ESP8266 ) || defined ( ESP32 )
    MFMSer.begin(_baud, (EspSoftwareSerial::Config)_config, _rx_pin, _tx_pin);
#else
    MFMSer.begin(_baud);
#endif
#endif

#if defined ( USE_HARDWARESERIAL ) && defined ( ESP8266 )
    if (_swapuart)
      MFMSer.swap();
#endif
    if (_dere_pin != NOT_A_PIN) {
        pinMode(_dere_pin,
                OUTPUT);                                                 //set output pin mode for DE/RE pin when used (for control MAX485)
    }
    dereSet(LOW);                                                                 //set init state to receive from MFM -> DE Disable, /RE Enable (for control MAX485)
}

float MFM::readVal(uint16_t reg, uint8_t node) {
    uint16_t temp;
    unsigned long resptime;
    uint8_t MFMarr[FRAMESIZE] = {node, MFM_B_02, 0, 0, MFM_B_05, MFM_B_06, 0, 0, 0};
    float res = NAN;
    uint16_t readErr = MFM_ERR_NO_ERROR;

    MFMarr[2] = highByte(reg);
    MFMarr[3] = lowByte(reg);

    temp = calculateCRC(MFMarr,
                        FRAMESIZE - 3);                                   //calculate out crc only from first 6 bytes

    MFMarr[6] = lowByte(temp);
    MFMarr[7] = highByte(temp);

#if !defined ( USE_HARDWARESERIAL )
    MFMSer.listen();                                                              //enable softserial rx interrupt
#endif

    flush();                                                                      //read serial if any old data is available

    dereSet(HIGH);                                                                //transmit to MFM  -> DE Enable, /RE Disable (for control MAX485)

    delay(2);                                                                     //fix for issue (nan reading) by sjfaustino: https://github.com/reaper7/MFM_Energy_Meter/issues/7#issuecomment-272111524

    MFMSer.write(MFMarr, FRAMESIZE - 1);                                          //send 8 bytes

    MFMSer.flush();                                                               //clear out tx buffer

    dereSet(LOW);                                                                 //receive from MFM -> DE Disable, /RE Enable (for control MAX485)

    resptime = millis();

    while (MFMSer.available() < FRAMESIZE) {
        if (millis() - resptime > msturnaround) {
            readErr = MFM_ERR_TIMEOUT;                                                //err debug (4)
            break;
        }
        yield();
    }

    if (readErr == MFM_ERR_NO_ERROR) {                                            //if no timeout...

        if (MFMSer.available() >= FRAMESIZE) {

            for (int n = 0; n < FRAMESIZE; n++) {
                MFMarr[n] = MFMSer.read();
            }

            if (MFMarr[0] == node && MFMarr[1] == MFM_B_02 && MFMarr[2] == MFM_REPLY_BYTE_COUNT) {

                if ((calculateCRC(MFMarr, FRAMESIZE - 2)) == ((MFMarr[8] << 8) |
                                                              MFMarr[7])) {  //calculate crc from first 7 bytes and compare with received crc (bytes 7 & 8)
                    ((uint8_t * ) & res)[3] = MFMarr[3]; //TODO: CHECK BYTE ORDER OF MFM384
                    ((uint8_t * ) & res)[2] = MFMarr[4];
                    ((uint8_t * ) & res)[1] = MFMarr[5];
                    ((uint8_t * ) & res)[0] = MFMarr[6];
                } else {
                    readErr = MFM_ERR_CRC_ERROR;                                          //err debug (1)
                }

            } else {
                readErr = MFM_ERR_WRONG_BYTES;                                          //err debug (2)
            }

        } else {
            readErr = MFM_ERR_NOT_ENOUGHT_BYTES;                                      //err debug (3)
        }

    }

    flush(mstimeout);                                                             //read serial if any old data is available and wait for RESPONSE_TIMEOUT (in ms)

    if (MFMSer.available())                                                       //if serial rx buffer (after RESPONSE_TIMEOUT) still contains data then something spam rs485, check node(s) or increase RESPONSE_TIMEOUT
        readErr = MFM_ERR_TIMEOUT;                                                  //err debug (4) but returned value may be correct

    if (readErr !=
        MFM_ERR_NO_ERROR) {                                            //if error then copy temp error value to global val and increment global error counter
        readingerrcode = readErr;
        readingerrcount++;
    } else {
        ++readingsuccesscount;
    }

#if !defined ( USE_HARDWARESERIAL )
    MFMSer.stopListening();                                                       //disable softserial rx interrupt
#endif

    return (res);
}

uint16_t MFM::getErrCode(bool _clear) {
    uint16_t _tmp = readingerrcode;
    if (_clear == true)
        clearErrCode();
    return (_tmp);
}

uint32_t MFM::getErrCount(bool _clear) {
    uint32_t _tmp = readingerrcount;
    if (_clear == true)
        clearErrCount();
    return (_tmp);
}

uint32_t MFM::getSuccCount(bool _clear) {
    uint32_t _tmp = readingsuccesscount;
    if (_clear == true)
        clearSuccCount();
    return (_tmp);
}

void MFM::clearErrCode() {
    readingerrcode = MFM_ERR_NO_ERROR;
}

void MFM::clearErrCount() {
    readingerrcount = 0;
}

void MFM::clearSuccCount() {
    readingsuccesscount = 0;
}

void MFM::setMsTurnaround(uint16_t _msturnaround) {
    if (_msturnaround < MFM_MIN_DELAY)
        msturnaround = MFM_MIN_DELAY;
    else if (_msturnaround > MFM_MAX_DELAY)
        msturnaround = MFM_MAX_DELAY;
    else
        msturnaround = _msturnaround;
}

void MFM::setMsTimeout(uint16_t _mstimeout) {
    if (_mstimeout < MFM_MIN_DELAY)
        mstimeout = MFM_MIN_DELAY;
    else if (_mstimeout > MFM_MAX_DELAY)
        mstimeout = MFM_MAX_DELAY;
    else
        mstimeout = _mstimeout;
}

uint16_t MFM::getMsTurnaround() {
    return (msturnaround);
}

uint16_t MFM::getMsTimeout() {
    return (mstimeout);
}

uint16_t MFM::calculateCRC(uint8_t *array, uint8_t len) {
    uint16_t _crc, _flag;
    _crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        _crc ^= (uint16_t) array[i];
        for (uint8_t j = 8; j; j--) {
            _flag = _crc & 0x0001;
            _crc >>= 1;
            if (_flag)
                _crc ^= 0xA001;
        }
    }
    return _crc;
}

void MFM::flush(unsigned long _flushtime) {
    unsigned long flushstart = millis();
    while (MFMSer.available() || (millis() - flushstart < _flushtime)) {
        if (MFMSer.available())                                                     //read serial if any old data is available
            MFMSer.read();
        delay(1);
    }
}

void MFM::dereSet(bool _state) {
    if (_dere_pin != NOT_A_PIN)
        digitalWrite(_dere_pin,
                     _state);                                            //receive from MFM -> DE Disable, /RE Enable (for control MAX485)
}

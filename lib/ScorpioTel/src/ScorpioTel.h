#ifndef SCORPIOTEL_H
#define SCORPIOTEL_H

#include "utils.h"

namespace ScorpioTel {

    class ScorpioTel {
    public:
        ScorpioTel(HardwareSerial& serial) 
            : m_serial(serial)
        {
            
        }

        void init(long baudRate = 38400) {
            m_serial.begin(baudRate, SERIAL_8N1, 16, 17);
            delay(100);

            m_serial.println("Hello from scorpion telemetry sniffer");
            delay(100);
        }

        void read(){

            handleTimeout();

            if (not (m_serial.available() > 0)) return;

            const uint8_t sbyte = m_serial.read();

            if ((not m_startPacket) and (sbyte == START_BYTE)) {
                m_startPacket = true;
                m_packetFinished = false;
                m_ipay = 0;
                m_payl[m_ipay] = sbyte;
                m_ipay++;
                return;
            }

            if (m_startPacket) {

                if (m_ipay == (HEADER_LEN+PAYLOAD_LEN+CRC_LEN)) {
                    m_startPacket = false;
                    
                    m_ipay = 0;

                    // checking if packet len is 22 to determine if packet valid
                    if (m_payl[2] == 22) {

                        m_packetFinished = true;
                        m_timeLastComm = millis();

                        // Calculate crc -> doesnt work for now...
                        //const uint16_t crc_calc = calcCRC16(m_payl, (HEADER_LEN+PAYLOAD_LEN+CRC_LEN));
                        
                        // Copy the array into the struct using memcpy
                        memcpy(&m_scorpioPack, m_payl, sizeof(m_scorpioPack));

                        // Packet to decoded log format
                        memcpy(&m_scorpioLog, m_scorpioPack.payload, sizeof(m_scorpioPack.payload));


                    }


                } else {
                    m_payl[m_ipay] = sbyte;
                    m_ipay++;
                }

            }


        }


        void handleTimeout() {
            
            // Fill all packets to 0 when no data
            if (millis() - m_timeLastComm > 2000) {

                byte zeroMemory_1[sizeof(m_scorpioPack)] = {0};
                memcpy(&m_scorpioPack, zeroMemory_1, sizeof(m_scorpioPack));

                byte zeroMemory_2[sizeof(m_scorpioLog)] = {0};
                memcpy(&m_scorpioLog, zeroMemory_2, sizeof(m_scorpioLog));

            }
        }

        // Getters

        const float getBatteryVoltage(){
            return 0.1 * (float)m_scorpioLog.voltage;
        }

        const float getCurrent(){
            return 0.1 * (float)m_scorpioLog.current;
        }

        const unsigned int getConsumption(){
            return m_scorpioLog.consumption;
        }

        const unsigned int getTemperature(){
            return (uint16_t)m_scorpioLog.temperature;
        }

        const unsigned int getRpm(){
            return m_scorpioLog.rpm;
        }

        private:

            uint16_t calcCRC16(uint8_t *pt, uint8_t msgLen)
            {
                uint16_t crc16_data = 0;
                uint8_t data=0;

                for (uint8_t mlen = 0; mlen < msgLen; mlen++){

                    data = pt[mlen] ^ ((uint8_t)(crc16_data) & (uint8_t)(0xFF));
                    data ^= data << 4;

                    crc16_data = ((((uint16_t)data << 8) | ((crc16_data & 0xFF00) >> 8))
                                ^ (uint8_t)(data >> 4)
                                ^ ((uint16_t)data << 3));
                }

                return(crc16_data);
            }

            HardwareSerial& m_serial;

            bool m_startPacket{false};
            bool m_packetFinished{false};
            int m_ipay{0};

            unsigned long m_timeLastComm{0};

            uint8_t m_payl[HEADER_LEN+PAYLOAD_LEN+CRC_LEN];
            Utils::ScorpioPacket m_scorpioPack;
            Utils::ScorpioLog m_scorpioLog;

    };
}


#endif

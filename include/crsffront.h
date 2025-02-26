#ifndef SCORPIOTEL_CRSF_FRONT_H
#define SCORPIOTEL_CRSF_FRONT_H

#include <AlfredoCRSF.h>

namespace CrsfFront
{

    class CrsfTelemetry
    {

    public:
        CrsfTelemetry() = default;

        void begin(HardwareSerial &serial)
        {
            m_crsf.begin(serial);
        }

        void sendEscData(const float &voltage, const float &current, const float &capacity, const float &remaining)
        {
            crsf_sensor_battery_t crsfBatt = {0};

            // Values are MSB first (BigEndian)
            crsfBatt.voltage = htobe16((uint16_t)(voltage * 10.0)); // Volts
            crsfBatt.current = htobe16((uint16_t)(current * 10.0)); // Amps
            crsfBatt.capacity = htobe16((uint16_t)(capacity)) << 8; // mAh (with this implemetation max capacity is 65535mAh)
            crsfBatt.remaining = (uint8_t)(remaining);

            m_crsf.queuePacket(CRSF_SYNC_BYTE, CRSF_FRAMETYPE_BATTERY_SENSOR, &crsfBatt, sizeof(crsfBatt));
        }

        // we are using roll-pitch-yaw packet for temp, currently
        // temperature is sent as T[degC] / 100
        // so 27deg is 0.27
        //
        void sendEscTemperature(const float &temp, const float alive)
        {

            crsf_sensor_attitude_t crsfAttitude = {0};

            // Values are MSB first (BigEndian)
            crsfAttitude.pitch = htobe16((uint16_t)(temp * 10000.0));
            crsfAttitude.roll = htobe16((uint16_t)(alive * 10000.0));
            crsfAttitude.yaw = htobe16((uint16_t)(0.0 * 10000.0));

            m_crsf.queuePacket(CRSF_SYNC_BYTE, CRSF_FRAMETYPE_ATTITUDE, &crsfAttitude, sizeof(crsfAttitude));
        }

    private:
        AlfredoCRSF m_crsf;
    };

}

#endif
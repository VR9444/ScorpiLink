#include <Arduino.h>
#include <crsffront.h>
#include <ScorpioTel.h>
#include <NOWManager.h>
#include <Struct.h>

ScorpioTel::ScorpioTel ScnTel;
CrsfFront::CrsfTelemetry Crsf;
SysData::SysData sysData;
NOWManager nowManager(sysData);

unsigned long time0 = millis();

int i_send = 10; // 10 - send packet esc
                 // 20 - send packet temperatue (via roll-pitch-yaw)

#define BATT_CAP 4500.0 // mAh

void setup()
{
  Serial.begin(38400);
  delay(10);

  ScnTel.init();
  delay(10);

  Serial1.begin(CRSF_BAUDRATE, SERIAL_8N1, SERIAL_TX_ONLY, 2);
  delay(10);

  Crsf.begin(Serial1);
  delay(10);

  nowManager.init();
  delay(10);
}

void loop()
{

  for (int i = 0; i < 30; i++)
  {
    ScnTel.read();
    if (not Serial.available())
      break;
  }

  if (millis() - time0 > 100)
  {

    const float time_seconds = 0.1 * millis();

    if (i_send == 10)
    {
      float remaining = BATT_CAP - (float)ScnTel.getConsumption();
      remaining /= BATT_CAP;
      remaining *= 100.0;

      Crsf.sendEscData(ScnTel.getBatteryVoltage(),
                       ScnTel.getCurrent(),
                       (float)ScnTel.getConsumption(),
                       remaining);

      i_send = 20;
    }
    else if (i_send == 20)
    {

      Crsf.sendEscTemperature(ScnTel.getTemperature(),
                              sin(time_seconds));

      i_send = 10;
    }

    // Serial.println(" Voltage: " + String(ScnTel.getBatteryVoltage()));
    // Serial.println(" Current: " + String(ScnTel.getCurrent()));
    // Serial.println("    Temp: " + String(ScnTel.getTemperature()));
    // Serial.println("     Rpm: " + String(ScnTel.getRpm()));
    // Serial.println();

    time0 = millis();
  }

  // Run processReceivedMessages at 20 Hz (every 50 ms)
  static unsigned long lastProcessTime = 0;
  if (millis() - lastProcessTime >= 50)
  {
    nowManager.processReceivedMessages();
    nowManager.checkIfSent(); // if acknowledge packet is not received it sends Release status again
    lastProcessTime = millis();
  }

  // Run sendReleaseStatus every 5 seconds with a random number between 0 and 2
  static unsigned long lastSendReleaseTime = 0;
  if (millis() - lastSendReleaseTime >= 5000)
  {
    int randomStatus = random(0, 3); // random() returns 0, 1, or 2
    nowManager.sendReleaseStatus(randomStatus);
    lastSendReleaseTime = millis();
  }
}

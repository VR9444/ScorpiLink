#include <Arduino.h>
#include <crsffront.h>
#include <ScorpioTel.h>
#include <NOWManager.h>
#include <Struct.h>


ScorpioTel::ScorpioTel ScnTel(Serial1);
CrsfFront::CrsfTelemetry Crsf;
HardwareSerial CrsfSerial(2);


SysData::SysData sysData;
NOWManager nowManager(sysData);

unsigned long time0 = millis();
unsigned long timeCrsf = millis();
static unsigned long lastProcessTime = 0;

int i_send = 10; // 10 - send packet esc
                 // 20 - send packet temperatue (via roll-pitch-yaw)

#define BATT_CAP 4500.0 // mAh

void setup()
{

  Serial.begin(115200);
  delay(10);

  // Init Scorpiotel
  ScnTel.init();
  delay(10);

  // Init Crossfire (rx 22, tx25);
  CrsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, 22, 25);
  delay(10);
  Crsf.begin(CrsfSerial);
  delay(10);

  
  // Init ESP Now
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

  if (millis() - timeCrsf > 10) {

    Crsf.readChannels();
    timeCrsf = millis();
  }

  if (millis() - time0 > 100)
  {

    // Read release ch14 value
    const int releaseCh = Crsf.getReleaseCh14();
    //Serial.println("Ch 14: " + String(releaseCh) +  " mil=" + String(millis()));

    int releaseStatus = 2;
    if (releaseCh > 1900) releaseStatus = 0;

    nowManager.sendReleaseStatus(releaseStatus);

    // Update telemetry
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


      //Crsf.sendEscData((float)(millis()/150.0),(float)(millis()/1000.0),0,0);

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


  // Commment next if statement if acknowledge packet is not needed
  // Run processReceivedMessages at 20 Hz (every 50 ms)
  if (millis() - lastProcessTime >= 50)
  {
    nowManager.processReceivedMessages();
    nowManager.checkIfSent(); // if acknowledge packet is not received it sends Release status again
    lastProcessTime = millis();
  }


  //
  //
  // Run sendReleaseStatus every 5 seconds with a random number between 0 and 2
  //static unsigned long lastSendReleaseTime = 0;
  //if (millis() - lastSendReleaseTime >= 5000)
  //{
  //  int randomStatus = random(0, 3); // random() returns 0, 1, or 2
  //  nowManager.sendReleaseStatus(randomStatus);
  //  lastSendReleaseTime = millis();
  //}


}





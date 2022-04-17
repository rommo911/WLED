#include "wled.h"
/*
 * This v1 usermod file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend #define EEPSIZE in const.h)
 * If you just need 8 bytes, use 2551-2559 (you do not need to increase EEPSIZE)
 * 
 * Consider the v2 usermod API if you need a more advanced feature set!
 */

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)

class LDR : public Usermod
{
private:
  const int LIGHT_PIN = A0;       // define analog pin
  const long UPDATE_MS = 3000;   // Upper threshold between mqtt messages
  const int CHANGE_THRESHOLD = 5; // Change threshold in percentage to send before UPDATE_MS

  // variables
  long lastTime = 0;
  long timeDiff = 0;
  long readTime = 0;
  int lightValue = 0;
  float lightPercentage = 0;
  float lastPercentage = 0;

public:
  void setup()
  {

    pinMode(LIGHT_PIN, INPUT);
  }

  void loop()
  {
    // Read only every 500ms, otherwise it causes the board to hang
    if (millis() - readTime > 500)
    {
      readTime = millis();
      timeDiff = millis() - lastTime;

      // Convert value to percentage
      lightValue = analogRead(LIGHT_PIN);
      lightPercentage = ((float)lightValue * -1 + 1024) / (float)1024 * 100;

      // Send MQTT message on significant change or after UPDATE_MS
      if (abs(lightPercentage - lastPercentage) > CHANGE_THRESHOLD || timeDiff > UPDATE_MS)
      {
        doPublishMqtt= true;
        lastTime = millis();
        lastPercentage = lightPercentage;
      }
    }
  }

  void addToJsonInfo(JsonObject &root)
  {
    JsonObject user = root[F("u")];
    if (user.isNull())
      user = root.createNestedObject(F("u"));

    JsonArray light = user.createNestedArray(F("light"));
    light.add(lightPercentage);
    light.add("lux");
  }
};

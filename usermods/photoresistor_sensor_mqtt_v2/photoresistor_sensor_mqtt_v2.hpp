#ifndef WLED_CUSTOM_USERMOD_HPP
#define WLED_CUSTOM_USERMOD_HPP

#include "wled.h"

class photoresistor_sensor_mqtt_v2 : public Usermod
{
private:
  const int LIGHT_PIN = A0;
  long UPDATE_MS = 3000;
  int CHANGE_THRESHOLD = 2;
  const char *MQTT_TOPIC = "/ldr";
  int ldrPin = A0;
  long lastTime = 0;
  long timeDiff = 0;
  long readTime = 0;
  int lightValue = 0;
  float lightPercentage = 0;
  float lastPercentage = 0;
  bool hassDiscoverySent = false;
  bool ldrEnabled = true;
  bool inverted = false;
  bool initDone = false;
  static const char _name[];

public:
  photoresistor_sensor_mqtt_v2() {}

  void setup()
  {
    pinMode(LIGHT_PIN, INPUT);
    initDone = true;
  }

  void connected()
  {
    publishDiscovery();
  }

  void loop()
  {
    if (millis() - readTime > 500)
    {
      readTime = millis();
      timeDiff = millis() - lastTime;

      lightValue = analogRead(LIGHT_PIN);
      if (inverted)
      {
        lightPercentage = 100.0 - ((float)lightValue * -1 + 1024) / (float)1024 * 100;
      }
      else
      {
        lightPercentage = ((float)lightValue * -1 + 1024) / (float)1024 * 100;
      }

      if (WLED_MQTT_CONNECTED)
      {
        if (abs(lightPercentage - lastPercentage) > CHANGE_THRESHOLD || timeDiff > UPDATE_MS)
        {
          publishMqtt(lightPercentage);
          lastTime = millis();
          lastPercentage = lightPercentage;
        }
      }
    }
  }

  void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top["Enabled"] = ldrEnabled;
    top["LDR Pin"] = ldrPin;
    top["LDR update interval"] = UPDATE_MS;
    top["LDR inverted"] = inverted;
  }

  bool readFromConfig(JsonObject &root)
  {
    int8_t oldLdrPin = ldrPin;
    JsonObject top = root[FPSTR(_name)];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top["Enabled"], ldrEnabled);
    configComplete &= getJsonValue(top["LDR Pin"], ldrPin);
    configComplete &= getJsonValue(top["LDR update interval"], UPDATE_MS);
    configComplete &= getJsonValue(top["LDR inverted"], inverted);

    if (initDone && (ldrPin != oldLdrPin))
    {
      // pin changed - un-register previous pin, register new pin
      if (oldLdrPin >= 0)
        pinManager.deallocatePin(oldLdrPin, PinOwner::UM_LDR_DUSK_DAWN);
      setup(); // setup new pin
    }
    return configComplete;
  }

  void addToJsonInfo(JsonObject &root)
  {
    // If "u" object does not exist yet we need to create it
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");

    JsonArray LDR_Enabled = user.createNestedArray("LDR enabled");
    LDR_Enabled.add(ldrEnabled);
    if (!ldrEnabled)
      return; // do not add more if usermod is disabled

    JsonArray LDR_Reading = user.createNestedArray("LDR reading");
    LDR_Reading.add(lightPercentage);

    JsonArray LDR_interval = user.createNestedArray("LDR interval");
    LDR_interval.add(UPDATE_MS);
  }

  uint16_t getId()
  {
    return USERMOD_ID_LDR_MQTT_V2;
  }

  void publishMqtt(float state)
  {
    if (!hassDiscoverySent)
    {
      publishDiscovery();
    }
    if (mqtt != nullptr)
    {
      char subuf[38];
      strcpy(subuf, mqttDeviceTopic);
      strcat(subuf, MQTT_TOPIC);
      mqtt->publish(subuf, 0, true, String(state).c_str());
    }
  }

  void publishDiscovery()
  {
    if (WLED_MQTT_CONNECTED)
    {
      StaticJsonDocument<600> doc;
      char uid[24], json_str[1024], buf[128];

      sprintf_P(buf, PSTR("%s Light Sensor"), serverDescription);
      doc[F("name")] = buf;

      sprintf_P(buf, PSTR("%s%s"), mqttDeviceTopic, MQTT_TOPIC);
      doc[F("stat_t")] = buf;

      sprintf_P(uid, PSTR("%s_ldr"), escapedMac.c_str());
      doc[F("uniq_id")] = uid;
      doc[F("dev_cla")] = F("illuminance");
      doc[F("unit_of_meas")] = "%";
      doc[F("val_tpl")] = F("{{ value }}");

      JsonObject device = doc.createNestedObject(F("device"));
      device[F("name")] = serverDescription;
      device[F("ids")] = String(F("wled-sensor-")) + mqttClientID;
      device[F("mf")] = F(WLED_BRAND);
      device[F("mdl")] = F(WLED_PRODUCT_NAME);
      device[F("sw")] = versionString;

      sprintf_P(buf, PSTR("homeassistant/sensor/%s/ldr/config"), uid);

      size_t payload_size = serializeJson(doc, json_str);
      mqtt->publish(buf, 1, true, json_str, payload_size);
      hassDiscoverySent = true;
    }
  }
};

const char photoresistor_sensor_mqtt_v2::_name[] PROGMEM = "photoresistor_sensor_mqtt_v2";

#endif // WLED_CUSTOM_USERMOD_HPP

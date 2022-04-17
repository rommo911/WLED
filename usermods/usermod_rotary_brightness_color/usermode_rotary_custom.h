#pragma once

#include "wled.h"

// v2 usermod that allows to change brightness and color using a rotary encoder,
// change between modes by pressing a button (many encoder have one included)
template <typename T>
T& incrementEnum(T& var)
{
  var = static_cast<T>(static_cast<int>(var) + 1);
  return var;
}
class RotaryEncoderSet : public Usermod
{
private:
  // Private class members. You can declare variables and functions only accessible to your usermod here
  unsigned long lastTime = 0;
  /*
   ** Rotary Encoder Example
   ** Use the Sparkfun Rotary Encoder to vary brightness of LED
   **
   ** Sample the encoder at 500Hz using the millis() function
   */

  int fadeAmount = 10; // how many points to fade the Neopixel with each step
  unsigned long currentTime;
  unsigned long loopTime;
  const int pinA = D6; // DT from encoder
  const int pinB = D7; // CLK from encoder
  const int pinC = D5; // SW from encoder

  enum SelectedState_t
  {
    Birghtness = 0,
    Color = 1,
    Effect = 2
  };
  SelectedState_t select_state = static_cast<SelectedState_t>(0);
  unsigned char button_state = HIGH;
  unsigned char prev_button_state = HIGH;
  CRGB fastled_col;
  CHSV prim_hsv;
  int16_t new_val;

  unsigned char Enc_A;
  unsigned char Enc_B;
  unsigned char Enc_A_prev = 0;
  const byte brightnessSteps[16] = { 5, 7, 9, 12, 16, 20, 26, 34, 43, 56, 72, 93, 119, 154, 198, 255 };
  const size_t numBrightnessSteps = sizeof(brightnessSteps) / sizeof(uint8_t);

  // increment `bri` to the next `brightnessSteps` value
  void incBrightness()
  {
    // dumb incremental search is efficient enough for so few items
    for (uint8_t index = 0; index < numBrightnessSteps; ++index)
    {
      if (brightnessSteps[index] > bri)
      {
        bri = brightnessSteps[index];
        break;
      }
    }
  }
  void decBrightness()
  {
    // dumb incremental search is efficient enough for so few items
    for (int index = numBrightnessSteps - 1; index >= 0; --index)
    {
      if (brightnessSteps[index] < bri)
      {
        bri = brightnessSteps[index];
        break;
      }
    }
  }

  void NextFX()
  {
    effectCurrent += 1;
    if (effectCurrent >= MODE_COUNT)
      effectCurrent = 0;
  }
  void PreviousFX()
  {
    effectCurrent--;
    if (effectCurrent < 0)
      effectCurrent = (MODE_COUNT - 1);
  }

public:
  // Functions called by WLED

  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * You can use it to initialize variables, sensors or similar.
   */
  void setup()
  {
    // Serial.println("Hello from my usermod!");
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(pinC, INPUT_PULLUP);
    currentTime = millis();
    loopTime = currentTime;
  }

  /*
   * connected() is called every time the WiFi is (re)connected
   * Use it to initialize network interfaces
   */
  void connected()
  {
    // Serial.println("Connected to WiFi!");
  }

  /*
   * loop() is called continuously. Here you can check for events, read sensors, etc.
   *
   * Tips:
   * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
   *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
   *
   * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
   *    Instead, use a timer check as shown here.
   */
  uint32_t lastboutonpressed = 0;
  void loop()
  {
    currentTime = millis(); // get the current elapsed time
    if (currentTime > (lastboutonpressed + 15000))
    {
      select_state = Birghtness;
    }
    if (currentTime >= (loopTime + 2)) // 2ms since last check of encoder = 500Hz
    {
      button_state = digitalRead(pinC);
      if (prev_button_state != button_state)
      {
        if (button_state == LOW)
        {
          if (select_state == Effect)
            select_state = Birghtness;
          else
          {
            lastboutonpressed = currentTime;
            incrementEnum<SelectedState_t>(select_state);
          }
        }
        prev_button_state = button_state;
      }
      int Enc_A = digitalRead(pinA); // Read encoder pins
      int Enc_B = digitalRead(pinB);
      if ((!Enc_A) && (Enc_A_prev))
      { // A has gone from high to low
        if (Enc_B == HIGH)
        { // B is high so clockwise
          if (select_state == Birghtness)
          {
            incBrightness();
          }
          else if (select_state == Color)
          {
            fastled_col.red = col[0];
            fastled_col.green = col[1];
            fastled_col.blue = col[2];
            prim_hsv = rgb2hsv_approximate(fastled_col);
            new_val = (int16_t)prim_hsv.h + fadeAmount;
            if (new_val > 255)
              new_val -= 255; // roll-over if  bigger than 255
            if (new_val < 0)
              new_val += 255; // roll-over if smaller than 0
            prim_hsv.h = (byte)new_val;
            hsv2rgb_rainbow(prim_hsv, fastled_col);
            col[0] = fastled_col.red;
            col[1] = fastled_col.green;
            col[2] = fastled_col.blue;
          }
          else if (select_state == Effect)
          {
            NextFX();
          }
        }
        else if (Enc_B == LOW)
        { // B is low so counter-clockwise
          if (select_state == Birghtness)
          {
            decBrightness();
          }
          else if (select_state == Color)
          {
            fastled_col.red = col[0];
            fastled_col.green = col[1];
            fastled_col.blue = col[2];
            prim_hsv = rgb2hsv_approximate(fastled_col);
            new_val = (int16_t)prim_hsv.h - fadeAmount;
            if (new_val > 255)
              new_val -= 255; // roll-over if  bigger than 255
            if (new_val < 0)
              new_val += 255; // roll-over if smaller than 0
            prim_hsv.h = (byte)new_val;
            hsv2rgb_rainbow(prim_hsv, fastled_col);
            col[0] = fastled_col.red;
            col[1] = fastled_col.green;
            col[2] = fastled_col.blue;
          }
          else if (select_state == Effect)
          {
            PreviousFX();
          }
        }
        // call for notifier -> 0: init 1: direct change 2: button 3: notification 4: nightlight 5: other (No notification)
        //  6: fx changed 7: hue 8: preset cycle 9: blynk 10: alexa
        colorUpdated(CALL_MODE_NOTIFICATION);
        updateInterfaces(CALL_MODE_NOTIFICATION);
      }
      Enc_A_prev = Enc_A;     // Store value of A for next time
      loopTime = currentTime; // Updates loopTime
    }
  }

  /*
   * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
   * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
   * Below it is shown how this could be used for e.g. a light sensor
   */
   /*
     void addToJsonInfo(JsonObject& root)
     {
       int reading = 20;
       //this code adds "u":{"Light":[20," lux"]} to the info object
       JsonObject user = root["u"];
       if (user.isNull()) user = root.createNestedObject("u");

       JsonArray lightArr = user.createNestedArray("Light"); //name
       lightArr.add(reading); //value
       lightArr.add(" lux"); //unit
     }
     */

     /*
      * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
      * Values in the state object may be modified by connected clients
      */
  void addToJsonState(JsonObject& root)
  {
    // root["user0"] = userVar0;
  }

  /*
   * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
   * Values in the state object may be modified by connected clients
   */
  void readFromJsonState(JsonObject& root)
  {
    userVar0 = root["user0"] | userVar0; // if "user0" key exists in JSON, update, else keep old value
    // if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
  }

  /*
   * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
   * This could be used in the future for the system to determine whether your usermod is installed.
   */
  uint16_t getId()
  {
    return 0xABCD;
  }

  // More methods can be added in the future, this example will then be extended.
  // Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};

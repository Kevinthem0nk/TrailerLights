//=================================================
// Default Includes
#include <Arduino.h>
#include <EEPROM.h>
// #include <WiFi.h> // For Later

//=================================================
// Button Define
#define buttonPin 25 // Wired from 0v to Input

// Logic Control Variables
bool wLEDtrigger = false;                // Trigger White LED Cycle
bool rLEDtrigger = false;                // Trigger Red LED Cycle
bool gLEDtrigger = false;                // Trigger Green LED Cycle
bool bLEDtrigger = false;                // Trigger Blue LED Cycle
bool bPower = false;                     // Light Commanded State
bool lightsOff = true;                   // Lights Actually Off
bool bRamp = false;                      // Ramp Enable
unsigned long RampSpeed;                 // Current Speed in MS Between RGB Value Increases
bool bRampChg = false;                   // Signals a change has happened to RampSpeed
int RampVal = 0;                         // Current Ramp Value
unsigned long lastTick = millis();       // Initialize lastTick
bool bDeepSleepTmr = false;              // Deep Sleep Timer Started
unsigned long deepSleepMark = millis();  // Deep Sleep Start Point
unsigned long transitionTime = millis(); // Case Timer
static int state = 0;                    // Case State
bool ClearTriggers = false;              // Avoid Trigger Cleared Spam
bool pressed = false;                    // Button Pressed
int DCgap = 300;                         // max ms between clicks for a double click event
int holdTime = 2000;                     // ms hold period: how long to wait for press+hold event
int longHoldTime = 4000;                 // ms long hold period: how long to wait for press+hold event

//Tuning Parameters
int eeAddress = 0;                   //Location we want the data to store in EEprom.
int RampJump = 1;                    // RGB Value Increase
int RampChangePt = 25;               // RGB Value to speed up ramping
unsigned long RampSpeedSlw = 250;    // ms between RGB Values up to RampChangePt
unsigned long RampSpeedFst = 10;     // ms between RGB Values after RampChangePt
unsigned long DeepSleepWait = 10000; // Time before Deep Sleep Starts

//=================================================
// EEPROM Structure Setup

struct LightStruct
{
  bool Pon;
  int r;
  int g;
  int b;
};
LightStruct LightStatus = {false, 0, 0, 0}; // Initialize Structure

//=================================================
// Fast LED Includes
#include <FastLED.h>
#define CHIPSET NEOPIXEL
#define NUM_LEDS 8
#define DATA_PIN 26 // To LED strip input, 0v is tied to common 0v
CRGB leds[NUM_LEDS];

//=================================================
// define button logic

void IRAM_ATTR singleClick()
{
  LightStatus = EEPROM.get(eeAddress, LightStatus);
  bPower = LightStatus.Pon;
  if (!bPower && !bRamp)
  {
    wLEDtrigger = true;
    bPower = true;
    bRamp = true;
    ClearTriggers = false;
  }
  else if (bPower && bRamp)
  {
    bRamp = false;
    Serial.println("Ramped to " + String(RampVal));
  }
  else if (bPower && !bRamp)
  {
    bPower = false;
    lightsOff = false;
  }
}

void IRAM_ATTR doubleClick()
{
  if (!bPower && !bRamp)
  {
    rLEDtrigger = true;
    bPower = true;
    bRamp = true;
    ClearTriggers = false;
  }
}

void IRAM_ATTR longClick()
{
  if (!bPower && !bRamp)
  {
    bLEDtrigger = true;
    bPower = true;
    bRamp = true;
    ClearTriggers = false;
  }
}

void IRAM_ATTR extralongClick()
{
  if (!bPower && !bRamp)
  {
    gLEDtrigger = true;
    bPower = true;
    bRamp = true;
    ClearTriggers = false;
  }
}

void IRAM_ATTR WakeUp()
{
  Serial.println("Interupt Triggered");
  bDeepSleepTmr = false;
}

void setColor(unsigned char r, unsigned char g, unsigned char b)
{
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB(r, g, b);
}

void setup()
{
  // Set button input pin
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(GPIO_NUM_25, WakeUp, FALLING);

  //=================================================
  // Serial Test
  Serial.begin(9600);
  Serial.println("Serial Enabled");

  //=================================================
  // Fast LED Setup
  FastLED.addLeds<CHIPSET, DATA_PIN>(leds, NUM_LEDS);
  setColor(50, 0, 0);
  FastLED.show();
  delay(5);
  setColor(0, 0, 0);
  FastLED.show();

  //=================================================
  // Setup EEprom
  EEPROM.put(eeAddress, LightStatus); // Initial EEPROM Write

  //=================================================
  // Setup Deep Sleep
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
}

void loop()
{
  //=================================================
  // Thank you @Luksab for this bit of code!!!
  // Button Case
  pressed = !digitalRead(buttonPin);
  switch (state)
  {
  case 0:
    if (pressed)
    {
      state = 1;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
    }
    break;
  case 1:
    if (!pressed)
    {
      state = 4;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
    }
    else if (millis() > transitionTime + holdTime)
    {
      state = 2;
      Serial.println("Case State - " + String(state) + " - Blue Enabled");
      setColor(0, 0, 50);
      FastLED.show();
      transitionTime = millis();
      delay(5);
      setColor(0, 0, 0);
      FastLED.show();
    }
    break;
  case 2:
    if (!pressed)
    {
      state = 0;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
      longClick();
    }
    else if (millis() > transitionTime + longHoldTime - holdTime)
    {
      state = 3;
      Serial.println("Case State - " + String(state) + " - Green Enabled");
      setColor(0, 50, 0);
      FastLED.show();
      transitionTime = millis();
      delay(5);
      setColor(0, 0, 0);
      FastLED.show();
    }
    break;
  case 3:
    if (!pressed)
    {
      state = 0;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
      extralongClick();
    }
    break;
  case 4:
    if (pressed)
    {
      state = 5;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
    }
    else if (millis() > transitionTime + DCgap)
    {
      state = 0;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
      singleClick();
    }
    break;
  case 5:
    if (!pressed)
    {
      state = 0;
      Serial.println("Case State - " + String(state));
      transitionTime = millis();
      doubleClick();
    }
    break;
  default:
    break;
  }

  if (bRamp)
  {
    if (RampVal <= 255)
    {
      if ((RampVal < RampChangePt) && (RampSpeed != RampSpeedSlw))
      {
        RampSpeed = RampSpeedSlw;
        bRampChg = true;
      }
      else if ((RampVal >= RampChangePt) && (RampSpeed != RampSpeedFst))
      {
        RampSpeed = RampSpeedFst;
        bRampChg = true;
      }
    }
    if (millis() >= (lastTick + RampSpeed))
    {
      RampVal = RampVal + RampJump;
      lastTick = millis();
      if (RampVal >= 255)
      {
        bRamp = false;
        RampVal = 255;
        Serial.println("Ramp Max");
      }
    }
    if (bRampChg == true)
    {
      Serial.println("RampSpeed " + String(RampSpeed));
      bRampChg = false;
    }
  }

  if (wLEDtrigger)
  {
    setColor(RampVal, RampVal, RampVal);
    FastLED.show();
    LightStatus = {bPower, RampVal, RampVal, RampVal};

    if (RampVal == 255)
    {
      Serial.println("White Lights Max");
    }
  }

  if (rLEDtrigger)
  {
    setColor(RampVal, 0, 0);
    FastLED.show();
    LightStatus = {bPower, RampVal, 0, 0};

    if (RampVal == 255)
    {
      Serial.println("Red Lights Max");
    }
  }

  if (gLEDtrigger)
  {
    setColor(0, RampVal, 0);
    FastLED.show();
    LightStatus = {true, 0, RampVal, 0};

    if (RampVal == 255)
    {
      Serial.println("Green Lights Max");
    }
  }

  if (bLEDtrigger)
  {
    setColor(0, 0, RampVal);
    FastLED.show();
    LightStatus = {true, 0, 0, RampVal};

    if (RampVal == 255)
    {
      Serial.println("Blue Lights Max");
    }
  }

  if (!bRamp && !ClearTriggers)
  {
    wLEDtrigger = false;
    rLEDtrigger = false;
    gLEDtrigger = false;
    bLEDtrigger = false;
    ClearTriggers = true;
    bDeepSleepTmr = false;
    Serial.println("Triggers Cleared");
  }

  //Turn Lights Off
  if (!bPower and !lightsOff)
  {
    setColor(0, 0, 0);
    FastLED.show();
    LightStatus = {bPower, 0, 0, 0};
    Serial.println("Lights Off");
    RampVal = 0;
    lightsOff = true;
  }

  // Deep Sleep After Time
  if (ClearTriggers && !bRamp)
  {
    if (!bDeepSleepTmr)
    {
      deepSleepMark = millis();
      bDeepSleepTmr = true;
      Serial.println("Sleep Timer Started");
    }
    if (bDeepSleepTmr)
    {
      if (millis() >= (deepSleepMark + DeepSleepWait))
      {
        EEPROM.put(eeAddress, LightStatus); // Save State
        Serial.println("Going to sleep now");
        bDeepSleepTmr = false;
        delay(1000);
        esp_deep_sleep_start();
        Serial.println("I Broke");
      }
    }
  }

  delay(25);
}

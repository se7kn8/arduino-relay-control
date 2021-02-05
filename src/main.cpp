#include "Arduino.h"

#define MILLIS_MAX 4294967295
#define ON_OFF MILLIS_MAX
#define minutes(X) X * 60 * 1000
#define seconds(X) X * 1000

struct Relay
{
  unsigned long time;
  unsigned long endTime;
  bool active;
  int lastButtonState;
  unsigned int lastChange;

  Relay(unsigned long time) : time(time)
  {
    active = false;
    endTime = 0;
  }

  void start()
  {
    if (!active)
    {
      active = true;
      if (time == MILLIS_MAX)
      {
        endTime = MILLIS_MAX;
      }
      else
      {
        endTime = millis() + time;
      }
    }
  }

  void reset()
  {
    endTime = 0;
    active = false;
  }

  bool shouldBeOn()
  {
    return millis() <= endTime;
  }
};

/**
 * 
 * 74hc595 pins:
 * DS = D10
 * SH_CP = D11
 * ST_CP = D12
 * 
 */
#define dataPin 10
#define clockPin 11
#define latchPin 12

#define inputStart 2
#define inputs 8

byte value = B11111111;
byte oldValue = B11111111;

Relay relais[inputs] = {
    Relay(minutes(30)),       // relay 1
    Relay(seconds(45)),       // relay 2
    Relay(ON_OFF),            // relay 3
    Relay(ON_OFF),            // relay 4
    Relay(ON_OFF),            // relay 5
    // Currently not in use
    Relay(1000000),       // relay 6
    Relay(1000000),       // relay 7
    Relay(1000000)        // relay 8
};

void shiftData(byte data)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, data);
  digitalWrite(latchPin, HIGH);
}

void checkChange()
{
  if (value != oldValue)
  {
    // Print the inverted state
    Serial.println(~value, BIN);
    shiftData(value);
    oldValue = value;
  }
}

void setup()
{
  for (size_t i = 0; i < inputs; i++)
  {
    pinMode(i + inputStart, INPUT_PULLUP);
  }

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  shiftData(oldValue);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  Serial.begin(9600);
}

void loop()
{
  checkChange();
  for (size_t i = 0; i < inputs; i++)
  {
    int state = digitalRead(i + inputStart);

    // Only trigger when a low level signal is triggered, after a high one (plus debouncing 100ms)
    if (state != relais[i].lastButtonState && state == LOW && (millis() - relais[i].lastChange) > 100)
    {
      if (!relais[i].active)
      {
        Serial.println("Activate button");
        bitClear(value, i);
        relais[i].start();
      }
      else
      {
        Serial.println("Deactivate button");
        bitSet(value, i);
        relais[i].reset();
      }
    }

        // If time is over, deactive relay
    if (!relais[i].shouldBeOn() && relais[i].active)
    {
      bitSet(value, i);
      relais[i].reset();
      Serial.println("Reset button");
    }

    // Change last know state if it differs from current state
    if (relais[i].lastButtonState != state)
    {
      relais[i].lastButtonState = state;
      relais[i].lastChange = millis();
    }
  }
}
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <math.h>
// libraries for led screen and accel
#include "SparkFun_BMI270_Arduino_Library.h"
#include <SparkFun_Qwiic_OLED.h>

// Create appropriate obj for led and accel

QwiicMicroOLED myOLED;
BMI270 imu;

volatile int buttonCounter = 0;
bool prevPressed = false;
int switchPin = 9;
int offButton = 6;
unsigned long debounceDelay = 100;
unsigned long doublePressTime = 500;
unsigned long prevTime = 0;
int longTime = 3000;
int longStart = 0;

float theta = 0.0;
float psi = 0.0;
float phi = 0.0;

int arrLx[] = {0, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3};
int arrLy[] = {0, -1, 0, 1, -2, -1, 0, 1, 2, -3, -2, -1, 0, 1, 2, 3};
char pout[30];

enum PressType
{
  NoPress,     // 0
  SinglePress, // 1
  DoublePress, // 2
  LongPress    // 3
};

enum MachineState
{
  OffState,
  TwoAxis,
  XAxis,
  YAxis,
  RawData,
  StateLength
};

volatile PressType currentPress = NoPress;
volatile MachineState currentState = OffState;

// Triangle draw function
void drawTriangle(int xOff, int yOff, int xDir, int yDir, bool swap)
{
  if (!swap)
  {
    for (int i = 0; i < 16; i++)
    {
      myOLED.pixel(xOff + xDir * arrLx[i], yOff + yDir * arrLy[i], 255);
    }
  }
  else
  {
    for (int i = 0; i < 16; i++)
    {
      myOLED.pixel(xOff + yDir * arrLy[i], yOff + xDir * arrLx[i], 255);
    }
  }
}

float getXangle()
{
  return atan2(imu.data.accelX, sqrt(imu.data.accelY * imu.data.accelY + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI;
}

float getYangle()
{
  return atan2(imu.data.accelY, sqrt(imu.data.accelX * imu.data.accelX + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI;
  // phi = atan2(sqrt(imu.data.accelY * imu.data.accelY + imu.data.accelX * imu.data.accelX), imu.data.accelZ);
}

void buttonPress()
{
  unsigned long currentTime = millis();
  if (currentTime - prevTime > debounceDelay)
  {

    /*if (currentTime - prevTime > 1000)
    {
      currentPress = LongPress;
    }*/
    if (currentTime - prevTime < doublePressTime)
    {
      currentPress = DoublePress;
    }
    else
    {
      currentPress = SinglePress;
    }
    prevTime = currentTime;
  }
}

void setup()
{
  delay(500);
  Serial.begin(9600);
  delay(4000);
  while (!Serial)
  {
    yield();
  }
  pinMode(switchPin, INPUT_PULLDOWN);
  pinMode(offButton, INPUT_PULLDOWN);
  attachInterrupt(switchPin, buttonPress, RISING);
  delay(100);
  // Start accel
  Wire.begin();
  while (imu.beginI2C(0x68) != BMI2_OK)
  {
    Serial.println("imu failed");
    delay(1000);
  }
  delay(100);
  // Start LED screen
  while (!myOLED.begin())
  {
    Serial.println("OLED failed");
    delay(1000);
  }
  Serial.println("Everything started!!!!!");

  myOLED.text(0, 0, "clearingthescreen");
  myOLED.text(0, 10, "clearingthescreen");
  myOLED.text(0, 20, "clearingthescreen");
  myOLED.text(0, 30, "clearingthescreen");
  myOLED.text(0, 40, "clearingthescreen");
  // For some reason the screen doesn't clear until you 'overwrite' the random pixels there
  myOLED.display();
  delay(500);
  myOLED.erase();
  myOLED.display();
}

void loop()
{
  if (digitalRead(switchPin) == HIGH)
  {

    if (longStart == 0)
    {
      longStart = millis();
    }

    if (millis() - longStart > longTime)
    {
      currentState = OffState;
    }
  }
  else
  {
    longStart = 0;
  }

  if (currentPress == DoublePress)
  { //&& currentState != OffState
    currentState = (MachineState)(((int)currentState + 1) % (int)StateLength);
    currentState = (MachineState)max((int)currentState, 1);
  }

  if (currentPress != NoPress)
  {
    Serial.print("Current State type: ");
    Serial.println((int)currentState);
    currentPress = NoPress;
  }
  if (currentPress == LongPress)
  {
    currentState = OffState;
  }

  if (digitalRead(offButton) == HIGH)
  {
    currentState = OffState;
  }

  // erase screen
  myOLED.erase();
  // get imu data
  imu.getSensorData();
  theta = getXangle();
  psi = getYangle();
  switch (currentState)
  {
  case OffState:
    myOLED.erase();
    myOLED.display();
    break;
  case TwoAxis:
    if (theta > 0.0)
    {
      drawTriangle(0, 23, 1, 1, false);
    }
    else
    {
      drawTriangle(63, 23, -1, 1, false);
    }
    if (psi > 0.0)
    {
      drawTriangle(32, 45, -1, -1, true);
    }
    else
    {
      drawTriangle(32, 0, 1, 1, true);
    }

    break;
  case XAxis:
    if (theta > 0.0)
    {
      drawTriangle(0, 23, 1, 1, false);
    }
    else
    {
      drawTriangle(63, 23, -1, 1, false);
    }
    break;
  case YAxis:
    if (psi > 0.0)
    {
      drawTriangle(32, 45, -1, -1, true);
    }
    else
    {
      drawTriangle(32, 0, 1, 1, true);
    }
    break;
  case RawData:
    sprintf(pout, "ax: %.2f", imu.data.accelX);
    myOLED.text(0, 0, pout);
    sprintf(pout, "ay: %.2f", imu.data.accelY);
    myOLED.text(0, 10, pout);
    sprintf(pout, "az: %.2f", imu.data.accelZ);
    myOLED.text(0, 20, pout);

    break;
  default:
    break;
  }
  // display
  myOLED.display();
  // delay(100);
}
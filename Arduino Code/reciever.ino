//////////////////////////   1-Axis Motorized Camera Dolly  ////////////////////////////////
// RECIEVER
//#include "PinChangeInt.h"
#include <EEPROM.h>
#include <SPI.h>
#include "RF24.h"
#include <Wire.h>
// 72700
#define limitLeft 2
#define limitRight 3
#define dirPin 4
#define stepPin 5
#define enable 6
#define shutter 7
#define buzzer 8
#define lowPowerPin A0

RF24 myRadio (9, 10);
byte addresses[][6] = {"2"};
long startPosition = 0, endPosition = 0;
long totalSteps = 0, offset = 0;
int delayVal = 0;
boolean connBreak = false, lS = false;

struct package9
{
  byte stay = 0;
  byte lockSlider = 0;
  byte releaseSlider = 0;
  byte autoHome = 0;
  byte calibrateSlider = 0;
  byte security_Check = 0;
};
typedef struct package9 Package9;
Package9 settingsData;

struct package3
{
  byte mode = 0;
  byte sliderSpeed = 0;
  byte loopVal = 0;
  byte offset = 0;
  byte start = 0;
  byte Pause = 0;
  byte forceExit = 0;
};
typedef struct package3 Package3;
Package3 cncData;

struct package2
{
  byte mode = 200; // homeScreen nothing selected
};
typedef package2 Package2;
Package2 modeData;

struct package1
{
  byte mode = 0;
  byte dir = 0;
  byte sliderSpeed = 0;
};

typedef struct package1 Package1;
Package1 manualData;

struct package8
{
  byte cal = 0;
  byte sliderHomingEnd = 0;
};

typedef struct package8 Package8;
Package8 calData;

struct package15
{
  byte shutterSpeed = 0;
  byte tMode = 0; // 1 for interval timer shooting 2 for time lapse photography
  byte offset = 0;
  byte intervalSec = 0;
  byte intervalMin = 0;
  byte totalTimeHrs = 0;
  byte totalTimeMin = 0;
  byte start = 0;
  byte Pause = 0;
  byte forceExit = 0;
};
typedef struct package15 Package15;
Package15 TimeLapseData;


volatile boolean leftLimit = false, rightLimit = false;

void limitLeftHit() {
  digitalWrite(dirPin, LOW);
  leftLimit = true;
}
void limitRightHit() {
  digitalWrite(dirPin, HIGH);
  rightLimit = true;
}

void homeSlider() {
  digitalWrite(enable, LOW);
  digitalWrite(dirPin, LOW);

  if (securityCheck() && !lowPowerCheck()) {
    while (digitalRead(limitRight) == HIGH) {

      digitalWrite(stepPin, HIGH);
      delayMicroseconds(50);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(50);
    }
  }
}

boolean clickShutter() {
  digitalWrite(shutter, HIGH);
  delay(120);
  digitalWrite(shutter, LOW);
}

boolean lowPowerCheck () {
  int power = map(analogRead(lowPowerPin), 0, 500, 1, 500);
  if (power < 385) { // less power left in battery
    digitalWrite(enable, HIGH);
    if (digitalRead(buzzer) == LOW)
      digitalWrite(buzzer, HIGH);
    else
      digitalWrite(buzzer, LOW);
    delay(1000);
    return true;
  }
  else {
    return false;
    digitalWrite(buzzer, LOW);
  }
}


void calibrate() {
  if (!lowPowerCheck()) {

    homeSlider();
    digitalWrite(dirPin, HIGH);
    leftLimit = false;
    totalSteps = 0;
    delay(1500);

    lowPowerCheck();

    while (digitalRead(limitLeft) == HIGH) {

      digitalWrite(stepPin, HIGH);
      delayMicroseconds(120);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(120);

      totalSteps++;
      if (leftLimit) {
        leftLimit = false;
        break;
      }
    }

    int ts_divisor = (totalSteps / 255);
    EEPROM.write(0, (totalSteps % 255));
    int i = 1;
    while (true) {
      if ( ts_divisor > 255) {
        EEPROM.write(i, 255);
        ts_divisor -= 255;
        i++;
      }
      else if (ts_divisor < 255 && ts_divisor > 0) {
        EEPROM.write(i, ts_divisor);
        break;
      }
    }

    delay(1000);
    homeSlider();
    lowPower(true);
  }
}


boolean chooseStartEndPos() {
  long presentPos = 0;
  startPosition = 0, endPosition = 0;
  long totalAllowedSteps = (totalSteps - (map(TimeLapseData.offset, 1, 15, 763, 11450)));
  lowPowerCheck();
  manualData.mode = 1;
  manualData.dir = 0;

  while (true) {

    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&manualData, sizeof(manualData));
      delayVal = 100;
      if (manualData.dir == 1) {
        digitalWrite(dirPin, HIGH);
      }
      if (manualData.dir == 2) {
        digitalWrite(dirPin, LOW);
      }
      if (manualData.mode != 1)
        break;
    }

    ///////////////
    if (manualData.dir == 0) {
      lowPowerCheck();

      continue;
    }

    if (digitalRead(limitRight) == LOW) {
      lowPowerCheck();
      return false;
    }
    if (presentPos == (totalAllowedSteps) && manualData.dir == 1) {
      lowPowerCheck();

      continue;
    }

    if (presentPos <= totalAllowedSteps) {

      if (presentPos == 0 && manualData.dir == 2) {
        lowPowerCheck();

        continue;
      }

      if (manualData.dir == 1) { // going left
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);

        presentPos++;

        if (presentPos == (totalSteps - offset) && manualData.dir == 1) {
          lowPowerCheck();

          continue;
        }
        if (digitalRead(limitRight) == LOW) {
          lowPowerCheck();
          return false;
        }
      }
      if (manualData.dir == 2) {
        // going right
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);
        presentPos--;

      }
    }
  }
  startPosition = presentPos;
  manualData.dir = 0;
  manualData.mode = 1;

  while (true) {

    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&manualData, sizeof(manualData));
      delayVal = 100;
      if (manualData.dir == 1) {
        digitalWrite(dirPin, HIGH);
      }
      if (manualData.dir == 2) {
        digitalWrite(dirPin, LOW);
      }
      if (manualData.mode != 1)
        break;
    }

    ///////////////
    if (manualData.dir == 0) {
      lowPowerCheck();

      continue;
    }

    if (digitalRead(limitRight) == LOW) {
      lowPowerCheck();
      return false;
    }
    if (presentPos == (totalAllowedSteps) && manualData.dir == 1) {
      lowPowerCheck();

      continue;
    }

    if (presentPos <= totalAllowedSteps) {

      if (presentPos == 0 && manualData.dir == 2) {
        lowPowerCheck();

        continue;
      }

      if (manualData.dir == 1) { // going left
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);

        presentPos++;

        if (presentPos == (totalSteps - offset) && manualData.dir == 1) {
          lowPowerCheck();

          continue;
        }
        if (digitalRead(limitRight) == LOW) {
          lowPowerCheck();
          return false;
        }
      }
      if (manualData.dir == 2) {
        // going right
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);
        presentPos--;

      }
    }
  }


  endPosition = presentPos;
  if (startPosition == endPosition)
    return false;
  else
    return true;
}



void shutDown() {
  digitalWrite(enable,  HIGH);
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzer, HIGH);
    delay(1000);
    digitalWrite(buzzer, LOW);
    delay(1000);
  }
  while (true) {
    digitalWrite(buzzer, HIGH);
    delay(50);
    digitalWrite(buzzer, LOW);
    delay(50);
    digitalWrite(buzzer, HIGH);
    delay(50);
    digitalWrite(buzzer, LOW);
    delay(1000);
  }
}

void lowPower(boolean type) {
  if (type && lS == false)
    digitalWrite(enable, HIGH);
  else
    digitalWrite(enable, LOW);
}


boolean securityCheck() {
  if (digitalRead(limitLeft) == LOW && digitalRead(limitRight) == LOW) {
    digitalWrite(enable, HIGH);
    return false;
  }
  else
    return true;
}

boolean checkForMode() {
  if (modeData.mode == 0) {
    return false;
  }
  else if (modeData.mode == 1) {
    lowPower(false);
    manual();
    digitalWrite(enable, LOW);
    return true;
  }
  else if (modeData.mode == 2) {
    lowPower(false);
    homeSlider();

    cncMode();
    return true;
  }
  else if (modeData.mode == 3) {
    //homeSlider();
    lowPower(false);
    TimeLapse();
    return true;
  }
  else if (modeData.mode == 4) {
    settings();
    return true;
  }
  else if (modeData.mode == 5) {
    shutDown();
    return true;
  }
  else {
    return false;
  }
}



boolean settings() {
  while (true) {
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&settingsData, sizeof(settingsData));
      if (settingsData.stay == 0) {
        //Serial.println("Back selected");
        return true;
      }
      else
        executeSetting();
    }
  }

}
boolean executeSetting() {
  if (settingsData.lockSlider == 1) {
    lS = true;
    if (securityCheck()) {
      digitalWrite(enable, LOW);
    }
  }

  if (settingsData.releaseSlider == 1) {
    lS = false;
    digitalWrite(enable, HIGH);
  }
  if (settingsData.autoHome == 1)
    homeSlider();

  if (settingsData.calibrateSlider == 1) {
    struct package11
    {
      byte csDone = 0;
    };

    typedef struct package11 Package11;
    Package11 calSlider;
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    delay(1);
    for (int i = 0; i < 10; i++)
      myRadio.write(&calSlider, sizeof(calSlider));
    calibrate();
    calSlider.csDone = 1;
    delay(1);
    myRadio.write(&calSlider, sizeof(calSlider));
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    delay(1);
  }

  if (settingsData.security_Check == 1) {
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    delay(1);
    struct package10
    {
      byte sc = 0;
    };

    typedef struct package10 Package10;
    Package10 sCk;
    if (securityCheck())
      sCk.sc = 1;
    else
      sCk.sc = 0;
    for (int i = 0; i < 100; i++)
      myRadio.write(&sCk, sizeof(sCk));
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    delay(1);
  }
}


///////////////////////////////////////////////////////////////////////////// NOT YET DEVELOPED
boolean manual() {

  boolean stp = true, onceRun = false;
  digitalWrite(enable, LOW);
  lowPowerCheck();
  long lpVal = 0;
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();
  delay(1);
  struct package4 {
    byte homing = 0;
  };
  typedef package4 Package4;
  Package4 cncDataExp;
  homeSlider();
  cncDataExp.homing = 1;
  for (int i = 0; i < 10; i++)
    myRadio.write(&cncDataExp, sizeof(cncDataExp));
  delay(1);

  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();

  while (true) {
    lowPowerCheck();
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncData, sizeof(cncData));
      break;
    }
  }
  if (cncData.forceExit == 1) {
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.loopVal = 0;
    cncData.offset = 0;
    cncData.start = 0;
    cncData.Pause = 0;
    cncData.forceExit = 0;
    return false;
  }

  if (cncData.mode == 0) {
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.loopVal = 0;
    cncData.offset = 0;
    cncData.start = 0;
    cncData.Pause = 0;
    cncData.forceExit = 0;
    return false;

  }
  if (cncData.offset > 0) {
    if (securityCheck() && !lowPowerCheck()) {
      digitalWrite(enable, LOW);
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
      myRadio.stopListening();
      struct package5 {
        byte movToStartPos = 0;
      };
      typedef package5 Package5;
      Package5 cnc_MoveToStartPos;

      lpVal = map(cncData.offset, 1, 15, 763, 11450);
      digitalWrite(dirPin, HIGH);
      if (totalSteps > (lpVal * 2)) {
        for (long j = 1; j <= lpVal; j++) {
          lowPowerCheck();
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(40);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(40);
        }
        cnc_MoveToStartPos.movToStartPos = 1;
        for (int i = 0; i < 10; i++)
          myRadio.write(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
        delay(1);
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openReadingPipe(1, addresses[0]);
        myRadio.startListening();
      }
    }
  }
  long presentPos = 0;
  long totalAllowedSteps = (totalSteps - lpVal);
  lowPowerCheck();
  while (true) {

    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&manualData, sizeof(manualData));
      delayVal = map(manualData.sliderSpeed, 1, 200, 700, 10);
      if (manualData.dir == 1) {
        digitalWrite(dirPin, HIGH);
      }
      if (manualData.dir == 2) {
        digitalWrite(dirPin, LOW);
      }
      if (manualData.mode != 1)
        break;
    }

    ///////////////
    if (manualData.dir == 0) {
      lowPowerCheck();

      continue;
    }
    if (manualData.mode != 1)
      return true;

    if (digitalRead(limitRight) == LOW) {
      lowPowerCheck();
      return false;
    }
    if (presentPos == (totalAllowedSteps) && manualData.dir == 1) {
      lowPowerCheck();

      continue;
    }

    if (presentPos <= totalAllowedSteps) {

      if (presentPos == 0 && manualData.dir == 2) {
        lowPowerCheck();

        continue;
      }

      if (manualData.dir == 1) { // going left
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);

        presentPos++;

        if (presentPos == (totalSteps - offset) && manualData.dir == 1) {
          lowPowerCheck();

          continue;
        }
        if (digitalRead(limitRight) == LOW) {
          lowPowerCheck();
          return false;
        }
      }
      if (manualData.dir == 2) {
        // going right
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(delayVal);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(delayVal);
        presentPos--;

      }
    }
  }
}



boolean cncMode() {
  if (!lowPowerCheck()) {
    long lpVal = 0;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    struct package4 {
      byte homing = 0;
    };
    typedef package4 Package4;
    Package4 cncDataExp;
    homeSlider();
    cncDataExp.homing = 1;
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncDataExp, sizeof(cncDataExp));
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();

    while (true) {
      lowPowerCheck();
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cncData, sizeof(cncData));
        break;
      }
    }
    if (cncData.forceExit == 1) {
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      cncData.Pause = 0;
      cncData.forceExit = 0;
      return false;
    }

    if (cncData.mode == 0) {
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      cncData.Pause = 0;
      cncData.forceExit = 0;
      return false;

    }
    if (cncData.offset > 0) {
      offset = cncData.offset;
      if (securityCheck() && !lowPowerCheck()) {
        digitalWrite(enable, LOW);
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openWritingPipe( addresses[0]);
        myRadio.stopListening();
        struct package5 {
          byte movToStartPos = 0;
        };
        typedef package5 Package5;
        Package5 cnc_MoveToStartPos;

        lpVal = map(cncData.offset, 1, 15, 763, 11450);
        digitalWrite(dirPin, HIGH);
        if (totalSteps > (lpVal * 2)) {
          if (securityCheck() && !lowPowerCheck()) {
            for (long j = 1; j <= lpVal; j++) {
              lowPowerCheck();
              digitalWrite(stepPin, HIGH);
              delayMicroseconds(70);
              digitalWrite(stepPin, LOW);
              delayMicroseconds(70);
            }
            cnc_MoveToStartPos.movToStartPos = 1;
            for (int i = 0; i < 10; i++)
              myRadio.write(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
            delay(1);
            myRadio.begin();
            myRadio.setChannel(15);
            myRadio.setPALevel(RF24_PA_MIN);
            myRadio.setDataRate( RF24_250KBPS ) ;
            myRadio.openReadingPipe(1, addresses[0]);
            myRadio.startListening();
          }
        }
      }
    }
    while (true) {
      lowPowerCheck();
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cncData, sizeof(cncData));
        if (cncData.start == 0 || cncData.mode == 0) {
          cncData.mode = 0;
          cncData.sliderSpeed = 0;
          cncData.loopVal = 0;
          cncData.offset = 0;
          cncData.start = 0;
          cncData.Pause = 0;
          cncData.forceExit = 0;
          return false;
        }
        else
          break;
      }
    }
    if (totalSteps > (lpVal)) {
      long lp_Value = 0;
      if (cncData.loopVal == 0)
        lp_Value = 1;
      else
        lp_Value = (cncData.loopVal * 2);
      boolean exF = false;
      boolean poss = false;
      if ((totalSteps - (lpVal)) > 4000)
        poss = true;
      else
        poss = false;
      int dV = delayVal;

      for (long i = 0; i < lp_Value; i++) {

        if (exF)
          break;
        lowPowerCheck();
        if (i % 2 == 0)
          digitalWrite(dirPin, HIGH);
        else
          digitalWrite(dirPin, LOW);

        delayVal = (map(cncData.sliderSpeed, 1, 200, 700, 10));



        for (long j = 1; j < (totalSteps - (lpVal)); j++) {
          //lowPowerCheck();
          ////////////////////////////////////////

          //Serial.println("Decreasing velocity at the end");
          if (poss) {
            if (j == 1) {
              lowPowerCheck;
              dV = delayVal + 500;
              for (int k = 0; k < 4000; k++) {
                digitalWrite(stepPin, HIGH);
                delayMicroseconds(dV);
                digitalWrite(stepPin, LOW);
                delayMicroseconds(dV);
                if (k % 8 == 0)
                  dV--;
              }
              j += 4000;
            }
            if (((totalSteps - lpVal) - j) <= 4000) {
              lowPowerCheck;
              dV = delayVal;
              for (int k = 0; k < ((totalSteps - lpVal) - j); k++) {
                if (k % 8 == 0)
                  dV++;
                digitalWrite(stepPin, HIGH);
                delayMicroseconds(dV);
                digitalWrite(stepPin, LOW);
                delayMicroseconds(dV);
              }
              j = (totalSteps - lpVal);
            }
          }
          /////////////////////////////////////////////////////////////////////
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(delayVal);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(delayVal);

          if (myRadio.available()) {
            lowPowerCheck();
            while (myRadio.available())
              myRadio.read(&cncData, sizeof(cncData));
            if (cncData.forceExit == 1) {
              cncData.mode = 0;
              cncData.sliderSpeed = 0;
              cncData.loopVal = 0;
              cncData.offset = 0;
              cncData.start = 0;
              cncData.Pause = 0;
              cncData.forceExit = 0;
              exF = true;
              break;
            }
            if (cncData.Pause == 1) {
              while (true) {
                if (myRadio.available()) {
                  while (myRadio.available())
                    myRadio.read(&cncData, sizeof(cncData));
                  if (cncData.Pause == 0)
                    break;
                  lowPowerCheck();
                }
                delay(10);
              }
            }
          }


        }// inner loop


      }// outer loop


    }
    struct package6
    {
      byte jobEnd = 0;
    };
    typedef struct package6 Package6;
    Package6 cncDataJobEnd;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    cncDataJobEnd.jobEnd = 1;
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncDataJobEnd, sizeof(cncDataJobEnd));
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    ////////////////////////////////////////////////////////////////////////////////////////
  }
}


boolean TimeLapse () {
  TimeLapseData.offset = 0;
  TimeLapseData.shutterSpeed = 0;
  TimeLapseData.tMode = 0;
  TimeLapseData.forceExit = 0;
  TimeLapseData.Pause = 0;
  TimeLapseData.start = 0;
  TimeLapseData.intervalMin = 0;
  TimeLapseData.intervalSec = 0;
  TimeLapseData.totalTimeHrs = 0;
  TimeLapseData.totalTimeMin = 0;
  digitalWrite(enable, LOW);
  lowPowerCheck();
  long lpVal = 0;
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();
  delay(1);
  struct package4 {
    byte homing = 0;
  };
  typedef package4 Package4;
  Package4 cncDataExp;
  homeSlider();
  cncDataExp.homing = 1;
  for (int i = 0; i < 10; i++)
    myRadio.write(&cncDataExp, sizeof(cncDataExp));
  delay(1);

  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();

  while (true) {
    lowPowerCheck();
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncData, sizeof(cncData));
      break;
    }
  }
  if (cncData.forceExit == 1) {
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.loopVal = 0;
    cncData.offset = 0;
    cncData.start = 0;
    cncData.Pause = 0;
    cncData.forceExit = 0;
    TimeLapseData.offset = 0;
    TimeLapseData.shutterSpeed = 0;
    TimeLapseData.tMode = 0;
    TimeLapseData.forceExit = 0;
    TimeLapseData.Pause = 0;
    TimeLapseData.start = 0;
    TimeLapseData.intervalSec = 0;
    TimeLapseData.intervalMin = 0;
    return false;
  }

  if (cncData.mode == 0) {
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.loopVal = 0;
    cncData.offset = 0;
    cncData.start = 0;
    cncData.Pause = 0;
    cncData.forceExit = 0;
    TimeLapseData.offset = 0;
    TimeLapseData.shutterSpeed = 0;
    TimeLapseData.tMode = 0;
    TimeLapseData.forceExit = 0;
    TimeLapseData.Pause = 0;
    TimeLapseData.start = 0;
    TimeLapseData.intervalMin = 0;
    TimeLapseData.intervalSec = 0;

    TimeLapseData.totalTimeHrs = 0;
    TimeLapseData.totalTimeMin = 0;
    return false;

  }
  if (cncData.offset > 0) {
    if (securityCheck() && !lowPowerCheck()) {
      digitalWrite(enable, LOW);
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
      myRadio.stopListening();
      struct package5 {
        byte movToStartPos = 0;
      };
      typedef package5 Package5;
      Package5 cnc_MoveToStartPos;

      lpVal = map(cncData.offset, 1, 15, 763, 11450);
      digitalWrite(dirPin, HIGH);
      if (totalSteps > (lpVal * 2)) {
        for (long j = 1; j <= lpVal; j++) {
          lowPowerCheck();
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(40);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(40);
        }
        cnc_MoveToStartPos.movToStartPos = 1;
        for (int i = 0; i < 10; i++)
          myRadio.write(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
        cncData.mode = 0;
      }
    }
  }
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();

  while (true) {

    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&TimeLapseData, sizeof(TimeLapseData));
      if (TimeLapseData.forceExit == 1 || TimeLapseData.tMode < 1 || TimeLapseData.tMode > 2) {
        TimeLapseData.offset = 0;
        TimeLapseData.shutterSpeed = 0;
        TimeLapseData.tMode = 0;
        TimeLapseData.forceExit = 0;
        TimeLapseData.Pause = 0;
        TimeLapseData.start = 0;
        TimeLapseData.intervalMin = 0;
        TimeLapseData.intervalSec = 0;
        TimeLapseData.totalTimeHrs = 0;
        TimeLapseData.totalTimeMin = 0;
        break;
      }
      else {
        executeTimeLapse(TimeLapseData.tMode);
        break;
      }
    }
  }
  modeData.mode = 0;
}

boolean executeTimeLapse(int p) {
  if (p == 1) {
    // Time Lapse
    chooseStartEndPos();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&TimeLapseData, sizeof(TimeLapseData));
        if (TimeLapseData.forceExit == 1) {
          TimeLapseData.offset = 0;
          TimeLapseData.shutterSpeed = 0;
          TimeLapseData.tMode = 0;
          TimeLapseData.forceExit = 0;
          TimeLapseData.Pause = 0;
          TimeLapseData.start = 0;
          TimeLapseData.intervalMin = 0;
          TimeLapseData.intervalSec = 0;
          TimeLapseData.totalTimeHrs = 0;
          TimeLapseData.totalTimeMin = 0;
          return false;
        }
        else
          break;
      }
    }
    if ((TimeLapseData.intervalSec > 0 || TimeLapseData.intervalMin > 0) && ( TimeLapseData.totalTimeMin > 0 || TimeLapseData.totalTimeHrs > 0)) {
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
      struct package16 {
        byte startPosReached = 0;
      };
      typedef struct package16 Package16;
      Package16 stpReached;
      lowPowerCheck();
      if (startPosition > endPosition && !lowPowerCheck()) {
        digitalWrite(dirPin, HIGH);
        for (int i = 1; i < (startPosition - endPosition); i++) {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(80);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(80);
        }
        digitalWrite(dirPin, LOW);
      }
      else {
        digitalWrite(dirPin, LOW);
        for (int i = 1; i < (endPosition - startPosition); i++) {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(80);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(80);
        }
        digitalWrite(dirPin, HIGH);
      }
      stpReached.startPosReached = 1;
      for (int i = 0; i < 10; i++)
        myRadio.write(&stpReached, sizeof(stpReached));
      delay(1);
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openReadingPipe(1, addresses[0]);
      myRadio.startListening();
    }
    while (true) {
      lowPowerCheck();
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cncData, sizeof(cncData));
        if (cncData.start == 0 || cncData.mode == 0) {
          cncData.mode = 0;
          cncData.start = 0;
          return false;
        }
        else {
          cncData.mode = 0;
          cncData.start = 0;
          break;
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////
    boolean exF = false;
    long totalMoves = (((TimeLapseData.totalTimeMin + (TimeLapseData.totalTimeHrs * 60)) * 60) / (TimeLapseData.shutterSpeed + (TimeLapseData.intervalSec + (TimeLapseData.intervalMin * 60))));
    if (startPosition > endPosition)
      startPosition -= endPosition;
    else
      startPosition = (endPosition - startPosition);

    if (startPosition % 2 != 0)
      startPosition += 1;

    if (startPosition < totalMoves) // many photo but less distance
      return false;

    if (startPosition % totalMoves > 0)
      startPosition -= (startPosition % totalMoves);

    long waitingTime = ((1000 * (TimeLapseData.shutterSpeed + (TimeLapseData.intervalSec + (TimeLapseData.intervalMin * 60)))) - (((startPosition / totalMoves) * 160) / 1000)) - 120;

    for ( long k = totalMoves ; k > 0 ; k--) {
      lowPowerCheck();
      if (exF)
        break;
      for (long j = 0; j < (startPosition / totalMoves); j++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(80);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(80);
        if (myRadio.available()) {
          lowPowerCheck();
          while (myRadio.available())
            myRadio.read(&cncData, sizeof(cncData));
          if (cncData.forceExit == 1) {
            cncData.mode = 0;
            cncData.sliderSpeed = 0;
            cncData.loopVal = 0;
            cncData.offset = 0;
            cncData.start = 0;
            cncData.Pause = 0;
            cncData.forceExit = 0;
            exF = true;
            break;
          }
          if (cncData.Pause == 1) {
            while (true) {
              if (myRadio.available()) {
                while (myRadio.available())
                  myRadio.read(&cncData, sizeof(cncData));
                if (cncData.Pause == 0) {
                  cncData.mode = 0;
                  cncData.sliderSpeed = 0;
                  cncData.offset = 0;
                  cncData.loopVal = 0;
                  cncData.start = 0;
                  break;
                }
                lowPowerCheck();
              }
              delay(10);
            }
          }
        }
      }
      clickShutter();
      delay(waitingTime);
    }

    struct package6
    {
      byte jobEnd = 0;
    };
    typedef struct package6 Package6;
    Package6 cncDataJobEnd;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    cncDataJobEnd.jobEnd = 1;
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncDataJobEnd, sizeof(cncDataJobEnd));
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    TimeLapseData.offset = 0;
    TimeLapseData.shutterSpeed = 0;
    TimeLapseData.tMode = 0;
    TimeLapseData.forceExit = 0;
    TimeLapseData.Pause = 0;
    TimeLapseData.start = 0;
    TimeLapseData.intervalMin = 0;
    TimeLapseData.intervalSec = 0;
    TimeLapseData.totalTimeHrs = 0;
    TimeLapseData.totalTimeMin = 0;

  }
  else if (p == 2) {
    // Interval Shooting
    if (!chooseStartEndPos())
      return false;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  }
  else
    return false;
}


void setup() {
  //Serial.begin(9600);


  pinMode(limitLeft, INPUT);
  pinMode(limitRight, INPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(enable, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(lowPowerPin, INPUT);
  digitalWrite(enable, LOW);
  lowPowerCheck();

  attachInterrupt(0, limitLeftHit, FALLING);
  attachInterrupt(1, limitRightHit, FALLING);
  //attachPinChangeInterrupt(fault, detectFault, FALLING);
  long c = 0;
  if (EEPROM.read(1) == 1)
    totalSteps = 0;
  else {
    for (int i = 1; i <= 1000; i++) {
      if (EEPROM.read(i) == 1)
        break;
      else c += EEPROM.read(i);
    }
    totalSteps = ((long)(EEPROM.read(0))) + (c * 255);

  }


  if (!lowPowerCheck()) {
    if (totalSteps == 0) {
      calibrate();
      delay(1000);
    }

    digitalWrite(enable, LOW);
    lowPowerCheck();

    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
  }

}

void loop() {
  lowPower(true);
  if (!lowPowerCheck()) {

    connBreak = false;
    if ( myRadio.available())
    {
      lowPowerCheck();
      //Serial.println("Transmitter responded");
      while (myRadio.available())
        myRadio.read( &modeData, sizeof(modeData) );
      lowPowerCheck();
      checkForMode();
    }
    else {
      modeData.mode = 200;
    }
    if (modeData.mode == 200)
      //Serial.println("No response from transmitter");
      delay(10);
  }

}

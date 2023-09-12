//////////////////////////   1-Axis Motorized Camera Dolly   ////////////////////////////////
// TRANSMITTER
#include <EEPROM.h>
#include "PinChangeInt.h"
#include <SPI.h>
#include "RF24.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define R_clk 2 // 2
#define R_button 3 // 3
#define R_dt 4
#define B_middle 5

#define pot1 A0

LiquidCrystal_I2C lcd(0x27, 20, 4);

RF24 myRadio (9, 10); // (CE, CSN)
byte addresses[][6] = {"2"};

int  cursorPosition = 1, speedVal = 1, cache = 0, LoopValue = 0, offsetVal = 0, settingPosition = 0;

String homeScreenElements[] = {
  "Manual",
  "CNC Mode",
  "Time Lapse",
  "Menu",
  "Shut Down"
};

String settingsElements[] = {
  "Back",
  "Lock Slider",
  "Release Slider",
  "Auto Home",
  "Measure Length",
  "Clearance",
  "Security Check",
  "Key-Test",
  "Reset Factory"
};

byte arrow[8] = {
  0b01000,
  0b01100,
  0b01110,
  0b01111,
  0b01111,
  0b01110,
  0b01100,
  0b01000,
};
byte sparrow[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
};
byte slider_ends[8] = {
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
};
byte slider_rail[8] = {
  0b00000,
  0b11111,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b00000,
};
byte slider_chassey1[8] = {
  0b11111,
  0b10010,
  0b11000,
  0b11001,
  0b11001,
  0b11000,
  0b10010,
  0b11111,
};
byte slider_chassey2[8] = {
  0b11111,
  0b01001,
  0b00011,
  0b10011,
  0b10011,
  0b00011,
  0b01001,
  0b11111,
};


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


struct package8
{
  byte cal = 0;
  byte sliderHomingEnd = 0;
};

typedef struct package8 Package8;
Package8 calData;

struct package2
{
  byte mode = 0; // Home Screen
};

typedef struct package2 Package2;
Package2 modeData;

struct package1
{
  byte mode = 0;
  byte dir = 0;
  byte sliderSpeed = 0;
};

typedef struct package1 Package1;
Package1 manualData;

volatile boolean TurnDetected = false, up = false, ButtonPressed = false, middleButton = false;
boolean jobDone = false;

void initialise () {
  TurnDetected = false;
  up = false;
  ButtonPressed = false;
  middleButton = false;
  cursorPosition = 1;
  speedVal = 1;
}

void reset_Factory() {
  TurnDetected = false;
  up = false;
  ButtonPressed = false;
  middleButton = false;
  cursorPosition = 1;
  speedVal = 1;
  EEPROM.write(0, 15);
}


void rotary_encoder () {
  Serial.println("r");
  TurnDetected = true;
  up = (digitalRead(R_clk) == digitalRead(R_dt));
}

void rotary_button () {
  ButtonPressed = true;
  //Serial.println("Button Pressed");
  delay(1);
}

void middle_button () {
  middleButton = true;
  delay(1);
}


void shutDown() {
  drawSlider(1, true);
  lcd.setCursor(6, 2);
  lcd.print("Good Bye");
  delay(3000);
  lcd.clear();
  lcd.init();
  while (true) {
    delay(500);
  }
}




void sendManualData () {
  myRadio.write( &manualData, sizeof(manualData));
  delay(1);
}


boolean chooseStartEndPos() {
  boolean l = false, r = false;
  manualData.mode = 1;
  manualData.dir = 0;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(1, 1);
  lcd.print("Select Start point");
  lcd.setCursor(5, 2);
  lcd.print("Slider is-");
  lcd.setCursor(7, 3);
  lcd.print("Steady");
  delay(100);
  while (true) {
    if (TurnDetected) {
      if (up) {
        if (!r) {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Time Lapse Mode-");
          lcd.setCursor(1, 1);
          lcd.print("Select Start point");
          lcd.setCursor(2, 2);
          lcd.print("Slider Moving to");
          lcd.setCursor(15, 3);
          lcd.print("Right");
        }
        r = true;
        l = false;
        manualData.dir = 2; // Right
      }
      else {
        if (!l) {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Time Lapse Mode-");
          lcd.setCursor(1, 1);
          lcd.print("Select Start point");
          lcd.setCursor(2, 2);
          lcd.print("Slider Moving to");
          lcd.setCursor(0, 3);
          lcd.print("Left");
        }
        l = true;
        r = false;
        manualData.dir = 1; // Left
      }
      TurnDetected = false;
    }
    if (ButtonPressed) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(1, 1);
      lcd.print("Select Start point");
      lcd.setCursor(5, 2);
      lcd.print("Slider is-");
      lcd.setCursor(7, 3);
      lcd.print("Steady");
      manualData.dir = 0;
      delay(50);
      l = false;
      r = false;
      ButtonPressed = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW)
        delay(10);
      middleButton = false;
      manualData.mode = 0;
      manualData.dir = 0;
      middleButton = false;
      sendManualData();
      break;
    }
    sendManualData();
  }
  delay(100);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(2, 1);
  lcd.print("Select End point");
  lcd.setCursor(5, 2);
  lcd.print("Slider is-");
  lcd.setCursor(7, 3);
  lcd.print("Steady");
  manualData.mode = 1;
  manualData.dir = 0;
  while (true) {
    if (TurnDetected) {
      if (up) {
        if (!r) {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Time Lapse Mode-");
          lcd.setCursor(2, 1);
          lcd.print("Select End point");

          lcd.setCursor(2, 2);
          lcd.print("Slider Moving to");
          lcd.setCursor(15, 3);
          lcd.print("Right");
        }
        r = true;
        l = false;
        manualData.dir = 2; // Right
      }
      else {
        if (!l) {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Time Lapse Mode-");
          lcd.setCursor(2, 1);
          lcd.print("Select End point");
          lcd.setCursor(2, 2);
          lcd.print("Slider Moving to");
          lcd.setCursor(0, 3);
          lcd.print("Left");
        }
        l = true;
        r = false;
        manualData.dir = 1; // Left
      }
      TurnDetected = false;
    }
    if (ButtonPressed) {
      manualData.dir = 0;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(2, 1);
      lcd.print("Select End point");
      lcd.setCursor(5, 2);
      lcd.print("Slider is-");
      lcd.setCursor(7, 3);
      lcd.print("Steady");
      delay(50);
      l = false;
      r = false;
      ButtonPressed = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW)
        delay(10);
      middleButton = false;
      manualData.mode = 0;
      manualData.dir = 0;
      middleButton = false;
      sendManualData();
      break;
    }
    sendManualData();
  }
  middleButton = false;
}




boolean drawLcdSetting(int p) {
  lcd.clear();
  if (p >= 0 && p <= 3) {
    for (int i = 0; i < 4; i++) {
      lcd.setCursor(1, i);
      lcd.print(settingsElements[i]);
    }
    lcd.setCursor(0, p);
    lcd.write(1);
  }
  else if (p >= 4 && p <= 7) {
    for (int i = 4; i <= 7; i++) {
      lcd.setCursor(1, (i - 4));
      lcd.print(settingsElements[i]);
    }
    lcd.setCursor(0, (p - 4));
    lcd.write(1);
  }
  else if (p == 8) {
    lcd.setCursor(1, (p - 8));
    lcd.print(settingsElements[p]);
    lcd.setCursor(0, (p - 8));
    lcd.write(1);
  }
  else
    return false;
}

void updateSettingData(int p) {
  if (p == 1)
    settingsData.lockSlider = 1;
  else if (p == 2)
    settingsData.releaseSlider = 1;
  else if (p == 3)
    settingsData.autoHome = 1;
  else if (p == 4)
    settingsData.calibrateSlider = 1;
  else if (p == 6)
    settingsData.security_Check = 1;
}

boolean executeSetting2 (int p) {
  if (p == 5) {
    int cVal = EEPROM.read(0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Clearance Value-");
    lcd.setCursor(8, 2);
    lcd.write(2);
    lcd.setCursor(9, 2);
    lcd.print(cVal);
    lcd.setCursor(12, 2);
    lcd.print("cm");
    delay(100);
    while (true) {
      delay(10);
      if (TurnDetected) {
        if (up) {
          cVal++;
          if (cVal > 15)
            cVal = 1;
        }
        else {
          cVal--;
          if (cVal < 1)
            cVal = 15;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Clearance Value-");
        lcd.setCursor(8, 2);
        lcd.write(2);
        lcd.setCursor(9, 2);
        lcd.print(cVal);
        lcd.setCursor(12, 2);
        lcd.print("cm");
        TurnDetected = false;

      }
      if (ButtonPressed) {
        delay(50);
        EEPROM.write(0, cVal);
        ButtonPressed = false;
        break;
      }
      if (middleButton) {
        while (digitalRead(B_middle) == LOW) {
          delay(1);
        }
        middleButton = false;
        middleButton = false;
        break;
      }
    }
  }

  if (p == 7) {
    boolean exitButton = false, knob = false;
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("Key Test");
    lcd.setCursor(1, 1);
    lcd.print("Button is OK");
    delay(100);
    while (true) {
      delay(10);
      if (TurnDetected) {
        knob = true;
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("Key Test");
        lcd.setCursor(1, 1);
        lcd.print("Button is OK");
        if (knob) {
          lcd.setCursor(1, 2);
          lcd.print("Knob is OK");
        }
        if (exitButton) {
          lcd.setCursor(1, 3);
          lcd.print("Exit button is OK");
        }
        delay(10);
        TurnDetected = false;
      }
      if (middleButton && !exitButton) {
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("Key Test");
        lcd.setCursor(1, 1);
        lcd.print("Button is OK");
        exitButton = true;
        if (knob) {
          lcd.setCursor(1, 2);
          lcd.print("Knob is OK");
        }
        if (exitButton) {
          lcd.setCursor(1, 3);
          lcd.print("Exit button is OK");
        }
        delay(10);
        middleButton = false;
      }
      if (middleButton && exitButton) {
        delay(50);
        middleButton = false;
        break;
      }
    }
  }


  if (p == 8) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Factory--Reset");
    lcd.setCursor(3, 2);
    lcd.print("Are you sure ?");
    lcd.setCursor(15, 3);
    lcd.write(1);
    lcd.setCursor(3, 3);
    lcd.print("Yes");
    lcd.setCursor(16, 3);
    lcd.print("No");
    int b = 15; // temporary cursor
    middleButton = false;
    TurnDetected = false;
    ButtonPressed = false;
    delay(100);
    while (true) {
      delay(10);
      if (middleButton) {
        while (digitalRead(B_middle) == LOW) {
          delay(1);
        }
        middleButton = false;
        middleButton = false;
        break;
      }
      if (TurnDetected) {
        if (up) {
          b = 15;
        }
        else
          b = 2;
        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print("Factory--Reset");
        lcd.setCursor(3, 2);
        lcd.print("Are you sure ?");
        lcd.setCursor(b, 3);
        lcd.write(1);
        lcd.setCursor(3, 3);
        lcd.print("Yes");
        lcd.setCursor(16, 3);
        lcd.print("No");
        TurnDetected = false;
      }
      if (ButtonPressed) {
        delay(50);
        //Serial.println(b);
        if (b == 2) {
          reset_Factory();
        }

        ButtonPressed = false;
        break;
      }

    }
  }
}


boolean executeSetting(int p) {
  if (p == 4) {
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
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    delay(1);
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("Please Wait!");
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&calSlider, sizeof(calSlider));
        lcd.clear();
        if (calSlider.csDone == 1) {
          lcd.setCursor(2, 1);
          lcd.print("Length Measured!");
          delay(1500);
          break;
        }
        else {
          lcd.setCursor(1, 1);
          lcd.print("Slider Calibrating");
          lcd.setCursor(4, 2);
          lcd.print("Please Wait!");
        }

      }
    }
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    delay(1);
    drawLcdSetting(p);
  }
  if (p == 6) {
    struct package10
    {
      byte sc = 0;
    };

    typedef struct package10 Package10;
    Package10 sCk;
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    delay(1);
    lcd.clear();
    lcd.setCursor(4, 1);
    lcd.print("Please Wait!");
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&sCk, sizeof(sCk));
        lcd.clear();
        if (sCk.sc == 1) {
          lcd.setCursor(4, 1);
          lcd.print("Slider is OK");
        }
        else {
          lcd.setCursor(3, 1);
          lcd.print("Slider Damaged");
          lcd.setCursor(0, 2);
          lcd.print("Check Limit Switches");
        }
        delay(1500);
        break;
      }
    }
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    delay(1);
    drawLcdSetting(p);
  }
}

boolean settings() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(1);
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(1, i);
    lcd.print(settingsElements[i]);
  }
  delay(100);
  TurnDetected = false;
  middleButton = false;
  ButtonPressed = false;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        settingPosition++;
        if (settingPosition >= 9)
          settingPosition = 0;
      }
      else {
        settingPosition--;
        if (settingPosition < 0)
          settingPosition = 8;
      }
      TurnDetected = false;
      drawLcdSetting(settingPosition);
    }
    if (middleButton) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      middleButton = false;
      settingsData.stay = 0;
      settingsData.lockSlider = 0;
      settingsData.releaseSlider = 0;
      settingsData.autoHome = 0;
      settingsData.security_Check = 0;
      return true;
    }
    if (ButtonPressed) {
      delay(10);
      ButtonPressed = false;
      TurnDetected = false;
      middleButton = false;
      if (settingPosition == 0 || settingPosition == 5 || settingPosition == 7 || settingPosition == 8) {
        settingsData.stay = 0; // asking slider to stay to loop

        myRadio.write(&settingsData, sizeof(settingsData));
        if (settingPosition == 0) {
          settingsData.stay = 0;
          settingsData.lockSlider = 0;
          settingsData.releaseSlider = 0;
          settingsData.autoHome = 0;
          settingsData.security_Check = 0;
          return true;
        }
        else {
          executeSetting2(settingPosition);
          drawLcdSetting(settingPosition);
        }
      }
      else {
        settingsData.stay = 1;
        updateSettingData(settingPosition);
        myRadio.write(&settingsData, sizeof(settingsData));
        settingsData.lockSlider = 0;
        settingsData.releaseSlider = 0;
        settingsData.autoHome = 0;
        settingsData.security_Check = 0;
      }

      executeSetting(settingPosition);
      drawLcdSetting(settingPosition);
      TurnDetected = false;
      middleButton = false;
      ButtonPressed = false;
    }
  }
}



void homeScreen(int cursorPos) {
  lcd.clear();
  switch (cursorPos) {
    case 1:
      lcd.setCursor(0, 1);
      lcd.write(1);
      break;
    case 2:
      lcd.setCursor(0, 2);
      lcd.write(1);
      break;
    case 3:
      lcd.setCursor(0, 3);
      lcd.write(1);
      break;
    case 4:
      lcd.setCursor(10, 1);
      lcd.write(1);
      break;
    case 5:
      lcd.setCursor(10, 2);
      lcd.write(1);
      break;
    default:
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("System Failed!");
  }

  drawSlider(0, false);
  lcd.setCursor(1, 1);
  lcd.print(homeScreenElements[0]);
  lcd.setCursor(1, 2);
  lcd.print(homeScreenElements[1]);
  lcd.setCursor(1, 3);
  lcd.print(homeScreenElements[2]);
  lcd.setCursor(11, 1);
  lcd.print(homeScreenElements[3]);
  lcd.setCursor(11, 2);
  lcd.print(homeScreenElements[4]);
}

void enterMode(int modeVal) {
  switch (modeVal) {
    case 1:
      modeData.mode = 1;
      manualData.mode = 1;
      myRadio.write(&modeData, sizeof(modeData));
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      manual();

      modeData.mode = 0;
      myRadio.write(&modeData, sizeof(modeData));
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      homeScreen(cursorPosition);
      break;
    case 2:
      modeData.mode = 2;
      cncData.mode = 2;
      myRadio.write(&modeData, sizeof(modeData));
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      cncMode();
      modeData.mode = 0;
      myRadio.write(&modeData, sizeof(modeData));
      cncData.mode = 0;
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      homeScreen(cursorPosition);
      break;
    case 3:
      modeData.mode = 3;
      manualData.mode = 3;
      myRadio.write(&modeData, sizeof(modeData));
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      TimeLapse();
      modeData.mode = 0;
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      myRadio.write(&modeData, sizeof(modeData));
      homeScreen(cursorPosition);
      break;
    case 4:
      modeData.mode = 4;
      manualData.mode = 4;
      myRadio.write(&modeData, sizeof(modeData));
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      settings();
      modeData.mode = 0;
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      myRadio.write(&modeData, sizeof(modeData));
      homeScreen(cursorPosition);
      break;
    case 5:
      modeData.mode = 5;
      myRadio.write(&modeData, sizeof(modeData));
      shutDown();
      break;
    default:
      lcd.clear();
      lcd.setCursor(3, 1);
      TurnDetected = false;
      up = false;
      ButtonPressed = false;
      middleButton = false;
      lcd.print("System Failed!");
  }
}

boolean readyForManual() {
  cncData.mode = 2;
  cncData.offset = EEPROM.read(0);

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Manual Mode");
  lcd.setCursor(4, 1);
  lcd.print("Please Wait-");
  lcd.setCursor(3, 2);
  lcd.print("Slider Homing!");
  ButtonPressed = false;
  middleButton = false;
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  delay(1);
  struct package4 {
    byte homing = 0;
  };
  typedef package4 Package4;
  Package4 cncDataExp;
  while (true) {
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncDataExp, sizeof(cncDataExp));
    }
    if (cncDataExp.homing == 1)
      break;
    if (middleButton) {
      delay(50);
      middleButton = false;
      cncData.forceExit = 1;
    }
  }

  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();
  delay(1);
  if (cncData.offset > 0) {
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncData, sizeof(cncData));

    delay(1);
    if (cncData.forceExit == 1) {
      cncData.forceExit = 0;
      cncData.mode = 0;
      middleButton = false;
      ButtonPressed = false;
      TurnDetected = false;
      return false;

    }
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    struct package5 {
      byte movToStartPos = 0;
    };
    typedef package5 Package5;
    Package5 cnc_MoveToStartPos;
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Manual Mode");
    lcd.setCursor(4, 1);
    lcd.print("Please Wait-");
    lcd.setCursor(2, 2);
    lcd.print("Slider Moving to");
    lcd.setCursor(3, 3);
    lcd.print("Start Position");
    delay(100);
    while (true) {
      //delay(10);
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
        if (cnc_MoveToStartPos.movToStartPos == 1)
          break;
      }
      else {
        if (middleButton) {
          middleButton = false;

          delay(1);

          myRadio.begin();
          myRadio.setChannel(15);
          myRadio.setPALevel(RF24_PA_MIN);
          myRadio.setDataRate( RF24_250KBPS ) ;
          myRadio.openWritingPipe( addresses[0]);
          myRadio.stopListening();
          cncData.forceExit = 1;
          delay(1);
          myRadio.write(&cncData, sizeof(cncData));
          cncData.mode = 0;
          cncData.sliderSpeed = 0;
          cncData.loopVal = 0;
          cncData.offset = 0;
          cncData.start = 0;
          cncData.Pause = 0;
          cncData.forceExit = 0;
          return false;
        }


      }
      delayMicroseconds(1);
    }
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    // slider moved to start position
    cncData.mode = 0;
    return true;
  }

}


boolean manual () {
  if (readyForManual()) {
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("Manual Mode-");
    lcd.setCursor(4, 1);
    lcd.print("Slider Speed");
    speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
    lcd.setCursor(8, 2);
    lcd.write(2);
    lcd.setCursor(9, 2);
    lcd.print(speedVal);
    lcd.setCursor(7, 3);
    lcd.print("Steady");
    ButtonPressed = false;
    manualData.sliderSpeed = speedVal;

    boolean l = false, r = false;
    cache = analogRead(pot1);

    while (true) {
      if (TurnDetected) {
        if (up) {
          if (!r) {
            lcd.clear();
            lcd.setCursor(4, 0);
            lcd.print("Manual Mode-");
            lcd.setCursor(4, 1);
            lcd.print("Slider Speed");
            lcd.setCursor(8, 2);
            lcd.write(2);
            lcd.setCursor(9, 2);
            lcd.print(speedVal);
            lcd.setCursor(15, 3);
            lcd.print("Right");
          }
          r = true;
          l = false;
          manualData.dir = 2; // Right
        }
        else {
          if (!l) {
            lcd.clear();
            lcd.setCursor(4, 0);
            lcd.print("Manual Mode-");
            lcd.setCursor(4, 1);
            lcd.print("Slider Speed");
            lcd.setCursor(8, 2);
            lcd.write(2);
            lcd.setCursor(9, 2);
            lcd.print(speedVal);
            lcd.setCursor(0, 3);
            lcd.print("Left");
          }
          l = true;
          r = false;
          manualData.dir = 1; // Left
        }
        TurnDetected = false;
      }
      if (ButtonPressed) {
        manualData.dir = 0;
        delay(50);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Manual Mode-");
        lcd.setCursor(4, 1);
        lcd.print("Slider Speed");
        lcd.setCursor(8, 2);
        lcd.write(2);
        lcd.setCursor(9, 2);
        lcd.print(speedVal);
        lcd.setCursor(7, 3);
        lcd.print("Steady");
        l = false;
        r = false;
        ButtonPressed = false;
      }
      if (middleButton) {
        delay(50);
        manualData.mode = 0;
        manualData.dir = 0;
        manualData.sliderSpeed = 0;
        middleButton = false;
        break;
      }
      // If speed is changed during process
      if ((cache - (analogRead(pot1))) > 7 || (cache - (analogRead(pot1))) < -7) {
        cache = analogRead(pot1);
        speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
        manualData.sliderSpeed = speedVal;
        delay(100);
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Manual Mode-");
        lcd.setCursor(4, 1);
        lcd.print("Slider Speed");
        lcd.setCursor(8, 2);
        lcd.write(2);
        lcd.setCursor(9, 2);
        lcd.print(speedVal);
        if (l) {
          lcd.setCursor(0, 3);
          lcd.print("Left");
        }
        else if (r) {
          lcd.setCursor(15, 3);
          lcd.print("Right");
        }
        else {
          lcd.setCursor(7, 3);
          lcd.print("Steady");
        }
      }
      sendManualData();
    }
  }
}

// Transmitter
boolean cncMode() {
  int c = EEPROM.read(0);
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("CNC Mode");
  lcd.setCursor(4, 1);
  lcd.print("Please Wait-");
  lcd.setCursor(3, 2);
  lcd.print("Slider Homing!");
  ButtonPressed = false;
  middleButton = false;
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  struct package4 {
    byte homing = 0;
  };
  typedef package4 Package4;
  Package4 cncDataExp;
  while (true) {
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncDataExp, sizeof(cncDataExp));
    }
    if (cncDataExp.homing == 1)
      break;
  }
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("CNC Mode");
  lcd.setCursor(4, 1);
  lcd.print("Select Speed");
  lcd.setCursor(7, 2);
  lcd.write(2);
  lcd.setCursor(8, 2);
  cache = analogRead(pot1);
  speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
  lcd.print(speedVal);
  lcd.setCursor(2, 3);
  lcd.print("Clearance- ");

  lcd.print(c);
  lcd.print(" cm");

  TurnDetected = false;
  ButtonPressed = false;
  // leftButton = false;
  //rightButton = false;
  delay(100);
  while (true) {
    delay(10);
    if ((cache - (analogRead(pot1))) > 7 || (cache - (analogRead(pot1))) < -7) {
      cache = analogRead(pot1);
      speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("CNC Mode");
      lcd.setCursor(4, 1);
      lcd.print("Select Speed");
      lcd.setCursor(7, 2);
      lcd.write(2);
      lcd.setCursor(8, 2);
      lcd.print(speedVal);
      lcd.setCursor(2, 3);
      lcd.print("Clearance- ");

      lcd.print(c);
      lcd.print(" cm");

      delay(100);
    }
    if (ButtonPressed) {
      ButtonPressed = false;
      delay(100);
      break;
    }
    if (middleButton) {
      middleButton = false;
      cncData.forceExit = 1;

      for (int i = 0; i < 50; i++) {
        myRadio.write(&cncData, sizeof(cncData));
        delay(1);
      }
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      cncData.Pause = 0;
      cncData.forceExit = 0;
      return false;
    }
  }
  cncData.mode = 2;
  cncData.sliderSpeed = speedVal;

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("CNC Mode");
  lcd.setCursor(4, 1);
  lcd.print("Select Loops");
  lcd.setCursor(7, 2);
  lcd.write(2);
  lcd.setCursor(8, 2);
  lcd.print(LoopValue);
  lcd.setCursor(2, 3);
  lcd.print("Clearance- ");

  lcd.print(c);
  lcd.print(" cm");

  TurnDetected = false;
  ButtonPressed = false;
  middleButton = false;
  //leftButton = false;
  //rightButton = false;
  delay(100);
  while (true) {
    delay(10);
    if (TurnDetected) {
      TurnDetected = false;
      if (up) {
        LoopValue++;
        if (LoopValue > 200)
          LoopValue = 0;
      }
      else {
        LoopValue--;
        if (LoopValue < 0)
          LoopValue = 200;
      }

      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("CNC Mode");
      lcd.setCursor(4, 1);
      lcd.print("Select Loops");
      lcd.setCursor(7, 2);
      lcd.write(2);
      lcd.setCursor(8, 2);
      lcd.print(LoopValue);
      lcd.setCursor(2, 3);
      lcd.print("Clearance- ");

      lcd.print(c);
      lcd.print(" cm");

    }
    if (ButtonPressed) {
      TurnDetected = false;
      ButtonPressed = false;
      middleButton = false;
      //leftButton = false;
      //rightButton = false;
      delay(100);
      break;
    }
    if (middleButton) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      cncData.forceExit = 1;

      for (int i = 0; i < 50; i++) {
        myRadio.write(&cncData, sizeof(cncData));
        delay(1);
      }
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      cncData.Pause = 0;
      cncData.forceExit = 0;
      return false;
    }
  }
  cncData.loopVal = LoopValue;
  cncData.offset = EEPROM.read(0);

  if (cncData.offset > 0) {
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncData, sizeof(cncData));

    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    struct package5 {
      byte movToStartPos = 0;
    };
    typedef package5 Package5;
    Package5 cnc_MoveToStartPos;
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("CNC Mode");
    lcd.setCursor(4, 1);
    lcd.print("Please Wait-");
    lcd.setCursor(2, 2);
    lcd.print("Slider Moving to");
    lcd.setCursor(3, 3);
    lcd.print("Start Position");
    delay(100);
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
        if (cnc_MoveToStartPos.movToStartPos == 1)
          break;
      }
      else {
        if (middleButton) {
          middleButton = false;

          delay(1);
          myRadio.begin();
          myRadio.setChannel(15);
          myRadio.setPALevel(RF24_PA_MIN);
          myRadio.setDataRate( RF24_250KBPS ) ;
          myRadio.openWritingPipe( addresses[0]);
          myRadio.stopListening();
          cncData.forceExit = 1;
          delay(1);
          myRadio.write(&cncData, sizeof(cncData));
          cncData.mode = 0;
          cncData.sliderSpeed = 0;
          cncData.loopVal = 0;
          cncData.offset = 0;
          cncData.start = 0;
          cncData.Pause = 0;
          cncData.forceExit = 0;
          return false;
        }


      }
      delayMicroseconds(1);
    }
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    // slider moved to start position
  }

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("CNC Mode");
  lcd.setCursor(5, 1);
  lcd.print("Speed- ");
  int sp = cncData.sliderSpeed;
  lcd.print(sp);
  lcd.setCursor(2, 3);
  lcd.write(1);
  lcd.setCursor(7, 2);
  lcd.print("Start?");
  lcd.setCursor(3, 3);
  lcd.print("Yes");
  lcd.setCursor(16, 3);
  lcd.print("No");
  int a = 2; // temporary cursor
  middleButton = false;
  TurnDetected = false;
  ButtonPressed = false;
  delay(100);
  while (true) {
    delay(10);
    if (middleButton) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      for (int i = 0; i < 10; i++)
        myRadio.write(&cncData, sizeof(cncData));
      delay(10);
      middleButton = false;
      return false;
    }
    if (TurnDetected) {
      TurnDetected = false;
      if (a == 2)
        a = 15;
      else
        a = 2;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("CNC Mode");
      lcd.setCursor(5, 1);
      lcd.print("Speed- ");
      sp = cncData.sliderSpeed;
      lcd.print(sp);
      lcd.setCursor(a, 3);
      lcd.write(1);
      lcd.setCursor(7, 2);
      lcd.print("Start?");
      lcd.setCursor(3, 3);
      lcd.print("Yes");
      lcd.setCursor(16, 3);
      lcd.print("No");
    }
    if (ButtonPressed) {
      speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
      delay(50);
      ButtonPressed = false;
      if (a == 15 || a != 2) {
        cncData.mode = 0;
        cncData.sliderSpeed = 0;
        cncData.loopVal = 0;
        cncData.offset = 0;
        cncData.start = 0;
        for (int i = 0; i < 10; i++)
          myRadio.write(&cncData, sizeof(cncData));
        return false;
      }
      else {
        cncData.start = 1;
        delay(500);
        ButtonPressed = false;
        myRadio.write(&cncData, sizeof(cncData));
      }
      break;
    }
  }
  struct package6
  {
    byte jobEnd = 0;
  };
  typedef struct package6 Package6;
  Package6 cncDataOnn;
  //cache = analogRead(pot1);
  ButtonPressed = false;
  middleButton = false;
  TurnDetected = false;
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("CNC Mode");
  lcd.setCursor(7, 1);
  lcd.print("Speed-");
  lcd.setCursor(8, 2);
  lcd.write(2);
  lcd.setCursor(9, 2);
  lcd.print(speedVal);
  lcd.setCursor(2, 3);
  lcd.print("Clearance- ");

  lcd.print(c);
  lcd.print(" cm");
  delay(100);
  while (true) {
    delay(10);
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncDataOnn, sizeof(cncDataOnn));
      if (cncDataOnn.jobEnd == 1)
        break;
    }
    if (ButtonPressed) {
      delay(1);
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
      myRadio.stopListening();

      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("CNC Mode");
      lcd.setCursor(7, 1);
      lcd.print("Speed-");
      lcd.setCursor(8, 2);
      lcd.write(2);
      lcd.setCursor(9, 2);
      lcd.print(speedVal);
      lcd.setCursor(3, 3);
      lcd.print("Slider Paused!");

      cncData.Pause = 1;

      for (int i = 0; i < 100; i++) {
        myRadio.write(&cncData, sizeof(cncData));
        delay(1);
      }
      ButtonPressed = false;
      while (true) {
        if (ButtonPressed) {
          cncData.Pause = 0;
          ButtonPressed = false;
          cncData.Pause = 0;
          for (int i = 0; i < 100; i++) {
            myRadio.write(&cncData, sizeof(cncData));
            delay(1);
          }
          lcd.clear();
          lcd.setCursor(6, 0);
          lcd.print("CNC Mode");
          lcd.setCursor(7, 1);
          lcd.print("Speed-");
          lcd.setCursor(8, 2);
          lcd.write(2);
          lcd.setCursor(9, 2);
          lcd.print(speedVal);
          lcd.setCursor(2, 3);
          lcd.print("Clearance- ");
          lcd.print(c);
          lcd.print(" cm");
          myRadio.begin();
          myRadio.setChannel(15);
          myRadio.setPALevel(RF24_PA_MIN);
          myRadio.setDataRate( RF24_250KBPS ) ;
          myRadio.openReadingPipe(1, addresses[0]);
          myRadio.startListening();
          break;
        }
      }
    }
    if (middleButton) {

      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
      myRadio.stopListening();
      middleButton = false;
      cncData.forceExit = 1;
      delay(1);
      for (int i = 0; i < 10; i++) {
        myRadio.write(&cncData, sizeof(cncData));
      }
      cncData.mode = 0;
      cncData.sliderSpeed = 0;
      cncData.loopVal = 0;
      cncData.offset = 0;
      cncData.start = 0;
      cncData.Pause = 0;
      cncData.forceExit = 0;
      break;
    }
    delay(1);
  }

  lcd.clear();
  cncData.mode = 0;
  cncData.sliderSpeed = 0;
  cncData.offset = 0;
  cncData.loopVal = 0;
  cncData.start = 0;
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();
}


boolean TimeLapse() {
  TimeLapseData.offset = EEPROM.read(0);
  TimeLapseData.shutterSpeed = 0;
  TimeLapseData.tMode = 0;
  TimeLapseData.forceExit = 0;
  TimeLapseData.Pause = 0;
  TimeLapseData.start = 0;
  TimeLapseData.intervalMin = 0;
  TimeLapseData.intervalSec = 0;

  TimeLapseData.totalTimeHrs = 0;
  TimeLapseData.totalTimeMin = 0;
  cncData.offset = EEPROM.read(0);
  cncData.mode = 2;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(4, 1);
  lcd.print("Please Wait-");
  lcd.setCursor(3, 2);
  lcd.print("Slider Homing!");
  ButtonPressed = false;
  middleButton = false;
  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  delay(1);
  struct package4 {
    byte homing = 0;
  };
  typedef package4 Package4;
  Package4 cncDataExp;
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    if (myRadio.available()) {
      while (myRadio.available())
        myRadio.read(&cncDataExp, sizeof(cncDataExp));
    }
    if (cncDataExp.homing == 1)
      break;
    if (middleButton) {
      delay(50);
      middleButton = false;
      cncData.forceExit = 1;
    }
  }

  delay(1);
  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);
  myRadio.stopListening();
  delay(1);
  if (cncData.offset > 0) {
    for (int i = 0; i < 10; i++)
      myRadio.write(&cncData, sizeof(cncData));

    delay(1);
    if (cncData.forceExit == 1) {
      cncData.forceExit = 0;
      cncData.mode = 0;
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 0;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.intervalMin = 0;
      middleButton = false;
      ButtonPressed = false;
      TurnDetected = false;
      return false;

    }
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();
    struct package5 {
      byte movToStartPos = 0;
    };
    typedef package5 Package5;
    Package5 cnc_MoveToStartPos;
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time Lapse Mode-");
    lcd.setCursor(4, 1);
    lcd.print("Please Wait-");
    lcd.setCursor(2, 2);
    lcd.print("Slider Moving to");
    lcd.setCursor(3, 3);
    lcd.print("Start Position");
    delay(100);
    ButtonPressed = false;
    TurnDetected = false;
    middleButton = false;
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cnc_MoveToStartPos, sizeof(cnc_MoveToStartPos));
        if (cnc_MoveToStartPos.movToStartPos == 1)
          break;
      }
      else {
        if (digitalRead(B_middle) == LOW) {
          middleButton = false;

          delay(1);

          myRadio.begin();
          myRadio.setChannel(15);
          myRadio.setPALevel(RF24_PA_MIN);
          myRadio.setDataRate( RF24_250KBPS ) ;
          myRadio.openWritingPipe( addresses[0]);
          myRadio.stopListening();
          cncData.forceExit = 1;
          delay(1);
          myRadio.write(&cncData, sizeof(cncData));
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
          return false;
        }


      }
      delayMicroseconds(1);
    }
    delay(1);
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
    // slider moved to start position
    cncData.mode = 0;
  }

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");

  lcd.setCursor(0, 1);
  lcd.write(1);

  lcd.setCursor(1, 1);
  lcd.print("Time-Lapse no_Focus");
  lcd.setCursor(1, 2);
  lcd.print("Time-Lapse Focus");

  middleButton = false;
  TurnDetected = false;
  ButtonPressed = false;
  int cp = 1;
  TimeLapseData.tMode = 1;
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    delay(10);
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
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
      delay(50);
      middleButton = false;
      return false;
    }
    if (TurnDetected) {
      if (up) {
        cp++;
        if (cp > 2)
          cp = 1;
        TimeLapseData.tMode = 1;
      }
      else {
        cp--;
        if (cp < 1)
          cp = 2;
        TimeLapseData.tMode = 2;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");

      lcd.setCursor(0, cp);
      lcd.write(1);

      lcd.setCursor(1, 1);
      lcd.print("Time-Lapse no_Focus");
      lcd.setCursor(1, 2);
      lcd.print("Time-Lapse Focus");


      delay(1);
      TurnDetected = false;
    }
    if (ButtonPressed) {
      delay(10);
      ButtonPressed = false;

      TimeLapseData.offset = EEPROM.read(0);
      TimeLapseData.forceExit = 0;

      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));

      executeTimeLapse(cp);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");

      lcd.setCursor(0, cp);
      lcd.write(1);

      lcd.setCursor(1, 1);
      lcd.print("Time-Lapse no_Focus");
      lcd.setCursor(1, 2);
      lcd.print("Time-Lapse Focus");

      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;

      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      ButtonPressed = false;
      TurnDetected = false;
      middleButton = false;
      break;
    }
  }





}


boolean setIntervalShootingTime(boolean type) {
  middleButton = false;
  ButtonPressed = false;
  TurnDetected = false;
  TimeLapseData.intervalSec = 0;
  TimeLapseData.intervalMin = 0;
  TimeLapseData.totalTimeHrs = 0;
  TimeLapseData.totalTimeMin = 0;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(4, 1);
  lcd.print("Set Interval");
  lcd.setCursor(4, 2);
  lcd.write(2);
  lcd.print(" ");
  if (type == false)
    TimeLapseData.intervalSec = 4;
  lcd.print((int)(TimeLapseData.intervalSec));
  lcd.print(" seconds");
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  if (type == false)
    TimeLapseData.intervalSec = 4;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        TimeLapseData.intervalSec++;
        if (TimeLapseData.intervalSec > 59 && type == true)
          TimeLapseData.intervalSec = 0;
        if (type == false && TimeLapseData.intervalSec > 59)
          TimeLapseData.intervalSec = 4;
      }
      else {
        TimeLapseData.intervalSec--;
        if (TimeLapseData.intervalSec < 0 || TimeLapseData.intervalSec >  59 && type == true)
          TimeLapseData.intervalSec = 59;
        if (type == false && (TimeLapseData.intervalSec < 5 || TimeLapseData.intervalSec >  59 ) )
          TimeLapseData.intervalSec = 59;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(4, 1);
      lcd.print("Set Interval");
      lcd.setCursor(4, 2);
      lcd.write(2);
      lcd.print(" ");
      lcd.print((int)(TimeLapseData.intervalSec));
      lcd.print(" seconds");
      TurnDetected = false;
    }
    if (middleButton) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      delay(50);
      middleButton = false;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      return false;

    }
    if (ButtonPressed) {
      delay(50);
      ButtonPressed = false;
      break;
    }
  }
  middleButton = false;
  ButtonPressed = false;
  TurnDetected = false;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(4, 1);
  lcd.print("Set Interval");
  lcd.setCursor(4, 2);
  lcd.write(2);
  lcd.print(" ");
  lcd.print((int)(TimeLapseData.intervalMin));
  lcd.print(" minutes");
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        TimeLapseData.intervalMin++;
        if (TimeLapseData.intervalMin > 10)
          TimeLapseData.intervalMin = 0;
      }
      else {
        TimeLapseData.intervalMin--;
        if (TimeLapseData.intervalMin < 0 || TimeLapseData.intervalMin > 10)
          TimeLapseData.intervalMin = 10;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(4, 1);
      lcd.print("Set Interval");
      lcd.setCursor(4, 2);
      lcd.write(2);
      lcd.print(" ");
      lcd.print((int)(TimeLapseData.intervalMin));
      lcd.print(" minutes");
      TurnDetected = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      delay(50);
      middleButton = false;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      return false;

    }
    if (ButtonPressed) {
      delay(50);
      ButtonPressed = false;
      break;
    }
  }
  TurnDetected = false;
  ButtonPressed = false;
  middleButton = false;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(3, 1);
  lcd.print("Shooting Time-");
  lcd.setCursor(4, 2);
  lcd.write(2);
  lcd.print(" ");
  lcd.print((int)(TimeLapseData.totalTimeMin));
  lcd.print(" minutes");
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        TimeLapseData.totalTimeMin++;
        if (TimeLapseData.totalTimeMin > 59)
          TimeLapseData.totalTimeMin = 0;
      }
      else {
        TimeLapseData.totalTimeMin--;
        if (TimeLapseData.totalTimeMin < 0 || TimeLapseData.totalTimeMin > 59)
          TimeLapseData.totalTimeMin = 59;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(3, 1);
      lcd.print("Shooting Time-");
      lcd.setCursor(4, 2);
      lcd.write(2);
      lcd.print(" ");
      lcd.print((int)(TimeLapseData.totalTimeMin));
      lcd.print(" minutes");
      TurnDetected = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      delay(50);
      middleButton = false;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      return false;

    }
    if (ButtonPressed) {
      delay(50);
      ButtonPressed = false;
      break;
    }
  }
  middleButton = false;
  ButtonPressed = false;
  TurnDetected = false;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(3, 1);
  lcd.print("Shooting Time-");
  lcd.setCursor(5, 2);
  lcd.write(2);
  lcd.print(" ");
  lcd.print((int)(TimeLapseData.totalTimeHrs));
  lcd.print(" hours");
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        TimeLapseData.totalTimeHrs++;
        if (TimeLapseData.totalTimeHrs > 7)
          TimeLapseData.totalTimeHrs = 0;
      }
      else {
        TimeLapseData.totalTimeHrs--;
        if (TimeLapseData.totalTimeHrs < 0 || TimeLapseData.totalTimeHrs > 7)
          TimeLapseData.totalTimeHrs = 7;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(3, 1);
      lcd.print("Shooting Time-");
      lcd.setCursor(5, 2);
      lcd.write(2);
      lcd.print(" ");
      lcd.print((int)(TimeLapseData.totalTimeHrs));
      lcd.print(" hours");
      TurnDetected = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      delay(50);
      middleButton = false;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      return false;

    }
    if (ButtonPressed) {
      delay(50);
      ButtonPressed = false;
      break;
    }
  }
  middleButton = false;
  ButtonPressed = false;
  TurnDetected = false;
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Time Lapse Mode-");
  lcd.setCursor(0, 1);
  lcd.print("Select Shutter Speed");
  if (TimeLapseData.shutterSpeed == 0) {
    lcd.setCursor(0, 2);
    lcd.print("Less than 1 second ?");
  }
  else {
    lcd.setCursor(4, 2);
    lcd.write(2);
    lcd.print(" ");
    lcd.print((int)(TimeLapseData.shutterSpeed));
    lcd.print(" seconds");
  }
  delay(100);
  ButtonPressed = false;
  TurnDetected = false;
  middleButton = false;
  while (true) {
    delay(10);
    if (TurnDetected) {
      if (up) {
        TimeLapseData.shutterSpeed++;
        if (TimeLapseData.shutterSpeed > 30)
          TimeLapseData.shutterSpeed = 0;
      }
      else {
        TimeLapseData.shutterSpeed--;
        if (TimeLapseData.shutterSpeed < 0 || TimeLapseData.shutterSpeed > 30)
          TimeLapseData.shutterSpeed = 30;
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(0, 1);
      lcd.print("Select Shutter Speed");
      if (TimeLapseData.shutterSpeed == 0) {
        lcd.setCursor(0, 2);
        lcd.print("Less than 1 second ?");
      }
      else {
        lcd.setCursor(4, 2);
        lcd.write(2);
        lcd.print(" ");
        lcd.print((int)(TimeLapseData.shutterSpeed));
        lcd.print(" seconds");
      }
      TurnDetected = false;
    }
    if (digitalRead(B_middle) == LOW) {
      while (digitalRead(B_middle) == LOW) {
        delay(1);
      }
      middleButton = false;
      TimeLapseData.forceExit = 1;
      delay(50);
      middleButton = false;
      myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
      TimeLapseData.offset = 0;
      TimeLapseData.shutterSpeed = 0;
      TimeLapseData.tMode = 1;
      TimeLapseData.forceExit = 0;
      TimeLapseData.Pause = 0;
      TimeLapseData.start = 0;
      TimeLapseData.intervalMin = 0;
      TimeLapseData.intervalSec = 0;
      TimeLapseData.totalTimeHrs = 0;
      TimeLapseData.totalTimeMin = 0;
      return false;

    }
    if (ButtonPressed) {
      delay(50);
      ButtonPressed = false;
      break;
    }
  }

  return true;
}



boolean executeTimeLapse(int p) {
  if (p == 1) {
    // Time Lapse no focus
    chooseStartEndPos();
    if (!setIntervalShootingTime(true))
      return false;
    myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
    if ((TimeLapseData.intervalSec > 0 || TimeLapseData.intervalMin > 0) && ( TimeLapseData.totalTimeMin > 0 || TimeLapseData.totalTimeHrs > 0)) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(4, 2);
      lcd.print("Please Wait!");

      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openReadingPipe(1, addresses[0]);
      myRadio.startListening();
      struct package16 {
        byte startPosReached = 0;
      };
      typedef struct package16 Package16;
      Package16 stpReached;
      delay(100);
      ButtonPressed = false;
      TurnDetected = false;
      middleButton = false;
      while (true) {
        if (middleButton) {
          delay(50);
          middleButton = false;
          return false;
        }
        if (myRadio.available()) {
          while (myRadio.available())
            myRadio.read(&stpReached, sizeof(stpReached));
          if (stpReached.startPosReached == 1)
            break;
        }
      }
      delay(1);
      lcd.clear();
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
    }
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time Lapse Mode-");
    lcd.setCursor(2, 3);
    lcd.write(1);
    lcd.setCursor(7, 2);
    lcd.print("Start?");
    lcd.setCursor(3, 3);
    lcd.print("Yes");
    lcd.setCursor(16, 3);
    lcd.print("No");
    int a = 2; // temporary cursor
    middleButton = false;
    TurnDetected = false;
    ButtonPressed = false;
    delay(100);
    while (true) {
      delay(10);
      if (digitalRead(B_middle) == LOW) {
        while (digitalRead(B_middle) == LOW) {
          delay(1);
        }
        middleButton = false;
        cncData.mode = 0;
        cncData.start = 0;
        for (int i = 0; i < 10; i++)
          myRadio.write(&cncData, sizeof(cncData));
        delay(10);
        middleButton = false;
        return false;
      }
      if (TurnDetected) {
        TurnDetected = false;
        if (a == 2)
          a = 15;
        else
          a = 2;
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Time Lapse Mode-");
        lcd.setCursor(a, 3);
        lcd.write(1);
        lcd.setCursor(7, 2);
        lcd.print("Start?");
        lcd.setCursor(3, 3);
        lcd.print("Yes");
        lcd.setCursor(16, 3);
        lcd.print("No");
      }
      if (ButtonPressed) {
        delay(50);
        ButtonPressed = false;
        if (a == 15 || a != 2) {
          cncData.mode = 0;
          cncData.start = 0;
          for (int i = 0; i < 10; i++)
            myRadio.write(&cncData, sizeof(cncData));
          return false;
        }
        else {
          cncData.start = 1;
          cncData.mode = 1;
          delay(500);
          ButtonPressed = false;
          myRadio.write(&cncData, sizeof(cncData));
          cncData.mode = 0;
          cncData.start = 0;
        }
        break;
      }
    }
    struct package6
    {
      byte jobEnd = 0;
    };
    typedef struct package6 Package6;
    Package6 cncDataOnn;
    //cache = analogRead(pot1);
    ButtonPressed = false;
    middleButton = false;
    TurnDetected = false;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time Lapse Mode-");
    lcd.setCursor(1, 2);
    lcd.print("Time Lapse Started");

    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cncDataOnn, sizeof(cncDataOnn));
        if (cncDataOnn.jobEnd == 1)
          break;
      }
      if (ButtonPressed) {
        delay(1);
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openWritingPipe( addresses[0]);
        myRadio.stopListening();
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Time Lapse Mode-");
        lcd.setCursor(1, 2);
        lcd.print("Press Key to Start");
        lcd.setCursor(1, 3);
        lcd.print("Time Lapse Paused!");

        cncData.Pause = 1;

        for (int i = 0; i < 100; i++) {
          myRadio.write(&cncData, sizeof(cncData));
          delay(1);
        }
        ButtonPressed = false;
        while (true) {
          if (ButtonPressed) {
            cncData.Pause = 0;
            ButtonPressed = false;
            cncData.Pause = 0;
            for (int i = 0; i < 10; i++) {
              myRadio.write(&cncData, sizeof(cncData));
              delay(1);
            }
            lcd.clear();
            lcd.setCursor(2, 0);
            lcd.print("Time Lapse Mode-");
            lcd.setCursor(1, 2);
            lcd.print("Time Lapse Started");
            myRadio.begin();
            myRadio.setChannel(15);
            myRadio.setPALevel(RF24_PA_MIN);
            myRadio.setDataRate( RF24_250KBPS ) ;
            myRadio.openReadingPipe(1, addresses[0]);
            myRadio.startListening();
            break;
          }
        }
      }
      if (middleButton) {
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openWritingPipe( addresses[0]);
        myRadio.stopListening();
        middleButton = false;
        cncData.forceExit = 1;
        delay(1);
        for (int i = 0; i < 10; i++) {
          myRadio.write(&cncData, sizeof(cncData));
        }
        cncData.mode = 0;
        cncData.sliderSpeed = 0;
        cncData.loopVal = 0;
        cncData.offset = 0;
        cncData.start = 0;
        cncData.Pause = 0;
        cncData.forceExit = 0;
        break;
      }
      delay(1);
    }

    lcd.clear();
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.offset = 0;
    cncData.loopVal = 0;
    cncData.start = 0;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();

  }
  else if (p == 2) {
    // Time Lapse with focus
    chooseStartEndPos();
    if (!setIntervalShootingTime(false))
      return false;
    myRadio.write(&TimeLapseData, sizeof(TimeLapseData));
    if ((TimeLapseData.intervalSec > 0 || TimeLapseData.intervalMin > 0) && ( TimeLapseData.totalTimeMin > 0 || TimeLapseData.totalTimeHrs > 0)) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Time Lapse Mode-");
      lcd.setCursor(4, 2);
      lcd.print("Please Wait!");

      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openReadingPipe(1, addresses[0]);
      myRadio.startListening();
      struct package16 {
        byte startPosReached = 0;
      };
      typedef struct package16 Package16;
      Package16 stpReached;
      delay(100);
      ButtonPressed = false;
      TurnDetected = false;
      middleButton = false;
      while (true) {
        if (middleButton) {
          delay(50);
          middleButton = false;
          return false;
        }
        if (myRadio.available()) {
          while (myRadio.available())
            myRadio.read(&stpReached, sizeof(stpReached));
          if (stpReached.startPosReached == 1)
            break;
        }
      }
      delay(1);
      lcd.clear();
      myRadio.begin();
      myRadio.setChannel(15);
      myRadio.setPALevel(RF24_PA_MIN);
      myRadio.setDataRate( RF24_250KBPS ) ;
      myRadio.openWritingPipe( addresses[0]);
    }
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time Lapse Mode-");
    lcd.setCursor(2, 3);
    lcd.write(1);
    lcd.setCursor(7, 2);
    lcd.print("Start?");
    lcd.setCursor(3, 3);
    lcd.print("Yes");
    lcd.setCursor(16, 3);
    lcd.print("No");
    int a = 2; // temporary cursor
    middleButton = false;
    TurnDetected = false;
    ButtonPressed = false;
    delay(100);
    while (true) {
      delay(10);
      if (digitalRead(B_middle) == LOW) {
        while (digitalRead(B_middle) == LOW) {
          delay(1);
        }
        middleButton = false;
        cncData.mode = 0;
        cncData.start = 0;
        for (int i = 0; i < 10; i++)
          myRadio.write(&cncData, sizeof(cncData));
        delay(10);
        middleButton = false;
        return false;
      }
      if (TurnDetected) {
        TurnDetected = false;
        if (a == 2)
          a = 15;
        else
          a = 2;
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Time Lapse Mode-");
        lcd.setCursor(a, 3);
        lcd.write(1);
        lcd.setCursor(7, 2);
        lcd.print("Start?");
        lcd.setCursor(3, 3);
        lcd.print("Yes");
        lcd.setCursor(16, 3);
        lcd.print("No");
      }
      if (ButtonPressed) {
        delay(50);
        ButtonPressed = false;
        if (a == 15 || a != 2) {
          cncData.mode = 0;
          cncData.start = 0;
          for (int i = 0; i < 10; i++)
            myRadio.write(&cncData, sizeof(cncData));
          return false;
        }
        else {
          cncData.start = 1;
          cncData.mode = 1;
          delay(500);
          ButtonPressed = false;
          myRadio.write(&cncData, sizeof(cncData));
          cncData.mode = 0;
          cncData.start = 0;
        }
        break;
      }
    }
    struct package6
    {
      byte jobEnd = 0;
    };
    typedef struct package6 Package6;
    Package6 cncDataOnn;
    //cache = analogRead(pot1);
    ButtonPressed = false;
    middleButton = false;
    TurnDetected = false;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openReadingPipe(1, addresses[0]);
    myRadio.startListening();

    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Time Lapse Mode-");
    lcd.setCursor(1, 2);
    lcd.print("Time Lapse Started");
    //drawSlider(3, false);
    while (true) {
      if (myRadio.available()) {
        while (myRadio.available())
          myRadio.read(&cncDataOnn, sizeof(cncDataOnn));
        if (cncDataOnn.jobEnd == 1)
          break;
      }
      if (ButtonPressed) {
        delay(1);
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openWritingPipe( addresses[0]);
        myRadio.stopListening();
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Time Lapse Mode-");
        lcd.setCursor(1, 2);
        lcd.print("Press Key to Start");
        lcd.setCursor(1, 3);
        lcd.print("Time Lapse Paused!");

        cncData.Pause = 1;

        for (int i = 0; i < 100; i++) {
          myRadio.write(&cncData, sizeof(cncData));
          delay(1);
        }
        ButtonPressed = false;
        while (true) {
          if (ButtonPressed) {
            cncData.Pause = 0;
            ButtonPressed = false;
            cncData.Pause = 0;
            for (int i = 0; i < 10; i++) {
              myRadio.write(&cncData, sizeof(cncData));
              delay(1);
            }
            lcd.clear();
            lcd.setCursor(2, 0);
            lcd.print("Time Lapse Mode-");
            lcd.setCursor(1, 2);
            lcd.print("Time Lapse Started");
            //drawSlider(3, false);
            myRadio.begin();
            myRadio.setChannel(15);
            myRadio.setPALevel(RF24_PA_MIN);
            myRadio.setDataRate( RF24_250KBPS ) ;
            myRadio.openReadingPipe(1, addresses[0]);
            myRadio.startListening();
            break;
          }
        }
      }
      if (middleButton) {
        myRadio.begin();
        myRadio.setChannel(15);
        myRadio.setPALevel(RF24_PA_MIN);
        myRadio.setDataRate( RF24_250KBPS ) ;
        myRadio.openWritingPipe( addresses[0]);
        myRadio.stopListening();
        middleButton = false;
        cncData.forceExit = 1;
        delay(1);
        for (int i = 0; i < 10; i++) {
          myRadio.write(&cncData, sizeof(cncData));
        }
        cncData.mode = 0;
        cncData.sliderSpeed = 0;
        cncData.loopVal = 0;
        cncData.offset = 0;
        cncData.start = 0;
        cncData.Pause = 0;
        cncData.forceExit = 0;
        break;
      }
      delay(1);
    }

    lcd.clear();
    cncData.mode = 0;
    cncData.sliderSpeed = 0;
    cncData.offset = 0;
    cncData.loopVal = 0;
    cncData.start = 0;
    myRadio.begin();
    myRadio.setChannel(15);
    myRadio.setPALevel(RF24_PA_MIN);
    myRadio.setDataRate( RF24_250KBPS ) ;
    myRadio.openWritingPipe( addresses[0]);
    myRadio.stopListening();
  }
  else
    return false;
}


void drawSlider(int p, boolean lcdClear) {

  if (lcdClear)
    lcd.clear();

  lcd.setCursor(0, p);
  lcd.write(3);
  for (int i = 1; i <= 8; i++)
    lcd.write(4);
  lcd.write(5);
  lcd.write(6);
  for (int i = 11; i <= 18; i++)
    lcd.write(4);
  lcd.write(3);
}


void setup() {
  //erial.begin(9600);
  //EEPROM.write(0,15);
  offsetVal = EEPROM.read(0);
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, arrow);
  lcd.createChar(2, sparrow);
  lcd.createChar(3, slider_ends);
  lcd.createChar(4, slider_rail);
  lcd.createChar(5, slider_chassey1);
  lcd.createChar(6, slider_chassey2);

  ////////////

  for (int i = 1; i <= 9; i += 2) {
    lcd.setCursor(0, 1);
    lcd.write(3);
    lcd.setCursor(i, 1);
    lcd.write(5);
    lcd.setCursor(i + 1, 1);
    lcd.write(6);
    for (int j = 1; j <= 18; j++) {
      if (j == i || j == (i + 1))
        continue;
      lcd.setCursor(j, 1);
      lcd.write(4);

    }
    lcd.setCursor(19, 1);
    lcd.write(3);
    delay(200);
    lcd.clear();
  }
  //delay(2000);
  lcd.clear();
  //////////////////

  lcd.setCursor(2, 0);
  drawSlider(0, true);
  delay(1);

  pinMode(B_middle, INPUT_PULLUP);
  pinMode(R_clk, INPUT);
  pinMode(R_dt, INPUT);
  pinMode(R_button, INPUT_PULLUP);
  pinMode(pot1, INPUT);

  myRadio.begin();
  myRadio.setChannel(15);
  myRadio.setPALevel(RF24_PA_MIN);
  myRadio.setDataRate( RF24_250KBPS ) ;
  myRadio.openWritingPipe( addresses[0]);

  delay(1000);

  attachInterrupt(0, rotary_encoder, CHANGE);
  attachInterrupt(1, rotary_button, FALLING);
  attachPinChangeInterrupt(B_middle, middle_button, FALLING);

  homeScreen(1);
  initialise();
}


void loop() {
  if (TurnDetected) {
    TurnDetected = false;
    if (up) {
      cursorPosition++;
      if (cursorPosition > 5)
        cursorPosition = 1;
    }
    else {

      cursorPosition--;
      if (cursorPosition < 1)
        cursorPosition = 5;
    }
    homeScreen(cursorPosition);
  }
  if (ButtonPressed) {
    ButtonPressed = false;
    delay(1);
    enterMode(cursorPosition);
  }
  speedVal = map(analogRead(pot1), 0, 1023, 1, 200);
  myRadio.write( &modeData, sizeof(modeData));
  delay(10);

}

#include "IRremote.h"
#include "IR.h"
#include <EEPROM.h>

#define VIBRATION_DIGITAL_IN_PIN 5
#define SOLENOID_DIGITAL_OUT 2
#define THRESHOLD_PWM_OUT_PIN 6
#define IR_RECEIVER_IN_PIN 11
#define ARMED_STATUS_OUT_PIN 10

IRrecv irrecv(IR_RECEIVER_IN_PIN);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

// core values for thump function
const int DEBOUNCE = 250; // milliseconds to not trigger again
int holdOpen = 100; // ms to hold open solenoid
unsigned long lastThump;
bool updatingHoldOpen = false; // logical flag for interval to change hold open on remote
unsigned long updatingHoldOpenSince; // timestamp for start of hold open update interval
int holdOpenBuffer [3] = {0, 0, 0}; // array to hold new hold open value
unsigned int holdOpenBufferLength = 0; // current length of holdOpenBuffer
const int EE_HOLD_OPEN_ADDRESS = 1;  // storage address for holdOpen

// remote constants
const float REMOTE_NUM_MAX = 9.0; // max numeric on remote
const int REMOTE_INTERVAL = 3000; // ms to record remote activity

// values to maintain thump threshold
int pwmValue; // PWM value fed into low-pass RC filter for 0-5V output for op-amp comparator
const int EE_THRESHOLD_ADDRESS = 0;  // storage address for pwmValue
const int THRESHOLD_DELTA = 1; // interval to change theshold each inc/dec
bool updatingThreshold = false; // logical flag for interval to change threshold on remote
unsigned long updatingThresholdSince; // timestamp for start of threshold update interval

// arming values
bool armedStatus = false; // logical flag for armed status
bool armedSignal = false; // maintain output signal to armed status LED
const int ARMED_SIGNAL_LEVEL = 100; // PWM value to light armed status LED
unsigned long previousMillis = 0; 


void setup() {
  pinMode(VIBRATION_DIGITAL_IN_PIN, INPUT);
  pinMode(SOLENOID_DIGITAL_OUT, OUTPUT);
  pinMode(THRESHOLD_PWM_OUT_PIN, OUTPUT);
  pinMode(ARMED_STATUS_OUT_PIN, OUTPUT);
  analogWrite(THRESHOLD_PWM_OUT_PIN, pwmValue);
  lastThump = millis();
  lookupStoredLevels();
  irrecv.enableIRIn();
}

void lookupStoredLevels() {
  pwmValue = EEPROM.read(EE_THRESHOLD_ADDRESS);
  analogWrite(THRESHOLD_PWM_OUT_PIN, pwmValue);
  holdOpen = EEPROM.read(EE_HOLD_OPEN_ADDRESS);
}

void triggerThump() {
  digitalWrite(SOLENOID_DIGITAL_OUT, HIGH);
  lastThump = millis();
}

void resetThump() {
  digitalWrite(SOLENOID_DIGITAL_OUT, LOW);
}

void incrementThreshold() {
  pwmValue = min(255, pwmValue + THRESHOLD_DELTA);
  analogWrite(THRESHOLD_PWM_OUT_PIN, pwmValue);
  EEPROM.write(EE_THRESHOLD_ADDRESS, pwmValue);
}

void decrementThreshold() {
  pwmValue = max(0, pwmValue - THRESHOLD_DELTA);
  analogWrite(THRESHOLD_PWM_OUT_PIN, pwmValue);
  EEPROM.write(EE_THRESHOLD_ADDRESS, pwmValue);
}

void jumpThreshold(int value) {
  pwmValue = round((value / REMOTE_NUM_MAX) * 255);
  analogWrite(THRESHOLD_PWM_OUT_PIN, pwmValue);
  EEPROM.write(EE_THRESHOLD_ADDRESS, pwmValue);
}

void flipArmedStatus() {
  armedStatus = !armedStatus;
}

void closeThresholdUpdateInterval() {
  updatingThreshold = false;
  updatingThresholdSince = 0;
}

void closeHoldOpenUpdateInterval() {
    updatingHoldOpen = false;
    updatingHoldOpenSince = 0;
    // convert array of digits into applicable integer; [1,2,3] == 123
    if (holdOpenBufferLength > 0) {
      int num = 0;
      int place = 1;
      for (int i = holdOpenBufferLength-1; i >= 0; i--) {
        num += holdOpenBuffer[i] * place;
        place *= 10;
      }
      holdOpen = num;
      EEPROM.write(EE_HOLD_OPEN_ADDRESS, holdOpen);
    }
}

void blinkStatusLight() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= 100) {
    previousMillis = currentMillis;
    armedSignal = !armedSignal;
    analogWrite(ARMED_STATUS_OUT_PIN, ARMED_SIGNAL_LEVEL * armedSignal);
  }
}

void receiveIR() {
  int tmpValue;
  
  if (irrecv.decode(&results)) // have we received an IR signal?
  {
    for (int i = 0; i < 23; i++)
    {
      if ((i<KEY_NUM) && (keyValue[i] == results.value))
      {
        tmpValue = results.value;

        if (results.value == KEY_VOL_ADD) {
          incrementThreshold();
        } else if (results.value == KEY_VOL_DE) {
          decrementThreshold();
        } else if (results.value == KEY_PAUSE) {
          triggerThump();
        } else if (results.value == KEY_POWER) {
          flipArmedStatus();
        } else if (results.value == KEY_FUNC_STOP) {
          // begin time interval wherein the thump vibration trigger threshold can be set
          closeHoldOpenUpdateInterval();
          updatingThreshold = true;
          updatingThresholdSince = millis();
        } else if (updatingThreshold == true && 0 <= keyBuf[i] <= 9) {
          jumpThreshold(atoi(keyBuf[i]));
        } else if (results.value == KEY_ST_REPT) {
          // close time interbal wherein the thump vibration trigger threshold can be set
          closeThresholdUpdateInterval();
          updatingHoldOpen = true;
          updatingHoldOpenSince = millis();
          for (int i = 0; i < 3; i++) {
            holdOpenBuffer[i] = 0;
          }
          holdOpenBufferLength = 0;
        } else if (updatingHoldOpen == true && holdOpenBufferLength < 3 && 0 <= keyBuf[i] <= 9) {
          // append digit of key to the current thump vibration trigger threshold value
          holdOpenBuffer[holdOpenBufferLength++] = atoi(keyBuf[i]);
        }
      }
      else if(VAL_REPEAT==i)
      {
        results.value = tmpValue;
      }
    }
    irrecv.resume(); // receive the next value
  }
}

void loop() {
  if (armedStatus == true && digitalRead(VIBRATION_DIGITAL_IN_PIN) == HIGH && millis() > lastThump + DEBOUNCE) {
    triggerThump();
  }

  if (digitalRead(SOLENOID_DIGITAL_OUT) == HIGH && millis() > lastThump + holdOpen) {
    resetThump();
  }
  
  receiveIR();

  if (updatingThreshold == true || updatingHoldOpen == true) {
    blinkStatusLight();

    if (updatingThreshold == true && millis() > updatingThresholdSince + REMOTE_INTERVAL) {
      closeThresholdUpdateInterval();
    } else if (updatingHoldOpen == true && millis() > updatingHoldOpenSince + REMOTE_INTERVAL) {
      closeHoldOpenUpdateInterval();
    }
  } else {
    if (armedStatus == true && armedSignal == false) {
      analogWrite(ARMED_STATUS_OUT_PIN, ARMED_SIGNAL_LEVEL);
      armedSignal = true;
    } else if (armedStatus == false && armedSignal == true) {
      analogWrite(ARMED_STATUS_OUT_PIN, 0);
      armedSignal = false;
    }
  }
}

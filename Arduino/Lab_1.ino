#define TOUCH_ACTIVATED_VALUE 600

// Pin Defintions
const int LED_PIN = 14;

const int TOUCH_PIN_32 = 32;
const int TOUCH_PIN_27 = 27;
const int TOUCH_PIN_13 = 13;

const int BUZZER_PIN_2 = 2;

bool isLEDOn = false;

// amount of time for each sensor to collect untouched baseline
const int BASELINE_TIME = 2000;

// amount sensor must drop below its untouched baseline before calibration begins
const int START_TOUCH_OFFSET = 120;

// active calibration time for each sensor
const int CALIBRATION_TIME = 10000;

// how far below the sensed valuse does it count as a touch
// const int TOUCH_OFFSET = 100;
// Changed to percentage of the calibration sensing range
const int TOUCH_PERCENT = 25;

// Maximum time allowed between sensors for a swipe
// This is only a little long because I ripped my copper tape. 
const long SWIPE_TIME = 600;

// Sensor calibration range data
struct sensorRange {
  int min;
  int max;
};

sensorRange range32;
sensorRange range27;
sensorRange range13;

// Gesture detection States
enum GestureState {
  IDLE,
  RIGHT_STARTED,
  RIGHT_MIDDEL,
  LEFT_STARTED,
  LEFT_MIDDLE
};

GestureState gestureState = IDLE;

// Start time of current gesture
long gestureStartTime = 0;

// New touch detected
bool previousTouched32 = false;
bool previousTouched27 = false;
bool previousTouched13 = false;

// Void gesture detection fun
void detectGesture(bool touched32, bool touched27, bool touched13) {
  // detect a new touch
  bool newTouch32 = touched32 && !previousTouched32;
  bool newTouch27 = touched27 && !previousTouched27;
  bool newTouch13 = touched13 && !previousTouched13;

  // Check for gesture timeout
  if (gestureState != IDLE) {

    if ((millis() - gestureStartTime) > SWIPE_TIME) {

      Serial.println("Gesture timed out.");

      gestureState = IDLE;
    }
  }

  // Switch states
  switch (gestureState) {

    // Waiting for new sensor
    case IDLE:
      // Sensor 32 detected - maybe right swipe
      if (newTouch32) {
        Serial.println("Possible RIGHT swipe started...");
        gestureState = RIGHT_STARTED;
        gestureStartTime = millis();

      } else if (newTouch13) {  // Sensor 13 starts a possible left swipe
        Serial.println("Possible LEFT swipe started...");
        gestureState = LEFT_STARTED;
        gestureStartTime = millis();

      } else if (newTouch27) {
        Serial.println("Sensor 27 pressed.");
        playPressSound();
      }
      break;
    // Right side steps
    case RIGHT_STARTED:
      // Sensor 32 - 27 detected - possible right swipe
      if (newTouch27) {
        Serial.println();
        Serial.println("Possible RIGHT swipe...");
      
        gestureState = RIGHT_MIDDEL;
      
      }
      break;

    case RIGHT_MIDDEL:
      // Sensor 32 - 27 - 13 detected - right swipe
      if (newTouch13) {
        Serial.println();
        Serial.println(".......................");
        Serial.println("RIGHT SWIPE DETECTED...");
        Serial.println(".......................");
        playRightSound();

        gestureState = IDLE;
      
      }
      break;

    // LEFT side steps
    case LEFT_STARTED:
      // Sensor 13 - 27 detected - possible LEFT swipe
      if (newTouch27) {
        Serial.println();
        Serial.println("Possible LEFT swipe...");
        
        gestureState = LEFT_MIDDLE;
      
      }
      break;

    case LEFT_MIDDLE:
      // Sensor 13 - 27 - 32 detected - LEFT swipe
      if (newTouch32) {
        Serial.println();
        Serial.println(".......................");
        Serial.println("LEFT SWIPE DETECTED...");
        Serial.println(".......................");
        playLeftSound();

        gestureState = IDLE;

      }
      break;
  }

  // Need the states for the next loop
  previousTouched32 = touched32;
  previousTouched27 = touched27;
  previousTouched13 = touched13;
}


/*
  Measures the untouched value of the sensor

  User should not touch sensor for this one.
*/
int getBaseline(int touchPin, long sampleTime) {

  long startTime = millis();

  long total = 0;
  int sampleCount = 0;

  while ((millis() - startTime) < sampleTime) {
    int sensedValue = touchRead(touchPin);

    total += sensedValue;
    sampleCount++;

    delay(10);
  }

  return total / sampleCount;
}



/* 
  Used for part 2 on...
  Calibration func 
  inputs: touchPin, and caliTime
  output: range (min and max during caliTime)

  Intended for the user to touch and relase the sensor several times.
  max - approximate untouched value
  min - approximate touched value
*/
sensorRange calibration(int touchPin, long caliTime) {
  sensorRange range;

  int sensedValue = touchRead(touchPin);

  // Measure untouched baseline
  Serial.println("DO NOT TOUCH THE SENSOR...");
  Serial.println("Measuring untouched baseline.");

  delay(1000);

  int baseline = getBaseline(touchPin, BASELINE_TIME);

  Serial.print("Untouched baseline: ");
  Serial.println(baseline);


  // Wait till user touches sensor
  Serial.print("Touch sensor ");
  Serial.print(touchPin);
  Serial.println(" to begin calibration...");

  while (true) {

    sensedValue = touchRead(touchPin);

    if (sensedValue < (baseline - START_TOUCH_OFFSET)) {
      Serial.println("Touch detected!");
      break;
    }

    delay(10);
  }

  Serial.println();
  Serial.println("Calibration started!");
  Serial.println("Touch and release the sensor SEVERAL times.");

  range.min = 10000;
  range.max = 0;

  long startTime = millis();

  while ((millis() - startTime) < caliTime) {

    sensedValue = touchRead(touchPin);

    if (sensedValue < range.min) {
      range.min = sensedValue;
    }

    if (sensedValue > range.max) {
      range.max = sensedValue;
    }

    delay(10);
  }


  Serial.println();
  Serial.println("Calibration period complete!");

  Serial.print("Min Touched: ");
  Serial.println(range.min);

  Serial.println("Max Touched:");
  Serial.println(range.max);

  return range;
}

/* 
  Used for part 2 on... (Approximate threshold)
  TouchedThreshold
  inputs: sensorRange range (max-min)
  output: threshold based on each sensor
*/
int touchedThreshold(sensorRange range) {

  int sensingRange = range.max - range.min;

  int threshold = range.max - ((sensingRange * TOUCH_PERCENT) / 100);

  return threshold;
}


/* 
  Used for part 2 on... (Was the sensor touched?)
  sensorTouch func 
  inputs: sensedValue and range
  output: true or false
*/
bool sensorTouched(int sensedValue, sensorRange range) {

  int threshold = touchedThreshold(range);

  return sensedValue < threshold;
}

// Part 4: Sound Functions (Buzzer connected)

void playPressSound() {
  // tone(pin, frequency, duration(milisec)
  tone(BUZZER_PIN_2, 500, 500);
}

void playLeftSound() {
  tone(BUZZER_PIN_2, 700, 1000);
}

void playRightSound() {
  tone(BUZZER_PIN_2, 1000, 1000);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN_2, OUTPUT);

  Serial.println("Starting calibration.");

  Serial.println();
  Serial.println();
  Serial.println("Calibrating sensor 32...");
  range32 = calibration(TOUCH_PIN_32, CALIBRATION_TIME);

  Serial.println();
  Serial.println();
  Serial.println("Calibrating sensor 27...");
  range27 = calibration(TOUCH_PIN_27, CALIBRATION_TIME);

  Serial.println();
  Serial.println();
  Serial.println("Calibrating sensor 13...");
  range13 = calibration(TOUCH_PIN_13, CALIBRATION_TIME);

  Serial.println();
  Serial.println("Calibration complete.");

  delay(1000);

  Serial.println();
  Serial.println();
  Serial.println("Starting main program");
}

void loop() {
  // Sensor variables
  int sensedValue32 = touchRead(TOUCH_PIN_32);
  int sensedValue27 = touchRead(TOUCH_PIN_27);
  int sensedValue13 = touchRead(TOUCH_PIN_13);

  // LED Brightness pin 32 (Part 1)
  int level32 = map(sensedValue32, range32.max, range32.min, 0, 255);
  level32 = constrain(level32, 0, 255);  // Had some negative numbers
  analogWrite(LED_PIN, level32);


  // Touch detection
  bool touched32 = sensorTouched(sensedValue32, range32);
  bool touched27 = sensorTouched(sensedValue27, range27);
  bool touched13 = sensorTouched(sensedValue13, range13);


  // Debugging statements
  // Serial.println();
  // Serial.println();
  // Serial.print("32: Baseline: ");
  // Serial.println(sensedValue32);
  // Serial.print(" - Touched: ");
  // Serial.println(touched32);
  // Serial.print(" - LED: ");
  // Serial.println(level32);

  // Serial.println();
  // Serial.println();
  // Serial.print("27: ");
  // Serial.println(sensedValue27);
  // Serial.print(" - ");
  // Serial.println(touched27);

  // Serial.println();
  // Serial.println();
  // Serial.print("13: ");
  // Serial.println(sensedValue13);
  // Serial.print(" - ");
  // Serial.println(touched13);


  /* Gesture detection
    Pins in order (32 - 27 - 13)
    right swipe 32 - 27 - 13
    left swipe 13 - 27 -32

    // Interactive sounds can be found in the switch statement inside detectGesture
  */
  detectGesture(touched32, touched27, touched13);

  delay(200);
}

/*
// Can't define a structure/function in the loop or setup()... (Don't know why I tried this)
  // Stuff used for part 1 somewhat part 2 when connecting the rest of the lights
  // int sensedValue_32 = touchRead(TOUCH_PIN_32);
  // int level_32 = map(sensedValue_32, 1580, 435, 0, 255);
  // if (level_32 < 0) level_32 = 0;
  // Could use level32 = constain(level32, 0, 255);
  // // Serial.print("0,2000");
  // Serial.print("32: ");
  // Serial.println(sensedValue_32);
  // Serial.println(level_32);
  // Serial.println(isLEDOn);

  // int sensedValue_13 = touchRead(TOUCH_PIN_13);
  // int level_13 = map(sensedValue_13, 1245, 415, 0, 255);
  // if (level_13 < 0) level_13 = 0;
  // Serial.print("13: ");
  // Serial.println(sensedValue_13);
  // Serial.println(level_13);
  // Serial.println(isLEDOn);

  // int level_27 = map(sensedValue_27, 1420, 435, 0, 255);
  // if (level_27 < 0) level_27 = 0;
  // Serial.print("27: ");
  // Serial.println(sensedValue_27);
  // Serial.println(level_27);
  // Serial.println(isLEDOn);
  // delay(1000);

  int sensedValue_27 = touchRead(TOUCH_PIN_27);
  int max = 0;
  int min = 0;
  Serial.print("27: ");
  Serial.println(sensedValue_27);
  delay(1000);

  if(max < sensedValue_27){
    max = sensedValue_27
  }

  int level_27 = map(sensedValue_27, max, 435, 0, 255);
  if (level_27 < 0) level_27 = 0;


  // if (sensedValue <= TOUCH_ACTIVATED_VALUE) {
    //digitalWrite(LED_PIN, HIGH);
    analogWrite(LED_PIN, level_32); // - only one at a time 
    analogWrite(LED_PIN, level_13);
    analogWrite(LED_PIN, level_27);
  //   isLEDOn = true;
  // }
  // else {
    //digitalWrite(LED_PIN, LOW);
  //   analogWrite(LED_PIN, LOW);
  //   isLEDOn = false;
  // }
  // delay(1000);


  // Part 2 ideas
  // This is to make a calibration funtion for this controller.
  Write a program that detects the different touches. You’ll notice that each sensor has a different sensing range; 
  their initial capacitance in the untouched state is different (e.g., one sensor may show 1300, while another shows 700). 
  This value depends on a variety of factors including what is around the sensor, and the size and type of conductor used for the touch region. 

  Touch the sensor with different fingers and hands. Have your lab partner do the same. Do the values drop the same amount 
  when visualized in the Serial Plotter or Console?

  To make the sensors useful, you will have to implement a calibration procedure. Each sensor will need to capture its initial untouched
  value for some period of time (e.g., in the setup). Then this value will be compared against an offset that you determine (e.g., 100).
  This will ensure each sensor can appropriately detect a touch. You may also consider implementing adaptive thresholding, whereby a 
  running average is used to determine whether a sensor is touched.
  

  // Have to write a function that takes in a value (Touch_pin), output (maximum average over a consitent time and minimum average over consitent time)
  // Place into map()
  int calibration(int x){



  }


  // Part 3 (Right or left swipe)

  //part 4 (more things when buttons are hit)

*/
const int DIY_BUTTON_PIN = 2; // Internal pullup
const int BUTTON_PIN = A0;  // External pulldown
const int PHOTOCELL_PIN = A1;
const int CONTROL_X_PIN = A3;
const int CONTROL_Y_PIN = A4;
const int CONTROL_BUTTON_PIN = A5;  // Internal pullup
const int RGB_RED_PIN = 6;
const int RGB_GREEN_PIN = 5;
const int RGB_BLUE_PIN = 3;
const int TIME_ON_EACH_FADE_VAL_MS = 50;
const int TIME_ON_CONTROLLER_MS = 1;
const int DEBOUNCE_DELAY_MS = 50;
const int MIN_COLOR_VALUE = 0;
const int MAX_COLOR_VALUE = 255;
const float GAMMA = 2.2;

// RGB Vars
enum RGB {
  RED,
  GREEN,
  BLUE,
  NUM_COLORS
};
int _rgbLedValues[] = {255, 0, 0}; // in RGB order
enum RGB _curFadingUpColor = GREEN;
enum RGB _curFadingDownColor = RED;
const int STEP_VALUE = 5;

// Global time vars
unsigned long _stepValueUpdatedTimestampMs = 0; // time since last color step
unsigned long _modeButtonUpdatedTimestampMs = 0; // time since last mode button press
unsigned long _colorLockButtonUpdatedTimestampMs = 0; // time since last mode button press
unsigned long _controllerUpdatedTimestampMs = 0; // time since last joystick change
unsigned long _diyPressUpdatedTimestampMs = 0; // time since last diy press

// Mode changing vars
int currMode = 0;
int diyColor = 0;
bool colorLock = HIGH;  // TRUE = joystick can change LED color, FALSE = locked
bool lastModeButtonState = LOW;
bool lastControllerButtonState = HIGH;
bool lastDiyButtonState = HIGH;

// Joystick vars
int x_zone = 1;
int y_zone = 1;
int zone = 5;

void setup() {
  // Setup pins
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);  // external pull down
  pinMode(CONTROL_BUTTON_PIN, INPUT_PULLUP);  // internal pull up
  pinMode(DIY_BUTTON_PIN, INPUT_PULLUP);  // internal pull up

  // Turn on Serial
  Serial.begin(9600);

  // Set initial colors
  setColor(_rgbLedValues[RED], _rgbLedValues[GREEN], _rgbLedValues[BLUE]);

  // Seed for diy random color order on unconnected analog pin
  randomSeed(analogRead(A2));
}

void loop() {
  // Current time since start of program
  unsigned long currTimestampMs = millis();

  bool modeButtonState = digitalRead(BUTTON_PIN);

  // Mode Swap Button Handler
  if (currTimestampMs - _modeButtonUpdatedTimestampMs >= DEBOUNCE_DELAY_MS) {
    if (modeButtonState == HIGH && lastModeButtonState == LOW) {
      colorLock = HIGH;
      currMode++;
      if (currMode > 3) {
        // Reset crossfade state
        _rgbLedValues[0] = 255;
        _rgbLedValues[1] = 0;
        _rgbLedValues[2] = 0;
        _curFadingUpColor = GREEN;
        _curFadingDownColor = RED;
        currMode = 0;
      }
      _modeButtonUpdatedTimestampMs = currTimestampMs;
    }
  }
  lastModeButtonState = modeButtonState;

  // Mode Actions
  switch(currMode) {
    case 0: {  // Off mode
      setColor(0, 0, 0);
      break;
    }

    case 1: {  // Crossfade mode
      // Check to see if "delay" period has passed
      if (currTimestampMs - _stepValueUpdatedTimestampMs >= TIME_ON_EACH_FADE_VAL_MS) {
        _stepValueUpdatedTimestampMs = currTimestampMs;
        _rgbLedValues[_curFadingUpColor] += STEP_VALUE;
        _rgbLedValues[_curFadingDownColor] -= STEP_VALUE;

        // Check to see if max color value for up color reached
        // If so, cap value and rotate color (R -> G -> B)
        if (_rgbLedValues[_curFadingUpColor] > MAX_COLOR_VALUE) {
          _rgbLedValues[_curFadingUpColor] = MAX_COLOR_VALUE;
          _curFadingUpColor = (RGB)((int)_curFadingUpColor + 1);

          if (_curFadingUpColor > (int)BLUE) {
            _curFadingUpColor = RED;
          }
        }

        // Check to see if min color value for down color reached
        // If so, cap value and rotate color (R -> G -> B)
        if (_rgbLedValues[_curFadingDownColor] < MIN_COLOR_VALUE) {
          _rgbLedValues[_curFadingDownColor] = MIN_COLOR_VALUE;
          _curFadingDownColor = (RGB)((int)_curFadingDownColor + 1);
          
          if (_curFadingDownColor > (int)BLUE) {
            _curFadingDownColor = RED;
          }
        }
        setColor(_rgbLedValues[RED], _rgbLedValues[GREEN], _rgbLedValues[BLUE]);
      }
      break;
    }

    case 2: {  // Lofi Input Mode
      colorLockHandler(currTimestampMs);
      bool diyButtonState = digitalRead(DIY_BUTTON_PIN);

      if (currTimestampMs - _diyPressUpdatedTimestampMs >= DEBOUNCE_DELAY_MS) {
        if (diyButtonState == LOW && lastDiyButtonState == HIGH) {
          if (colorLock) {  // ensure same color until color lock turned off
            diyColor = random(0, 7);  // Random number from 0 to 6 (7 total)
          }
          
          _diyPressUpdatedTimestampMs = currTimestampMs;
        }
      }
      lastDiyButtonState = diyButtonState;

      if (colorLock) {
        switch(diyColor) {
          case 0:
            setColor(255, 0, 0);
            break;

          case 1:
            setColor(0, 255, 0);
            break;

          case 2:
            setColor(0, 0, 255);
            break;

          case 3:
            setColor(255, 255, 0);
            break;

          case 4:
            setColor(0, 255, 255);
            break;

          case 5:
            setColor(255, 0, 255);
            break;

          case 6:
            setColor(255, 255, 255);
            break;
        }
      }
      break;
    }

    case 3: {  // Joystick Mode, basic controls based off of https://www.youtube.com/watch?v=vo7SbVhW3pE&t=466s
      colorLockHandler(currTimestampMs);
      
      if (colorLock) {
        if (currTimestampMs - _controllerUpdatedTimestampMs >= TIME_ON_CONTROLLER_MS) {
          int x_value = analogRead(CONTROL_X_PIN);
          int y_value = analogRead(CONTROL_Y_PIN);

          _controllerUpdatedTimestampMs = currTimestampMs;
          
          // x zones
          if (x_value < 350) {
            x_zone = 0;
          } else if (x_value > 650) {
            x_zone = 2;
          } else {
            x_zone = 1;
          }

          // y zones
          if (y_value < 350) {
            y_zone = 0;
          } else if (y_value > 650) {
            y_zone = 2;
          } else {
            y_zone = 1;
          }

          // zones 1-9 in numpad notation
          zone = y_zone * 3 + x_zone + 1;

          switch(zone) {
            case 1:  // -blue value
               _rgbLedValues[BLUE]--;
              break;

            case 2:  // -all values
              _rgbLedValues[RED]--;
              _rgbLedValues[GREEN]--;
              _rgbLedValues[BLUE]--;
              break;

            case 3:  // +blue value
               _rgbLedValues[BLUE]++;
              break;

            case 4:  // -green value
              _rgbLedValues[GREEN]--;
              break;

            case 5:  // idle state
              break;

            case 6:  // +green value
              _rgbLedValues[GREEN]++;
              break;

            case 7:  // -red value
              _rgbLedValues[RED]--;
              break;

            case 8:  // +all values
              _rgbLedValues[RED]++;
              _rgbLedValues[GREEN]++;
               _rgbLedValues[BLUE]++;
              break;

            case 9:  // +red value
              _rgbLedValues[RED]++;
              break;

            default:
              break;
          }

        _rgbLedValues[RED] = constrain(_rgbLedValues[RED], 0, 255);
        _rgbLedValues[GREEN] = constrain(_rgbLedValues[GREEN], 0, 255);
        _rgbLedValues[BLUE] = constrain( _rgbLedValues[BLUE], 0, 255);

        setColor(_rgbLedValues[RED], _rgbLedValues[GREEN], _rgbLedValues[BLUE]);
        }
      }
      break;
    }

    default: {
      Serial.println("SOMETHING BROKE");
      break;
    }
  }
}

void setColor(int red, int green, int blue) {
  if (currMode == 1) {
    // Read light level and set brightness caps
    int photocellVal = analogRead(PHOTOCELL_PIN);
    float normalizedLightVal = photocellVal / 1000.0;  // 0.0-1.0
    float invertedLightVal = 1.0 - normalizedLightVal;  // dark=1.0, bright=0.0
    float brightnessLevel = pow(invertedLightVal, 3);

    float rf = (red / 255.0) * brightnessLevel;
    float gf = (green / 255.0) * brightnessLevel;
    float bf = (blue / 255.0) * brightnessLevel;

    analogWrite(RGB_RED_PIN, applyGamma(rf));
    analogWrite(RGB_GREEN_PIN, applyGamma(gf));
    analogWrite(RGB_BLUE_PIN, applyGamma(bf));
  } else {
    analogWrite(RGB_RED_PIN, red);
    analogWrite(RGB_GREEN_PIN, green);
    analogWrite(RGB_BLUE_PIN, blue);
  }
  
}

// Makes RGB changes more natural to the human eye
int applyGamma(float input) {
  return (int) (pow(input, GAMMA) * 255);
}

// Handler for button that "locks" a chosen color
void colorLockHandler(unsigned long currTimestampMs) {
  bool controllerButtonState = digitalRead(CONTROL_BUTTON_PIN);

  if (currTimestampMs - _colorLockButtonUpdatedTimestampMs >= DEBOUNCE_DELAY_MS) {
    if (controllerButtonState == LOW && lastControllerButtonState == HIGH) {
      colorLock = !colorLock;
      _colorLockButtonUpdatedTimestampMs = currTimestampMs;
    }
  }
  lastControllerButtonState = controllerButtonState;
}


#include <Arduino.h>
#include <SD.h>
#include "SdLedsPlayer.h"

//////////// LEDS//////////
#define FRAME_INTERVAL 25 // Interval for 40 FPS in milliseconds

#define ledsPerStrip 51
const int numPins = 16;
// byte pinList[numPins] = {2,14,7,8,6,20,21,5,0,1,23,22,19,18,24,25};
byte pinList[numPins] = {6, 5, 4, 3, 2, 1, 0, 22, 29, 28, 33, 36, 37, 14, 25, 24};

// These buffers need to be large enough for all the pixels.
// The total number of pixels is "ledsPerStrip * numPins".
// Each pixel needs 3 bytes, so multiply by 3.  An "int" is
// 4 bytes, so divide by 4.  The array is created using "int"
// so the compiler will align it to 32 bit memory.
const int bytesPerLED = 3; // change to 4 if using RGBW
DMAMEM int displayMemory[ledsPerStrip * numPins * bytesPerLED / 4];
int drawingMemory[ledsPerStrip * numPins * bytesPerLED / 4];

const int config = WS2811_GRB | WS2811_800kHz;

SdLedsPlayer sd_leds_player(ledsPerStrip, displayMemory, drawingMemory, numPins, pinList);
const char *animations[] = {"5", "6", "7", "8", "9", "10", "11"};
int currentAnimation = 0;
bool isPlaying = false;

//////////// Logic handeling

// Define the input pins on the Teensy
const int pin1 = 19; // MSB
const int pin2 = 18;
const int pin3 = 17;
const int pin4 = 16; // LSB

// State machine states
enum State
{
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_4,
    STATE_5,
    STATE_6,
    STATE_7,
    STATE_8,
    STATE_9,
    STATE_10,
    STATE_11
};

State currentState = STATE_0;
State previousState = STATE_0;
unsigned long startTime = 0;
unsigned long momentaryStartTime = 0;
unsigned long buttonPressStartTime = 0;
const unsigned long TIMEOUT = 40000;                 // 20 seconds for testing, change to 5 * 60 * 1000 for 5 minutes
const unsigned long MOMENTARY_STATE_DURATION = 1000; // 1 second
const unsigned long STATE_11_DURATION = 20000;       // 20 seconds
unsigned long lastStatePrintTime = 0;
const unsigned long PRINT_INTERVAL = 3000; // 3 seconds
bool end_of_file;
// Variables for button press logic
int buttonPressCount = 0;
const int requiredPresses = 3;                    // Number of presses needed to transition
const unsigned long buttonPressTimeFrame = 30000; // 30 seconds for the presses

////////////////

unsigned long animationStartTime = 0;
bool endOfTrack = false;

void setup()
{
    Serial.begin(9600); // Initialize Serial Monitor
    Serial.println("hello.");

    // Set the input pins
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
    pinMode(pin3, INPUT);
    pinMode(pin4, INPUT);

    // Initialize the timers
    startTime = millis();
    lastStatePrintTime = millis();

    Serial.println("hello2.");

    while (!sd_leds_player.setup())
    {
        Serial.println("SD card setup failed, fix and reset to continue");
        delay(1000);
    }

    // if (!sd_leds_player.setup()) {
    //     Serial.println("LED SD card initialization failed.");
    // } else {
    //     Serial.println("LED SD card initialized.");
    // }
}

//////////// logic functions
void next_animation(State state)
{
    // Stop current animation and load the next one
    sd_leds_player.stop_file();

    sd_leds_player.load_file(animations[static_cast<int>(state)]);
    // Load and display the first frame of the new animation
    sd_leds_player.load_next_frame();
    sd_leds_player.show_next_frame();
    animationStartTime = millis(); // Reset the frame timer
}

void resetButtonPressCount()
{
    buttonPressCount = 0;
    buttonPressStartTime = 0;
}

void handleButtonPress(int triggerID, State correctResponseState, State correctTransitionState, State incorrectResponseState, State requiredState, bool correct)
{
    if (buttonPressCount == 0)
    {
        buttonPressStartTime = millis();
    }
    if (millis() - buttonPressStartTime <= buttonPressTimeFrame)
    {
        buttonPressCount++;
        if (buttonPressCount >= requiredPresses && correct)
        {
            Serial.print("Transition to ");
            Serial.println(correctTransitionState);
            previousState = currentState;
            currentState = correctTransitionState;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
            return;
        }
    }
    else
    {
        Serial.print("button timer time out ");
        resetButtonPressCount();
    }

    // Handle correct response
    if (correct)
    {
        Serial.print("Transition to ");
        Serial.println(correctResponseState);
        previousState = currentState;
        currentState = correctResponseState;
        momentaryStartTime = millis(); // Start the momentary state timer
    }

    // Handle incorrect response
    else
    {
        Serial.print("Transition to ");
        Serial.println(incorrectResponseState);
        previousState = currentState;
        currentState = incorrectResponseState;
        momentaryStartTime = millis(); // Start the momentary state timer
        resetButtonPressCount();
    }
}

//////////////// end of logic functions

// void loop()

// {

// // Print the current state every 3 seconds
//     if (millis() - lastStatePrintTime > PRINT_INTERVAL) {
//         Serial.print("Current State: ");
//             Serial.print("Current Animation: ");
//         Serial.println(animations[currentAnimation]);
//         Serial.print("Is Playing: ");
//         Serial.println(isPlaying ? "Yes" : "No");
//         lastStatePrintTime = millis(); // Reset the print timer

//     }
// }

void loop()
{
    // if(getStateNumber(currentState)!= currentAnimation)
    // {
    //     currentAnimation=currentState;
    //     next_animation(currentAnimation);

    // }
    // end_of_file= update_frame();
    // Background LED file loading
    if (sd_leds_player.is_file_playing() && millis() - animationStartTime >= FRAME_INTERVAL)
    {
        end_of_file = false;
        sd_leds_player.show_next_frame();
        sd_leds_player.load_next_frame();
        animationStartTime = millis();
    }
    if (!sd_leds_player.is_file_playing())
    {
        end_of_file = true;
        Serial.print("No file is playing, loading new file, number: ");
        Serial.println(animations[currentAnimation]);
        sd_leds_player.load_file(animations[currentAnimation]); // minus 1 to translate state to filename because IDLE state is 0
        // curr_file_i = (curr_file_i + 1) % (sizeof(files_iter_rr) / sizeof(files_iter_rr[0]));
        // frame_timestamp = sd_leds_player.load_next_frame();
        sd_leds_player.load_next_frame();
        animationStartTime = millis();
    }

    // Read the state of the pins
    int bit4 = digitalRead(pin1); // MSB
    int bit3 = digitalRead(pin2);
    int bit2 = digitalRead(pin3);
    int bit1 = digitalRead(pin4); // LSB

    // Calculate the trigger ID from the bits
    int triggerID = (bit4 << 3) | (bit3 << 2) | (bit2 << 1) | bit1;

    // Reset the timer on any trigger
    if (triggerID > 0)
    {
        startTime = millis();
    }

    // State machine
    switch (currentState)
    {
    case STATE_0:
        if (triggerID == 1)
        {
            Serial.println("Trigger 1: Timer reset.");
        }
        else if (triggerID == 2 || triggerID == 3)
        {
            Serial.println("Transition to STATE_1");
            currentState = STATE_1;
        }
        else if (triggerID == 4 || triggerID == 5)
        {
            Serial.println("Transition to STATE_2");
            currentState = STATE_2;
            resetButtonPressCount();
        }
        else if (triggerID == 6)
        {
            Serial.println("Transition to STATE_5");
            previousState = currentState;
            currentState = STATE_5;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
        }
        else if (triggerID == 7)
        {
            Serial.println("Transition to STATE_9");
            previousState = currentState;
            currentState = STATE_9;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
        }
        else if (triggerID == 8)
        {
            Serial.println("Transition to STATE_10");
            previousState = currentState;
            currentState = STATE_10;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
        }
        break;

    case STATE_1:
        if (millis() - startTime > TIMEOUT)
        {
            Serial.println("Timer overflow. Transition to STATE_0");
            currentState = STATE_0;
        }
        else if (triggerID == 4 || triggerID == 5)
        {
            Serial.println("Transition to STATE_2");
            currentState = STATE_2;
            resetButtonPressCount();
        }
        else if (triggerID == 6)
        {
            Serial.println("Transition to STATE_2");
            previousState = currentState;
            currentState = STATE_2;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
        }
        else if (triggerID == 7)
        {
            Serial.println("Transition to STATE_9");
            previousState = currentState;
            currentState = STATE_9;
            momentaryStartTime = millis(); // Start the momentary state timer
            resetButtonPressCount();
        }
        else if (triggerID == 8)
        {
            Serial.println("Transition to STATE_10");
            previousState = currentState;
            currentState = STATE_10;
            momentaryStartTime = millis(); // Start the momentary state timer
        }
        break;

    case STATE_2:
        if (millis() - startTime > TIMEOUT)
        {
            Serial.println("Timer overflow. Transition to STATE_0");
            currentState = STATE_0;
        }
        else if (triggerID == 6)
        {
            handleButtonPress(triggerID, STATE_5, STATE_3, STATE_8, STATE_2, 1);
        }
        else if (triggerID == 7)
        {
            handleButtonPress(triggerID, STATE_6, STATE_3, STATE_9, STATE_2, 0);
        }
        else if (triggerID == 8)
        {
            handleButtonPress(triggerID, STATE_7, STATE_3, STATE_10, STATE_2, 0);
        }
        break;

    case STATE_3:
        if (millis() - startTime > TIMEOUT)
        {
            Serial.println("Timer overflow. Transition to STATE_0");
            currentState = STATE_0;
        }
        else if (triggerID == 7)
        {
            handleButtonPress(triggerID, STATE_6, STATE_4, STATE_9, STATE_3, 1);
        }
        else if (triggerID == 6)
        {
            handleButtonPress(triggerID, STATE_5, STATE_4, STATE_8, STATE_3, 0);
        }
        else if (triggerID == 8)
        {
            handleButtonPress(triggerID, STATE_7, STATE_4, STATE_10, STATE_3, 0);
        }
        break;

    case STATE_4:
        if (millis() - startTime > TIMEOUT)
        {
            Serial.println("Timer overflow. Transition to STATE_0");
            currentState = STATE_0;
        }
        else if (triggerID == 8)
        {
            handleButtonPress(triggerID, STATE_7, STATE_11, STATE_10, STATE_4, 1);
        }
        else if (triggerID == 6)
        {
            handleButtonPress(triggerID, STATE_5, STATE_11, STATE_8, STATE_4, 0);
        }
        else if (triggerID == 7)
        {
            handleButtonPress(triggerID, STATE_6, STATE_11, STATE_9, STATE_4, 0);
        }
        break;

    case STATE_5:
    case STATE_6:
    case STATE_7:
    case STATE_8:
    case STATE_9:
    case STATE_10:
        // if (millis() - momentaryStartTime > MOMENTARY_STATE_DURATION) {
        if (end_of_file)
        {

            Serial.println("momentery animation finished");
            currentState = previousState;
        }
        break;

    case STATE_11:
        if (millis() - momentaryStartTime > STATE_11_DURATION)
        {
            Serial.println("STATE_11 duration ended. Transition to STATE_0.");
            currentState = STATE_0;
        }
        break;

    default:
        // Should never reach here
        Serial.println("Unknown state!");
        currentState = STATE_0;
        break;
    }
    if (end_of_file)
    {
        next_animation(currentState);
    }
    // Print the current state every 3 seconds
    if (millis() - lastStatePrintTime > PRINT_INTERVAL)
    {
        Serial.print("Current State: ");
        switch (currentState)
        {
        case STATE_0:
            Serial.println("STATE_0");
            break;
        case STATE_1:
            Serial.println("STATE_1");
            break;
        case STATE_2:
            Serial.println("STATE_2");
            break;
        case STATE_3:
            Serial.println("STATE_3");
            break;
        case STATE_4:
            Serial.println("STATE_4");
            break;
        case STATE_5:
            Serial.println("STATE_5");
            break;
        case STATE_6:
            Serial.println("STATE_6");
            break;
        case STATE_7:
            Serial.println("STATE_7");
            break;
        case STATE_8:
            Serial.println("STATE_8");
            break;
        case STATE_9:
            Serial.println("STATE_9");
            break;
        case STATE_10:
            Serial.println("STATE_10");
            break;
        case STATE_11:
            Serial.println("STATE_11");
            break;
        default:
            Serial.println("UNKNOWN STATE");
            break;
        }
        // Serial.print("Current Animation: ");
        // Serial.println(animations[currentAnimation]);
        // Serial.print("Is Playing: ");
        // Serial.println(isPlaying ? "Yes" : "No");
        lastStatePrintTime = millis(); // Reset the print timer
    }

    delay(100);
}

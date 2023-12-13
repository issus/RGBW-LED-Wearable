#include <Arduino.h>

int rPin = PIN_PB0;
int gPin = PIN_PA5;
int bPin = PIN_PA4;
int wPin = PIN_PB1;

int outPin = PIN_PA3;
int inPin = PIN_PA2;

//#define MODE_FADER 1 // fade all colours randomly
//#define MODE_FADESINGLE rPin // fade a single colour
#define MODE_CHAINSTARTER 1 // first microcontroller in a chain of chasing leds, which will generate the starting pulse
#define MODE_CHASING 1 // chasing leds, does one flash after receving a high input pulse

//                0bRGBW mask
#define MASK_FADE 0b1111 // which colours to use for fading

int min = 127;
int max = 255;

int minMod = 1;
int maxMod = 20;

int pins[] = { rPin, gPin, bPin, wPin };

// Create an array to store available pins based on the mask
int availablePins[4];
int availableCount = 0;

uint32_t generateRandomSeed();
void fadeAll();
void fadeSingleCh(int);
void fadeTwoCh(int, int);
void fadeRandom();

void toggleOut();

void allLedsOff();

uint8_t inputActive();

void setup() {
  // enable randomisation
  randomSeed(generateRandomSeed());

  pinMode(inPin, INPUT_PULLUP);
  pinMode(outPin, OUTPUT);

  digitalWrite(outPin, 1);

  // set all LEDs off
  allLedsOff();

  // Check each pin against the mask
  for (int i = 0; i < 4; i++) {
    if (MASK_FADE & (1 << 3 - i)) {
      availablePins[availableCount++] = pins[i];
    }
  }

  // cycle through each allowable LED and flash it to check it is functioning
  for (int i = 0; i < availableCount; i++)
  {
    analogWrite(availablePins[i], min); // led on
    delay(200);
    analogWrite(availablePins[i], max); // led off
  }

  #ifdef MODE_CHAINSTARTER
    delay(600); // give all other microcontrollers in the chain to finish bootup sequence
    toggleOut();
  #endif
}

void loop() {
  while (true) {
    #ifdef MODE_FADER
      fadeRandom();
    #endif

    #ifdef MODE_FADESINGLE
      fadeSingleCh(MODE_FADESINGLE);
    #endif

    #ifdef MODE_CHASING
      if (inputActive())
      {
        fadeRandom();

        toggleOut();
      }
    #endif

  }
}

/// @brief Debounced check if input has a signal
/// @return true if input remains low for 5ms
uint8_t inputActive()
{
  if (!digitalRead(inPin)) {
    for (int i = 0; i < 5; i++) {
      if (digitalRead(inPin)) return 0;

      delay(1);
    }

    return 1;
  }

  return 0;
}

/// @brief Toggle the output pin
void toggleOut() {
  digitalWrite(outPin, 0);
  delay(20);
  digitalWrite(outPin, 1);
}

/// @brief fades a random channel or channels of LEDs
void fadeRandom() {
  int rnd = random(0, 10);

  if (rnd >= 5 && rnd <= 7) {
    fadeAll();
  }
  else if (rnd > 8 && availableCount > 1) {
    // Select two random pins from the available pins
    int pin1 = availablePins[random(0, availableCount)];
    int pin2;

    do {
      pin2 = availablePins[random(0, availableCount)];
    } while (pin1 == pin2); // Ensure two different pins are selected

    fadeTwoCh(pin1, pin2);
  }
  else {
    fadeSingleCh(availablePins[random(0, availableCount)]);
  }
}

/// @brief Fades up and down using available LEDs based on the mask
void fadeAll() {
  // Create arrays for the values and modifiers of the available pins
  int values[availableCount];
  int modifiers[availableCount];

  // Initialize random values and modifiers for each available pin
  for (int i = 0; i < availableCount; i++) {
    values[i] = random(min, max);
    modifiers[i] = random(minMod, maxMod);
  }

  // fade for about 1.8 seconds
  for (int i = 0; i < 60; i++) {
    for (int j = 0; j < availableCount; j++) {
      // Check and modify the fade direction if limits are reached
      if (values[j] >= max - maxMod) modifiers[j] = -random(minMod, maxMod);
      if (values[j] <= min + maxMod) modifiers[j] = random(minMod, maxMod);
      
      // Apply the modifier
      values[j] += modifiers[j];

      // Write the new value to the LED
      analogWrite(availablePins[j], values[j]);
    }
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }

  allLedsOff();
}

/// @brief Fades a single LED up then down
/// @param pin LED channel to fade
void fadeSingleCh(int pin) {
  int chMod = random(minMod,maxMod);

  int ch = min;
  
  for (int i = 0; i < 80 && ch <= max - maxMod; i++)
  {
    if (ch >= max - maxMod) chMod = -random(minMod, maxMod);
    if (ch <= min + maxMod) chMod =  random(minMod, maxMod);
    
    ch += chMod;

    analogWrite(pin, ch);
    delay(30);
  }

  allLedsOff();
}

/// @brief Fades using two channels to make a mixed colour
/// @param pin1 First LED pin to use
/// @param pin2 Second LED pin to use
void fadeTwoCh(int pin1, int pin2) {
  if (pin1 == pin2) {
    fadeSingleCh(pin1);
    return;
  }

  int ch1 = min;
  int ch2 = random(min, max);
  
  int ch1Mod = random(minMod,maxMod);
  int ch2Mod = random(minMod,maxMod);
  
  for (int i = 0; i < 80 && ch1 <= max - maxMod; i++)
  {
    if (ch1 >= max - maxMod) ch1Mod = -random(minMod, maxMod);
    if (ch1 <= min + maxMod) ch1Mod =  random(minMod, maxMod);
    if (ch2 >= max - maxMod) ch2Mod = -random(minMod, maxMod);
    if (ch2 <= min + maxMod) ch2Mod =  random(minMod, maxMod);

    ch1 += ch1Mod;
    ch2 += ch2Mod;

    analogWrite(pin1, ch1);
    analogWrite(pin2, ch2);
    
    delay(30);
  }

  allLedsOff();
}

/// @brief Turn off all leds (set PWM to max)
void allLedsOff() {
  for (int i = 0; i < availableCount; i++)
  {
    analogWrite(availablePins[i], max); // led off
  }
}

// from: https://rheingoldheavy.com/better-arduino-random-values/
uint32_t generateRandomSeed()
{
  uint8_t  seedBitValue  = 0;
  uint8_t  seedByteValue = 0;
  uint32_t seedWordValue = 0;
 
  for (uint8_t wordShift = 0; wordShift < 4; wordShift++)     // 4 bytes in a 32 bit word
  {
    for (uint8_t byteShift = 0; byteShift < 8; byteShift++)   // 8 bits in a byte
    {
      for (uint8_t bitSum = 0; bitSum <= 8; bitSum++)         // 8 samples of analog pin
      {
        seedBitValue = seedBitValue + (analogRead(PIN_PA6) & 0x01);                // Flip the coin eight times, adding the results together
      }
      delay(1);                                                                    // Delay a single millisecond to allow the pin to fluctuate
      seedByteValue = seedByteValue | ((seedBitValue & 0x01) << byteShift);        // Build a stack of eight flipped coins
      seedBitValue = 0;                                                            // Clear out the previous coin value
    }
    seedWordValue = seedWordValue | (uint32_t)seedByteValue << (8 * wordShift);    // Build a stack of four sets of 8 coins (shifting right creates a larger number so cast to 32bit)
    seedByteValue = 0;                                                             // Clear out the previous stack value
  }
  return (seedWordValue);
}
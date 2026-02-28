/*
  MIDI note player

  This sketch shows how to use the serial transmit pin (pin 1) to send MIDI note data.
  If this circuit is connected to a MIDI synth, it will play the notes
  F#-0 (0x1E) to F#-5 (0x5A) in sequence.

  The circuit:
  - digital in 1 connected to MIDI jack pin 5
  - MIDI jack pin 2 connected to ground
  - MIDI jack pin 4 connected to +5V through 220 ohm resistor
  - Attach a MIDI cable to the jack, then to a MIDI synth, and play music.

  created 13 Jun 2006
  modified 13 Aug 2012
  by Tom Igoe

  This example code is in the public domain.

  https://docs.arduino.cc/built-in-examples/communication/Midi/
*/

const int PIN_SO = 11;
const int PIN_SI = 12;
const int PIN_SC = 13; // Builtin LED?
const int PIN_RC = 14;

// Odd major index is initial partial press, evens are full press.
const uint8_t matrix[16][16] = {
    {39, 38, 37, 36, 35, 34, 33, 32, 79, 78, 77, 76, 75, 74, 73, 72},
    {39, 38, 37, 36, 35, 34, 33, 32, 79, 78, 77, 76, 75, 74, 73, 72},
    {0, 0, 0, 0, 0, 0, 0, 0, 87, 86, 85, 84, 83, 82, 81, 80},
    {0, 0, 0, 0, 0, 0, 0, 0, 87, 86, 85, 84, 83, 82, 81, 80},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {7, 6, 5, 4, 3, 2, 1, 0, 47, 46, 45, 44, 43, 42, 41, 40},
    {7, 6, 5, 4, 3, 2, 1, 0, 47, 46, 45, 44, 43, 42, 41, 40},
    {15, 14, 13, 12, 11, 10, 9, 8, 55, 54, 53, 52, 51, 50, 49, 48},
    {15, 14, 13, 12, 11, 10, 9, 8, 55, 54, 53, 52, 51, 50, 49, 48},
    {23, 22, 21, 20, 19, 18, 17, 16, 63, 62, 61, 60, 59, 58, 57, 56},
    {23, 22, 21, 20, 19, 18, 17, 16, 63, 62, 61, 60, 59, 58, 57, 56},
    {31, 30, 29, 28, 27, 26, 25, 24, 71, 70, 69, 68, 67, 66, 65, 64},
    {31, 30, 29, 28, 27, 26, 25, 24, 71, 70, 69, 68, 67, 66, 65, 64}
};

const uint32_t NOPE_ITS_RELEASED = 0;
const uint32_t KEYBED_OFFSET_RIGHT = 9; 

// 0 means it is released
uint32_t micros_at_partial[88] = { NOPE_ITS_RELEASED };

uint8_t in_flight_velocities[88] = { 0 };

// Writes the given word to the output register (blue)
// 1 = LOW, 0 = HIGH
// MSB in shift registers is closest to teensy connector
void writeOutputs(unsigned int b) {
  // I sure hope the bit-order is correct...
  for (int i = 0; i < 16; i++) {
    if (b & (1 << i)) {
      digitalWrite(PIN_SO, HIGH);
    } else {
      digitalWrite(PIN_SO, LOW);
    }
    // TODO: Double check these delays with register datasheets
    // delay(2);
    digitalWrite(PIN_SC, HIGH);
    // delay(2);
    digitalWrite(PIN_SC, LOW);
  }

  // Actually latch the shifted-in values
  // delay(2);
  digitalWrite(PIN_RC, LOW);
  // delay(2);
  digitalWrite(PIN_RC, HIGH);
}

int indexOfFirstOnBitIn(unsigned int d) {
  unsigned int index = 0;
  while ((d >> index) % 2 == 0) {
    index++;
    if (index > 64) {
      return -1;
    }
  }
  return index;
}

unsigned int readInputs() {
  unsigned int r = 0;

  //digitalWrite(PIN_SC, LOW); // In case it was left on high
  digitalWrite(PIN_RC, LOW); // Enable load into parallel-in
  // delay(2);
  digitalWrite(PIN_RC, HIGH); // Disable load into parallel-in
  // delay(2);
  for (int i = 0; i < 16; i++) {
    if (digitalRead(PIN_SI) == LOW) { // Actually read value
      r |= (1 << i);
    }
    digitalWrite(PIN_SC, LOW); // Get ready for next edge
    // delay(2);
    digitalWrite(PIN_SC, HIGH); // Shift next value
    // delay(2);
  }
  return r;
}

uint8_t velocity_from_press_micros(uint32_t us) {
  int8_t vel = 127 - min(127, (127 * us) / 100000);
  return max(0, (uint8_t)vel);
}

void setup() {
  // Set MIDI baud rate:
  Serial.begin(115200);

  pinMode(PIN_SO, OUTPUT);
  pinMode(PIN_SI, INPUT_PULLUP);
  pinMode(PIN_SC, OUTPUT);
  pinMode(PIN_RC, OUTPUT);
  Serial.println("End of start()");
}

void loop() {
  for (int i = 0; i < 16; i++) {
    writeOutputs(~(1 << i));
    int all_bits_in_chunk = readInputs();
    for (int b = 0; b < 16; b++) {
      uint8_t key = matrix[i][b];
      bool is_bit_set = (all_bits_in_chunk & (1 << b)) != 0;
      bool is_bottom_scan = i % 2 == 0;
      bool is_partial_scan = i % 2 == 1;
      
      if (is_partial_scan) {
        if (is_bit_set) {
          // This key is partially pressed
          if (micros_at_partial[key] == NOPE_ITS_RELEASED) {
            // Store micros at first instant partial press is discovered
            // For travel time (force) calculation
            micros_at_partial[key] = micros();
            Serial.println("Partial press detected");
          }
        } else {
          // This is a key that is not even partially pressed...
          if (in_flight_velocities[key] != 0) {
            // But it is still ringing! Silence it!
            in_flight_velocities[key] = 0;
            usbMIDI.sendNoteOff(key + KEYBED_OFFSET_RIGHT, 0, 0);
          }
          micros_at_partial[key] = NOPE_ITS_RELEASED; // Clear previous partial press time
          continue; // Go to next bit no matter how you paste this around...
          
        }
      } else if (is_bottom_scan) {
        if (is_bit_set) {
          // This key is fully pressed...
          if (in_flight_velocities[key] == 0) {
            // But it has not made any sound yet! Make some noise
            uint32_t partial_to_bottom_micros = micros() - micros_at_partial[key];
            uint8_t vel = velocity_from_press_micros(partial_to_bottom_micros);
            usbMIDI.sendNoteOn(key + KEYBED_OFFSET_RIGHT, vel, 0);
            in_flight_velocities[key] = vel;
            Serial.print(key);
            Serial.print(" : ");
            Serial.print(partial_to_bottom_micros);
            Serial.println();
          }
        } else {
          // This key is not fully pressed
          // Nothing to do in particular in this case...
        }
      }
    }
  }
  
  while (usbMIDI.read()) {
    // ignore incoming MIDI messages from the computer
  }
}
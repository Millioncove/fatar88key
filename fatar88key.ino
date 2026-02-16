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

const int matrix[16][16] = { 
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 88, 87, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 88, 87, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
  {100, 82, 83, 84, 85, 86, 87, 99, 16, 17, 18, 19, 20, 21, 22, 23},
 };

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

void setup() {
  // Set MIDI baud rate:
  Serial.begin(115200);

  pinMode(PIN_SO, OUTPUT);
  pinMode(PIN_SI, INPUT_PULLUP); // Does pull matter..?
  pinMode(PIN_SC, OUTPUT);
  pinMode(PIN_RC, OUTPUT);
  Serial.println("End of start()");
}

void loop() {
  // play notes from F#-0 (0x1E) to F#-5 (0x5A):
  /*
  for (int note = 0x1E; note < 0x5A; note++) {
    //digitalWrite(PIN_SC, HIGH);
    usbMIDI.sendNoteOn(70, 95, 1);
    delay(1000);
    //digitalWrite(PIN_SC, LOW);
    usbMIDI.sendNoteOff(70, 0, 1);
    delay(1000);
  }
  */

  for (int i = 0; i < 16; i++) {
    writeOutputs(~(1 << i));
    int readInp = readInputs();
    if (readInp) {
      int bitIndex = indexOfFirstOnBitIn(readInp);
      Serial.print(i);
      Serial.print(" -> \t");
      Serial.print(bitIndex);
      Serial.print(" | \t");
      Serial.println(matrix[i][bitIndex]);
    }
  }

  while (usbMIDI.read()) {
    // ignore incoming MIDI messages from the computer
  }
}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}

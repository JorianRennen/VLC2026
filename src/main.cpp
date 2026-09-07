#include <Arduino.h>
#include "CRC16.h"

CRC16 crc;

#define SLEEP 1

#define TX

/*
 * The VLC receiver is equipped with an OPT101 photodiode. 
 * Pin 5 of the OPT101 is connected to A0 of the Arduino Due
 * Pin 1 of the OPT101 is connected to 5V of the Arduino Due
 * Pin 8 of the OPT101 is connected to GND of the Arduino Due
 */
#define PD A0 // PD: Photodiode

/*
  test_tx.ino: testing the VLC transmitter
  Course: CS4425 Visible Light Communication & Sensing
*/
const char payload[] = "Hello VLC&S 2026-2027";
static const uint8_t preamble[16] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0}; // 0xAA = 10101010
char buffer[256];

/*
 * The VLC transmitter is equipped with an RGB LED. 
 * The LED's three channels, R, G, B, can be controlled individually.
 * The R channel is connected to Pin 38 of the Arduino Due
 * The G channel is connected to Pin 42 of the Arduino Due
 * The B channel is connected to Pin 34 of the Arduino Due
 */
const int ledR= 38; // GPIO for controlling R channel
const int ledG= 42; // GPIO for controlling G channel
const int ledB= 34; // GPIO for controlling B channel

/*
 * Brightness of each channel.
 * The range of the brightness is [0, 255].
 * *  0 represents the highest brightness
 * *  255 represents the lowest brightness
 */
int britnessR = 255; // Default: lowest brightness
int britnessG = 255; // Default: lowest brightness
int britnessB = 255; // Default: lowest brightness


#ifdef TX

void send_bit(int bit) {
  if (bit == 0) {
    analogWrite(ledR, 0);
    delay(SLEEP); // TX frequency:  1s/400ms = 2.5 Hz
    analogWrite(ledR, 255);
    delay(SLEEP); // TX frequency:  1s/400ms = 2.5 Hz
  } else {
    analogWrite(ledR, 255);
    delay(SLEEP); // TX frequency:  1s/400ms = 2.5 Hz
    analogWrite(ledR, 0); 
    delay(SLEEP); // TX frequency:  1s/400ms = 2.5 Hz
  }
}

/*
 * Some configurations
 */
void setup() {
  Serial.begin(115200); // Set the Baud rate to 115200 bits/s
  while (Serial.available() > 0)
    Serial.read();

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  analogWrite(ledR, britnessR); // Turn OFF the R channel
  analogWrite(ledG, britnessG); // Turn OFF the G channel
  analogWrite(ledB, britnessB); // Turn OFF the B channel
}

/*
 * The Main function
 */
void loop() {
  uint8_t payload_len = strlen(payload);

  // Frame layout:
  // [0..1] Preamble (0xAA, 0xAA) - 2 bytes
  // [2]    Payload Length        - 1 byte
  // [3..N] Payload data          - payload_len bytes
  // [N+1..N+2] CRC16 Checksum    - 2 bytes

  size_t total_frame_len = 2 + 1 + payload_len + 2;
  uint8_t frame[total_frame_len];

  // 1. Pack Preamble (Big-Endian: 0xAAAA)
  frame[0] = 0xAA;
  frame[1] = 0xAA;

  // 2. Pack Length
  frame[2] = payload_len;

  // 3. Pack Payload
  memcpy(&frame[3], payload, payload_len);

  // 4. Calculate CRC over the payload (or over length + payload)
  crc.reset();
  crc.setPolynome(0x1021);
  crc.add(&frame[2], 1 + payload_len); // Includes length and payload bytes
  uint16_t checksum = crc.calc();

  // 5. Append CRC (Big-Endian: High byte first, then Low byte)
  frame[3 + payload_len]     = (checksum >> 8) & 0xFF;
  frame[3 + payload_len + 1] = checksum & 0xFF;

  for (int i = 0; i < sizeof(frame); i++) {
    char c = frame[i];
    // Serial.print(">char:");
    // Serial.println(c);
    for (int j = 7; j >= 0; j--) { 
      int bit = (c >> j) & 1;
      // Serial.print("bit:");
      Serial.print(bit);
      send_bit(bit);
    }
  }
  /*
   * In this simple test, only the R channel is used to transmit data.
   * The R channel is turned ON and OFF alternatively to transmit 1 and 0. 
   * The TX frequency is set to 10 Hz, i.e., sending a symbol every 100 ms
   */
  // analogWrite(ledR, britnessR);
  // britnessR = (britnessR == 0 ? 255 : 0);
  Serial.println();
  analogWrite(ledR, 255);
  delay(4); // TX frequency:  1s/400ms = 2.5 Hz
}

#else
/*
  test_rx.ino: testing the VLC receiver
  Course: CS4425 Visible Light Communication & Sensing
*/
/*
 * Some configurations
 */
void setup() {
  Serial.begin(115200);
}


/*
 * The Main function
 */
void loop() {
  uint32_t lightValues[4] = {0};
  uint8_t index = 0;
  uint8_t frame_stage = 0;
  uint8_t buffer_index = 0;
  uint16_t last_value = 0;
  uint16_t payload_len = 0;
  size_t total_frame_len = 0;
  char rec_payload[256]; // Assuming a maximum payload length of 256 bytes
  uint8_t rec_payload_index = 0;

  while (1)
  {
    uint16_t value = analogRead(PD);
    // Serial.print(">value:");
    // Serial.println(value);
    lightValues[index] = value;
    index = (index + 1) % 4;
    uint16_t lightValue = 0;
    for (int i = 0; i < 4; i++) {
      lightValue += lightValues[i];
    }
    lightValue /= 4;
    // Serial.print(">lightValue:");
    // Serial.println(lightValue);
    if (value > lightValue) {
      // Serial.print(">raw:");
      // Serial.println(1);
      if (last_value == 0) {
        // Serial.print(">bit:");
        // Serial.println(0);
        // Serial.print(1);
        buffer[buffer_index] = 1;
        buffer_index++;
        last_value = 2;
      } else {
        last_value = 1;
      }
    } else {
      // Serial.print(">raw:");
      // Serial.println(0);
      if (last_value == 1) {
        // Serial.print(">bit:");
        // Serial.println(1);
        // Serial.print(0);
        buffer[buffer_index] = 0;
        buffer_index++;
        last_value = 2;
      } else {
        last_value = 0;
      }
    }
    // Serial.print(">raw:");
    // Serial.println();

    char c = 0;

    if (frame_stage == 0) {
      if (buffer_index >= 16) {
        if (memcmp(&buffer[buffer_index - 16], preamble, 16) == 0) {
          buffer_index = 0;
          frame_stage = 1;
          Serial.println("\nDetected preamble: 0xAAAA");
        } else {
          // Keep only the most recent 15 bits, leaving room for the next sample
          memmove(buffer, &buffer[1], 15);
          buffer_index = 15;
        }
      }
    }
    else if (buffer_index == 8 && frame_stage == 1) {
      payload_len = 0;
      for (int i = 0; i < 8; i++) {
        payload_len = (payload_len << 1) | (buffer[i] & 0x01);
      }

      // Reject frames claiming lengths outside your protocol bounds
      if (payload_len == 0 || payload_len > 64) { 
        frame_stage = 0;
        buffer_index = 0;
      } else {
        Serial.print("Received length: ");
        Serial.println(payload_len, DEC);
        buffer_index = 0;
        rec_payload_index = 0;
        frame_stage = 2;
      }
    }
    else if (buffer_index == 8 && frame_stage == 2) {
      for (int i = 0; i < 8; i++) {
        c = (c << 1) | (buffer[i] & 0x01);
      }
      Serial.print(c);
      rec_payload[rec_payload_index++] = c;
      buffer_index = 0;
      payload_len--;

      if (payload_len == 0) {
        frame_stage = 3;
      }
    }
    else if (buffer_index == 16 && frame_stage == 3) {
      payload_len = rec_payload_index;
      total_frame_len = 2 + 1 + payload_len + 2;

      uint8_t frame[total_frame_len];
      frame[0] = 0xAA;
      frame[1] = 0xAA;
      frame[2] = payload_len;
      memcpy(&frame[3], rec_payload, payload_len);

      crc.reset();
      crc.setPolynome(0x1021);
      crc.add(&frame[2], 1 + payload_len);
      uint16_t checksum = crc.calc();

      uint16_t val = 0;
      for (int i = 0; i < 16; i++) {
        val = (val << 1) | (buffer[i] & 0x01);
      }

      Serial.println();
      Serial.print("Received checksum: 0x");
      Serial.println(val, HEX);
      Serial.print("Calculated checksum: 0x");
      Serial.println(checksum, HEX);

      if (val == checksum) {
        Serial.println("CRC Passed!");
      } else {
        Serial.println("CRC Mismatch!");
      }

      // Reset state machine for next frame
      buffer_index = 0;
      rec_payload_index = 0;
      frame_stage = 0;
      memset(buffer, 0, sizeof(buffer));
    }
    delay(SLEEP); // two times per second
  }
}

#endif
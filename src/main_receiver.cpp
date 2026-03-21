#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRac.h>

// ESP12E receiver pin connected to OUT of IR demodulator module.
// Change if your hardware uses a different GPIO.
const uint16_t kIrRecvPin = 14;  // D5 on many ESP8266 dev boards
const uint16_t kCaptureBufferSize = 500;
const uint8_t kTimeout = 13;  // milliseconds

IRrecv irrecv(kIrRecvPin, kCaptureBufferSize, kTimeout);
decode_results results;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("IR receiver started");
  Serial.printf("Listening on GPIO %u\n", kIrRecvPin);
  irrecv.enableIRIn();
}

void loop() {
  if (!irrecv.decode(&results)) {
    delay(20);
    return;
  }

  Serial.println("------------------------------");
  Serial.print(resultToHumanReadableBasic(&results));

  const String ac_desc = IRAcUtils::resultAcToString(&results);
  if (ac_desc.length()) {
    Serial.println("A/C decode: " + ac_desc);
  }

  if (results.decode_type == decode_type_t::KELON168) {
    Serial.println("Matched protocol: KELON168");
  }

  Serial.println(resultToSourceCode(&results));
  Serial.println();

  irrecv.resume();
}
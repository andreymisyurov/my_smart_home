#include <SPI.h>
#include <Ethernet.h>
#include "ping.h"

const uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setup() {
  Serial.begin(9600);
  while (! Serial);

  if (! Ethernet.begin((uint8_t*)mac)) {
    Serial.println(F("Failed to configure Ethernet using DHCP!"));
    while (true);
  }
  Serial.print(F("Local IP: "));
  Serial.println(Ethernet.localIP());
}

void loop() {
  static uint8_t step = 0;
  IPAddress target;

  if (! step) {
    target = Ethernet.gatewayIP();
    step = 1;
  } else {
    target = IPAddress(192, 168, 1, 118);
    step = 0;
  }
  Serial.print(F("ping "));
  Serial.print(target);
  Serial.print(F("... "));
  int32_t time = ping(target);
  if (time < 0)
    Serial.println(F("FAIL!"));
  else {
    if (time)
      Serial.print(time);
    else
      Serial.print(F("< 1"));
    Serial.println(F(" ms"));
  }
  delay(5000);
}

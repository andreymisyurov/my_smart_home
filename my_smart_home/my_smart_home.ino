#define F_CPU 16000000UL
#define LED_PIN 2
#define LOCK_PIN 3

#include <avr/io.h>
#include <util/delay.h>
#include <SPI.h>
#include <Ethernet.h>
#include "ping.h"

IPAddress phoneIP(192, 168, 1, 118);
IPAddress serverIP(35, 205, 211, 191);

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient ethClient;
String URL = "eqwjy1y2r6i5flmeakxnlc.hooks.webhookrelay.com";

unsigned long lastMessageTime = 0;
const long oneHourMillis = 3600000L;

bool isOnline();
int sendAlarmMessage();

void setup() {
  Serial.begin(9600);

  DDRD |= (1 << LED_PIN);
  DDRD &= ~(1 << LOCK_PIN);
  PORTD |= (1 << LOCK_PIN);

  Ethernet.begin(mac);
}

void loop() {
  unsigned long currentMillis = millis();
  bool gercon_state = (PIND & (1 << LOCK_PIN)) == 0;
  if(0 == isOnline()) {
    if(0 == gercon_state) {
      PORTD |= (1 << LED_PIN);
      Serial.println(currentMillis);
      Serial.println(lastMessageTime);
      Serial.println();
      if ((0 == lastMessageTime || ((currentMillis - lastMessageTime) > oneHourMillis)) && sendAlarmMessage() == 0) {
        lastMessageTime = currentMillis;
      }
    } else {
      PORTD &= ~(1 << LED_PIN);
    }
  } else {
    PORTD &= ~(1 << LED_PIN);
  }
  _delay_ms(100);
}

bool isOnline() {
  if (ping(phoneIP) < 0) {
    return false;
  }
  return true;
}

int sendAlarmMessage() {
  bool result = 1;
  if (ethClient.connect(serverIP, 80)) {
    ethClient.println("POST / HTTP/1.1");
    ethClient.println("Host: " + URL);
    ethClient.println("Connection: close");
    ethClient.println();
    result = 0;
    while (ethClient.available()) ethClient.read();
  } else {
    Serial.println("Connection to webhook relay failed");
  }
  ethClient.stop();
  return result;
}


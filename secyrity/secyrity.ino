/*
 * Smart Home Security System
 *
 * Developed for Arduino UNO with FreeRTOS.
 *
 * This program monitors the state of a door, checks the online status of a device, 
 * provides a web interface to lock or unlock the door, and sends an alarm message 
 * if the door is opened while locked.
 *
 * Author: Andrey Misyurov 2023
 */

#include <Arduino_FreeRTOS.h>
#include <Ethernet.h>
#include "ping.h"

#define F_CPU 16000000UL
#define LED_PIN 2
#define OPEN_PIN 3
#define SPEAKER_PIN 4

// Data structure to hold various system states
typedef struct Data {
  bool isOnline :1; // Flag to check if a owner's phone is online
  bool isOpen   :1; // Flag to check if the door is open
  bool isLock   :1; // Flag to check if the door is locked
} Data ;

Data params = {1, 0, 0};
EthernetServer server(80);

void TaskLEDAndOpen(void *pvParam);
void TaskPing(void *pvParam);
void TaskLock(void *pvParam);
void TaskMessage(void *pvParam);

void setup() {
  server.begin();

  const byte mac[] PROGMEM = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  const byte ip[] PROGMEM = {192, 168, 1, 100};
  Ethernet.begin(mac, ip);

  DDRD |= (1 << LED_PIN);
  DDRD |= (1 << SPEAKER_PIN);
  DDRD &= ~(1 << OPEN_PIN);
  PORTD |= (1 << OPEN_PIN);

  xTaskCreate(
    TaskLEDAndOpen
    ,  "LEDAndOpen"
    ,  64
    ,  &params
    ,  4
    ,  NULL );

  xTaskCreate(
    TaskPing
    ,  "Ping"
    ,  128
    ,  &params
    ,  3
    ,  NULL );

  xTaskCreate(
    TaskLock
    ,  "Lock"
    ,  256
    ,  &params
    ,  2
    ,  NULL );

  xTaskCreate(
    TaskMessage
    ,  "Message"
    ,  128
    ,  &params
    ,  1
    ,  NULL );
    
    vTaskStartScheduler();
}

void loop(){ }

// Task to manage LED status, read door status and siren manage
void TaskLEDAndOpen(void *pvParam) {
  Data *data = (Data*)pvParam;

  for (;;) {
    data->isOpen = (PIND & (1 << OPEN_PIN)) != 0;
    if(false == data->isOpen) {
      PORTD &= ~(1 << LED_PIN);
      noTone(SPEAKER_PIN);
    } else {
      if (false == data->isOnline) {
        tone(SPEAKER_PIN, 500);
      }
      PORTD |= (1 << LED_PIN);
    }
    vTaskDelay(281 / portTICK_PERIOD_MS);
  }
}

// Task to check the online status of a owner's phone using ping
void TaskPing(void *pvParam) {
  IPAddress PROGMEM phoneIP(192, 168, 1, 2);
  Data *data = (Data*)pvParam;
  for (;;) {
    data->isOnline = ping(phoneIP) >= 0;
    vTaskDelay(10042 / portTICK_PERIOD_MS);
  }
}

// Task to handle HTTP requests to lock/unlock the door
void TaskLock(void *pvParam) {
  Data *data = (Data*)pvParam;
  
  for (;;) {
    EthernetClient client = server.available();
    if (client) {
      if (client.available()) {
        String request = client.readStringUntil('\r');
        client.flush();

        if (request.indexOf("GET /l") != -1) {
          data->isLock = true;
          client.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nlock"));
        } else if (request.indexOf("GET /u") != -1) {
          data->isLock = false;
          client.println(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nunlock"));
        } else {
          client.println(F("HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n\r\n"
                          "<html><body><h1>Lock/Unlock</h1>"
                          "<button onclick=\"window.location.href='/l'\">L</button>"
                          "<button onclick=\"window.location.href='/u'\">U</button>"
                          "</body></html>"));
        }
        
        vTaskDelay(10);
        client.stop();
      }
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Task to send alarm messages if the door is opened while locked
void TaskMessage(void *pvParam) {
  Data *data = (Data*)pvParam;
  EthernetClient ethClient;
  const IPAddress PROGMEM serverIP(35, 205, 211, 191);

  auto sendAlarmMessage = [&]() -> int {
    if (ethClient.connect(serverIP, 80)) {
      ethClient.println("POST / HTTP/1.1");
      ethClient.println("Host: eqwjy1y2r6i5flmeakxnlc.hooks.webhookrelay.com");
      ethClient.println("Connection: close");
      ethClient.println();
      while (ethClient.available()) ethClient.read();
      ethClient.stop();
      return 0;
    }
    return 1;
  };

  for(;;){
    if(data->isOpen && data->isLock) {
          if(sendAlarmMessage() == 0) {
            vTaskDelay(3600000);
          }
    }
    vTaskDelay(418 / portTICK_PERIOD_MS);
  }
}

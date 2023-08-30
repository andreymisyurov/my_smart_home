#include <Arduino_FreeRTOS.h>
#include <Ethernet.h>
#include "ping.h"

#define F_CPU 16000000UL
#define LED_PIN 2
#define OPEN_PIN 3

typedef struct Data {
  bool isOnline;
  bool isOpen;
  bool isLock;
} Data ;

Data params = {1, 0, 0};
EthernetServer server(80);

void TaskLEDAndOpen(void *pvParameters);

void TaskPing(void *pvParameters);
void TaskLock(void *pvParameters);
void TaskMessage(void *pvParameters);

void setup() {

  byte mac[] PROGMEM = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  Ethernet.begin(mac);
  server.begin();

  DDRD |= (1 << LED_PIN);
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

void TaskLEDAndOpen(void *pvParameters) {
  Data *data = (Data*)pvParameters;

  for (;;) {
    data->isOpen = (PIND & (1 << OPEN_PIN)) != 0;
    data->isOpen ? PORTD |= (1 << LED_PIN) : PORTD &= ~(1 << LED_PIN);
    // (data->isOpen && data->isOnline) ? PORTD |= (1 << LED_PIN) : PORTD &= ~(1 << LED_PIN);
    vTaskDelay(281 / portTICK_PERIOD_MS);
  }
}

void TaskPing(void *pvParameters) {
  IPAddress PROGMEM phoneIP(192, 168, 1, 2);
  Data *data = (Data*)pvParameters;
  for (;;) {
    data->isOnline = ping(phoneIP) >= 0;
    vTaskDelay(10042 / portTICK_PERIOD_MS);
  }
}


void TaskLock(void *pvParameters) {
  Data *data = (Data*)pvParameters;
  
  for (;;) {
    EthernetClient client = server.available();
    if (client) {
      PORTD |= (1 << LED_PIN);
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

void TaskMessage(void *pvParameters) {
  Data *data = (Data*)pvParameters;
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
    if( ((data->isOpen) && (data->isLock)) ||
        ((data->isOpen) && (data->isOnline) == 0) ) {
          if(sendAlarmMessage() == 0) {
            vTaskDelay(3600000);
          }
    }
    vTaskDelay(418 / portTICK_PERIOD_MS);
  }
}

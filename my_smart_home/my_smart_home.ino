#define F_CPU 16000000UL
#define LED_PIN 2
#define LOCK_PIN 3

#include <avr/io.h>
#include <util/delay.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "ping.h"

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress phoneIP(192, 168, 1, 118);
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

bool isOnline();

void setup() {
  Serial.begin(9600);

  DDRD |= (1 << LED_PIN);
  DDRD &= ~(1 << LOCK_PIN);
  PORTD |= (1 << LOCK_PIN);

  int check = Ethernet.begin(mac);

  mqttClient.setServer("broker.hivemq.com", 1883);
  if (mqttClient.connect("YOUR_UNIQUE_CLIENT_ID")) {
      Serial.println("Connected to MQTT Broker!");
  } else {
      Serial.println("Failed to connect to MQTT Broker.");
  }

}

void loop() {

  mqttClient.loop();

    bool gercon_state = (PIND & (1 << LOCK_PIN)) == 0;
    if(0 == isOnline()) {
      if(0 == gercon_state) {
        PORTD |= (1 << LED_PIN);
        mqttClient.publish("GABELLA", "Hello from Arduino!");
        Serial.println("I did it!");
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













// #include <SPI.h>
// #include <Ethernet.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>

// OneWire oneWire(3);
// DallasTemperature sensors(&oneWire);

// byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// IPAddress ip(192, 168, 1, 177);
// EthernetServer server(80);
// int semafor = 1;

// void setup()
// {
//   Ethernet.begin(mac, ip);
//   server.begin();
//   Serial.begin(9600);
//   sensors.begin();
//   pinMode(8, OUTPUT);
// }

// void sendOn(EthernetClient in_client) {
//   in_client.println("HTTP/1.1 200 OK");
//   in_client.println("Content-Type: text/html");
//   in_client.println("Connection: close");
//   in_client.println();
//   in_client.println("<!DOCTYPE HTML>");
//   in_client.println("<html>");
//   in_client.println("<h1>Light on!</h1>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/off\">Turn Off</a></h2>");
//   in_client.println("</html>");
// }

// void sendOff(EthernetClient in_client) {
//   in_client.println("HTTP/1.1 200 OK");
//   in_client.println("Content-Type: text/html");
//   in_client.println("Connection: close");
//   in_client.println();
//   in_client.println("<!DOCTYPE HTML>");
//   in_client.println("<html>");
//   in_client.println("<h1>Light off!</h1>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/on\">Turn On</a></h2>");
//   in_client.println("</html>");
// }

// void sendHome(EthernetClient in_client) {
//   in_client.println("HTTP/1.1 200 OK");
//   in_client.println("Content-Type: text/html");
//   in_client.println("Connection: close");
//   in_client.println();
//   in_client.println("<!DOCTYPE HTML>");
//   in_client.println("<html>");
//   in_client.println("<h1>You can turn On or Off your light</h1>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/on\">Turn On</a></h2>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/off\">Turn Off</a></h2>");
//   in_client.println("</html>");
// }

// void sendTemp(EthernetClient in_client, float in_temp) {
//   in_client.println("HTTP/1.1 200 OK");
//   in_client.println("Content-Type: text/html");
//   in_client.println("Connection: close");
//   in_client.println();
//   in_client.println("<!DOCTYPE HTML>");
//   in_client.println("<html>");
//   in_client.print("<h1>Your temperature:</h1>");
//   in_client.println(in_temp);
//   in_client.println("<h1>You can turn On or Off your light</h1>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/on\">Turn On</a></h2>");
//   in_client.println("<h2><a href=\"http://192.168.1.177/off\">Turn Off</a></h2>");
//   in_client.println("</html>");
// }

// void loop() {

//   if (semafor == 0) {
//     digitalWrite(8, LOW);
//   } else if (semafor == 1) {
//     digitalWrite(8, HIGH);
//   }

//   EthernetClient client = server.available();

//   if (client) {
//     String request = "";
//     while (client.connected())
//     {
//       if (client.available())
//       {
//         char c = client.read();
//         if (c == '\n')
//         {
//           break;
//         }
//         request += c;
//       }
//     }

//     if (request.indexOf("temp") >= 0) {
//       sensors.requestTemperatures(); 
//       sendTemp(client, sensors.getTempCByIndex(0));
//     } else if (request.indexOf("on") >= 0) {
//       sendOn(client);
//       semafor = 1;
//     } else if (request.indexOf("off") >= 0) {
//       sendOff(client);
//       semafor = 0;
//     } else {
//       sendHome(client);
//     }
//   client.stop();
//   }
// }


// #include <OneWire.h>
// #include <DallasTemperature.h>

// #define ONE_WIRE_BUS 2  // Подключение датчика DS18B20 к пину D2

// OneWire oneWire(ONE_WIRE_BUS);
// DallasTemperature sensors(&oneWire);

// void setup() {
//   Serial.begin(9600);  // Начинаем серийное соединение со скоростью 9600 бод
//   sensors.begin();     // Начинаем работу с датчиком температуры
// }

// void loop() {
//   sensors.requestTemperatures();  // Запрашиваем температуру с датчика
//   float temperature = sensors.getTempCByIndex(0);  // Считываем температуру с первого датчика на шине
//   Serial.print("Temperature: ");
//   Serial.println(temperature);  // Печатаем значение температуры в Serial Monitor
//   delay(1000);  // Пауза в 1 секунду
// }
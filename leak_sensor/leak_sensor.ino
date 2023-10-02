#include <ESP8266WiFi.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// You should define 
// const char* ssid = "YOUR_WIFI_LOGIN"
// const char* password = "YOUR_WIFI_PASSWORD"
// #define BOT_TOKEN "Your-BOT-TOKEN"
// #define CHAT_ID "Your-CHAT-ID"

// I did it here
#include "credentials.h"


// definition for gpio registers
#define GPIO_OUT_REG          0x60000300
#define GPIO_IN_REG           0x6000030C
#define GPIO_ENABLE_REG       0x60000310
#define GPIO_PIN_REG(pin)     (0x60000344 + (pin * 4))

WiFiClientSecure wifiClient;
UniversalTelegramBot bot(BOT_TOKEN, wifiClient);

const int analogPin = A0;
const long SLEEP_DURATION = 1 * 60 * 1e6 / 3; // sleep 10s in ms
const long ALARM_DELAY = 1 * 60 * 1e6; // sleep 1 min in ms

void setup() {
  Serial.begin(115200);
  int analogValue = analogRead(analogPin);
  delay(100);
  Serial.println(analogValue);

  if(analogValue > 120) {
    Serial.println("Утечка обнаружена!");

    connectWifi();
    sendMessage("Утечка обнаружена!");
    WiFi.disconnect();

    ESP.deepSleep(ALARM_DELAY);
  } else {
    Serial.println("Все в порядке.");
  }

  ESP.deepSleep(SLEEP_DURATION);
}

void loop() { }

void connectWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  // Trying to connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  Serial.println("");
  Serial.println("WiFi подключено");
  Serial.println("IP адрес: ");
  Serial.println(WiFi.localIP());

  // Ignore certificate check
  wifiClient.setInsecure();

}

void sendMessage(String message) {
  if (bot.sendMessage(CHAT_ID, message, "Markdown")) {
    Serial.println("Сообщение успешно отправлено");
  } else {
    Serial.println("Ошибка отправки сообщения");
  }
  delay(300);
}

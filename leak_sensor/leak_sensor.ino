#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <UniversalTelegramBot.h>

// I defined const char* ssid, password, bot_token, chat_id here
#include "credentials.h"

constexpr long kSensorSensitivity = 120;
constexpr long kSleepDuration = 1 * 60 * 1e6 / 6; // sleep 10s in ms
constexpr long kAlarmDelay = 1 * 60 * 1e6 / 6; // sleep 1 min in ms
const String kAlarmMessage = "Leak Detected!";

struct {
  uint32_t is_sleep_gsm;
} rtcData;

void setup() {
  Serial.begin(9600);
  
  delay(3000);

  // We have only one analog PIN for esp8266, thus I use hardcode value for it.
  int analogValue = analogRead(A0);
  delay(100);

  Serial.println("Sensor data: " + analogValue);

  if(analogValue > kSensorSensitivity) {
    Serial.println("Leak Detected!");

    send_message_to_telegram(kSsid, kPassword, kChatId, kBotToken, kAlarmMessage);
    send_SMS(kPhoneNumber, kAlarmMessage);

    delay(5000);
    // sleep_gsm();
    ESP.deepSleep(kAlarmDelay);
  } else {
    Serial.println("Everything's fine.");
  }
  // sleep_gsm();
  ESP.deepSleep(kSleepDuration);

}

void loop() { }


/*
* Sending SMS
*/
void send_SMS(String phone_number, String message){
  SoftwareSerial sim800l(14, 12); // D5, D6 ----> RX, TX
  sim800l.begin(9600);
  saveData(1);

  sim800l.println("AT");
  delay(1000);
  // String response = readResponse(sim800l);
  // if (response.indexOf("OK") == -1) {
  //     Serial.println("Error: No response for AT command");
  //     sim800l.println("AT+CSCLK=1");
  //     updateSerial(sim800l);
  //     saveData(0);
  //     return;
  // }

  sim800l.println("AT+CMGF=1"); // Configuring TEXT mode
  sim800l.println("AT+CMGS=\"" + phone_number + "\"");
  updateSerial(sim800l);
  sim800l.print(message);
  updateSerial(sim800l);
  Serial.println("SMS Sent");
  sim800l.write(26);
  // sim800l.println("AT+CSCLK=1");
  // updateSerial(sim800l);
  saveData(0);
}

void sleep_gsm() {
  if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    uint32_t value = rtcData.is_sleep_gsm;
    Serial.print("rtc:");
    Serial.println(value);
    if(0 != value) {
      SoftwareSerial sim800l(14, 12); // D5, D6 ----> RX, TX
      sim800l.begin(9600);
      sim800l.println("AT+CSCLK=1");
      updateSerial(sim800l);
      saveData(0);
    }
  }
}

void saveData(uint32_t value) {
  rtcData.is_sleep_gsm = value;
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));
}

String readResponse(SoftwareSerial &sim800l) {
    String response = "";
    long startTime = millis();
    while (millis() - startTime < 2000) {  // 2-секундный тайм-аут
        while (sim800l.available()) {
            char c = sim800l.read();
            response += c;
        }
    }
    return response;
}

/*
* Working with Wifi
*/

void send_message_to_telegram(const char* ssid, const char* password, const char* chat_id, const char* bot_token, String message) {

  WiFiClientSecure wifiClient;
  UniversalTelegramBot bot(bot_token, wifiClient);


  if(0 == connect_wifi(ssid, password, wifiClient)) {
    if(0 == send_to_bot(bot, chat_id, message)) {
      Serial.println("telegram message successfully sent");
    } else {
      Serial.println("Error sending telegram message");
    }
    WiFi.disconnect();
  } else {
    Serial.println("Error wifi connection");
  }
}

int connect_wifi(const char* ssid, const char* password, WiFiClientSecure &wifiClient) {

  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  long lastcmd = millis();
  // Trying to connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    if(millis() - lastcmd > 7000) {
      lastcmd = millis();
      Serial.println("\nWiFi can not be connected");
      return 1;
    }
  }

  Serial.print("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
  wifiClient.setInsecure();
  return 0;
}

int send_to_bot(UniversalTelegramBot &bot, const char* chat_id, String message) {
  if (0 == bot.sendMessage(chat_id, message)) {
    return 1;
  }
  delay(300);
  return 0;
}

// ----------------------------------------------------------------------------------------------------


/*
* Logging
*/

void test_module(SoftwareSerial &sim800l)
{
  Serial.println(sim800l);
  sim800l.println("AT+CSQ");
  updateSerial(sim800l);
  sim800l.println("AT+CCID");
  updateSerial(sim800l);
  sim800l.println("AT+CREG?");
  updateSerial(sim800l);
  sim800l.println("AT+CBC");
  updateSerial(sim800l);
  Serial.println();
}

void updateSerial(SoftwareSerial &sim800l)
{
  delay(500);
  while (Serial.available())
  {
    sim800l.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (sim800l.available())
  {
    Serial.write(sim800l.read());//Forward what Software Serial received to Serial Port
  }
}



// AT инициализации
// AT+CMGF=1 переход к смс
// ATE0 хз что
// AT+CMGS="+77051796703" указывам номер для смс

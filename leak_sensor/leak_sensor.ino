#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <UniversalTelegramBot.h>

// I defined const char* ssid, password, bot_token, chat_id here
#include "credentials.h"

#define A0_ANALOG A0
#define D1_SIREN  5
#define D2_RELAY  4
#define D5_RX     14
#define D6_TX     12

const String kAlarmMessage            = "Leak Detected!";   // message for sending to telegram and sms
constexpr int kTone                   = 200;                // value for siren tone
constexpr long kUartSpeed             = 9600;               // speed for uart
constexpr long kSensorSensitivity     = 120;                // setting sensor sensetivity
constexpr long kTimeForWifiConnection = 7000;               // time for try to cinnect to Wifi
constexpr long kMicroDelay            = 100;                // delay for analog sensor
constexpr long kLittleDelay           = 500;                // delay 500 ms
constexpr long kWakeUpGsmDelay        = 15000;              // delay for registering sim
constexpr long kSendSmsDelay          = 5000;               // delay for registering sim
constexpr long kSleepDuration         = 1 * 60 * 1e6 / 15;  // sleep 10s in ms
constexpr long long kAlarmDelay       = 1 * 60 * 1e6 / 30;  // sleep 1 min in ms

void setup() {

  Serial.begin(kUartSpeed);

  int analogValue = analogRead(A0_ANALOG);

  delay(kMicroDelay);

  Serial.println("Sensor data: " + analogValue);

  if(analogValue > kSensorSensitivity) {
    Serial.println("Leak Detected!");

    send_message_to_telegram(kSsid, kPassword, kChatId, kBotToken, kAlarmMessage);
    send_SMS(kPhoneNumber, kAlarmMessage);

    ESP.deepSleep(kAlarmDelay);
  } else {
    Serial.println("Everything's fine.");
  }
  ESP.deepSleep(kSleepDuration);
}

void loop() { }


/*
* Sending SMS
*/
void send_SMS(String phone_number, String message){
  SoftwareSerial sim800l(D5_RX, D6_TX);     // gpio 14, 12 ----> RX, TX

  delay(kWakeUpGsmDelay);                   // time for ware up gsm module
  
  sim800l.begin(kUartSpeed);

  pinMode(D1_SIREN, OUTPUT);                // siren turn on
  tone(D1_SIREN, kTone);                    // select tone for siren
  
  delay(kLittleDelay);
  sim800l.println("AT");
  delay(kLittleDelay);
  
  // TODO validate answers from GSM module
  // String response = readResponse(sim800l);
  // if (response.indexOf("OK") == -1) {
  //     Serial.println("Error: No response for AT command");
  //     return;
  // }

  sim800l.println("AT+CMGF=1");             // configuring TEXT mode
  sim800l.println("AT+CMGS=\"" + phone_number + "\"");
  updateSerial(sim800l);
  sim800l.print(message);
  updateSerial(sim800l);
  Serial.println("SMS Sent");
  sim800l.write(26);
  updateSerial(sim800l);
  // TODO recieve answer from gsm instead this delay.
  delay(kSendSmsDelay);
  digitalWrite(D2_RELAY, HIGH);             // gsm tern off
  pinMode(D2_RELAY, OUTPUT);                // init pin for relay
  digitalWrite(D2_RELAY, LOW);              // activate relay, powering gsm module
  delay(kSendSmsDelay);                     // delay for siren
  noTone(D1_SIREN);                         // siren turn off
}


/*
* template funcs
*
void sleep_gsm() {
  if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    uint32_t value = rtcData.is_sleep_gsm;
    Serial.print("rtc:");
    Serial.println(value);
    if(0 != value) {
      SoftwareSerial sim800l(14, 12); // D5, D6 ----> RX, TX
      sim800l.begin(kUartSpeed);
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
*/

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
  // Trying wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(kLittleDelay);
    Serial.print(".");
    
    if(millis() - lastcmd > kTimeForWifiConnection) {
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

/*
* template func
*
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
*/

void updateSerial(SoftwareSerial &sim800l)
{
  delay(kLittleDelay);
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
// AT+CMGS="+77051796703" указывам номер для смс

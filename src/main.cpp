#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define IR_SENSOR_PIN GPIO_NUM_14

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */
// HW-006 v1.3
#define WAKEUP_ON_ENTER_SILVER_DOT 1
#define WAKEUP_ON_EXIT_SILVER_DOT 0

RTC_DATA_ATTR int gas_count = 0;

String jsondata;
StaticJsonDocument<64> doc;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// uint8_t broadcastAddress[] = {0xBC, 0xDD, 0xC2, 0x88, 0x81, 0x6};

// function for Sending data
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void initESPNow()
{
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  for (int ii = 0; ii < 6; ++ii)
  {
    peerInfo.peer_addr[ii] = (uint8_t)broadcastAddress[ii];
  }
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
}

void sendData()
{
  initESPNow();
  jsondata.clear();
  doc.clear();
  doc["_gas"] = gas_count;
  doc["_temp"] = 2;
  doc["_batt"] = 3;
  serializeJson(doc, jsondata);                                                   // Serilizing JSON
  Serial.println(jsondata);
  esp_now_send(broadcastAddress, (uint8_t *)jsondata.c_str(), jsondata.length()); // Sending "jsondata"
}

void setup()
{
  Serial.begin(115200);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  esp_sleep_enable_ext0_wakeup(IR_SENSOR_PIN, WAKEUP_ON_ENTER_SILVER_DOT); // 1 = High, 0 = Low

  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    gas_count++;
    sendData();
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    sendData();
    Serial.println("GO TO SLEEP...");
    esp_deep_sleep_start();
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void loop()
{
  for (int i = 0; i < 5; i++)
  {
    int ir_pin = digitalRead(IR_SENSOR_PIN);
    if (ir_pin == 1)
    {
      delay(1000);
    }
    else
    {
      Serial.println("GO TO SLEEP...");
      esp_deep_sleep_start();
    }
  }
  Serial.println("STOPPED ON SILVER DOT -> CHANGE WAKEUP LEVEL");
  gas_count--; // on wakeup gas_count++ going to be called
  esp_sleep_enable_ext0_wakeup(IR_SENSOR_PIN, WAKEUP_ON_EXIT_SILVER_DOT); // 1 = High, 0 = Low
  Serial.println("GO TO SLEEP...");
  esp_deep_sleep_start();
}
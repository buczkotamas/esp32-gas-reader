#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define IR_SENSOR_PIN GPIO_NUM_14

#define TIME_TO_SLEEP_5s 5 * 1000000
#define TIME_TO_SLEEP_1h 60 * 60 * 1000000
// HW-006 v1.3
#define PIN_LEVEL_ON_SILVER_DOT 1
#define PIN_LEVEL_NOT_ON_SILVER_DOT 0

RTC_DATA_ATTR int gas_count = 0;
RTC_DATA_ATTR int wakeup_level = PIN_LEVEL_ON_SILVER_DOT;

String jsondata;
StaticJsonDocument<64> doc;
esp_now_peer_info_t peerInfo;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// uint8_t broadcastAddress[] = {0xBC, 0xDD, 0xC2, 0x88, 0x81, 0x6};

void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

esp_err_t init_esp_now()
{
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return ESP_FAIL;
  }
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return ESP_FAIL;
  }
  // esp_now_register_send_cb(esp_now_send_cb);
  return ESP_OK;
}

float get_battery_voltage()
{
  return 42;
}

float get_temperature()
{
  return 42;
}

void send_data()
{
  if (init_esp_now() != ESP_OK)
  {
    return;
  }
  jsondata.clear();
  doc.clear();
  doc["_gas"] = gas_count;
  doc["_temp"] = get_temperature();
  doc["_batt"] = get_battery_voltage();
  serializeJson(doc, jsondata); // Serilizing JSON
  Serial.println(jsondata);
  esp_now_send(broadcastAddress, (uint8_t *)jsondata.c_str(), jsondata.length()); // Sending "jsondata"
}

void setup()
{
  Serial.begin(115200);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_5s);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
  {
    if (wakeup_level == PIN_LEVEL_ON_SILVER_DOT)
    {
      gas_count++;
    }
  }
  send_data();
}

void loop()
{
  wakeup_level = PIN_LEVEL_NOT_ON_SILVER_DOT;
  for (int i = 0; i < 5; i++)
  {
    int sensor_pin = digitalRead(IR_SENSOR_PIN);
    if (sensor_pin == PIN_LEVEL_ON_SILVER_DOT)
    {
      delay(1000); // silver dot still visible
    }
    else
    {
      wakeup_level = PIN_LEVEL_ON_SILVER_DOT;
      break;
    }
  }
  Serial.printf("Go to sleep, wakeup on %d\n", wakeup_level);
  esp_sleep_enable_ext0_wakeup(IR_SENSOR_PIN, wakeup_level);
  esp_deep_sleep_start();
}
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "Adafruit_INA219.h"
#include "DHT.h"

#define DHTPIN 4 // Digital pin connected to the DHT sensor
                 // Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
                 // Pin 15 can work but DHT must be disconnected during program upload.

#define DHTTYPE DHT11 // DHT 11
// #define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
//  #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

#define IR_SENSOR_PIN GPIO_NUM_2

#define TIME_TO_SLEEP_5s 5 * 1000000

// HW-006 v1.3
#define PIN_LEVEL_ON_SILVER_DOT 0
#define PIN_LEVEL_NOT_ON_SILVER_DOT 1

#define ESP_NOW_MESSAGE_DELAY 10

RTC_DATA_ATTR int gas_count = 0;
RTC_DATA_ATTR int wakeup_level = PIN_LEVEL_ON_SILVER_DOT;

uint64_t TIME_TO_SLEEP_1h = 1ULL * 60 * 60 * 1000 * 1000;
esp_now_peer_info_t peerInfo;
Adafruit_INA219 ina219;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0x5C, 0x08, 0x31};

// #define DEBUG 1;

void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
#ifdef DEBUG
  Serial.printf("Last Packet Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
#endif
}

esp_err_t init_esp_now()
{
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
  {
#ifdef DEBUG
    Serial.println("Error initializing ESP-NOW");
#endif
    return ESP_FAIL;
  }
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
#ifdef DEBUG
    Serial.println("Failed to add peer");
#endif
    return ESP_FAIL;
  }
  // esp_now_register_send_cb(esp_now_send_cb);
  return ESP_OK;
}

void send_data(const char *param, float value)
{
  String str = String(param);
  str.concat("=");
  if (isnan(value))
  {
    str.concat("NaN");
  }
  else
  {
    str.concat(value);
  }
  esp_now_send(broadcastAddress, (uint8_t *)str.c_str(), str.length() + 1);
#ifdef DEBUG
  Serial.println(str);
#endif
}

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif
  ina219.begin();
  dht.begin();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_1h);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
  {
    if (wakeup_level == PIN_LEVEL_ON_SILVER_DOT)
    {
      gas_count++;
    }
  }
  if (init_esp_now() != ESP_OK)
  {
    return;
  }
  send_data("gas", gas_count);
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("humidity", dht.readHumidity());
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("temp", dht.readTemperature());
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("current_mA", ina219.getCurrent_mA());
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("bus_V", ina219.getBusVoltage_V());
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("power_mW", ina219.getPower_mW());
  delay(ESP_NOW_MESSAGE_DELAY);
  send_data("shunt_mV", ina219.getShuntVoltage_mV());
}

void loop()
{
  wakeup_level = PIN_LEVEL_NOT_ON_SILVER_DOT;
  for (int i = 0; i < 5; i++)
  {
    int sensor_pin = digitalRead(IR_SENSOR_PIN);
#ifdef DEBUG
    Serial.printf("sensor_pin level = %d\n", sensor_pin);
#endif
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
#ifdef DEBUG
  Serial.printf("Go to sleep, wakeup on %s\n", wakeup_level == PIN_LEVEL_NOT_ON_SILVER_DOT ? "NOT_ON_SILVER_DOT" : "ON_SILVER_DOT");
#endif
  esp_sleep_enable_ext0_wakeup(IR_SENSOR_PIN, wakeup_level);
  esp_deep_sleep_start();
}
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "Adafruit_INA219.h"
#include "DHT.h"

#define DHTPIN 4 // Digital pin connected to the DHT sensor
                 // Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
                 // Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
// #define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

#define IR_SENSOR_PIN GPIO_NUM_14

#define TIME_TO_SLEEP_5s 5 * 1000000

// HW-006 v1.3
#define PIN_LEVEL_ON_SILVER_DOT 0
#define PIN_LEVEL_NOT_ON_SILVER_DOT 1

RTC_DATA_ATTR int gas_count = 0;
RTC_DATA_ATTR int wakeup_level = PIN_LEVEL_ON_SILVER_DOT;

uint64_t TIME_TO_SLEEP_1h = 1ULL * 60 * 60 * 1000 * 1000;
esp_now_peer_info_t peerInfo;
Adafruit_INA219 ina219;

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

void send_data()
{
  if (init_esp_now() != ESP_OK)
  {
    return;
  }
  char message[250];

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
  }

  int n = sprintf(message, "humidity=%.2f", isnan(h) ? 0 : h);
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "temp=%.2f", isnan(t) ? 0 : t);
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "bus_V=%.2f", ina219.getBusVoltage_V());
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "shunt_mV=%.2f", ina219.getShuntVoltage_mV());
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "current_mA=%.2f", ina219.getCurrent_mA());
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "power_mW=%.2f", ina219.getPower_mW());
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);

  n = sprintf(message, "gas=%d", gas_count);
  esp_now_send(broadcastAddress, (uint8_t *)message, n);
  Serial.println(message);
}

void setup()
{
  Serial.begin(115200);
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
  send_data();
}

void loop()
{
  wakeup_level = PIN_LEVEL_NOT_ON_SILVER_DOT;
  for (int i = 0; i < 5; i++)
  {
    int sensor_pin = digitalRead(IR_SENSOR_PIN);
    Serial.printf("sensor_pin level = %d\n", sensor_pin);
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
  Serial.printf("Go to sleep, wakeup on %s\n", wakeup_level == PIN_LEVEL_NOT_ON_SILVER_DOT ? "NOT_ON_SILVER_DOT" : "ON_SILVER_DOT");
  esp_sleep_enable_ext0_wakeup(IR_SENSOR_PIN, wakeup_level);
  esp_deep_sleep_start();
}
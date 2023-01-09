#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

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

void setup()
{
  Serial.begin(115200);
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

void loop()
{
  jsondata.clear();
  doc.clear();
  doc["_gas"] = 1;
  doc["_temp"] = 2;
  doc["_batt"] = 3;
  serializeJson(doc, jsondata); // Serilizing JSON

  esp_now_send(broadcastAddress, (uint8_t *)jsondata.c_str(), jsondata.length()); // Sending "jsondata"
  Serial.println(jsondata);
  delay(1000);
}
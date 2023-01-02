#include <WiFi.h>
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins
//******************************************************************************
// serial definitions for LoRa
#define SERIAL1_RXPIN 23 // TO Serial
#define SERIAL1_TXPIN 22 // TO Serial
// MAC ADDRESSES USED
uint8_t newMACAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// NEEDED VARIABLES
String StringToTransmit;
String StringToBroadcast;
//******************************************************************************
void Transmit() {
  // LETS SEND SOMETHING:
  Serial.println("Transmitting"); // printing to serial monitor
  Serial1.print(StringToTransmit); // used to send a data packet
}
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
// Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
 
  // Make sure we are null terminated
  buffer[msgLen] = 0;
 
  // Format the MAC address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
 
  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);
  
  // store and separate the buffer
  StringToTransmit = String(buffer);
  Transmit();  
}
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// Called when data is sent
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
void broadcast(const String &message)
// Emulates a broadcast
{
  // Broadcast a message to every device in range
  //uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
 
  // Print results to serial monitor
  if (result == ESP_OK)
  {
    Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESP-NOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}
// object to used to change esp default mac to custom mac
void ChangeMACaddress() {
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}
void IntializeESPNOW() {
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }
}
//****************************************************
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("Starting WiFi");
  WiFi.mode(WIFI_STA);
  esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
  esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
  //esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  //esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  //***********************************************************************
  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  //***********************************************************************
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing Serial");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
}
//***************************************************************************
void loop() {
  // LISTEN TO SERIAL INPUT FOR DATA TO SEND VIA ESPNOW
  if(Serial1.available()>0){ 
    // checking to see if there is anything incoming from the SERIAL PINS
    StringToBroadcast = Serial1.readStringUntil('\n');
    Serial.println("Serial Data Received: " + StringToBroadcast);
    broadcast(StringToBroadcast);
  }
}

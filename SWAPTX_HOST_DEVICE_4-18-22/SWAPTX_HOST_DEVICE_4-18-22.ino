
// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <Arduino_JSON.h>#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 100 // use 0 for OTA and 1 for Tagger ID
// EEPROM Definitions:
// EEPROM 0 is used for number of taggers

// serial definitions for LoRa
#define SERIAL1_RXPIN 23 // TO LORA TX
#define SERIAL1_TXPIN 22 // TO LORA RX


int TaggersOwned = 64; // how many taggers do you own or will play?

// Replace with your network credentials
const char* ssid = "swaptxscoreboard";
const char* password = "123456789";

// wifi credentials for remote updating over local wifi
String OTAssid = "dontchangeme"; // network name to update OTA
String OTApassword = "dontchangeme"; // Network password for OTA
#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/cbin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/controllerfw.bin"
bool OTAMODE = false;
void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();
bool UPTODATE = false;

String FirmwareVer = {"4.8"};

// text inputs
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

int WebSocketData;
bool ledState = 0;
const int ledPin = 2;
//const int LED_BUILTIN = 2;

int id = 0;
int pid = 0;
byte ListOfPlayers[64]; // 0 if not playing, 1 if playing

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
char *ptr = NULL; // used for initializing and ending string break up.
int Team=0; // team selection used when allowed for custom configuration

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected

bool UPDATEWEBAPP = false; // used to update json scores to web app

long WebAppUpdaterTimer = 0;

int WebAppUpdaterProcessCounter = 0;

//*****************************************************************************************
// ESP Now Objects:
//*****************************************************************************************
// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};

// data to process espnow data
String TokenStrings[20];

// scoring variables
byte LeadPlayerID = 99;
byte LeadTeamID = 99;
byte HighestScore1 = 0;
byte HighestScore2 = 0;
byte HighestScore3 = 0;
byte PlayerDeaths[64];
byte PlayerGameScore[64];
int PlayerOverallScore[64];
int PlayerDominationGameScore[64];
int TeamGameScore[4];
int TeamOverallScore[4];
int TeamDominationGameScore[4];
int TeamDominationOverallScore[4];
byte TeamCaptureTheFlagGameScore[4];
byte TeamCaptureTheFlagOverallScore[4];
byte JboxPlayerID[21]; // used for incoming jbox ids for players with posessions
byte JboxTeamID[21]; // used for incoiming jbox ids for teams with posessions
byte JboxInPlay[21]; // used as an identifier to verify if a jbox id is in play or not

bool MULTIBASEDOMINATION = false;

// Game Settings - Config:
byte SettingsGameMode = 0; // default is team battle, 1 is royale
byte SettingsLives = 0; // default low, 1 is high
byte SettingsLighting = 0; // High is default, 1 is low
byte SettingsGameTime = 0; // off is default, 1 is on
byte SettingsRespawn = 0; // auto is default, 1 is manual
byte ObjectiveMode = 0;

String SendStartBeacon = "null";
String ConfirmBeacon = "null";


bool INGAME = false;

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
  String IncomingData = String(buffer);
  char *ptr = strtok((char*)IncomingData.c_str(), ","); // looks for commas as breaks to split up the string
  int index = 0;
  while (ptr != NULL)
  {
    TokenStrings[index] = ptr; // saves the individual characters divided by commas as individual strings
    index++;
    ptr = strtok(NULL, ",");  // takes a list of delimiters
  }
  Serial.println("We have found " + String(index ) + " tokens");
  int stringnumber = 0;
  while (stringnumber < index) {
    Serial.print("Token " + String(stringnumber) + ": ");
    Serial.println(TokenStrings[stringnumber]);
    stringnumber++;
  }
  if (TokenStrings[1] == "65") {
    Serial.println("Received a Host New Game Start Beacon");
    INGAME = true;
    byte i = 0;
    // reset in game scores
    while (i < 4) {
      TeamGameScore[i] = 0;
      i++;
      vTaskDelay(0);
    }
    i = 0;
    while (i < 64) {
      PlayerGameScore[i] = 0;
      PlayerDeaths[i] = 0;
      i++;
      vTaskDelay(0);
    }
    while (i < 21) {
      JboxInPlay[i] = 0;
      i++;
      vTaskDelay(0);
    }
  }
  if (TokenStrings[1] == "101") {
    Serial.println("Received a Team Kill and PlayerScore Announcement");
    // update player/team kills
    int tempplayerid = TokenStrings[2].toInt();
    int tempteamid = TokenStrings[3].toInt();
    int tempkillcount = TokenStrings[5].toInt();
    int tempdeathcount = TokenStrings[6].toInt();
    // CHECK IF REPEAT
    if (PlayerGameScore[tempplayerid] == tempkillcount) {
      // duplicate data
    } else {
      PlayerGameScore[tempplayerid] = tempkillcount;
      TeamGameScore[tempteamid]++;
      PlayerDeaths[tempplayerid] = tempdeathcount;
      Serial.print("Player " + String(tempplayerid)+ " has " + String(PlayerGameScore[tempplayerid]) + "Kills");
      Serial.println("Team " + String(tempteamid) + " score is: " + String(TeamGameScore[tempteamid]));
      UPDATEWEBAPP = true;
    }
  }
  //if (TokenStrings[1] == "75") {
    //Serial.println("Received a Tag Confirmation");
  //}
  //if (TokenStrings[1] == "68") {
    //Serial.println("Received a Kill Confirmation");
    //int killedplayerid = TokenStrings[2].toInt();
  //}
  // process the incoming data
  //if (TokenStrings[1] == "79") {
    //Serial.println("Received a Host Discover Beacon, Sending Lockout Beacon");
    // respond with 36,97,12,3,0,0,0,0,0,42 to lock out the player, and any others around
    // broadcast("36,97,16,3,0,0,0,0,0,42");
  //}
  if (TokenStrings[1] == "69") {
    Serial.println("Received a game over winner Announcement");
    // add game scores to overall scores
    byte i = 0;
    // store last game totals
    while (i < 4) {
      TeamOverallScore[i] = TeamOverallScore[i] + TeamGameScore[i];
      i++;
      vTaskDelay(0);
    }
    i = 0;
    while (i < 64) {
      PlayerOverallScore[i] = PlayerOverallScore[i] + PlayerGameScore[i];
      i++;
      vTaskDelay(0);
    }
    EndJBoxGame(); 
  }
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
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
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
void RecallHighScores() {
  byte j = EEPROM.read(0);
  if (j == 255) {
    HighestScore1 = 0;
  } else {
    HighestScore1 = j;
  }
  j = EEPROM.read(1);
  if (j == 255) {
    HighestScore2 = 0;
  } else {
    HighestScore2 = j;
  }
  j = EEPROM.read(2);
  if (j == 255) {
    HighestScore3 = 0;
  } else {
    HighestScore3 = j;
  }
}

// object to used to change esp default mac to custom mac
void ChangeMACaddress() {
  //WiFi.mode(WIFI_STA);
  
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
  
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

void IntializeESPNOW() {
  // Set up the onboard LED
  pinMode(2, OUTPUT);
  
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  
  // Set device as a Wi-Fi Station
  //WiFi.mode(WIFI_STA);
  
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
// WebServer 
//****************************************************

JSONVar board;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebServer server1(81);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// callback function that will be executed when data is received
void UpdateWebApp0() { 
  id = 0; // team id place holder
  pid = 0; // player id place holder
  while (id < 4) {
    board["temporaryteamscore"+String(id)] = TeamGameScore[id];
    board["temporaryteamobjectives"+String(id)] = TeamDominationGameScore[id];
    id++;
  }
  while (pid < TaggersOwned) {
    board["temporaryplayerscore"+String(pid)] = PlayerGameScore[pid];
    pid++;
  }
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  if (TeamGameScore[0] > TeamGameScore[1] && TeamGameScore[0] > TeamGameScore[2] && TeamGameScore[0] > TeamGameScore[3]) {
    LeadTeamID = 0;
  }
  if (TeamGameScore[1] > TeamGameScore[0] && TeamGameScore[1] > TeamGameScore[2] && TeamGameScore[1] > TeamGameScore[3]) {
    LeadTeamID = 1;
  }
  if (TeamGameScore[2] > TeamGameScore[0] && TeamGameScore[2] > TeamGameScore[3] && TeamGameScore[2] > TeamGameScore[1]) {
    LeadTeamID = 2;
  }
  if (TeamGameScore[3] > TeamGameScore[0] && TeamGameScore[3] > TeamGameScore[2] && TeamGameScore[3] > TeamGameScore[1]) {
    LeadTeamID = 3;
  }
}
void UpdateWebApp1() { 
  pid = 0; // player id place holder
  while (pid < TaggersOwned) {
    board["temporaryplayerdeaths"+String(pid)] = PlayerDeaths[pid];
    pid++;
  }
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
}
void UpdateWebApp2() { 
  // now we calculate and send leader board information
      Serial.println("Game Highlights:");
      Serial.println();
      int KillsLeader[3];
      int ObjectiveLeader[3];
      int DeathsLeader[3];
      int LeaderScore[3];
      // first Leader for Kills:
      int kmax=0; 
      int highest=0;
      for (int k=0; k<64; k++)
      if (PlayerGameScore[k] > highest) {
        highest = PlayerGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerGameScore[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerGameScore[k] > highest) {
        highest = PlayerGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerGameScore[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerGameScore[k] > highest) {
        highest = PlayerGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerGameScore[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      Serial.println("Most Deadliest, Players with the most kills:");
      Serial.println("1st place: Player " + String(KillsLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(KillsLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(KillsLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();
      LeadPlayerID = KillsLeader[0];
      board["KillsLeader0"] = KillsLeader[0]+1;
      board["Leader0Kills"] = LeaderScore[0];
      board["KillsLeader1"] = KillsLeader[1]+1;
      board["Leader1Kills"] = LeaderScore[1];
      board["KillsLeader2"] = KillsLeader[2]+1;
      board["Leader2Kills"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerGameScore[KillsLeader[0]] = LeaderScore[0];
      PlayerGameScore[KillsLeader[1]] = LeaderScore[1];
      PlayerGameScore[KillsLeader[2]] = LeaderScore[2];

      // Now Leader for Objectives:
      kmax=0; highest=0;
      for (int k=0; k<64; k++)
      if (PlayerDominationGameScore[k] > highest) {
        highest = PlayerDominationGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerDominationGameScore[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerDominationGameScore[k] > highest) {
        highest = PlayerDominationGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerDominationGameScore[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerDominationGameScore[k] > highest) {
        highest = PlayerDominationGameScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerDominationGameScore[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      Serial.println("The Dominator, Players with the most objective points:");
      Serial.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();   
      board["ObjectiveLeader0"] = ObjectiveLeader[0]+1;
      board["Leader0Objectives"] = LeaderScore[0];
      board["ObjectiveLeader1"] = ObjectiveLeader[1]+1;
      board["Leader1Objectives"] = LeaderScore[1];
      board["ObjectiveLeader2"] = ObjectiveLeader[2]+1;
      board["Leader2Objectives"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerDominationGameScore[ObjectiveLeader[0]] = LeaderScore[0];
      PlayerDominationGameScore[ObjectiveLeader[1]] = LeaderScore[1];
      PlayerDominationGameScore[ObjectiveLeader[2]] = LeaderScore[2];

      // Now Leader for Deaths (this is opposite:
      kmax=0; highest=0;
      for (int k=0; k<64; k++)
      if (PlayerDeaths[k] > highest) {
        highest = PlayerDeaths[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      DeathsLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerDeaths[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerDeaths[k] > highest) {
        highest = PlayerDeaths[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      DeathsLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerDeaths[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerDeaths[k] > highest) {
        highest = PlayerDeaths[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      DeathsLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerDeaths[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      Serial.println("Easy Target, Players with the most deaths:");
      Serial.println("1st place: Player " + String(DeathsLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(DeathsLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(DeathsLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();
      board["DeathsLeader0"] = DeathsLeader[0]+1;
      board["Leader0Deaths"] = LeaderScore[0];
      board["DeathsLeader1"] = DeathsLeader[1]+1;
      board["Leader1Deaths"] = LeaderScore[1];
      board["DeathsLeader2"] = DeathsLeader[2]+1;
      board["Leader2Deaths"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerDeaths[DeathsLeader[0]] = LeaderScore[0];
      PlayerDeaths[DeathsLeader[1]] = LeaderScore[1];
      PlayerDeaths[DeathsLeader[2]] = LeaderScore[2];
  
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
}

const char index_html1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial;
    display: inline-block;
    text-align: center;
  }
  p {  font-size: 1.2rem;}
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
   .stopnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
   .scontent { padding: 20px; }
   .scard { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
   .scards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
   .sreading { font-size: 2.8rem; }
   .spacket { color: #bebebe; }
   .scard.red { color: #FC0000; }
   .scard.blue { color: #003DFC; }
   .scard.yellow { color: #E5D200; }
   .scard.green { color: #00D02C; }
   .scard.black { color: #000000; }
  </style>
<title>SWAPTX HOST</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>SWAPTX HOST</h1>
  </div>
  <div class="content">
    <div class="card">
      <p><button id="pbhost" class="button">Host Players</button></p>
      <p><button id="pbblock" class="button">Allow Team Changes</button></p>
      <h2>Indoor/Outdoor</h2>
      <p><select name="pbindoor" id="pbindoorid">
        <option value="ZZ">Select One</option>
        <option value="0">Indoor Mode</option>
        <option value="1">Outdoor Outdoor</option>
        </select>
      </p>
      <h2>Game Mode</h2>
      <p><select name="pbgame" id="pbgameid">
        <option value="ZZ">Select One</option>
        <option value="2">Team Battle</option>
        <option value="3">Battle Royale</option>
        </select>
      </p>
      <h2>Health/Lives</h2>
      <p><select name="pblives" id="pblivesid">
        <option value="ZZ">Select One</option>
        <option value="4">Low/5</option>
        <option value="5">High/Unlimited</option>
        </select>
      </p>
      <h2>Game Time/Storm</h2>
      <p><select name="pbtime" id="pbtimeid">
        <option value="ZZ">Select One</option>
        <option value="6">On/10min</option>
        <option value="7">Off/Unlimited</option>
        </select>
      </p>
      <h2>Respawn</h2>
      <p><select name="pbspawn" id="pbspawnid">
        <option value="ZZ">Select One</option>
        <option value="8">Trigger</option>
        <option value="9">Respawn Station</option>
        </select>
      </p>
      <h2>Objective Gameplay</h2>
      <h3>Overrides Lives/Mode/Time</h2>
      <p><select name="pbobj" id="pbobjid">
        <option value="ZZ">Select One</option>
        <option value="10">F4A 10 kills</option>
        <option value="11">F4A 20 Kills</option>
        <option value="12">Team 25 Kills</option>
        <option value="13">Team 50 Kills</option>
        <option value="14">5 Min Domination - JBOX</option>
        <option value="15">10 Min Domination - JBOX</option>
        <option value="16">20 Min Domination - JBOX</option>
        <option value="17">CTF 1 - JBOX</option>
        <option value="18">CTF 5 - JBOX</option>
        </select>
      </p>
      <p><button id="pbstart" class="button">Start Game</button></p>
      <p><button id="pbend" class="button">End Game</button></p>
    </div>
  </div>
  
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

}
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
  }
  
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('pbgameid').addEventListener('change', handlepbgame, false);
    document.getElementById('pblivesid').addEventListener('change', handlepblives, false);
    document.getElementById('pbtimeid').addEventListener('change', handlepbtime, false);
    document.getElementById('pbspawnid').addEventListener('change', handlepbspawn, false);
    document.getElementById('pbindoorid').addEventListener('change', handlepbindoor, false);
    document.getElementById('pbblock').addEventListener('click', togglepbblock);
    document.getElementById('pbstart').addEventListener('click', togglepbstart);
    document.getElementById('pbend').addEventListener('click', togglepbend);
    document.getElementById('pbhost').addEventListener('click', togglehost);
    document.getElementById('pbobjid').addEventListener('change', handlepbobj, false);
  }
  function togglehost(){
    websocket.send('togglehost');
  }
  function togglepbblock(){
    websocket.send('togglepbblock');
  }
  function togglepbend(){
    websocket.send('togglepbend');
  }
  function togglepbstart(){
    websocket.send('togglepbstart');
  }
  function handlepbgame() {
    var xs = document.getElementById("pbgameid").value;
    websocket.send(xs);
  }
  function handlepblives() {
    var xs = document.getElementById("pblivesid").value;
    websocket.send(xs);
  }
  function handlepbtime() {
    var xs = document.getElementById("pbtimeid").value;
    websocket.send(xs);
  }
  function handlepbspawn() {
    var xs = document.getElementById("pbspawnid").value;
    websocket.send(xs);
  }
  function handlepbindoor() {
    var xs = document.getElementById("pbindoorid").value;
    websocket.send(xs);
  }
  function handlepbobj() {
    var xs = document.getElementById("pbobjid").value;
    websocket.send(xs);
  }
</script>
</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial;
    display: inline-block;
    text-align: center;
  }
  p {  font-size: 1.2rem;}
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
   .stopnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
   .scontent { padding: 20px; }
   .scard { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
   .scards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
   .sreading { font-size: 2.8rem; }
   .spacket { color: #bebebe; }
   .scard.red { color: #FC0000; }
   .scard.blue { color: #003DFC; }
   .scard.yellow { color: #E5D200; }
   .scard.green { color: #00D02C; }
   .scard.black { color: #000000; }
  </style>
<title>JEDGE 3.0</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body> 
  <div class="stopnav">
    <h3>SWAPTX Score</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard red">
        <h4>Alpha Team</h4>
        <p><span class="reading">Kills:<span id="tk0"></span></span><span class="reading"> Points:<span id="to0"></span></p>
      </div>
      <div class="scard blue">
        <h4>Bravo Team</h4><p>
        <p><span class="reading">Kills:<span id="tk1"></span><span class="reading"> Points:<span id="to1"></span></p>
      </div>
      <div class="scard yellow">
        <h4>Charlie Team</h4>
        <p><span class="reading">Kills:<span id="tk2"></span><span class="reading"> Points:<span id="to2"></span></p>
      </div>
      <div class="scard green">
        <h4>Delta Team</h4>
        <p><span class="reading">Kills:<span id="tk3"></span><span class="reading"> Points:<span id="to3"></span></p>
      </div>
    </div>
  </div>
  <div class="stopnav">
    <h3>Game Highlights</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard black">
        <h2>Most Kills</h2>
        <p><span class="reading">Player <span id="MK0"></span><span class="reading"> : <span id="MK10"></span></p>
        <p><span class="reading">Player <span id="MK1"></span><span class="reading"> : <span id="MK11"></span></p>
        <p><span class="reading">Player <span id="MK2"></span><span class="reading"> : <span id="MK12"></span></p>
        <h2>Most Deaths</h2>
        <p><span class="reading">Player <span id="MD0"></span><span class="reading"> : <span id="MD10"></span></p>
        <p><span class="reading">Player <span id="MD1"></span><span class="reading"> : <span id="MD11"></span></p>
        <p><span class="reading">Player <span id="MD2"></span><span class="reading"> : <span id="MD12"></span></p>
        <h2>Most Points</h2>
        <p><span class="reading">Player <span id="MO0"></span><span class="reading"> : <span id="MO10"></span></p>
        <p><span class="reading">Player <span id="MO1"></span><span class="reading"> : <span id="MO11"></span></p>
        <p><span class="reading">Player <span id="MO2"></span><span class="reading"> : <span id="MO12"></span></p>
      </div>
    </div>
  </div>
  <div class="stopnav">
    <h3>Player Scores</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard black">
        <h4>Player 1</h4><p><span class="reading">Kills:<span id="pk0"></span><span class="reading"> Deaths:<span id="pd0"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 2</h4><p><span class="reading">Kills:<span id="pk1"></span><span class="reading"> Deaths:<span id="pd1"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 3</h4><p><span class="reading">Kills:<span id="pk2"></span><span class="reading"> Deaths:<span id="pd2"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 4</h4><p><span class="reading">Kills:<span id="pk3"></span><span class="reading"> Deaths:<span id="pd3"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 5</h4><p><span class="reading">Kills:<span id="pk4"></span><span class="reading"> Deaths:<span id="pd4"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 6</h4><p><span class="reading">Kills:<span id="pk5"></span><span class="reading"> Deaths:<span id="pd5"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 7</h4><p><span class="reading">Kills:<span id="pk6"></span><span class="reading"> Deaths:<span id="pd6"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 8</h4><p><span class="reading">Kills:<span id="pk7"></span><span class="reading"> Deaths:<span id="pd7"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 9</h4><p><span class="reading">Kills:<span id="pk8"></span><span class="reading"> Deaths:<span id="pd8"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 10</h4><p><span class="reading">Kills:<span id="pk9"></span><span class="reading"> Deaths:<span id="pd9"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 11</h4><p><span class="reading">Kills:<span id="pk10"></span><span class="reading"> Deaths:<span id="pd10"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 12</h4><p><span class="reading">Kills:<span id="pk11"></span><span class="reading"> Deaths:<span id="pd11"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 13</h4><p><span class="reading">Kills:<span id="pk12"></span><span class="reading"> Deaths:<span id="pd12"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 14</h4><p><span class="reading">Kills:<span id="pk13"></span><span class="reading"> Deaths:<span id="pd13"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 15</h4><p><span class="reading">Kills:<span id="pk14"></span><span class="reading"> Deaths:<span id="pd14"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 16</h4><p><span class="reading">Kills:<span id="pk15"></span><span class="reading"> Deaths:<span id="pd15"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 17</h4><p><span class="reading">Kills:<span id="pk16"></span><span class="reading"> Deaths:<span id="pd16"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 18</h4><p><span class="reading">Kills:<span id="pk17"></span><span class="reading"> Deaths:<span id="pd17"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 19</h4><p><span class="reading">Kills:<span id="pk18"></span><span class="reading"> Deaths:<span id="pd18"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 20</h4><p><span class="reading">Kills:<span id="pk19"></span><span class="reading"> Deaths:<span id="pd19"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 21</h4><p><span class="reading">Kills:<span id="pk20"></span><span class="reading"> Deaths:<span id="pd20"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 22</h4><p><span class="reading">Kills:<span id="pk21"></span><span class="reading"> Deaths:<span id="pd21"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 23</h4><p><span class="reading">Kills:<span id="pk22"></span><span class="reading"> Deaths:<span id="pd22"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 24</h4><p><span class="reading">Kills:<span id="pk23"></span><span class="reading"> Deaths:<span id="pd23"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 25</h4><p><span class="reading">Kills:<span id="pk24"></span><span class="reading"> Deaths:<span id="pd24"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 26</h4><p><span class="reading">Kills:<span id="pk25"></span><span class="reading"> Deaths:<span id="pd25"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 27</h4><p><span class="reading">Kills:<span id="pk26"></span><span class="reading"> Deaths:<span id="pd26"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 28</h4><p><span class="reading">Kills:<span id="pk27"></span><span class="reading"> Deaths:<span id="pd27"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 29</h4><p><span class="reading">Kills:<span id="pk28"></span><span class="reading"> Deaths:<span id="pd28"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 30</h4><p><span class="reading">Kills:<span id="pk29"></span><span class="reading"> Deaths:<span id="pd29"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 31</h4><p><span class="reading">Kills:<span id="pk30"></span><span class="reading"> Deaths:<span id="pd30"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 32</h4><p><span class="reading">Kills:<span id="pk31"></span><span class="reading"> Deaths:<span id="pd31"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 33</h4><p><span class="reading">Kills:<span id="pk32"></span><span class="reading"> Deaths:<span id="pd32"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 34</h4><p><span class="reading">Kills:<span id="pk33"></span><span class="reading"> Deaths:<span id="pd33"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 35</h4><p><span class="reading">Kills:<span id="pk34"></span><span class="reading"> Deaths:<span id="pd34"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 36</h4><p><span class="reading">Kills:<span id="pk35"></span><span class="reading"> Deaths:<span id="pd35"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 37</h4><p><span class="reading">Kills:<span id="pk36"></span><span class="reading"> Deaths:<span id="pd36"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 38</h4><p><span class="reading">Kills:<span id="pk37"></span><span class="reading"> Deaths:<span id="pd37"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 39</h4><p><span class="reading">Kills:<span id="pk38"></span><span class="reading"> Deaths:<span id="pd38"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 40</h4><p><span class="reading">Kills:<span id="pk39"></span><span class="reading"> Deaths:<span id="pd39"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 41</h4><p><span class="reading">Kills:<span id="pk40"></span><span class="reading"> Deaths:<span id="pd40"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 42</h4><p><span class="reading">Kills:<span id="pk41"></span><span class="reading"> Deaths:<span id="pd41"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 43</h4><p><span class="reading">Kills:<span id="pk42"></span><span class="reading"> Deaths:<span id="pd42"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 44</h4><p><span class="reading">Kills:<span id="pk43"></span><span class="reading"> Deaths:<span id="pd43"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 45</h4><p><span class="reading">Kills:<span id="pk44"></span><span class="reading"> Deaths:<span id="pd44"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 46</h4><p><span class="reading">Kills:<span id="pk45"></span><span class="reading"> Deaths:<span id="pd45"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 47</h4><p><span class="reading">Kills:<span id="pk46"></span><span class="reading"> Deaths:<span id="pd46"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 48</h4><p><span class="reading">Kills:<span id="pk47"></span><span class="reading"> Deaths:<span id="pd47"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 49</h4><p><span class="reading">Kills:<span id="pk48"></span><span class="reading"> Deaths:<span id="pd48"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 50</h4><p><span class="reading">Kills:<span id="pk49"></span><span class="reading"> Deaths:<span id="pd49"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 51</h4><p><span class="reading">Kills:<span id="pk50"></span><span class="reading"> Deaths:<span id="pd50"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 52</h4><p><span class="reading">Kills:<span id="pk51"></span><span class="reading"> Deaths:<span id="pd51"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 53</h4><p><span class="reading">Kills:<span id="pk52"></span><span class="reading"> Deaths:<span id="pd52"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 54</h4><p><span class="reading">Kills:<span id="pk53"></span><span class="reading"> Deaths:<span id="pd53"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 55</h4><p><span class="reading">Kills:<span id="pk54"></span><span class="reading"> Deaths:<span id="pd54"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 56</h4><p><span class="reading">Kills:<span id="pk55"></span><span class="reading"> Deaths:<span id="pd55"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 57</h4><p><span class="reading">Kills:<span id="pk56"></span><span class="reading"> Deaths:<span id="pd56"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 58</h4><p><span class="reading">Kills:<span id="pk57"></span><span class="reading"> Deaths:<span id="pd57"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 59</h4><p><span class="reading">Kills:<span id="pk58"></span><span class="reading"> Deaths:<span id="pd58"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 60</h4><p><span class="reading">Kills:<span id="pk59"></span><span class="reading"> Deaths:<span id="pd59"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 61</h4><p><span class="reading">Kills:<span id="pk60"></span><span class="reading"> Deaths:<span id="pd60"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 62</h4><p><span class="reading">Kills:<span id="pk61"></span><span class="reading"> Deaths:<span id="pd61"></span></p>
      </div>      
      <div class="scard black">
        <h4>Player 63</h4><p><span class="reading">Kills:<span id="pk62"></span><span class="reading"> Deaths:<span id="pd62"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 64</h4><p><span class="reading">Kills:<span id="pk63"></span><span class="reading"> Deaths:<span id="pd63"></span></p>
      </div>
    </div>
  </div>  
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("tk0").innerHTML = obj.temporaryteamscore0;
  document.getElementById("to0").innerHTML = obj.temporaryteamobjectives0;
  document.getElementById("tk1").innerHTML = obj.temporaryteamscore1;
  document.getElementById("to1").innerHTML = obj.temporaryteamobjectives1;
  document.getElementById("tk2").innerHTML = obj.temporaryteamscore2;
  document.getElementById("to2").innerHTML = obj.temporaryteamobjectives2;
  document.getElementById("tk3").innerHTML = obj.temporaryteamscore3;  
  document.getElementById("to3").innerHTML = obj.temporaryteamobjectives3;
  
  document.getElementById("pk0").innerHTML = obj.temporaryplayerscore0;
  document.getElementById("pd0").innerHTML = obj.temporaryplayerdeaths0;
  document.getElementById("pk1").innerHTML = obj.temporaryplayerscore1;
  document.getElementById("pd1").innerHTML = obj.temporaryplayerdeaths1;
  document.getElementById("pk2").innerHTML = obj.temporaryplayerscore2;
  document.getElementById("pd2").innerHTML = obj.temporaryplayerdeaths2;
  document.getElementById("pk3").innerHTML = obj.temporaryplayerscore3;
  document.getElementById("pd3").innerHTML = obj.temporaryplayerdeaths3;
  document.getElementById("pk4").innerHTML = obj.temporaryplayerscore4;
  document.getElementById("pd4").innerHTML = obj.temporaryplayerdeaths4;
  document.getElementById("pk5").innerHTML = obj.temporaryplayerscore5;
  document.getElementById("pd5").innerHTML = obj.temporaryplayerdeaths5;
  document.getElementById("pk6").innerHTML = obj.temporaryplayerscore6;
  document.getElementById("pd6").innerHTML = obj.temporaryplayerdeaths6;
  document.getElementById("pk7").innerHTML = obj.temporaryplayerscore7;
  document.getElementById("pd7").innerHTML = obj.temporaryplayerdeaths7;
  document.getElementById("pk8").innerHTML = obj.temporaryplayerscore8;
  document.getElementById("pd8").innerHTML = obj.temporaryplayerdeaths8;
  document.getElementById("pk9").innerHTML = obj.temporaryplayerscore9;
  document.getElementById("pd9").innerHTML = obj.temporaryplayerdeaths9;
  document.getElementById("pk10").innerHTML = obj.temporaryplayerscore10;
  document.getElementById("pd10").innerHTML = obj.temporaryplayerdeaths10;
  document.getElementById("pk11").innerHTML = obj.temporaryplayerscore11;
  document.getElementById("pd11").innerHTML = obj.temporaryplayerdeaths11;
  document.getElementById("pk12").innerHTML = obj.temporaryplayerscore12;
  document.getElementById("pd12").innerHTML = obj.temporaryplayerdeaths12;
  document.getElementById("pk13").innerHTML = obj.temporaryplayerscore13;
  document.getElementById("pd13").innerHTML = obj.temporaryplayerdeaths13;
  document.getElementById("pk14").innerHTML = obj.temporaryplayerscore14;
  document.getElementById("pd14").innerHTML = obj.temporaryplayerdeaths14;
  document.getElementById("pk15").innerHTML = obj.temporaryplayerscore15;
  document.getElementById("pd15").innerHTML = obj.temporaryplayerdeaths15;
  document.getElementById("pk16").innerHTML = obj.temporaryplayerscore16;
  document.getElementById("pd16").innerHTML = obj.temporaryplayerdeaths16;
  document.getElementById("pk17").innerHTML = obj.temporaryplayerscore17;
  document.getElementById("pd17").innerHTML = obj.temporaryplayerdeaths17;
  document.getElementById("pk18").innerHTML = obj.temporaryplayerscore18;
  document.getElementById("pd18").innerHTML = obj.temporaryplayerdeaths18;
  document.getElementById("pk19").innerHTML = obj.temporaryplayerscore19;
  document.getElementById("pd19").innerHTML = obj.temporaryplayerdeaths19;
  document.getElementById("pk20").innerHTML = obj.temporaryplayerscore20;
  document.getElementById("pd20").innerHTML = obj.temporaryplayerdeaths20;
  document.getElementById("pk21").innerHTML = obj.temporaryplayerscore21;
  document.getElementById("pd21").innerHTML = obj.temporaryplayerdeaths21;
  document.getElementById("pk22").innerHTML = obj.temporaryplayerscore22;
  document.getElementById("pd22").innerHTML = obj.temporaryplayerdeaths22;
  document.getElementById("pk23").innerHTML = obj.temporaryplayerscore23;
  document.getElementById("pd23").innerHTML = obj.temporaryplayerdeaths23;
  document.getElementById("pk24").innerHTML = obj.temporaryplayerscore24;
  document.getElementById("pd24").innerHTML = obj.temporaryplayerdeaths24;
  document.getElementById("pk25").innerHTML = obj.temporaryplayerscore25;
  document.getElementById("pd25").innerHTML = obj.temporaryplayerdeaths25;
  document.getElementById("pk26").innerHTML = obj.temporaryplayerscore26;
  document.getElementById("pd26").innerHTML = obj.temporaryplayerdeaths26;
  document.getElementById("pk27").innerHTML = obj.temporaryplayerscore27;
  document.getElementById("pd27").innerHTML = obj.temporaryplayerdeaths27;
  document.getElementById("pk28").innerHTML = obj.temporaryplayerscore28;
  document.getElementById("pd28").innerHTML = obj.temporaryplayerdeaths28;
  document.getElementById("pk29").innerHTML = obj.temporaryplayerscore29;
  document.getElementById("pd29").innerHTML = obj.temporaryplayerdeaths29;
  document.getElementById("pk30").innerHTML = obj.temporaryplayerscore30;
  document.getElementById("pd30").innerHTML = obj.temporaryplayerdeaths30;
  document.getElementById("pk31").innerHTML = obj.temporaryplayerscore31;
  document.getElementById("pd31").innerHTML = obj.temporaryplayerdeaths31;
  document.getElementById("pk32").innerHTML = obj.temporaryplayerscore32;
  document.getElementById("pd32").innerHTML = obj.temporaryplayerdeaths32;
  document.getElementById("pk33").innerHTML = obj.temporaryplayerscore33;
  document.getElementById("pd33").innerHTML = obj.temporaryplayerdeaths33;
  document.getElementById("pk34").innerHTML = obj.temporaryplayerscore34;
  document.getElementById("pd34").innerHTML = obj.temporaryplayerdeaths34;
  document.getElementById("pk35").innerHTML = obj.temporaryplayerscore35;
  document.getElementById("pd35").innerHTML = obj.temporaryplayerdeaths35;
  document.getElementById("pk36").innerHTML = obj.temporaryplayerscore36;
  document.getElementById("pk36").innerHTML = obj.temporaryplayerscore36;
  document.getElementById("pd36").innerHTML = obj.temporaryplayerdeaths36;
  document.getElementById("pk37").innerHTML = obj.temporaryplayerscore37;
  document.getElementById("pd37").innerHTML = obj.temporaryplayerdeaths37;
  document.getElementById("pk38").innerHTML = obj.temporaryplayerscore38;
  document.getElementById("pd38").innerHTML = obj.temporaryplayerdeaths38;
  document.getElementById("pk39").innerHTML = obj.temporaryplayerscore39;
  document.getElementById("pd39").innerHTML = obj.temporaryplayerdeaths39;
  document.getElementById("pk40").innerHTML = obj.temporaryplayerscore40;
  document.getElementById("pd40").innerHTML = obj.temporaryplayerdeaths40;
  document.getElementById("pk41").innerHTML = obj.temporaryplayerscore41;
  document.getElementById("pd41").innerHTML = obj.temporaryplayerdeaths41;
  document.getElementById("pk42").innerHTML = obj.temporaryplayerscore42;
  document.getElementById("pd42").innerHTML = obj.temporaryplayerdeaths42;
  document.getElementById("pk43").innerHTML = obj.temporaryplayerscore43;
  document.getElementById("pd43").innerHTML = obj.temporaryplayerdeaths43;
  document.getElementById("pk44").innerHTML = obj.temporaryplayerscore44;
  document.getElementById("pd44").innerHTML = obj.temporaryplayerdeaths44;
  document.getElementById("pk45").innerHTML = obj.temporaryplayerscore45;
  document.getElementById("pd45").innerHTML = obj.temporaryplayerdeaths45;
  document.getElementById("pk46").innerHTML = obj.temporaryplayerscore46;
  document.getElementById("pd46").innerHTML = obj.temporaryplayerdeaths46;
  document.getElementById("pk47").innerHTML = obj.temporaryplayerscore47;
  document.getElementById("pd47").innerHTML = obj.temporaryplayerdeaths47;
  document.getElementById("pk48").innerHTML = obj.temporaryplayerscore48;
  document.getElementById("pd48").innerHTML = obj.temporaryplayerdeaths48;
  document.getElementById("pk49").innerHTML = obj.temporaryplayerscore49;
  document.getElementById("pd49").innerHTML = obj.temporaryplayerdeaths49;
  document.getElementById("pk50").innerHTML = obj.temporaryplayerscore50;
  document.getElementById("pd50").innerHTML = obj.temporaryplayerdeaths50;
  document.getElementById("pk51").innerHTML = obj.temporaryplayerscore51;
  document.getElementById("pd51").innerHTML = obj.temporaryplayerdeaths51;
  document.getElementById("pk52").innerHTML = obj.temporaryplayerscore52;
  document.getElementById("pd52").innerHTML = obj.temporaryplayerdeaths52;
  document.getElementById("pk53").innerHTML = obj.temporaryplayerscore53;
  document.getElementById("pd53").innerHTML = obj.temporaryplayerdeaths53;
  document.getElementById("pk54").innerHTML = obj.temporaryplayerscore54;
  document.getElementById("pd54").innerHTML = obj.temporaryplayerdeaths54;
  document.getElementById("pk55").innerHTML = obj.temporaryplayerscore55;
  document.getElementById("pd55").innerHTML = obj.temporaryplayerdeaths55;
  document.getElementById("pk56").innerHTML = obj.temporaryplayerscore56;
  document.getElementById("pd56").innerHTML = obj.temporaryplayerdeaths56;
  document.getElementById("pk57").innerHTML = obj.temporaryplayerscore57;
  document.getElementById("pd57").innerHTML = obj.temporaryplayerdeaths57;
  document.getElementById("pk58").innerHTML = obj.temporaryplayerscore58;
  document.getElementById("pd58").innerHTML = obj.temporaryplayerdeaths58;
  document.getElementById("pk59").innerHTML = obj.temporaryplayerscore59;
  document.getElementById("pd59").innerHTML = obj.temporaryplayerdeaths59;
  document.getElementById("pk60").innerHTML = obj.temporaryplayerscore60;
  document.getElementById("pd60").innerHTML = obj.temporaryplayerdeaths60;
  document.getElementById("pk61").innerHTML = obj.temporaryplayerscore61;
  document.getElementById("pd61").innerHTML = obj.temporaryplayerdeaths61;
  document.getElementById("pk62").innerHTML = obj.temporaryplayerscore62;
  document.getElementById("pd62").innerHTML = obj.temporaryplayerdeaths62;
  document.getElementById("pk63").innerHTML = obj.temporaryplayerscore63;
  document.getElementById("pd63").innerHTML = obj.temporaryplayerdeaths63;

  document.getElementById("MK0").innerHTML = obj.KillsLeader0;
  document.getElementById("MK10").innerHTML = obj.Leader0Kills;
  document.getElementById("MK1").innerHTML = obj.KillsLeader1;
  document.getElementById("MK11").innerHTML = obj.Leader1Kills;
  document.getElementById("MK2").innerHTML = obj.KillsLeader2;
  document.getElementById("MK12").innerHTML = obj.Leader2Kills;

  document.getElementById("MO0").innerHTML = obj.ObjectiveLeader0;
  document.getElementById("MO10").innerHTML = obj.Leader0Objectives;
  document.getElementById("MO1").innerHTML = obj.ObjectiveLeader1;
  document.getElementById("MO11").innerHTML = obj.Leader1Objectives;
  document.getElementById("MO2").innerHTML = obj.ObjectiveLeader2;
  document.getElementById("MO12").innerHTML = obj.Leader2Objectives;

  document.getElementById("MD0").innerHTML = obj.DeathsLeader0;
  document.getElementById("MD10").innerHTML = obj.Leader0Deaths;
  document.getElementById("MD1").innerHTML = obj.DeathsLeader1;
  document.getElementById("MD11").innerHTML = obj.Leader1Deaths;
  document.getElementById("MD2").innerHTML = obj.DeathsLeader2;
  document.getElementById("MD12").innerHTML = obj.Leader2Deaths;
 }, false);
}
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
  }
  
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('syncscores').addEventListener('click', toggle9);    
  }
  function toggle9(){
    websocket.send('toggle9');
  }


</script>
</body>
</html>
)rawliteral";

void notifyClients() {
  ws.textAll(String(ledState));
}
void notifyClients1() {
  ws.textAll(String(WebSocketData));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "togglepbblock") == 0) {
      Serial.println("unlock taggers to change teams");
      broadcast("36,65,0,1,0,3,1,0,1,42");
    }
    if (strcmp((char*)data, "togglehost") == 0) {
      Serial.println("host taggers");
      broadcast("36,97,17,3,0,0,0,0,0,42");
    }
    if (strcmp((char*)data, "togglepbstart") == 0) {
      Serial.println("start game");
      SendStartBeacon = "36,65,1," + String(SettingsLighting) + "," + String(SettingsRespawn) + ",1," + String(SettingsGameTime) + "," + String(SettingsGameMode) + "," + String(SettingsLives) + ",42";
      ConfirmBeacon = "36,97,17,1," + String(SettingsLighting) + "," + String(SettingsRespawn) + "," + String(SettingsGameTime) + "," + String(SettingsGameMode) + "," + String(SettingsLives) + ",42";
      byte i = 0;
      // reset in game scores
      while (i < 4) {
        TeamGameScore[i] = 0;
        i++;
        vTaskDelay(0);
      }
      i = 0;
      while (i < 64) {
        PlayerGameScore[i] = 0;
        PlayerDeaths[i] = 0;
        i++;
        vTaskDelay(0);
      }
      i = 0;
      while (i < 21) {
        JboxInPlay[i] = 0;
        i++;
        vTaskDelay(0);
      }
      broadcast(SendStartBeacon);
      broadcast(SendStartBeacon);
      INGAME = true;
    }      
    if (strcmp((char*)data, "togglepbend") == 0) {
      EndGame();
    }
    if (strcmp((char*)data, "0") == 0) {
      Serial.println("indoor mode");
      SettingsLighting = 1;
    }
    if (strcmp((char*)data, "1") == 0) {
      Serial.println("outdoor mode");
      SettingsLighting = 0;
    }
    if (strcmp((char*)data, "2") == 0) {
      Serial.println("team battle");
      SettingsGameMode  = 0;
    }
    if (strcmp((char*)data, "3") == 0) {
      Serial.println("battle royale");
      SettingsGameMode = 1;
    }
    if (strcmp((char*)data, "4") == 0) {
      Serial.println("lives low");
      SettingsLives = 0;
    }
    if (strcmp((char*)data, "5") == 0) {
      Serial.println("lives high");
      SettingsLives = 1;
    }
    if (strcmp((char*)data, "6") == 0) {
      Serial.println("game time on");
      SettingsGameTime = 1;
    }
    if (strcmp((char*)data, "7") == 0) {
      Serial.println("game time off");
      SettingsGameTime = 0;
    }
    if (strcmp((char*)data, "8") == 0) {
      Serial.println("auto respawn");
      SettingsRespawn = 0;
    }
    if (strcmp((char*)data, "9") == 0) {
      Serial.println("manual respawn");
      SettingsRespawn = 1;
    }
    if (strcmp((char*)data, "10") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 10;
    }
    if (strcmp((char*)data, "11") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 11;
    }
    if (strcmp((char*)data, "12") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 12;
    }
    if (strcmp((char*)data, "13") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 13;
    }
    if (strcmp((char*)data, "14") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 14;
    }
    if (strcmp((char*)data, "15") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 15;
    }
    if (strcmp((char*)data, "16") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 16;
    }
    if (strcmp((char*)data, "17") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 17;
    }
    if (strcmp((char*)data, "18") == 0) {
      Serial.println("Changed Objective Mode");
      SettingsGameTime = 0;
      SettingsLives = 1;
      SettingsGameMode  = 0;
      ObjectiveMode = 18;
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
}


void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(OTAssid.c_str(), OTApassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    vTaskDelay(250);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(2, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      UPTODATE = true;
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;} 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      
      return 1;
    }
  } 
  return 0;  
}

//*********************** LORA VARIABLE DELCARATIONS **********************
//*************************************************************************
String LoRaDataReceived; // string used to record data coming in
String tokenStrings[5]; // used to break out the incoming data for checks/balances
String LoRaDataToSend; // used to store data being sent from this device

bool LORALISTEN = false; // a trigger used to enable the radio in (listening)
bool ENABLELORATRANSMIT = false; // a trigger used to enable transmitting data

int sendcounter = 0;
int receivecounter = 0;

unsigned long lorapreviousMillis = 0;
const long lorainterval = 3000;
long previousMillis0 = 0;
//*************************************************************************
//*************************** LORA OBJECTS ********************************
//*************************************************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(Serial1.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    LoRaDataReceived = Serial1.readString(); // stores the incoming data to the pre set string
    Serial.print("LoRaDataReceived: "); // printing data received
    Serial.println(LoRaDataReceived); // printing data received
    if(LoRaDataReceived.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)LoRaDataReceived.c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        tokenStrings[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        }
        // the data is now stored into individual strings under tokenStrings[]
        // print out the individual string arrays separately
        Serial.println("received LoRa communication");
        Serial.print("Token 0: ");
        Serial.println(tokenStrings[0]);
        Serial.print("Token 1: ");
        Serial.println(tokenStrings[1]);
        Serial.print("Token 2: ");
        Serial.println(tokenStrings[2]);
        Serial.print("Token 3: ");
        Serial.println(tokenStrings[3]);
        Serial.print("Token 4: ");
        Serial.println(tokenStrings[4]);
        Serial.print("Token 5: ");
        Serial.println(tokenStrings[5]);
        Serial.print("Token 6: ");
        Serial.println(tokenStrings[6]);
        Serial.print("Token 7: ");
        Serial.println(tokenStrings[7]);
        Serial.print("Token 8: ");
        Serial.println(tokenStrings[8]);
        Serial.print("Token 9: ");
        Serial.println(tokenStrings[9]);
        if (tokenStrings[2] = "CAPTURE") {
          // THIS IS FROM A NODE DOMINATION BASE THAT HAS BEEN CAPTURED
          // check game mode:
            // This base is a master, time to process the data package
            // token 3 is the base id, token 4 is the team id, token 5 is the player id
            int incomingjboxid = tokenStrings[3].toInt();
            incomingjboxid = incomingjboxid - 100;
            JboxPlayerID[incomingjboxid] = tokenStrings[5].toInt();
            JboxTeamID[incomingjboxid] = tokenStrings[4].toInt();
            JboxInPlay[incomingjboxid] = 1;
            MULTIBASEDOMINATION = true;
            Serial.println("processed update from node lora jbox");
        }
        if (tokenStrings[2] = "ASSAULT") {
          // THIS IS FROM AN ASSAULT BASE WHERE A HOSTILE TIME HAS GAINED A POINT
          // token 3 is the base id, token 4 is the team id, token 5 is the player id
          int incomingjboxid = tokenStrings[4].toInt();
          TeamDominationGameScore[incomingjboxid]++; // added one point to the team that just captured a base
          incomingjboxid = tokenStrings[5].toInt();
          PlayerDominationGameScore[incomingjboxid]++;  // added one point to the player that captured that same base
          // now enable the web server to update the score         
          Serial.println("Enable Score Posting");
          UPDATEWEBAPP = true;          
          Serial.println("processed update from node lora jbox");
        }
        receivercounter();
        //TRANSMIT = true; // setting the transmission trigger to enable transmission
      }
    }
}
void EndJBoxGame() {
  String FirstToken = "GAMEOVER"; // 0 for red, 100 for blue, 200 for yellow, 300 for green, example team blue with player 16, 10116
  byte SecondToken= 99; // for directing who it is From
  byte ThirdToken = 99; // What team captured base
  byte FourthToken = 99;  // Player ID
  Serial.println("sending team and player alignment to other jboxes, here is the value being sent: ");
  LoRaDataToSend = FirstToken + "," + String(SecondToken) + "," + String(ThirdToken) + "," + String(FourthToken);
  Serial.println(LoRaDataToSend);
  ENABLELORATRANSMIT = true;
}
void transmissioncounter() {
  sendcounter++;
  Serial.println("Added 1 to sendcounter");
  Serial.println("counter value = "+String(sendcounter));
}
void receivercounter() {
  receivecounter++;
  Serial.println("Added 1 to receivercounter and sent to blynk app");
  Serial.println("counter value = "+String(receivecounter));
}
void TransmitLoRa() {
  // LETS SEND SOMETHING:
  // delay(5000);
  // String LoRaDataToSend = (String(valueA)+String(valueB)+String(valueC)+String(valueD)+String(valueE)); 
  // If needed to send a response to the sender, this function builds the needed string
  Serial.println("Transmitting Via LoRa"); // printing to serial monitor
  Serial1.print("AT+SEND=0,"+String(LoRaDataToSend.length())+","+LoRaDataToSend+"\r\n"); // used to send a data packet
  //  Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //  Serial.println("Sent the '5' to device 0");
  transmissioncounter();
}
void EndGame() {
  Serial.println("eend a game early");
      broadcast("36,69,0,1,42");
      INGAME = false;
      MULTIBASEDOMINATION = false;
      Serial.println("Received a game over winner Announcement");
      // add game scores to overall scores
      byte i = 0;
      // store last game totals
      while (i < 4) {
        TeamOverallScore[i] = TeamOverallScore[i] + TeamGameScore[i];
        i++;
        vTaskDelay(0);
      }
      i = 0;
      while (i < 16) {
        PlayerOverallScore[i] = PlayerOverallScore[i] + PlayerGameScore[i];
        i++;
        vTaskDelay(0);
      }
      EndJBoxGame();
}

void loop0(void *pvParameters) {
  Serial.print("Comms loop running on core ");
  Serial.println(xPortGetCoreID());
  for(;;) { // starts the forever loop 
    // put all the serial activities in here for communication with the brx
    // LoRa Receiver Main Functions:
    ReceiveTransmission();
    if (ENABLELORATRANSMIT) {
      TransmitLoRa();
      ENABLELORATRANSMIT = false;
    }
    delay(1); // this has to be here or it will just continue to restart the esp32
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(ledPin, LOW);

  //Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  int eepromtaggercount = EEPROM.read(0);
  if (eepromtaggercount < 255 && eepromtaggercount < 65) {
    TaggersOwned = eepromtaggercount;
  }
  // setting up eeprom based SSID:
  String esid;
  for (int i = 1; i < 33; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  // Setting up EEPROM Password
  String epass = "";
  for (int i = 34; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  // now updating the OTA credentials to match
  OTAssid = esid;
  OTApassword = epass;
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  initWebSocket();
  
  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  // Route for root / web page1
  server1.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html1, processor);
  });
  
  // json events
    events.onConnect([](AsyncEventSourceClient *client){
      if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      // send event with message "hello!", id current millis
      // and set reconnect delay to 1 second
      client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      OTAssid = inputMessage;
      Serial.println("clearing eeprom");
      for (int i = 1; i < 34; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom ssid:");
      int j = 1;
      for (int i = 0; i < OTAssid.length(); ++i)
      {
        EEPROM.write(j, OTAssid[i]);
        Serial.print("Wrote: ");
        Serial.println(OTAssid[i]);
        j++;
      }
      EEPROM.commit();
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      OTApassword = inputMessage;
      Serial.println("clearing eeprom");
      for (int i = 34; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom Password:");
      int k = 34;
      for (int i = 0; i < OTApassword.length(); ++i)
      {
        EEPROM.write(k, OTApassword[i]);
        Serial.print("Wrote: ");
        Serial.println(OTApassword[i]);
        k++;
      }
      EEPROM.commit();
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      String BLEName = inputMessage;
      //Serial.println("clearing eeprom");
      //for (int i = 34; i < 96; ++i) {
      //  EEPROM.write(i, 0);
      //}
      //Serial.println("writing eeprom Password:");
      //int k = 34;
      //for (int i = 0; i < OTApassword.length(); ++i)
      //{
      //  EEPROM.write(k, OTApassword[i]);
      //  Serial.print("Wrote: ");
      //  Serial.println(OTApassword[i]);
      //  k++;
      //}
      //EEPROM.commit();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    Serial.print("OTA SSID = ");
    Serial.println(OTAssid);
    Serial.print("OTA Password = ");
    Serial.println(OTApassword);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });

  // Start server
  server.begin();
  server1.begin();

  //***********************************************************************
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS=1\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial1.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  Serial1.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
  Serial1.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial.println("LoRa Module Initialized");
  //***********************************************************************
  // initialize dual cores and dual loops
  Serial.print("Apps runninf on core: ");
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop0, "loop0", 10000, NULL, 1, NULL, 0);
}
String incomingserial = "0";
void loop() {
  ws.cleanupClients();
  if (UPDATEWEBAPP) {
    if (WebAppUpdaterProcessCounter < 3) {
      unsigned long UpdaterCurrentMillis = millis();
      if (WebAppUpdaterProcessCounter == 0) {
        UpdateWebApp0();
      }
      if (WebAppUpdaterProcessCounter == 1) {
        UpdateWebApp1();
      }
      if (WebAppUpdaterProcessCounter == 2) {
        UpdateWebApp2();
      }
      WebAppUpdaterProcessCounter++;
    } else {
      UPDATEWEBAPP = false;
      WebAppUpdaterProcessCounter = 0;
    }
    if (ObjectiveMode > 1) {
      if (ObjectiveMode = 10) {
        if (PlayerGameScore[LeadPlayerID] > 9) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 11) {
        if (PlayerGameScore[LeadPlayerID] > 19) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 12) {
        if (TeamGameScore[LeadTeamID] > 24) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 13) {
        if (TeamGameScore[LeadTeamID] > 49) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 14) {
        if (TeamDominationGameScore[0] > 299) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[1] > 299) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[2] > 299) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[3] > 299) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 15) {
        if (TeamDominationGameScore[0] > 599) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[1] > 599) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[2] > 599) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[3] > 599) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 16) {
        if (TeamDominationGameScore[0] > 1199) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[1] > 1199) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[2] > 1199) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[3] > 1199) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 17) {
        if (TeamDominationGameScore[0] > 0) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[1] > 0) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[2] > 0) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[3] > 0) {
          // end game
          EndGame();
        }
      }
      if (ObjectiveMode = 18) {
        if (TeamDominationGameScore[0] > 4) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[1] > 4) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[2] > 4) {
          // end game
          EndGame();
        }
        if (TeamDominationGameScore[3] > 4) {
          // end game
          EndGame();
        }
      }
    }
  }
  if (MULTIBASEDOMINATION) {
    long currentMillis0 = millis();
    if (currentMillis0 - previousMillis0 > 1000) {  // will store last time LED was updated
      // NEED TO APPLY POINTS FOR OTHER DOMINATION BOXES POSESSIONS
      previousMillis0 = currentMillis0;
      int processcounter = 0;
      while (processcounter < 21) {
        if (JboxInPlay[processcounter] == 1) {
          PlayerDominationGameScore[JboxPlayerID[processcounter]]++;
          TeamDominationGameScore[JboxTeamID[processcounter]]++;
          Serial.println("added a point for a multi base domination JBOX");
        }
        processcounter++;
      }
      UPDATEWEBAPP = true;
    }
  }
}
// end of loop

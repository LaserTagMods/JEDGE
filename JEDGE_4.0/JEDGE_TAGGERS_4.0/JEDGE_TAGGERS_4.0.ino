/*
 * updates log
 * 
 * 7/11 added in ability to send wifi credentials to tagger for updating.
 * 7/12 added in ability to assign gun generation to esp32 for flash memory
 * 8/4  added in offline game mode settings
 * 8/14 fixed offline game mode bugs
 */

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

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 100 // use 0 for OTA and 1 for Tagger ID
// EEPROM Definitions:
// EEPROM 0 is used for number of taggers

int GunID = 99; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
int GunGeneration = 2; // change to gen 1, 2, 3
int TaggersOwned = 64; // how many taggers do you own or will play?

// Replace with your network credentials
const char* ssid = "JEDGECONTROLLER";
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
int updatenotification;
//unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long otainterval = 1000;
const long mini_interval = 1000;
String FirmwareVer = {"4.4"};

// text inputs
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";

int WebSocketData;
bool ledState = 0;
const int ledPin = 2;
int Menu[25]; // used for menu settings storage

unsigned long currentmilliseconds = 0;
unsigned long lasttempcheck = 0;
int tempcheckinterval = 30000;
int espnowpushinterval = 30000;
unsigned long lastespnowpush = 0;

// internal temp check
 #ifdef __cplusplus
  extern "C" {
 #endif

  uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

int id = 0;
//int tempteamscore;
//int tempplayerscore;
int teamscore[4];
int playerscore[64];
int pid = 0;

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
char *ptr = NULL; // used for initializing and ending string break up.
int Team=0; // team selection used when allowed for custom configuration

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
unsigned long ledpreviousMillis = 0;  // will store last time LED was updated
const long ledinterval = 1500;  // interval at which to blink (milliseconds)

bool DEMOTOGGLE = false;
bool DEMOMODE = false; // used to run demo mode
bool SCORESYNC = false; // used for initializing score syncronization
int ScoreRequestCounter = 0; // counter used for score syncronization
String ScoreTokenStrings[73]; // used for disecting incoming score string/char
int PlayerDeaths[64]; // used for score accumilation
int PlayerKills[64]; // used for score accumilation
int PlayerObjectives[64]; // used for score accumilation
int TeamKills[4]; // used for score accumilation
int TeamObjectives[4]; // used for score accumilation
int TeamDeaths[4];
int Team0[10];
int Team1[10];
int Team2[10];
int Team3[10];
String ScoreString = "0";
long ScorePreviousMillis = 0;
bool UPDATEWEBAPP = false; // used to update json scores to web app
long WebAppUpdaterTimer = 0;
int WebAppUpdaterProcessCounter = 0;

//*****************************************************************************************
// ESP Now Objects:
//*****************************************************************************************
// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09}; 

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};


// Register peer
esp_now_peer_info_t peerInfo;

// Define variables to store and to be sent
int datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
int datapacket2 = 32700; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
int datapacket3 = GunID; // From - device ID
String datapacket4 = "X"; // used for score reporting only

int lastdatapacket2 = 0; // used for repeat data send


// Define variables to store incoming readings
int incomingData1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases
int incomingData2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
int incomingData3; // From - device ID
String incomingData4; // used for score reporting only

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
    int DP2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
    int DP3; // From - device ID
    char DP4[200]; // used for score reporting
} struct_message;

// Create a struct_message called DataToBroadcast to hold sensor readings
struct_message DataToBroadcast;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// timer settings
unsigned long ESPNOWCurrentMillis = 0;
int ESPNOWinterval = 1000;
long ESPNOWPreviousMillis = 0;

// trigger for activating data broadcast
bool BROADCASTESPNOW = false; 

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}
// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  //digitalWrite(2, HIGH);
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingData1 = incomingReadings.DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  incomingData2 = incomingReadings.DP2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  incomingData3 = incomingReadings.DP3; // From
  incomingData4 = String(incomingReadings.DP4);
  Serial.println("DP1: " + String(incomingData1)); // INTENDED RECIPIENT
  Serial.println("DP2: " + String(incomingData2)); // FUNCTION/COMMAN
  Serial.println("DP3: " + String(incomingData3)); // From - device ID
  Serial.print("DP4: "); // used for scoring
  //Serial.println(incomingData4);
  Serial.write(incomingReadings.DP4);  
  Serial.println(incomingData4);
  //Serial.println;
  ProcessIncomingCommands();
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
}

// object to generate random numbers to send
void getReadings(){
  // Set values to send
  DataToBroadcast.DP1 = datapacket1;
  DataToBroadcast.DP2 = datapacket2;
  DataToBroadcast.DP3 = datapacket3;
  datapacket4.toCharArray(DataToBroadcast.DP4, 200);
}

void ResetReadings() {
  lastdatapacket2 = datapacket2;
  datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  datapacket2 = 32700; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  datapacket3 = GunID; // From - device ID
  datapacket4 = "X";
}

// object for broadcasting the data packets
void BroadcastData() {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &DataToBroadcast, sizeof(DataToBroadcast));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
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
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);  
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;   // this is the channel being used
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}


//****************************************************
// WebServer 
//****************************************************

JSONVar board;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// callback function that will be executed when data is received
void UpdateWebApp0() { 
  id = 0; // team id place holder
  pid = 0; // player id place holder
  while (id < 4) {
    //TeamKills[id] = random(400);
    //board["id"] = id;
    board["temporaryteamscore"+String(id)] = TeamKills[id];
    board["temporaryteamdeaths"+String(id)] = TeamDeaths[id];
    board["temporaryteamobjectives"+String(id)] = TeamObjectives[id];
    //String jsonString = JSON.stringify(board);
    //Serial.println("team ID: " + String(id));
    //Serial.println("t value: " + String(TeamKills[id]));
    //events.send(jsonString.c_str(), "new_readings", millis());
    id++;
    //delay(1);
  }
  while (pid < TaggersOwned) {
    //PlayerKills[pid] = random(25);
    //board["pid"] = pid;
    board["temporaryplayerscore"+String(pid)] = PlayerKills[pid];
    //board["temporaryplayerdeaths"+String(pid)] = PlayerDeaths[pid];
    //board["temporaryplayerobjectives"+String(pid)] = PlayerObjectives[pid];
    //Serial.println("p value: " + String(PlayerKills[pid]));
    //Serial.println("player ID: " + String(pid));
    //String jsonString = JSON.stringify(board);
    //events.send(jsonString.c_str(), "new_readings", millis());
    pid++;
    //delay(1);
  }
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
}
void UpdateWebApp1() { 
  pid = 0; // player id place holder
  while (pid < TaggersOwned) {
    //PlayerKills[pid] = random(25);
    //board["pid"] = pid;
    //board["temporaryplayerscore"+String(pid)] = PlayerKills[pid];
    board["temporaryplayerdeaths"+String(pid)] = PlayerDeaths[pid];
    //board["temporaryplayerobjectives"+String(pid)] = PlayerObjectives[pid];
    //Serial.println("p value: " + String(PlayerKills[pid]));
    //Serial.println("player ID: " + String(pid));
    //String jsonString = JSON.stringify(board);
    //events.send(jsonString.c_str(), "new_readings", millis());
    pid++;
    //delay(1);
  }
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
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
      if (PlayerKills[k] > highest) {
        highest = PlayerKills[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerKills[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerKills[k] > highest) {
        highest = PlayerKills[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerKills[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerKills[k] > highest) {
        highest = PlayerKills[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      KillsLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerKills[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      Serial.println("Most Deadliest, Players with the most kills:");
      Serial.println("1st place: Player " + String(KillsLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(KillsLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(KillsLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();
      board["KillsLeader0"] = KillsLeader[0];
      board["Leader0Kills"] = LeaderScore[0];
      board["KillsLeader1"] = KillsLeader[1];
      board["Leader1Kills"] = LeaderScore[1];
      board["KillsLeader2"] = KillsLeader[2];
      board["Leader2Kills"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerKills[KillsLeader[0]] = LeaderScore[0];
      PlayerKills[KillsLeader[1]] = LeaderScore[1];
      PlayerKills[KillsLeader[2]] = LeaderScore[2];

      // Now Leader for Objectives:
      kmax=0; highest=0;
      for (int k=0; k<64; k++)
      if (PlayerObjectives[k] > highest) {
        highest = PlayerObjectives[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerObjectives[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerObjectives[k] > highest) {
        highest = PlayerObjectives[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerObjectives[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerObjectives[k] > highest) {
        highest = PlayerObjectives[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerObjectives[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      Serial.println("The Dominator, Players with the most objective points:");
      Serial.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();      
      board["ObjectiveLeader0"] = ObjectiveLeader[0];
      board["Leader0Objectives"] = LeaderScore[0];
      board["ObjectiveLeader1"] = ObjectiveLeader[1];
      board["Leader1Objectives"] = LeaderScore[1];
      board["ObjectiveLeader2"] = ObjectiveLeader[2];
      board["Leader2Objectives"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerObjectives[ObjectiveLeader[0]] = LeaderScore[0];
      PlayerObjectives[ObjectiveLeader[1]] = LeaderScore[1];
      PlayerObjectives[ObjectiveLeader[2]] = LeaderScore[2];

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
      board["DeathsLeader0"] = DeathsLeader[0];
      board["Leader0Deaths"] = LeaderScore[0];
      board["DeathsLeader1"] = DeathsLeader[1];
      board["Leader1Deaths"] = LeaderScore[1];
      board["DeathsLeader2"] = DeathsLeader[2];
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
  <div class="topnav">
    <h1>JEDGE 4.0 Controller</h1>
  </div>
  <div class="content">
    <div class="card">
      <p><button id="pbblock" class="button">Lockout Players</button></p>
      <h2>Tagger Volume</h2>
      <p><select name="adjustvolume" id="adjustvolumeid">
        <option value="1301">1</option>
        <option value="1302">2</option>
        <option value="1303">3</option>
        <option value="1304">4</option>
        <option value="1305">5</option>
        </select>
      </p>
      <h2>Indoor/Outdoor</h2>
      <p><select name="pbindoor" id="pbindoorid">
        <option value="2141">Outdoor Mode</option>
        <option value="2142">Indoor Mode</option>
        <option value="2151">Outdoor Stealth</option>
        <option value="2152">Indoor Stealth</option>
        </select>
      </p>
      <h2>Select A Game</h2>
      <p><select name="pbgame" id="pbgameid">
        <option value="2100">Free For All</option>
        <option value="2101">Death Match</option>
        <option value="2102">Generals</option>
        <option value="2103">Supremacy</option>
        <option value="2104">Commanders</option>
        <option value="2105">Survival</option>
        <option value="2106">The Swarm</option>
        </select>
      </p>
      <h2>Game Customizations</h2>
      <p><select name="presetgamemodes" id="presetgamemodesid">
        <option value="2144">None</option>
        <option value="2145">Battle Royale</option>
        <option value="2146">Team Assimilation</option>
        <option value="2147">Gun Game</option>
        <option value="2148">Contra</option>
        <option value="2149">StarWars</option>
        <option value="2150">Halo</option>
        </select>
      </p>
      <h2>Lives</h2>
      <p><select name="pblives" id="pblivesid">
        <option value="2123">1 Life</option>
        <option value="2124">3 Lives</option>
        <option value="2125">5 Lives</option>
        <option value="2126">10 Lives</option>
        <option value="2127">15 Lives</option>
        <option value="2128">Infinite</option>
        </select>
      </p>
      <h2>Game Time</h2>
      <p><select name="pbtime" id="pbtimeid">
        <option value="2129">5 Minutes</option>
        <option value="2130">10 Minutes</option>
        <option value="2131">15 Minutes</option>
        <option value="2132">20 Minutes</option>
        <option value="2133">30 Minutes</option>
        <option value="2134">Infinite</option>
        </select>
      </p>
      <h2>Respawn Time</h2>
      <p><select name="pbspawn" id="pbspawnid">
        <option value="2135">Off</option>
        <option value="2136">15 Seconds</option>
        <option value="2137">30 Seconds</option>
        <option value="2138">60 Seconds</option>
        <option value="2139">Ramp 45 Minutes</option>
        <option value="2140">Ramp 90</option>
        </select>
      </p>
      <h2>Select A Team/Faction</h2>
      <p><select name="pbteam" id="pbteamid">
        <option value="2107">Alpha - Resistance - Human</option>
        <option value="2108">Bravo - Nexus</option>
        <option value="2109">Delta - Vanguard - Infected</option>
        <option value="2143">Charlie</option>
        <option value="502">Two Teams (Odds Vs Evens)</option>
        <option value="503">Three Teams (every third)</option>
        <option value="504">Four Teams (every fourth)</option>
        <option value="2153">Randomized - Two Teams</option>
        <option value="2154">Randomized - Three Teams</option>
        <option value="2155">Randomized - Four Teams</option>
        </select>
      </p>
      <h2>Player/Team Selector</h2>
      <p><select name="playerselector" id="playerselectorid">
        <option value="1668">All</option>
        <option value="1664">Red</option>
        <option value="1665">Blue</option>
        <option value="1666">Yellow</option>
        <option value="1667">Green</option>
        <option value="1600">Player 0</option>
        <option value="1601">Player 1</option>
        <option value="1602">Player 2</option>
        <option value="1603">Player 3</option>
        <option value="1604">Player 4</option>
        <option value="1605">Player 5</option>
        <option value="1606">Player 6</option>
        <option value="1607">Player 7</option>
        <option value="1608">Player 8</option>
        <option value="1609">Player 9</option>
        <option value="1610">Player 10</option>
        <option value="1611">Player 11</option>
        <option value="1612">Player 12</option>
        <option value="1613">Player 13</option>
        <option value="1614">Player 14</option>
        <option value="1615">Player 15</option>
        <option value="1616">Player 16</option>
        <option value="1617">Player 17</option>
        <option value="1618">Player 18</option>
        <option value="1619">Player 19</option>
        <option value="1620">Player 20</option>
        <option value="1621">Player 21</option>
        <option value="1622">Player 22</option>
        <option value="1623">Player 23</option>
        <option value="1624">Player 24</option>
        <option value="1625">Player 25</option>
        <option value="1626">Player 26</option>
        <option value="1627">Player 27</option>
        <option value="1628">Player 28</option>
        <option value="1629">Player 29</option>
        <option value="1630">Player 30</option>
        <option value="1631">Player 31</option>
        <option value="1632">Player 32</option>
        <option value="1633">Player 33</option>
        <option value="1634">Player 34</option>
        <option value="1635">Player 35</option>
        <option value="1636">Player 36</option>
        <option value="1637">Player 37</option>
        <option value="1638">Player 38</option>
        <option value="1639">Player 39</option>
        <option value="1640">Player 40</option>
        <option value="1641">Player 41</option>
        <option value="1642">Player 42</option>
        <option value="1643">Player 43</option>
        <option value="1644">Player 44</option>
        <option value="1645">Player 45</option>
        <option value="1646">Player 46</option>
        <option value="1647">Player 47</option>
        <option value="1648">Player 48</option>
        <option value="1649">Player 49</option>
        <option value="1650">Player 50</option>
        <option value="1651">Player 51</option>
        <option value="1652">Player 52</option>
        <option value="1653">Player 53</option>
        <option value="1654">Player 54</option>
        <option value="1655">Player 55</option>
        <option value="1656">Player 56</option>
        <option value="1657">Player 57</option>
        <option value="1658">Player 58</option>
        <option value="1659">Player 59</option>
        <option value="1660">Player 60</option>
        <option value="1661">Player 61</option>
        <option value="1662">Player 62</option>
        <option value="1663">Player 63</option>
        </select>
      </p>
      <h2>Select A Weapon/Class</h2>
      <p><select name="pbweap" id="pbweapid">
        <option value="2110">M4 - Heavy - Sentinel - Wraith</option>
        <option value="2111">SR-100 - Soldier - Maurader - Sniper</option>
        <option value="2112">SMGX3 - Medic - Guardian - Technician</option>
        <option value="2113">Tac-87 - Valkyrie - Grenadier - Viper</option>
        <option value="2114">MG-7 - Mercenary</option>
        <option value="2115">TAR-33 - Commander/General</option>
        <option value="2116">Silenced AR</option>
        </select>
      </p>
      <h2>Select A Perk - General & Deathmatch</h2>
      <p><select name="pbperk" id="pbperkid">
        <option value="2117">Grenade Launcher</option>
        <option value="2118">Medkit</option>
        <option value="2119">Body Armor</option>
        <option value="2120">Extended Mags</option>
        <option value="2121">Concussion Grenades</option>
        <option value="2122">Critical Strike</option>
        </select>
      </p>
      <p><button id="resendlastsetting" class="button">Resend Last Setting</button></p>
      <p><button id="pbstart" class="button">Start Game</button></p>
      <p><button id="pbstarta" class="button">15 Second Start</button></p>
      <p><button id="pbend" class="button">End Game</button></p>
      <p><button id="syncscores" class="button">Sync Scores</button></p>
    </div>
  </div>    
  <div class="stopnav">
    <h3>Score Reporting</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard red">
        <h4>Alpha Team</h4>
        <p><span class="reading">Kills:<span id="tk0"></span><span class="reading"> Deaths:<span id="td0"></span><span class="reading"> Points:<span id="to0"></span></p>
      </div>
      <div class="scard blue">
        <h4>Bravo Team</h4><p>
        <p><span class="reading">Kills:<span id="tk1"></span><span class="reading"> Deaths:<span id="td1"></span><span class="reading"> Points:<span id="to1"></span></p>
      </div>
      <div class="scard yellow">
        <h4>Charlie Team</h4>
        <p><span class="reading">Kills:<span id="tk2"></span><span class="reading"> Deaths:<span id="td2"></span><span class="reading"> Points:<span id="to2"></span></p>
      </div>
      <div class="scard green">
        <h4>Delta Team</h4>
        <p><span class="reading">Kills:<span id="tk3"></span><span class="reading"> Deaths:<span id="td3"></span><span class="reading"> Points:<span id="to3"></span></p>
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
        <h4>Player 0</h4><p><span class="reading">Kills:<span id="pk0"></span><span class="reading"> Deaths:<span id="pd0"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 1</h4><p><span class="reading">Kills:<span id="pk1"></span><span class="reading"> Deaths:<span id="pd1"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 2</h4><p><span class="reading">Kills:<span id="pk2"></span><span class="reading"> Deaths:<span id="pd2"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 3</h4><p><span class="reading">Kills:<span id="pk3"></span><span class="reading"> Deaths:<span id="pd3"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 4</h4><p><span class="reading">Kills:<span id="pk4"></span><span class="reading"> Deaths:<span id="pd4"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 5</h4><p><span class="reading">Kills:<span id="pk5"></span><span class="reading"> Deaths:<span id="pd5"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 6</h4><p><span class="reading">Kills:<span id="pk6"></span><span class="reading"> Deaths:<span id="pd6"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 7</h4><p><span class="reading">Kills:<span id="pk7"></span><span class="reading"> Deaths:<span id="pd7"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 8</h4><p><span class="reading">Kills:<span id="pk8"></span><span class="reading"> Deaths:<span id="pd8"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 9</h4><p><span class="reading">Kills:<span id="pk9"></span><span class="reading"> Deaths:<span id="pd9"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 10</h4><p><span class="reading">Kills:<span id="pk10"></span><span class="reading"> Deaths:<span id="pd10"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 11</h4><p><span class="reading">Kills:<span id="pk11"></span><span class="reading"> Deaths:<span id="pd11"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 12</h4><p><span class="reading">Kills:<span id="pk12"></span><span class="reading"> Deaths:<span id="pd12"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 13</h4><p><span class="reading">Kills:<span id="pk13"></span><span class="reading"> Deaths:<span id="pd13"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 14</h4><p><span class="reading">Kills:<span id="pk14"></span><span class="reading"> Deaths:<span id="pd14"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 15</h4><p><span class="reading">Kills:<span id="pk15"></span><span class="reading"> Deaths:<span id="pd15"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 16</h4><p><span class="reading">Kills:<span id="pk16"></span><span class="reading"> Deaths:<span id="pd16"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 17</h4><p><span class="reading">Kills:<span id="pk17"></span><span class="reading"> Deaths:<span id="pd17"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 18</h4><p><span class="reading">Kills:<span id="pk18"></span><span class="reading"> Deaths:<span id="pd18"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 19</h4><p><span class="reading">Kills:<span id="pk19"></span><span class="reading"> Deaths:<span id="pd19"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 20</h4><p><span class="reading">Kills:<span id="pk20"></span><span class="reading"> Deaths:<span id="pd20"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 21</h4><p><span class="reading">Kills:<span id="pk21"></span><span class="reading"> Deaths:<span id="pd21"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 22</h4><p><span class="reading">Kills:<span id="pk22"></span><span class="reading"> Deaths:<span id="pd22"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 23</h4><p><span class="reading">Kills:<span id="pk23"></span><span class="reading"> Deaths:<span id="pd23"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 24</h4><p><span class="reading">Kills:<span id="pk24"></span><span class="reading"> Deaths:<span id="pd24"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 25</h4><p><span class="reading">Kills:<span id="pk25"></span><span class="reading"> Deaths:<span id="pd25"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 26</h4><p><span class="reading">Kills:<span id="pk26"></span><span class="reading"> Deaths:<span id="pd26"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 27</h4><p><span class="reading">Kills:<span id="pk27"></span><span class="reading"> Deaths:<span id="pd27"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 28</h4><p><span class="reading">Kills:<span id="pk28"></span><span class="reading"> Deaths:<span id="pd28"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 29</h4><p><span class="reading">Kills:<span id="pk29"></span><span class="reading"> Deaths:<span id="pd29"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 30</h4><p><span class="reading">Kills:<span id="pk30"></span><span class="reading"> Deaths:<span id="pd30"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 31</h4><p><span class="reading">Kills:<span id="pk31"></span><span class="reading"> Deaths:<span id="pd31"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 32</h4><p><span class="reading">Kills:<span id="pk32"></span><span class="reading"> Deaths:<span id="pd32"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 33</h4><p><span class="reading">Kills:<span id="pk33"></span><span class="reading"> Deaths:<span id="pd33"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 34</h4><p><span class="reading">Kills:<span id="pk34"></span><span class="reading"> Deaths:<span id="pd34"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 35</h4><p><span class="reading">Kills:<span id="pk35"></span><span class="reading"> Deaths:<span id="pd35"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 36</h4><p><span class="reading">Kills:<span id="pk36"></span><span class="reading"> Deaths:<span id="pd36"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 37</h4><p><span class="reading">Kills:<span id="pk37"></span><span class="reading"> Deaths:<span id="pd37"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 38</h4><p><span class="reading">Kills:<span id="pk38"></span><span class="reading"> Deaths:<span id="pd38"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 39</h4><p><span class="reading">Kills:<span id="pk39"></span><span class="reading"> Deaths:<span id="pd39"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 40</h4><p><span class="reading">Kills:<span id="pk40"></span><span class="reading"> Deaths:<span id="pd40"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 41</h4><p><span class="reading">Kills:<span id="pk41"></span><span class="reading"> Deaths:<span id="pd41"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 42</h4><p><span class="reading">Kills:<span id="pk42"></span><span class="reading"> Deaths:<span id="pd42"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 43</h4><p><span class="reading">Kills:<span id="pk43"></span><span class="reading"> Deaths:<span id="pd43"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 44</h4><p><span class="reading">Kills:<span id="pk44"></span><span class="reading"> Deaths:<span id="pd44"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 45</h4><p><span class="reading">Kills:<span id="pk45"></span><span class="reading"> Deaths:<span id="pd45"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 46</h4><p><span class="reading">Kills:<span id="pk46"></span><span class="reading"> Deaths:<span id="pd46"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 47</h4><p><span class="reading">Kills:<span id="pk47"></span><span class="reading"> Deaths:<span id="pd47"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 48</h4><p><span class="reading">Kills:<span id="pk48"></span><span class="reading"> Deaths:<span id="pd48"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 49</h4><p><span class="reading">Kills:<span id="pk49"></span><span class="reading"> Deaths:<span id="pd49"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 50</h4><p><span class="reading">Kills:<span id="pk50"></span><span class="reading"> Deaths:<span id="pd50"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 51</h4><p><span class="reading">Kills:<span id="pk51"></span><span class="reading"> Deaths:<span id="pd51"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 52</h4><p><span class="reading">Kills:<span id="pk52"></span><span class="reading"> Deaths:<span id="pd52"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 53</h4><p><span class="reading">Kills:<span id="pk53"></span><span class="reading"> Deaths:<span id="pd53"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 54</h4><p><span class="reading">Kills:<span id="pk54"></span><span class="reading"> Deaths:<span id="pd54"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 55</h4><p><span class="reading">Kills:<span id="pk55"></span><span class="reading"> Deaths:<span id="pd55"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 56</h4><p><span class="reading">Kills:<span id="pk56"></span><span class="reading"> Deaths:<span id="pd56"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 57</h4><p><span class="reading">Kills:<span id="pk57"></span><span class="reading"> Deaths:<span id="pd57"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 58</h4><p><span class="reading">Kills:<span id="pk58"></span><span class="reading"> Deaths:<span id="pd58"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 59</h4><p><span class="reading">Kills:<span id="pk59"></span><span class="reading"> Deaths:<span id="pd59"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 60</h4><p><span class="reading">Kills:<span id="pk60"></span><span class="reading"> Deaths:<span id="pd60"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 61</h4><p><span class="reading">Kills:<span id="pk61"></span><span class="reading"> Deaths:<span id="pd61"></span></p>
      </div>      
      <div class="scard black">
        <h4>Player 62</h4><p><span class="reading">Kills:<span id="pk62"></span><span class="reading"> Deaths:<span id="pd62"></span></p>
      </div>
      <div class="scard black">
        <h4>Player 63</h4><p><span class="reading">Kills:<span id="pk63"></span><span class="reading"> Deaths:<span id="pd63"></span></p>
      </div>
    </div>
  </div>
  <div class="stopnav">
    <h3>Debug Controls</h3>
  </div>
  <div class="content">
    <div class="card">
      <h2>Player ID Assignment</h2>
      <p><select name="playerassignment" id="playerassignmentid">
        <option value="1965">Select Player ID 0</option>
        <option value="1900">Player 0</option>
        <option value="1901">Player 1</option>
        <option value="1902">Player 2</option>
        <option value="1903">Player 3</option>
        <option value="1904">Player 4</option>
        <option value="1905">Player 5</option>
        <option value="1906">Player 6</option>
        <option value="1907">Player 7</option>
        <option value="1908">Player 8</option>
        <option value="1909">Player 9</option>
        <option value="1910">Player 10</option>
        <option value="1911">Player 11</option>
        <option value="1912">Player 12</option>
        <option value="1913">Player 13</option>
        <option value="1914">Player 14</option>
        <option value="1915">Player 15</option>
        <option value="1916">Player 16</option>
        <option value="1917">Player 17</option>
        <option value="1918">Player 18</option>
        <option value="1919">Player 19</option>
        <option value="1920">Player 20</option>
        <option value="1921">Player 21</option>
        <option value="1922">Player 22</option>
        <option value="1923">Player 23</option>
        <option value="1924">Player 24</option>
        <option value="1925">Player 25</option>
        <option value="1926">Player 26</option>
        <option value="1927">Player 27</option>
        <option value="1928">Player 28</option>
        <option value="1929">Player 29</option>
        <option value="1930">Player 30</option>
        <option value="1931">Player 31</option>
        <option value="1932">Player 32</option>
        <option value="1933">Player 33</option>
        <option value="1934">Player 34</option>
        <option value="1935">Player 35</option>
        <option value="1936">Player 36</option>
        <option value="1937">Player 37</option>
        <option value="1938">Player 38</option>
        <option value="1939">Player 39</option>
        <option value="1940">Player 40</option>
        <option value="1941">Player 41</option>
        <option value="1942">Player 42</option>
        <option value="1943">Player 43</option>
        <option value="1944">Player 44</option>
        <option value="1945">Player 45</option>
        <option value="1946">Player 46</option>
        <option value="1947">Player 47</option>
        <option value="1948">Player 48</option>
        <option value="1949">Player 49</option>
        <option value="1950">Player 50</option>
        <option value="1951">Player 51</option>
        <option value="1952">Player 52</option>
        <option value="1953">Player 53</option>
        <option value="1954">Player 54</option>
        <option value="1955">Player 55</option>
        <option value="1956">Player 56</option>
        <option value="1957">Player 57</option>
        <option value="1958">Player 58</option>
        <option value="1959">Player 59</option>
        <option value="1960">Player 60</option>
        <option value="1961">Player 61</option>
        <option value="1962">Player 62</option>
        <option value="1963">Player 63</option>
        </select>
      </p>
      <h2>Taggers Owned</h2>
      <p><select name="playercount" id="playercountid">
        <option value="2065">Select a Count</option>
        <option value="2000">0</option>
        <option value="2001">1</option>
        <option value="2002">2</option>
        <option value="2003">3</option>
        <option value="2004">4</option>
        <option value="2005">5</option>
        <option value="2006">6</option>
        <option value="2007">7</option>
        <option value="2008">8</option>
        <option value="2009">9</option>
        <option value="2010">10</option>
        <option value="2011">11</option>
        <option value="2012">12</option>
        <option value="2013">13</option>
        <option value="2014">14</option>
        <option value="2015">15</option>
        <option value="2016">16</option>
        <option value="2017">17</option>
        <option value="2018">18</option>
        <option value="2020">20</option>
        <option value="2020">20</option>
        <option value="2021">21</option>
        <option value="2022">22</option>
        <option value="2023">23</option>
        <option value="2024">24</option>
        <option value="2025">25</option>
        <option value="2026">26</option>
        <option value="2027">27</option>
        <option value="2028">28</option>
        <option value="2029">29</option>
        <option value="2030">30</option>
        <option value="2031">31</option>
        <option value="2032">32</option>
        <option value="2033">33</option>
        <option value="2034">34</option>
        <option value="2035">35</option>
        <option value="2036">36</option>
        <option value="2037">37</option>
        <option value="2038">38</option>
        <option value="2039">39</option>
        <option value="2040">40</option>
        <option value="2041">41</option>
        <option value="2042">42</option>
        <option value="2043">43</option>
        <option value="2044">44</option>
        <option value="2045">45</option>
        <option value="2046">46</option>
        <option value="2047">47</option>
        <option value="2048">48</option>
        <option value="2049">49</option>
        <option value="2050">50</option>
        <option value="2051">51</option>
        <option value="2052">52</option>
        <option value="2053">53</option>
        <option value="2054">54</option>
        <option value="2055">55</option>
        <option value="2056">56</option>
        <option value="2057">57</option>
        <option value="2058">58</option>
        <option value="2059">59</option>
        <option value="2060">60</option>
        <option value="2061">61</option>
        <option value="2062">62</option>
        <option value="2063">63</option>
        </select>
      </p>
      <h2>Tagger Gen</h2>
      <p><select name="genassignment" id="genassignmentid">
        <option value="1899">Select Gun Generation</option>
        <option value="1971">Gen 1</option>
        <option value="1972">Gen 2</option>
        <option value="1973">Gen 3</option>
        </select>
      </p>
      <form action="/get">
        wifi ssid: <input type="text" name="input1">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        wifi pass: <input type="text" name="input2">
        <input type="submit" value="Submit">
      </form><br>
      <p><button id="initializeotaupdate" class="button">Enable Tagger OTA</button></p>
      <p><button id="initializecontrollerupdate" class="button">Enable Controller OTA</button></p>
      <p><button id="resetcontrollers" class="button">Reset Controllers</button></p>
      <p><button id="rundemo" class="button">Demo Mode</button></p>
      <p><button id="taggerunlocks" class="button">Unlock Hidden Menu Options</button></p>
      
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
  document.getElementById("td0").innerHTML = obj.temporaryteamdeaths0;
  document.getElementById("to0").innerHTML = obj.temporaryteamobjectives0;
  document.getElementById("tk1").innerHTML = obj.temporaryteamscore1;
  document.getElementById("td1").innerHTML = obj.temporaryteamdeaths1;
  document.getElementById("to1").innerHTML = obj.temporaryteamobjectives1;
  document.getElementById("tk2").innerHTML = obj.temporaryteamscore2;
  document.getElementById("td2").innerHTML = obj.temporaryteamdeaths2;
  document.getElementById("to2").innerHTML = obj.temporaryteamobjectives2;
  document.getElementById("tk3").innerHTML = obj.temporaryteamscore3;  
  document.getElementById("td3").innerHTML = obj.temporaryteamdeaths3;  
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
    document.getElementById('resendlastsetting').addEventListener('click', toggle15A);
    document.getElementById('initializeotaupdate').addEventListener('click', toggle15D);
    document.getElementById('initializecontrollerupdate').addEventListener('click', toggle15E);
    document.getElementById('resetcontrollers').addEventListener('click', toggle15B);
    document.getElementById('rundemo').addEventListener('click', toggle15F);
    document.getElementById('taggerunlocks').addEventListener('click', toggle15C);
    document.getElementById('presetgamemodesid').addEventListener('change', handleGMode, false);
    document.getElementById('adjustvolumeid').addEventListener('change', handlevolumesetting, false);
    document.getElementById('syncscores').addEventListener('click', toggle9);
    document.getElementById('genassignmentid').addEventListener('change', handlegenassignment, false);
    document.getElementById('playerselectorid').addEventListener('change', handleplayerselector, false);
    document.getElementById('playerassignmentid').addEventListener('change', handleplayerassignment, false);
    document.getElementById('playercountid').addEventListener('change', handleplayercount, false);
    document.getElementById('pbgameid').addEventListener('change', handlepbgame, false);
    document.getElementById('pbteamid').addEventListener('change', handlepbteam, false);
    document.getElementById('pbweapid').addEventListener('change', handlepbweap, false);
    document.getElementById('pbperkid').addEventListener('change', handlepbperk, false);
    document.getElementById('pblivesid').addEventListener('change', handlepblives, false);
    document.getElementById('pbtimeid').addEventListener('change', handlepbtime, false);
    document.getElementById('pbspawnid').addEventListener('change', handlepbspawn, false);
    document.getElementById('pbindoorid').addEventListener('change', handlepbindoor, false);
    document.getElementById('pbblock').addEventListener('click', togglepbblock);
    document.getElementById('pbstart').addEventListener('click', togglepbstart);
    document.getElementById('pbstarta').addEventListener('click', togglepbstarta);
    document.getElementById('pbend').addEventListener('click', togglepbend);
    
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
  function togglepbstarta(){
    websocket.send('togglepbstarta');
  }
  function toggle9(){
    websocket.send('toggle9');
  }
  function toggle14s(){
    websocket.send('toggle14s');
  }
  function toggle14e(){
    websocket.send('toggle14e');
  }
  function handlegenassignment() {
    var xq = document.getElementById("genassignmentid").value;
    websocket.send(xq);
  }
  function handleplayerassignment() {
    var xp = document.getElementById("playerassignmentid").value;
    websocket.send(xp);
  }
  function handleplayercount() {
    var xr = document.getElementById("playercountid").value;
    websocket.send(xr);
  }
  function handleplayerperk() {
    var xo = document.getElementById("playerperkid").value;
    websocket.send(xo);
  }
  function handlepbgame() {
    var xs = document.getElementById("pbgameid").value;
    websocket.send(xs);
  }
  function handlepbteam() {
    var xs = document.getElementById("pbteamid").value;
    websocket.send(xs);
  }
  function handlepbweap() {
    var xs = document.getElementById("pbweapid").value;
    websocket.send(xs);
  }
  function handlepbperk() {
    var xs = document.getElementById("pbperkid").value;
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
  function handleplayerselector() {
    var xn = document.getElementById("playerselectorid").value;
    websocket.send(xn);
  }
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handlesecondary() {
    var xb = document.getElementById("secondaryid").value;
    websocket.send(xb);
  }
  function handlemelee() {
    var xc = document.getElementById("meleesettingid").value;
    websocket.send(xc);
  }
  function handleGMode() {
    var xd = document.getElementById("presetgamemodesid").value;
    websocket.send(xd);
  }
  function handlelives() {
    var xe = document.getElementById("playerlivesid").value;
    websocket.send(xe);
  }
  function handlehealth() {
    var xea = document.getElementById("playerhealthid").value;
    websocket.send(xea);
  }
  function handlegametime() {
    var xf = document.getElementById("gametimeid").value;
    websocket.send(xf);
  }
  function handleambience() {
    var xg = document.getElementById("ambienceid").value;
    websocket.send(xg);
  }
  function handleteams() {
    var xg = document.getElementById("teamselectionid").value;
    websocket.send(xg);
  }
  function handlerespawn() {
    var xh = document.getElementById("respawnid").value;
    websocket.send(xh);
  }
  function handlestartimer() {
    var xi = document.getElementById("starttimerid").value;
    websocket.send(xi);
  }
  function handlefriendlyfire() {
    var xj = document.getElementById("friendlyfireid").value;
    websocket.send(xj);
  }
  function handleammo() {
    var xl = document.getElementById("ammoid").value;
    websocket.send(xl);
  }
  function handlevolumesetting() {
    var xm = document.getElementById("adjustvolumeid").value;
    websocket.send(xm);
  }
  function toggle15(){
    websocket.send('toggle15');
  }
  function toggle15A(){
    websocket.send('toggle15A');
  }
  function toggle15D(){
    websocket.send('toggle15D');
  }
  function toggle15E(){
    websocket.send('toggle15E');
  }
  function toggle15F(){
    websocket.send('toggle15F');
  }
  function toggle15B(){
    websocket.send('toggle15B');
  }
  function toggle15C(){
    websocket.send('toggle15C');
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
    if (strcmp((char*)data, "toggle9") == 0) { // score sync
      Menu[9] = 901;
      ledState = !ledState;
      notifyClients();
      Serial.println(" menu = " + String(Menu[9]));
      datapacket2 = 901;
      datapacket1 = 99;
      ClearScores();
      // BROADCASTESPNOW = true;
      SCORESYNC = true;
      Serial.println("Commencing Score Sync Process");
      ScoreRequestCounter = 0;
    }
    if (strcmp((char*)data, "togglepbblock") == 0) { // block players
      Menu[14] = 2197;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "togglepbend") == 0) { // game start
      Menu[14] = 2196;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "togglepbstart") == 0) { // game start
      Menu[14] = 2199;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "togglepbstarta") == 0) { // game start w delay
      Menu[14] = 2198;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle14s") == 0) { // game start
      Menu[14] = 1401;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle14e") == 0) { // game end
      Menu[14] = 1400;
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "1") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "3") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 3;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "4") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 4;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "5") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 5;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "6") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 6;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "7") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 7;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "8") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 8;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "9") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 9;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "10") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 10;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "11") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 11;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "12") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 12;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "13") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 13;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "14") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 14;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "15") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 15;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "16") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 16;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "17") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 17;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "18") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 18;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "19") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 19;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "20") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 20;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "21") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 21;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "101") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 101;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "102") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 102;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "103") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 103;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "104") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 104;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "105") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 105;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "106") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 106;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "107") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 107;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "108") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 108;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "109") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 109;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "110") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 110;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "111") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 111;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "112") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 112;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "113") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 113;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "114") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 114;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "115") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 115;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "116") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 116;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "117") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 117;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "118") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 118;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "119") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 119;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "120") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 120;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "201") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 201;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "202") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 202;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "203") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 203;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "204") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 204;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "205") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 205;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "206") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 206;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "207") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 207;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "210") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 210;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "211") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 211;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "212") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 212;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "301") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 301;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "302") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 302;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "303") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 303;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "304") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 304;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "305") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 305;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "306") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 306;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "307") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 307;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "308") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 308;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "401") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 401;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "402") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 402;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "403") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 403;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "404") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 404;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "501") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 501;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "502") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 502;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "503") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 503;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "504") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 504;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "505") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 505;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "601") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 601;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "602") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 602;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "603") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 603;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "604") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 604;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "605") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 605;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "606") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 606;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "607") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 607;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "608") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 608;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "609") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 609;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "610") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 610;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "611") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 611;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "612") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 612;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "613") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 613;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "614") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[6] = 614;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "701") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 701;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "702") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 702;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "703") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 703;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "704") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 704;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "705") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 705;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "706") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 706;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "707") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 707;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "708") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 708;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "709") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 709;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "801") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 801;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "802") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 802;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "803") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 803;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "804") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 804;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "805") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 805;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "806") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 806;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "807") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 807;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "808") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 808;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "809") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 809;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1101") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1101;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1102") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1102;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1103") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1103;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1200") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1200;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1201") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1201;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1301") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1301;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1302") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1302;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1303") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1303;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1304") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1304;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1305") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1305;
      PrimaryMenu();
    }
    
    if (strcmp((char*)data, "1600") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1600;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1601") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1601;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1602") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1602;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1603") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1603;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1604") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1604;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1605") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1605;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1606") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1606;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1607") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1607;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1608") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1608;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1609") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1609;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1610") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1610;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1611") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1611;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1612") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1612;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1613") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1613;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1614") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1614;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1615") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1615;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1616") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1616;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1617") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1617;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1618") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1618;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1619") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1619;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1620") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1620;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1621") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1621;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1622") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1622;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1623") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1623;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1624") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1624;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1625") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1625;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1626") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1626;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1627") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1627;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1628") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1628;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1629") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1629;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1630") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1630;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1631") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1631;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1632") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1632;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1633") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1633;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1634") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1634;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1635") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1635;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1636") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1636;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1637") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1637;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1638") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1638;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1639") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1639;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1640") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1640;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1641") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1641;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1642") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1642;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1643") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1643;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1644") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1644;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1645") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1645;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1646") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1646;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1647") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1647;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1648") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1648;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1649") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1649;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1650") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1650;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1651") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1651;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1652") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1652;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1653") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1653;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1654") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1654;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1655") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1655;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1656") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1656;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1657") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1657;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1658") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1658;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1659") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1659;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1660") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1660;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1661") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1661;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1662") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1662;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1663") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1663;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1664") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1664;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1665") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1665;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1666") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1666;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1667") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1667;
      PrimaryMenu();
    }

    if (strcmp((char*)data, "1668") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1668;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1700") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1700;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1701") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1701;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1702") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1702;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1800") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1800;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1801") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1801;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1802") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1802;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1803") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1803;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1804") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1804;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1805") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1805;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1806") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1806;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1807") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1807;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1808") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1808;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1900") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1900;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1901") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1901;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1902") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1902;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1903") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1903;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1904") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1904;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1905") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1905;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1906") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1906;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1907") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1907;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1908") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1908;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1909") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1909;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1910") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1910;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1911") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1911;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1912") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1912;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1913") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1913;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1914") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1914;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1915") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1915;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1916") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1916;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1917") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1917;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1918") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1918;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1919") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1919;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1920") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1920;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1921") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1921;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1922") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1922;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1923") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1923;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1924") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1924;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1925") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1925;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1926") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1926;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1927") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1927;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1928") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1928;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1929") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1929;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1930") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1930;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1931") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1931;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1932") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1932;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1933") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1933;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1934") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1934;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1935") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1935;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1936") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1936;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1937") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1937;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1938") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1938;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1939") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1939;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1940") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1940;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1941") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1941;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1942") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1942;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1943") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1943;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1944") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1944;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1945") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1945;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1946") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1946;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1947") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1947;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1948") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1948;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1949") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1949;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1950") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1950;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1951") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1951;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1952") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1952;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1953") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1953;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1954") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1954;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1955") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1955;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1956") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1956;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1957") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1957;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1958") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1958;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1959") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1959;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1960") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1960;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1961") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1961;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1962") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1962;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1963") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1963;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1971") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1971;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1972") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1972;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "1973") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 1973;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2000") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 0);
      TaggersOwned = EEPROM.read(0);      
      EEPROM.commit();
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2001") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 1);      
      EEPROM.commit();
      TaggersOwned = 1;
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2002") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 2);      
      EEPROM.commit();
      TaggersOwned = 2;
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2003") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 3);      
      EEPROM.commit();
      TaggersOwned = 3;
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2004") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 4);      
      EEPROM.commit();
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2005") == 0) {
      ledState = !ledState;   
      EEPROM.write(0, 5);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2006") == 0) {
      ledState = !ledState;     
      EEPROM.write(0, 6);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2007") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 7);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2008") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 8);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2009") == 0) {
      ledState = !ledState;     
      EEPROM.write(0, 9);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2010") == 0) {
      ledState = !ledState;     
      EEPROM.write(0, 10);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2011") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 11);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2012") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 12);       
      EEPROM.commit();     
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2013") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 13);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2014") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 14);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2015") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 15);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2016") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 16);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2017") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 17);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2018") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 18);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2019") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 19);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2020") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 20);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2021") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 21);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2022") == 0) {
      ledState = !ledState;
      EEPROM.write(0, 22);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2023") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 23);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2024") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 24);      
      EEPROM.commit();
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2025") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 25);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2026") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 26);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2027") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 27);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2028") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 28);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2029") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 29);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2030") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 30);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2031") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 31);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2032") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 32);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2033") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 33);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2034") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 34);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2035") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 35);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2036") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 36);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2037") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 37);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2038") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 38);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2039") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 39);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2040") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 40);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2041") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 41);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2042") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 42);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2043") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 43);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2044") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 44);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2045") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 45);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2046") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 46);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2047") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 47);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2048") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 48);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2049") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 49);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2050") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 50);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2051") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 51);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2052") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 52);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2053") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 53);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2054") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 54);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2055") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 55);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2056") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 56);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2057") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 57);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2058") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 58);        
      EEPROM.commit();    
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2059") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 59);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2060") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 60);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2061") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 61);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2062") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 62);      
      EEPROM.commit();      
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
    if (strcmp((char*)data, "2063") == 0) {
      ledState = !ledState;      
      EEPROM.write(0, 63);      
      EEPROM.commit();      
      EEPROM.commit();
      TaggersOwned = EEPROM.read(0);
      Serial.println("Player Count = " + String(TaggersOwned));
    }
        if (strcmp((char*)data, "2100") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2100;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2101") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2101;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2102") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2102;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2103") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2103;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2104") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2104;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2105") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2105;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2106") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2106;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2107") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2107;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2108") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2108;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2109") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2109;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2110") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2110;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2111") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2111;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2112") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2112;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2113") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2113;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2114") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2114;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2115") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2115;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2116") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2116;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2117") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2117;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2118") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2118;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2119") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2119;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2120") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2120;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2121") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2121;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2122") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2122;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2123") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2123;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2124") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2124;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2125") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2125;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2126") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2126;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2127") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2127;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2128") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2128;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2129") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2129;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2130") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2130;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2131") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2131;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2132") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2132;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2133") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2133;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2134") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2134;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2135") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2135;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2136") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2136;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2137") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2137;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2138") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2138;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2139") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2139;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2140") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2140;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2141") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2141;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2142") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2142;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2143") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2143;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2144") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2144;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2145") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2145;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2146") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2146;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2147") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2147;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2148") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2148;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2149") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2149;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2150") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2150;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2151") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2151;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2152") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2152;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2153") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2153;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2154") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2154;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2155") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2155;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2156") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2156;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2157") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2157;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2158") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2158;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2159") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2159;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2160") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2160;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2161") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2161;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2162") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2162;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2163") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2163;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2164") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2164;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2165") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2165;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2166") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2166;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2167") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2167;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2168") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2168;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2169") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2169;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2170") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2170;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2171") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2171;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2172") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2172;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2173") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2173;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2174") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2174;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2175") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2175;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2176") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2176;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2177") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2177;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2178") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2178;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2179") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2179;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2180") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2180;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2181") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2181;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2182") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2182;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2183") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2183;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2184") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2184;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2185") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2185;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2186") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2186;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2187") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2187;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2188") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2188;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2189") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2189;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2190") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2190;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2191") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2191;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2192") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2192;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2193") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2193;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2194") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2194;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2195") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2195;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2196") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2196;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2197") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2197;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2198") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2198;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "2199") == 0) {
      ledState = !ledState;
      notifyClients();
      Menu[0] = 2199;
      PrimaryMenu();
    }
    if (strcmp((char*)data, "toggle15") == 0) { // initialize jedge
      Menu[15] = 1501;
      WebSocketData = Menu[15];
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15A") == 0) { // initialize jedge
      Serial.println("Sending last setting again");
      datapacket2 = lastdatapacket2;
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15B") == 0) { // auto update
      Menu[15] = 1502;
      WebSocketData = Menu[15];
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15C") == 0) { // Unlock Tagger Menus
      ledState = !ledState;
      Serial.println("Unlocking Tagger Menus");
      datapacket2 = 1500;
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15D") == 0) { // auto update
      Menu[15] = 1505;
      WebSocketData = Menu[15];
      ledState = !ledState;
      notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15E") == 0) { // auto update
      Serial.println("Initializing OTA Update of Controller");
      delay(2000);
      connect_wifi();
      OTAMODE = true;
    }
    if (strcmp((char*)data, "toggle15F") == 0) { // auto update
      Serial.println("Initializing Demo Mode");
      ledState = !ledState;
      notifyClients();
      Menu[6] = 601;
      WebSocketData = Menu[6];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      DEMOMODE = true;
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
void ProcessIncomingCommands() { 
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  int IncomingTeam = 99;
  if (incomingData1 > 199) {
    IncomingTeam = incomingData1 - 200;
  }
  if (incomingData1 == GunID || incomingData1 == 99) { // data checked out to be intended for this player ID or for all players
    digitalWrite(2, HIGH);
    
    if (incomingData2 < 1000 && incomingData2 > 900) { // Syncing scores
      int b = incomingData2 - 900;
      if (b == 2) { // this is an incoming score from a player!
        AccumulateIncomingScores();
      }
      if (b == 3) { // this is an incoming score from a JBOX!, was 903
        DominationScores();
        UPDATEWEBAPP = true;
      }
    }
  }
  if (incomingData1 == 9999) { // this is a request for wifi credentials
    datapacket1 = incomingData3;
    datapacket2 = 904;
    datapacket4 = OTAssid;
    getReadings();
    BroadcastData(); // sending data via ESPNOW
    Serial.println("Sent Data Via ESPNOW");
    delay(1000);
    datapacket2 = 905;
    datapacket4 = OTApassword;
    getReadings();
    BroadcastData(); // sending data via ESPNOW
    Serial.println("Sent Data Via ESPNOW");
  }
  digitalWrite(2, LOW);
}

void RequestingScores() {
  datapacket2 = 901;
  datapacket1 = ScoreRequestCounter;
  getReadings();
  BroadcastData(); // sending data via ESPNOW
  Serial.println("Sent Data Via ESPNOW");
  ResetReadings();
  if (ScoreRequestCounter < TaggersOwned) {
    Serial.println("Sent Request for Score to Player "+String(ScoreRequestCounter)+" out of 63");
    ScoreRequestCounter++;
  } else {
    SCORESYNC = false; // disables the score requesting object until a score is reported back from a player
    ScoreRequestCounter = 0;
    Serial.println("All Scores Requested, Closing Score Request Process, Resetting Counter");
    UPDATEWEBAPP = true;
    WebAppUpdaterTimer = millis();
  }
  Serial.print("cOMMS loop running on core ");
  Serial.println(xPortGetCoreID());
}
void ClearScores() {
  int playercounter = 0;
  while (playercounter < 64) {
    PlayerKills[playercounter] = 0;
    PlayerDeaths[playercounter] = 0;
    PlayerObjectives[playercounter] = 0;
    playercounter++;
    delay(1);
  }
  int teamcounter = 0;
  while (teamcounter < 4) {
    TeamKills[teamcounter] = 0;
    TeamObjectives[teamcounter] = 0;
    TeamDeaths[teamcounter] = 0;
    teamcounter++;
  }
  Serial.println("reset all stored scores from previous game, done");
}
void DominationScores() {
    Serial.println(String(millis()));
    ScoreString = incomingData4; // saves incoming data as a temporary string within this object
    Serial.println("printing data received: ");
    Serial.println(ScoreString);
    char *ptr = strtok((char*)ScoreString.c_str(), ","); // looks for commas as breaks to split up the string
    int index = 0;
    while (ptr != NULL)
    {
      ScoreTokenStrings[index] = ptr; // saves the individual characters divided by commas as individual strings
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    // received a string that looks like this: 
    // (Team 0, token 0), (Team 1, token 1), (Team 2, token 2) (Team 3, tokens 3-8) 
    Serial.println("Score Data Recieved from a tagger:");
    int jboxcounter = incomingData3 - 100; // gets what base number we are dealing with
    int count = 0;
    int Data[4];
    while (count<4) {
      Data[count]=ScoreTokenStrings[count].toInt();
      // Serial.println("Converting String character "+String(count)+" to integer: "+String(Data[count]));
      count++;
    }
    Team0[jboxcounter] = Data[0];
    Team1[jboxcounter] = Data[1];
    Team2[jboxcounter] = Data[2];
    Team3[jboxcounter] = Data[3];
    
    int p = 0; // using for data characters 9-72
    int j = 0; // using for player id status counter
    Serial.println("Accumulating Team Objective Points...");
    while (p < 10) {
      j = j + Team0[p];
      p++;
    }
    if (j > TeamObjectives[0]) {
      TeamObjectives[0] = j;
    }
    j = 0;
    p = 0;
    while (p < 10) {
      j = j + Team1[p];
      p++;
    }
    if (j > TeamObjectives[1]) {
      TeamObjectives[1] = j;
    }
    j = 0;
    p = 0;
    while (p < 10) {
      j = j + Team2[p];
      p++;
    }
    if (j > TeamObjectives[2]) {
      TeamObjectives[2] = j;
    }
    j = 0;
    p = 0;
    while (p < 10) {
      j = j + Team3[p];
      p++;
    }
    if (j > TeamObjectives[3]) {
      TeamObjectives[3] = j;
    }
    j = 0;
    p = 0;
    Serial.println(String(millis()));
}
void AccumulateIncomingScores() {
    //Serial.print("cOMMS loop running on core ");
    //Serial.println(xPortGetCoreID());
    Serial.println(String(millis()));
    ScoreString = incomingData4; // saves incoming data as a temporary string within this object
    Serial.println("printing data received: ");
    Serial.println(ScoreString);
    char *ptr = strtok((char*)ScoreString.c_str(), ","); // looks for commas as breaks to split up the string
    int index = 0;
    while (ptr != NULL)
    {
      ScoreTokenStrings[index] = ptr; // saves the individual characters divided by commas as individual strings
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    // received a string that looks like this: 
    // (Player ID, token 0), (Player Team, token 1), (Player Objective Score, token 2) (Team scores, tokens 3-8), (player kill counts, tokens 9-72 
    Serial.println("Score Data Recieved from a tagger:");
  
    int Deaths=0;
    int Objectives=0;
    int Kills=0;
    int Team=0;
    int Player=0;
    int Data[73];
    int count=0;
    while (count<73) {
      Data[count]=ScoreTokenStrings[count].toInt();
      // Serial.println("Converting String character "+String(count)+" to integer: "+String(Data[count]));
      count++;
    }
    Player=Data[0];
    Serial.println("Player ID: " + String(Player));
    Deaths = Data[3] + Data[4] + Data[5] + Data[6]; // added the total team kills to accumulate the number of deaths of this player
    Serial.println("Player Deaths: "+String(Deaths));
    PlayerDeaths[Player] = Deaths; // just converted temporary data to this players death count record
    TeamKills[0] = TeamKills[0] + Data[3]; // accumulating team kill counts
    TeamKills[1] = TeamKills[1] + Data[4];
    TeamKills[2] = TeamKills[2] + Data[5];
    TeamKills[3] = TeamKills[3] + Data[6]; // note, not handling teams 5/6 because they dont actually report/exist
    Serial.println("Team Kill Count Accumulation Complete");
    Serial.println("Red Team Kills: " + String(TeamKills[0]) + "Blue Team Kills: " + String(TeamKills[1]) + "Green Team Kills: " + String(TeamKills[2]) + "Yellow Team Kills: " + String(TeamKills[3]));
    // accumulating player kill counts
    int p = 9; // using for data characters 9-72
    int j = 0; // using for player id status counter
    Serial.println("Accumulating Player kills against current player...");
    while (p < 73) {
      PlayerKills[j] = PlayerKills[j] + Data[p];
      // Serial.println("Player " + String(j) + " Killed this player " + String(Data[p]) + " times, Player's new score is " + String(PlayerKills[j]));
      p++;
      j++;
    }
    Team = Data[1]; // setting the player's team for accumulation of player objectives
    TeamDeaths[Team] = TeamDeaths[Team] + Deaths;
    PlayerObjectives[Player] = Data[2]; // converted temporary data storage to main memory for score reporting of player objectives
    Serial.println("Player Objectives completed: "+String(Data[2]));
    Serial.println("Player Objectives completed: "+String(PlayerObjectives[Player]));
    TeamObjectives[Team] = TeamObjectives[Team] + Data[2]; // added this player's objective score to his team's objective score
    Serial.println(String(millis()));
}
void PrimaryMenu() {
  WebSocketData = Menu[0];
      //notifyClients1();
      Serial.println("preset game mode menu = " + String(Menu[0]));
      datapacket2 = Menu[0];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
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
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
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

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  //Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  int eepromtaggercount = EEPROM.read(0);
  if (eepromtaggercount > 0 && eepromtaggercount < 65) {
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
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
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

  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();

  // set defaults
  Menu[0] = 3;
  Menu[1] = 102;
  Menu[2] = 207;
  Menu[3] = 308;
  Menu[4] = 401;
  Menu[5] = 501;
  Menu[6] = 601;
  Menu[7] = 701;
  Menu[8] = 801;
  Menu[9] = 901;
  Menu[10] = 1000;
  Menu[11] = 1102;
  Menu[12] = 1201;
  Menu[13] = 1303;
  Menu[14] = 1400;
  Menu[15] = 1500;
  Menu[17] = 1700;
  // End of setup.
}

void loop() {
  if (OTAMODE) {
    static int num=0;
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= otainterval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
    }
    if (UPTODATE) {
      ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
    }
  } else {
  while (DEMOMODE) {
    Menu[14] = 1401;
    ledState = !ledState;
    notifyClients();
    Serial.println("menu = " + String(Menu[14]));
    datapacket2 = Menu[14];
    datapacket1 = 99;
    getReadings();
    BroadcastData(); // sending data via ESPNOW
    Serial.println("Sent Data Via ESPNOW");
    ResetReadings();
    delay(12000);
    Menu[14] = 1400;
    ledState = !ledState;
    notifyClients();
    Serial.println("menu = " + String(Menu[14]));
    datapacket2 = Menu[14];
    datapacket1 = 99;
    getReadings();
    BroadcastData(); // sending data via ESPNOW
    Serial.println("Sent Data Via ESPNOW");
    ResetReadings();
    vTaskDelay(2000);
  }
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
  currentmilliseconds = millis();
  if (currentmilliseconds - lasttempcheck > tempcheckinterval) {
    lasttempcheck = currentmilliseconds;
    Serial.print("Temperature: ");
    // Convert raw temperature in F to Celsius degrees
    Serial.print((temprature_sens_read() - 32) / 1.8);
    Serial.println(" C");
    //delay(1000);
  }
  //if (currentmilliseconds - lastespnowpush > espnowpushinterval) {
    //lastespnowpush = currentmilliseconds;
    //SCORESYNC = true;
  //}
  if (BROADCASTESPNOW) {
    BROADCASTESPNOW = false;
    getReadings();
    BroadcastData(); // sending data via ESPNOW
    Serial.println("Sent Data Via ESPNOW");
    ResetReadings();
  }
  if (SCORESYNC) {
      unsigned long ScoreCurrentMillis = millis();
      if (ScoreCurrentMillis - ScorePreviousMillis > 200) {
        ScorePreviousMillis = ScoreCurrentMillis;
        RequestingScores();
      }
    }
  if (UPDATEWEBAPP) {
    if (WebAppUpdaterProcessCounter < 3) {
      unsigned long UpdaterCurrentMillis = millis();
      if (UpdaterCurrentMillis - WebAppUpdaterTimer > 1200) {
        WebAppUpdaterTimer = UpdaterCurrentMillis;
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
      }
    } else {
      UPDATEWEBAPP = false;
      WebAppUpdaterProcessCounter = 0;
    }
  }
  }
}
// end of loop

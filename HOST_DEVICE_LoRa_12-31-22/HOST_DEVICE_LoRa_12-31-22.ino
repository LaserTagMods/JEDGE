
// Import required libraries
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 200 // use 0 for OTA and 1 for Tagger ID
// EEPROM Definitions:
// EEPROM 0 to 96 used for wifi settings
// EEPROM 100 for game status
// EEPROM 101 for comms settings

// serial definitions for LoRa
#define SERIAL1_RXPIN 23 // TO Serial
#define SERIAL1_TXPIN 22 // TO Serial

byte CommsSetting = 0;

int TaggersOwned = 17; // how many taggers do you own or will play?

// Replace with your network credentials
const char* ssid = "JEDGE-CONTROLLER";
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

int WebSocketData;
bool ledState = 0;
const int ledPin = 2;
//const int LED_BUILTIN = 2;

int id = 0;
int pid = 0;

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
//uint8_t newMACAddress[] = {0x94, 0xB5, 0x55, 0x25, 0xED, 0xA8}; 
uint8_t newMACAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
//uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};
//uint8_t newMACAddress[] = {0x94, 0xB5, 0x55, 0x25, 0xED, 0xA5};

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
//uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};
uint8_t arcadebroadcastAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// data to process espnow data
String TokenStrings[20];
String PlayerUpdate[64];
int timestamp = 0;
long PreviousPlayerEval = 0;
byte ActivePlayerCount = 0;
byte ListOfPlayers[64]; // 0 if not playing, 1 if playing
byte PlayerTeamAssignment[64];
int LivesPool[4]; // lives count for assault
byte TeamCount = 2;
byte PlayersPerTeam[4];
bool ASSIGNTEAMS = false;
byte PBWeap = 0;
byte PBPerk = 0;

// scoring variables
int PreviousKillConfirmation[10];
byte KCcounter = 0;
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
int GameTime = 0;
long GameStartTime;
String RandomizedValue;

bool MULTIBASEDOMINATION = false;
long previngamemillis = 0;

// Game Settings - Config:
String SettingsHeader = "$AS"; // Admin Settings
byte SettingsAction = 4; // Lock out Taggers
byte SettingsGameMode = 0; // default is fREE fOR aLL
byte SettingsGameRules = 1; // Death Match Default
byte SettingsLighting = 0; // Outdoor
byte SettingsGameTime = 0; // off is default, 1 is on
long GameDuration;
byte SettingsRespawn = 0; // auto is default, 1 is manual
byte SettingsVolume = 60; // 60% is defaulted
byte SettingsLives = 0; // default low, 1 is high

String SendStartBeacon = "$AS,1," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
String ConfirmBeacon = "null";
String LockoutBeacon = "$AS,4," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
String UnlockBeacon = "$AS,3," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";

String StringToTransmit;

String incomingserial = "0";

bool INGAME = false;
bool ARCADEMODE = false;
int ingameminutes = 0;
int ArcadePlayerStatus[16];
int ArcadePlayerTime[16];
byte ArcadeCounter = 0;
byte MaxPlayers = 64;

String LoRaDataReceived; // string used to record data coming in
String LoRaDataToSend; // used to store data being sent from this device

bool LORALISTEN = false; // a trigger used to enable the radio in (listening)
bool ENABLETRANSMIT = false; // a trigger used to enable transmitting data

int sendcounter = 0;
int receivecounter = 0;

unsigned long gamepreviousMillis = 0;
long previousMillis0 = 0;
long PreviousMillis = 0;

String LastIncomingSerialData = "null";
long ClearCache = 0;

void PrepareStartBeacon() {
  SettingsAction = 1;
  SendStartBeacon = "$AS,1," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
  Serial.println(SendStartBeacon);
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
void StartGame() {
  Serial.println("start game");
  PrepareStartBeacon();
  StringToTransmit = SendStartBeacon;
  Transmit();
  GameStartTime = millis();
  gamepreviousMillis = millis();
  byte i = 0;
  // reset in game scores
  while (i < 4) {
    TeamGameScore[i] = 0;
    TeamDominationGameScore[i] = 0;
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
  INGAME = true;
  i = 0;
  while (i < 4) {
    LivesPool[i] = 5 * PlayersPerTeam[i];
    Serial.println("Team " + String(i) + " Player Count: " + String(PlayersPerTeam[i]) + ", Lives Pool: " + String(LivesPool[i]));
    i++;
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
  pid = 1; // player id place holder
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
      //Serial.println("Game Highlights:");
      //Serial.println();
      int KillsLeader[3];
      int ObjectiveLeader[3];
      int DeathsLeader[3];
      int LeaderScore[3];
      // first Leader for Kills:
      int kmax=0; 
      int highest=0;
      for (int k=0; k<16; k++)
      if (PlayerGameScore[k] > highest) {
        highest = PlayerGameScore[k];
        kmax = k;
      }
      // max now contains the largest kills value
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
      //Serial.println("Most Deadliest, Players with the most kills:");
      //Serial.println("1st place: Player " + String(KillsLeader[0]) + " with " + String(LeaderScore[0]));
      //Serial.println("2nd place: Player " + String(KillsLeader[1]) + " with " + String(LeaderScore[1]));
      //Serial.println("3rd place: Player " + String(KillsLeader[2]) + " with " + String(LeaderScore[2]));
      //Serial.println();
      LeadPlayerID = KillsLeader[0];
      board["KillsLeader0"] = KillsLeader[0];
      board["Leader0Kills"] = LeaderScore[0];
      board["KillsLeader1"] = KillsLeader[1];
      board["Leader1Kills"] = LeaderScore[1];
      board["KillsLeader2"] = KillsLeader[2];
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
      //Serial.println("The Dominator, Players with the most objective points:");
      //Serial.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      //Serial.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      //Serial.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      //Serial.println();   
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
      //Serial.println("Easy Target, Players with the most deaths:");
      //Serial.println("1st place: Player " + String(DeathsLeader[0]) + " with " + String(LeaderScore[0]));
      //Serial.println("2nd place: Player " + String(DeathsLeader[1]) + " with " + String(LeaderScore[1]));
      //Serial.println("3rd place: Player " + String(DeathsLeader[2]) + " with " + String(LeaderScore[2]));
      //Serial.println();
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

const char index_html1[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>JBOX</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  .collapsible {
    background-color: #23858B;
    color: white;
    cursor: pointer;
    padding: 18px;
    width: 100%;
    border: none;
    text-align: center;
    outline: none;
    font-size: 15px;
  }
  .active, .collapsible:hover {
    background-color: #4B5050;
  }
  .collapsible:after {
    content: '\002B';
    color: white;
    font-weight: bold;
    float: right;
    margin-left: 5px;
  }
  .active:after {
    content: "\2212";
  }
  .ccontent {
    padding: 0 18px;
    max-height: 0;
    overflow: hidden;
    transition: max-height 0.2s ease-out;
  }
  
  .select-block {
    width: 350px;
    margin: 0px auto 30px;
  }
  select {
    position: relative;
    border: solid 1px rgba(0, 214, 252, 0.3);
    background: none;
    color: rgba(0, 214, 252, 0.5);
    font-family: "Roboto", sans-serif;
    text-transform: uppercase;
    font-weight: normal;
    letter-spacing: 1.8px;
    height: 6vh;
    padding: 0;
    transition: all 0.25s ease;
    outline: none;
    width: 375px;
  }
  /* For IE <= 11 */
    select::-ms-expand {
    display: none;
  }
  select:hover,
    select:focus {
    color: #ffffff;
    background-color: #002B3C;
  }
  select:hover~.selectIcon,
  select:focus~.selectIcon {
    background-color: white;
  }
  select:hover~.selectIcon svg.icon,
    select:focus~.selectIcon svg.icon {
    fill: #3FA2BC;
  }
  html, body {
   font-family: Arial;
   display: inline-block;
   text-align: center;
   height:85vh;
  }
  .state {
    font-size: 1.6rem;
    color:#8c8c8c;
    font-weight: bold;
  }
  .button {
    position: relative;
    border: solid 1px rgba(0, 119, 139, 0.3);
    background: #ffffff;
    color: #000000;
    font-family: "Roboto", sans-serif;
    text-transform: uppercase;
    font-weight: normal;
    letter-spacing: 1.8px;
    padding-left: 10px;
    transition: all 0.25s ease;
    outline: none;
    width: 375px;
    appearance: none;
    height: 5vh;
  }
  .button:hover {
    box-shadow: 1px 1px 8px #ffffff;
    color: #ffffff;
    text-shadow: 0 0 8px rgba(0, 214, 252, 0.4);
  }
  .button:hover.button:before {
    left: 0;
    width: 120px;
  }
  .button:hover.button:after {
    right: 0;
    width: 120px;
  }
  .button:hover .button-line:before {
    bottom: 0;
  }
  .button:hover .button-line:after {
    top: 0;
  }
  .hr {
    width:375px;background-color:#417388; border:1px solid;
  }
  </style>
<title>JBOX 5.2</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  </head>
    <body style="background-color: #232E38;">
    <center>
    <table style="height: 85vh;border:solid 1px #167E91; background:#012A40;vertical-align:" class=".table">
    <tr>
    <br>
    <td align="center" style="height: 85vh;">
    <font style="size: 1.2em;color:#ffffff;">
  <h2>JEDGE CONTROLS</h2>
      <select name="pbindoor" id="pbindoorid">
        <option value="ZZ">Lighting - Select One</option>
        <option value="Indoor">Lighting -Indoor Mode</option>
        <option value="Outdoor">Lighting -Outdoor Mode</option>
        <option value="Stealth">Lighting -Stealth Mode</option>
      </select>
      <select name="pbgame" id="pbgameid">
        <option value="ZZ">Game Mode - Select One</option>
        <option value="FreeForAll">Game Mode - Free-4-All</option>
        <option value="DeathMatch">Game Mode - Death Match</option>
        <option value="Generals">Game Mode - Generals</option>
        <option value="Supremacy">Game Mode - Supremacy</option>
        <option value="Commanders">Game Mode - Commanders</option>
        <option value="Survival">Game Mode - Survival</option>
        <option value="TheSwarm">Game Mode - Swarm</option>
      </select>
      <select name="pbobj" id="pbobjid">
        <option value="ZZ">Game Rules - Select One</option>
        <option value="Death-Match">Game Rules - Timed</option>
        <option value="Assault">Game Rules - I25:T50 Death Match</option>
        <option value="BattleRoyale">Game Rules - Royale</option>
        <option value="Brawl">Game Rules - Gun Game</option>
        <option value="CaptureTheFlag">Game Rules - CTF</option>
        <option value="KingOfTheHill">Game Rules - KOTH</option>
      </select>
      <select name="pblives" id="pblivesid">
        <option value="ZZ">Volume - Select One</option>
        <option value="75">Volume - One</option>
        <option value="80">Volume - Two</option>
        <option value="85">Volume - Three</option>
        <option value="90">Volume - Four</option>
        <option value="95">Volume - Five</option>
      </select>
      <select name="pbtime" id="pbtimeid">
        <option value="ZZ">Time - Select One</option>
        <option value="5Minutes">Time - 5 Min</option>
        <option value="10Minutes">Time - 10 Min</option>
        <option value="15Minutes">Time - 15 Min</option>
        <option value="20Minutes">Time - 20 Min</option>
        <option value="30Minutes">Time - 30 Min</option>
        <option value="UnlimitedTime">Time - Unlimited</option>
      </select>
      <select name="pbspawn" id="pbspawnid">
        <option value="ZZ">Respawn - Select One</option>
        <option value="Off">Respawn - No Delay</option>
        <option value="15Seconds">Respawn - 15 Sec</option>
        <option value="30Seconds">Respawn - 30 Sec</option>
        <option value="60Seconds">Respawn - 60 Sec</option>
      </select>
      <br><br>
      <select name="pbteam" id="pbteamid">
        <option value="ZZ">Teams - Select One</option>
        <option value="TeamSelect">Player Choice</option>
        <option value="TwoTeams">Teams - 2 Teams</option>
        <option value="ThreeTeams">Teams - 3 Teams</option>
        <option value="FourTeams">Teams - 4 Teams</option>
      </select>
      <br><br>
      <select name="pbweap" id="pbweapid">
        <option value="ZZ">Weap/Class - Select One</option>
        <option value="WeaponSelect">Weap/Class - Player Choice</option>
        <option value="M4">Weap/Class - M4/Heavy/Sentinel/Wraith</option>
        <option value="Sniper">Weap/Class - Sniper/Soldier/Maurader/Sniper</option>
        <option value="Burst">Weap/Class - Burst/Medic/Guardian/Technician</option>
        <option value="ShotGun">Weap/Class - Shotgun/Valkyrie/Grenadier/Viper</option>
        <option value="HMG">Weap/Class - SMG/Merc/Merc/Merc</Merc>
        <option value="Tar">Weap/Class - Tar33/Commander/Commander/Commander</option>
        <option value="Silenced">Weap/Class - Silenced AR</option>
      </select>
      <br><br>
      <select name="pbperk" id="pbperkid">
        <option value="ZZ">Perk - Select One</option>
        <option value="PerkSelect">Perk - Player Choice</option>
        <option value="GrenadeLauncher">Perk - Grenade Launcher</option>
        <option value="Bandage">Perk - MedKit</option>
        <option value="BodyArmor">Perk - Body Armor</option>
        <option value="ExtendedMags">Perk - Extended Mags</option>
        <option value="ConcussionGrenade">Perk - Concussion Grenade</Merc>
        <option value="CriticalStrike">Perk - Critical Strike</option>
        <option value="Focus">Perk - Focus</option>
      </select>
      <br><br>
      <button id="pbstart" class="button">Start Game</button>
      <button id="pbend" class="button">End Game</button>
      <br>
      <font style="size: 1.2em;color:#ffffff;">Device Configuration<br><br>
      <br>
      <select name="rqhead" id="rqheadid">
        <option value="ZZ">Require Headset - Select One</option>
        <option value="yeshead">Require Headset - YES</option>
        <option value="nohead">Require Headset - NO</option>
      </select>
      <br>
      <select name="Comms" id="CommsID">
        <option value="ZZ">Select Wireless Settings - Select One</option>
        <option value="ESPNOW">ESPNOW LR</option>
        <option value="LORAFast">LoRa Fast</option>
        <option value="LORAMid">LoRa Mid Range</option>
        <option value="LORALR">LoRa Long Range</option>
      </select>
      <button id="pbhost" class="button">Prep Players</button>
      <button id="pbblock" class="button">Firmware Check</button>
      <form action="/get">
        WIFI Network: <input type="text" name="input1">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        WIFI Password: <input type="text" name="input2">
        <input type="submit" value="Submit">
      </form><br>      
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
    document.getElementById('rqheadid').addEventListener('change', handleheadset, false);
    document.getElementById('pbindoorid').addEventListener('change', handlepbindoor, false);
    document.getElementById('pbblock').addEventListener('click', togglepbblock);
    document.getElementById('pbstart').addEventListener('click', togglepbstart);
    document.getElementById('pbend').addEventListener('click', togglepbend);
    document.getElementById('pbhost').addEventListener('click', togglehost);
    document.getElementById('pbobjid').addEventListener('change', handlepbobj, false);
    document.getElementById('pbteamid').addEventListener('change', handlepbteamid, false);
    document.getElementById('pbweapid').addEventListener('change', handlepbweapid, false);
    document.getElementById('pbperkid').addEventListener('change', handlepbperkid, false);
    document.getElementById('CommsID').addEventListener('change', handleComms, false);
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
  function handleComms() {
    var xs = document.getElementById("CommsID").value;
    websocket.send(xs);
  }
  function handlepbweapid() {
    var xs = document.getElementById("pbweapid").value;
    websocket.send(xs);
  }
  function handlerqheadid() {
    var xs = document.getElementById("rqheadid").value;
    websocket.send(xs);
  }
  function handlepbperkid() {
    var xs = document.getElementById("pbperkid").value;
    websocket.send(xs);
  }
  function handlepbteamid() {
    var xs = document.getElementById("pbteamid").value;
    websocket.send(xs);
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
  function handleheadset() {
    var xs = document.getElementById("rqheadid").value;
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
   .scards { max-width: 1400; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
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
    <h3>Game Score Board</h3>
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
        <h2>Most Points</h2>
        <p><span class="reading">Player <span id="MO0"></span><span class="reading"> : <span id="MO10"></span></p>
        <p><span class="reading">Player <span id="MO1"></span><span class="reading"> : <span id="MO11"></span></p>
        <p><span class="reading">Player <span id="MO2"></span><span class="reading"> : <span id="MO12"></span></p>
      </div>
      <div class="scard black">
        <h2>Most Deaths</h2>
        <p><span class="reading">Player <span id="MD0"></span><span class="reading"> : <span id="MD10"></span></p>
        <p><span class="reading">Player <span id="MD1"></span><span class="reading"> : <span id="MD11"></span></p>
        <p><span class="reading">Player <span id="MD2"></span><span class="reading"> : <span id="MD12"></span></p>
      </div>
      <div class="scard black">
        <h2>Most Kills</h2>
        <p><span class="reading">Player <span id="MK0"></span><span class="reading"> : <span id="MK10"></span></p>
        <p><span class="reading">Player <span id="MK1"></span><span class="reading"> : <span id="MK11"></span></p>
        <p><span class="reading">Player <span id="MK2"></span><span class="reading"> : <span id="MK12"></span></p>
       </div>
    </div>
  </div>
  <div class="stopnav">
    <h3>Player Scores</h3>
  </div>
  <div class="scontent">
    <div class="scards">
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
    if (strcmp((char*)data, "yeshead") == 0) {
      Serial.println("DISABLE FAKE HEADSET");
      StringToTransmit = "$HS,0,*";
      Transmit();
    }
    if (strcmp((char*)data, "ESPNOW") == 0) {
      Serial.println("Set Primary Comms to ESPNOW LR");
      CommsSetting = 0;
      EEPROM.write(101,CommsSetting);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "LORAFast") == 0) {
      Serial.println("Set Primary Comms to LORA");
      CommsSetting = 1;
      EEPROM.write(101,CommsSetting);
      EEPROM.commit();
      FastLoraSetup();
    }
    if (strcmp((char*)data, "LORAMid") == 0) {
      Serial.println("Set Primary Comms to LORA");
      CommsSetting = 1;
      EEPROM.write(101,CommsSetting);
      EEPROM.commit();
      MidRangeLoraSetup();
    }
    if (strcmp((char*)data, "LORALR") == 0) {
      Serial.println("Set Primary Comms to LORA");
      CommsSetting = 1;
      EEPROM.write(101,CommsSetting);
      EEPROM.commit();
      LongRangeLoraSetup();
    }
    if (strcmp((char*)data, "nohead") == 0) {
      Serial.println("ENABLE FAKE HEADSET");
      StringToTransmit = "$HS,1,*";
      Transmit();
    }
    if (strcmp((char*)data, "TeamSelect") == 0) {
      Serial.println("unlock taggers to change teams");
      UnlockBeacon = "$AS,3," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
      StringToTransmit = UnlockBeacon;
      Transmit();
    }
    if (strcmp((char*)data, "WeaponSelect") == 0) {
      Serial.println("unlock taggers to change teams");
      UnlockBeacon = "$AS,5," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
      StringToTransmit = UnlockBeacon;
      Transmit();
    }
    if (strcmp((char*)data, "PerkSelect") == 0) {
      Serial.println("unlock taggers to change teams");
      UnlockBeacon = "$AS,2," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
      StringToTransmit = UnlockBeacon;
      Transmit();
    }
    if (strcmp((char*)data, "togglehost") == 0) {
      Serial.println("host taggers");
      LockoutBeacon = "$AS,4," + String(SettingsGameMode) + "," + String(SettingsGameRules) + "," + String(SettingsLighting) + "," + String(SettingsGameTime) + "," + String(SettingsRespawn) + "," + String(SettingsVolume) + ",*";
      StringToTransmit = LockoutBeacon;
      Transmit();
    }
    if (strcmp((char*)data, "togglepbblock") == 0) {
      Serial.println("Running Update Check on Taggers");
      // send wifi SSID to taggers
      StringToTransmit = "$SSID," + OTAssid + ",*";
      Serial.println("Update Command: " + StringToTransmit);
      Transmit();
      delay(250);
      // Send wifi password to taggers
      StringToTransmit = "$PASS," + OTApassword + ",*";
      Serial.println("Update Command: " + StringToTransmit);
      Transmit();
    }
    if (strcmp((char*)data, "togglepbstart") == 0) {
      StartGame();
    }      
    if (strcmp((char*)data, "togglepbend") == 0) {
      EndGame();
    }
    if (strcmp((char*)data, "M4") == 0) {
      Serial.println("M4");
      StringToTransmit = "$WEAP,0,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Sniper") == 0) {
      Serial.println("Sniper");
      StringToTransmit = "$WEAP,1,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Burst") == 0) {
      Serial.println("Burst");
      StringToTransmit = "$WEAP,2,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "ShotGun") == 0) {
      Serial.println("Shotgun");
      StringToTransmit = "$WEAP,3,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "HMG") == 0) {
      Serial.println("SMG");
      StringToTransmit = "$WEAP,4,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Tar") == 0) {
      Serial.println("Tar");
      StringToTransmit = "$WEAP,5,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Silenced") == 0) {
      Serial.println("Silenced");
      StringToTransmit = "$WEAP,6,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "GrenadeLauncher") == 0) {
      Serial.println("GrenadeLauncher");
      StringToTransmit = "$PERK,0,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Bandage") == 0) {
      Serial.println("MedKit");
      StringToTransmit = "$PERK,1,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "BodyArmor") == 0) {
      Serial.println("BodyArmor");
      StringToTransmit = "$PERK,2,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "ExtendedMags") == 0) {
      Serial.println("Extended Mags");
      StringToTransmit = "$PERK,3,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "ConcussionGrenade") == 0) {
      Serial.println("Concussion Grenade");
      StringToTransmit = "$PERK,4,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "CriticalStrike") == 0) {
      Serial.println("Critical Strike");
      StringToTransmit = "$PERK,5,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "Focus") == 0) {
      Serial.println("Focus");
      StringToTransmit = "$PERK,6,*"; 
      Transmit();
    }
    if (strcmp((char*)data, "TwoTeams") == 0) {
      Serial.println("Two Teams Selected");
      TeamCount = 2;
      ASSIGNTEAMS = true;
    }
    if (strcmp((char*)data, "ThreeTeams") == 0) {
      Serial.println("Three Teams Selected");
      TeamCount = 3;
      ASSIGNTEAMS = true;
    }
    if (strcmp((char*)data, "FourTeams") == 0) {
      Serial.println("Four Teams Selected");
      TeamCount = 4;
      ASSIGNTEAMS = true;
    }
    if (strcmp((char*)data, "Indoor") == 0) {
      Serial.println("indoor mode");
      SettingsLighting = 1;
    }
    if (strcmp((char*)data, "Outdoor") == 0) {
      Serial.println("outdoor mode");
      SettingsLighting = 0;
    }
    if (strcmp((char*)data, "Stealth") == 0) {
      Serial.println("Stealth mode");
      SettingsLighting = 2;
    }
    if (strcmp((char*)data, "FreeForAll") == 0) {
      Serial.println("Free For All");
      SettingsGameMode  = 0;
    }
    if (strcmp((char*)data, "DeathMatch") == 0) {
      Serial.println("Death Match");
      SettingsGameMode = 1;
    }
    if (strcmp((char*)data, "Generals") == 0) {
      Serial.println("Generals");
      SettingsGameMode = 2;
    }
    if (strcmp((char*)data, "Supremacy") == 0) {
      Serial.println("Supremacy");
      SettingsGameMode = 3;
    }
    if (strcmp((char*)data, "Commanders") == 0) {
      Serial.println("Commanders");
      SettingsGameMode = 4;
    }
    if (strcmp((char*)data, "Survival") == 0) {
      Serial.println("Survival");
      SettingsGameMode = 5;
    }
    if (strcmp((char*)data, "TheSwarm") == 0) {
      Serial.println("The Swarm");
      SettingsGameMode = 6;
    }
    if (strcmp((char*)data, "Death-Match") == 0) {
      Serial.println("Death Match");
      SettingsGameRules = 1;
    }
    if (strcmp((char*)data, "Assault") == 0) {
      Serial.println("Assault");
      SettingsGameRules = 2;
    }
    if (strcmp((char*)data, "BattleRoyale") == 0) {
      Serial.println("Battle Royale");
      SettingsGameRules = 3;
    }
    if (strcmp((char*)data, "Brawl") == 0) {
      Serial.println("Gun Game");
      SettingsGameRules = 4;
    }
    if (strcmp((char*)data, "CaptureTheFlag") == 0) {
      Serial.println("Capture The Flag");
      SettingsGameRules = 5;
    }
    if (strcmp((char*)data, "KingOfTheHill") == 0) {
      Serial.println("King Of The Hill");
      SettingsGameRules = 6;
    }
    if (strcmp((char*)data, "75") == 0) {
      Serial.println("Volume 1");
      SettingsVolume = 75;
    }
    if (strcmp((char*)data, "80") == 0) {
      Serial.println("Volume 2");
      SettingsVolume = 80;
    }
    if (strcmp((char*)data, "85") == 0) {
      Serial.println("Volume 3");
      SettingsVolume = 85;
    }
    if (strcmp((char*)data, "90") == 0) {
      Serial.println("Volume 4");
      SettingsVolume = 90;
    }
    if (strcmp((char*)data, "95") == 0) {
      Serial.println("Volume 5");
      SettingsVolume = 95;
    }
    if (strcmp((char*)data, "UnlimitedTime") == 0) {
      Serial.println("Unlimited / Off");
      SettingsGameTime = 0;
    }
    if (strcmp((char*)data, "5Minutes") == 0) {
      Serial.println("5 Minutes");
      SettingsGameTime = 5;
      GameDuration = 60000 * SettingsGameTime;
    }
    if (strcmp((char*)data, "10Minutes") == 0) {
      Serial.println("10 Minutes");
      SettingsGameTime = 10;
      GameDuration = 60000 * SettingsGameTime;
    }
    if (strcmp((char*)data, "15Minutes") == 0) {
      Serial.println("15 Minutes");
      SettingsGameTime = 15;
      GameDuration = 60000 * SettingsGameTime;
    }
    if (strcmp((char*)data, "20Minutes") == 0) {
      Serial.println("20 Minutes");
      SettingsGameTime = 20;
      GameDuration = 60000 * SettingsGameTime;
    }
    if (strcmp((char*)data, "30Minutes") == 0) {
      Serial.println("30 Minutes");
      SettingsGameTime = 30;
      GameDuration = 60000 * SettingsGameTime;
    }
    if (strcmp((char*)data, "Off") == 0) {
      Serial.println("Off");
      SettingsRespawn = 0;
    }
    if (strcmp((char*)data, "15Seconds") == 0) {
      Serial.println("15 Seconds");
      SettingsRespawn = 15;
    }
    if (strcmp((char*)data, "30Seconds") == 0) {
      Serial.println("30 Seconds");
      SettingsRespawn = 30;
    }
    if (strcmp((char*)data, "60Seconds") == 0) {
      Serial.println("60 Seconds");
      SettingsRespawn = 60;
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
// *************** OTA Updates ****************************
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
//********************* Serial input *********************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(Serial1.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    String IncomingSerialData = Serial1.readStringUntil('*'); // stores the incoming data to the pre set string
    Serial.print("IncomingSerialData: "); // printing data received
    Serial.println(IncomingSerialData); // printing data received
    if(IncomingSerialData.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)IncomingSerialData.c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        TokenStrings[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        Serial.println("Token " + String(index) + ": " + TokenStrings[index]);
      }
      // Adjust TokenString Locations for processing
      byte j = 0;
      byte k = 2;
      while (k < index) {
        TokenStrings[j] = TokenStrings[k];
        Serial.println("Token " + String(j) + ": " + TokenStrings[j]);
        j++;
        k++;
      }
    } else {
    
      char *ptr = strtok((char*)IncomingSerialData.c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        TokenStrings[index] = ptr;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        Serial.println("Token "+String(index)+": "+TokenStrings[index]);
        index++;
      }
    }
      // the data is now stored into individual strings under TokenStrings[]
      // $PH,GunID,TeamID,InGameStatus,LifeStatus,*
      if (TokenStrings[0] == "$PH") {
        // Received a player update
        // Add player to the player count and tell player to stop sending this command
        Serial.println("Received a ping from a tagger");
        int Index = TokenStrings[1].toInt();
        ListOfPlayers[Index] = 1;
        StringToTransmit = "$PT," + TokenStrings[1] + ",*";
        Transmit();
        //delay(200);
        //Serial1.println(PingTagger);
        long TimeStamp = millis();
        timestamp = TimeStamp/1000;
        PlayerUpdate[Index] = TokenStrings[2] + "," + TokenStrings[3] + "," + TokenStrings[4] + "," + String(timestamp) + ",*";
        // check if host is in game:
        if (INGAME) {
          // if in game, need to see if this tagger is in game as well
          if (TokenStrings[3] != "1") {
            // this tagger is rebooted or joining the game
            StringToTransmit = SendStartBeacon;
            Transmit();
          }
        }
      }
      if (TokenStrings[0] == "$DD") {
        Serial.println("Received a Kill Confirmation from a Player");
        // this is a kill confirmation from a player, record player/team point
        // make sure it is not a duplicate
        int incomingconfirmationid = TokenStrings[4].toInt();
        byte j = 0;
        bool NEWKILLCONFIRMATION = true;
        while (j < 5) {
          if (PreviousKillConfirmation[j] != incomingconfirmationid) {
            j++;
          } else {
            j = 5;
            NEWKILLCONFIRMATION = false;
          }
        }
        if (NEWKILLCONFIRMATION) {
          PreviousKillConfirmation[KCcounter] = incomingconfirmationid;
          KCcounter++;
          if (KCcounter > 4) {
            KCcounter = 0;
          }
          int KillerPlayerID = TokenStrings[1].toInt();
          int KillerTeamID = TokenStrings[2].toInt();
          int DeadPlayerID = TokenStrings[3].toInt();
          PlayerGameScore[KillerPlayerID]++;
          TeamGameScore[KillerTeamID]++;
          PlayerDeaths[DeadPlayerID]++;
          String KillConfirmationReceived = "$HKC," + TokenStrings[3] + ",*";
          StringToTransmit = KillConfirmationReceived;
          Transmit();
          Serial.println("New Kill Processed and Sent Receipt Confirmation to Player");
          if (SettingsGameRules == 2) {
            if (PlayerGameScore[KillerPlayerID] > 25 || TeamGameScore[KillerTeamID] > 50) {
              EndGame();
            }
          }
        }
      }
      // respawn request: $RR,TeamID,GunID,*
      if (TokenStrings[0] == "$RR") {
        // received a respawn request
        Serial.println("Received a Respawn Request");
        int i = TokenStrings[1].toInt();
        LivesPool[i]--;
        String RespawnPlayer = "$RP," + TokenStrings[2] + "," + String(LivesPool[i]) + ",*";
        Serial1.println(RespawnPlayer);
        //delay(20);
        //Serial1.println(RespawnPlayer);
      }
      // King of the Hill Points: $KOTH,(TEAM),*
      if (TokenStrings[0] == "$KOTH") {
        // received a koth POINT
        Serial.println("Received a KOTH point");
        int i = TokenStrings[1].toInt();
        TeamDominationGameScore[i]++;
      }
      if (TokenStrings[0] == "CAPTURE") {
          // THIS IS FROM A NODE DOMINATION BASE THAT HAS BEEN CAPTURED
          // check game mode:
            // This base is a master, time to process the data package
            // token 3 is the base id, token 4 is the team id, token 5 is the player id
            int incomingjboxid = TokenStrings[1].toInt();
            incomingjboxid = incomingjboxid - 100;
            JboxPlayerID[incomingjboxid] = TokenStrings[3].toInt();
            JboxTeamID[incomingjboxid] = TokenStrings[2].toInt();
            JboxInPlay[incomingjboxid] = 1;
            MULTIBASEDOMINATION = true;
            Serial.println("processed update from node lora jbox");
        }
  }
}
void UnregisterTaggers() {
  Serial1.println("$UR,*");
  //delay(20);
  //Serial1.println("$UR,*");
  int index = 0;
  while (index < 64) {
    ListOfPlayers[index] = 0;
    index++;
  }
  ActivePlayerCount = 0;
}
void Transmit() {
  // LETS SEND SOMETHING:
  if (CommsSetting == 1) {
    String TransmitMe = "AT+SEND=0,"+String(StringToTransmit.length())+","+StringToTransmit+"\r\n";
    Serial.print("Transmitting Via LoRa: "); // printing to serial monitor
    Serial.println(TransmitMe); // used to send a data packet
    Serial1.print(TransmitMe); // used to send a data packet
  } else {
    Serial.println("Transmitting"); // printing to serial monitor
    Serial1.println(StringToTransmit); // used to send a data packet
    //delay(20);
    //Serial1.println(StringToTransmit);
  }
}
void EvaluatePlayerUpdates() {
  byte counter = 0;
  String PlayerTokens[8];
  ActivePlayerCount = 0; // Reset Player Count based upon those who have reported in
  long TimeStamp = millis();
  timestamp = TimeStamp/1000;
  byte ConfirmLivePlayers = 0;
  byte PlayersOnTeam[4];
  byte PlayersAlive = 0;
  byte TeamAlignment = 0;
  byte PlayerAliveID = 0;
  while (counter < MaxPlayers) {
    if (PlayerUpdate[counter] != "null") {
      // if null, player has not reported in yet
      // convert the data into a string array for comparing data
      ListOfPlayers[counter] = 1;
      char *ptr = strtok((char*)PlayerUpdate[counter].c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        PlayerTokens[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        Serial.println("Token "+String(index)+": "+PlayerTokens[index]);
      }
      int playerupdatetimestamp = PlayerTokens[3].toInt();
      if (timestamp - playerupdatetimestamp > 90) {
        // checking for data time out, if older than 90 seconds the
        // the player has not reported in within a reasonable time and may
        // be turned off.
        PlayerUpdate[counter] = "null";
        ListOfPlayers[counter] = 0;
        // clearing this player out
      } else {
        ActivePlayerCount++; // add one player present
        if (PlayerTokens[2] == "0") {
          // player is alive
          PlayersAlive++;
          if (SettingsGameRules == 3) {
            // playing battle royale
            PlayerAliveID = counter;
            // just incase this is the only player alive... lets store it
          }
          // add live player count to teams
          TeamAlignment = PlayerTokens[0].toInt();
          PlayersOnTeam[TeamAlignment]++;
        }
      }
    }
    counter++;
  }
  // We now know how many players are active
  // we know how many are alive in the game... roughly
  // we know how many are alive on each team... roughly
  // lets add the battle royale logic.. if all players are dead except one, game over and that last player is the winner
  if (SettingsGameRules == 3 && PlayersAlive < 2) {
    // We have one player left!
    LeadPlayerID = PlayerAliveID;
    EndGame();
  }
  if (PlayersAlive < 1) {
    // no more players alive
    if (SettingsGameMode == 5 || SettingsGameMode == 6) {
      // all players are turned to zombie
      LeadTeamID = 3;
    }
    EndGame();
  }
}
// *************** Game Controls *********************************
void EndJBoxGame() {
  String FirstToken = "GAMEOVER"; // 0 for red, 100 for blue, 200 for yellow, 300 for green, example team blue with player 16, 10116
  byte SecondToken= LeadPlayerID; // for directing who it is From
  byte ThirdToken = LeadTeamID; // What team captured base
  byte FourthToken = 99;  // Player ID
  Serial.println("sending team and player alignment to other jboxes, here is the value being sent: ");
  LoRaDataToSend = FirstToken + "," + String(SecondToken) + "," + String(ThirdToken) + "," + String(FourthToken);
  Serial.println(LoRaDataToSend);
  ENABLETRANSMIT = true;
  //LastPosessionTeam = 99;
  MULTIBASEDOMINATION = false;
  int playercounter = 0;
  int teamcounter = 0;
  while (playercounter < 64) {
    PlayerDominationGameScore[playercounter] = 0;
    playercounter++;
    delay(1);
  }
  while (teamcounter < 4) {
    TeamDominationGameScore[teamcounter] = 0;
    teamcounter++;
  }
}
void EndGame() {
      if (SettingsGameMode == 0) {
        byte VictoryID = 100 + LeadPlayerID;
        StringToTransmit = "$SP," + String(VictoryID) + ",*";
      } else {
        StringToTransmit = "$SP," + String(LeadTeamID) + ",*";
      }
      Transmit();
      Serial.println("end a game early");
      INGAME = false;      
      EEPROM.write(100, 0);
      EEPROM.commit();
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
      // EndJBoxGame();
}
void RunGameControls() {
  unsigned long CurrentGameMillis = millis();
  if (CurrentGameMillis - PreviousMillis > 3000) {
    UPDATEWEBAPP = true;
    PreviousMillis = CurrentGameMillis;
  }
  // Manage Game Time
  if (SettingsGameTime > 0) {
    if (CurrentGameMillis - GameStartTime > GameDuration) {
      EndGame();
    } else {
      // Lets update the clock every 30 seconds
      if (CurrentGameMillis - gamepreviousMillis >= 30000) {
        gamepreviousMillis = CurrentGameMillis;
        long ActualGameDuration = CurrentGameMillis - GameStartTime;
        long RemainingGameTime = GameDuration - ActualGameDuration;
        // lets run some checks for notifications or game end
        if (RemainingGameTime < 122000 &&  RemainingGameTime > 118000) {
          // 2 minute warning = $UP,100,5,0,*
          Serial1.println("$UP,100,5,0,*");
          //delay(20);
          //Serial1.println("$UP,100,5,0,*");
        }
        if (RemainingGameTime < 62000 &&  RemainingGameTime > 58000) {
          // 1 minute warning = $UP,100,6,0,*
          Serial1.println("$UP,100,6,0,*");
          //delay(20);
          //Serial1.println("$UP,100,6,0,*");
        }
        if (RemainingGameTime < 32000 &&  RemainingGameTime > 28000) {
          // 1 minute warning = $UP,100,7,0,*
          Serial1.println("$UP,100,7,0,*");
          //delay(20);
          //Serial1.println("$UP,100,7,0,*");
        }
        if (RemainingGameTime < 12000 &&  RemainingGameTime > 8000) {
          // 1 minute warning = $UP,100,8,0,*
          Serial1.println("$UP,100,8,0,*");
          //delay(20);
          //Serial1.println("$UP,100,8,0,*");
        }
      }
    }
  }
}
void MakeTeamAssignments() {
  // Team Assignments commands look like: $TA,GunID,TeamID,*
  ASSIGNTEAMS = false;
  if (SettingsGameMode > 0) {
    // dont waster your time if on free for all
    // there are three different groups of game play options: 
    //    deathmatch, supremacy and survival
    //    survival, max 2 teams
    //    supremacy, max 3 teams
    //    deathmatch max 4 teams
    byte index = 0;
    byte counter = 0;
    String TeamAssignmentString;
    bool Refresh = false;
    if (SettingsGameMode == 5 || SettingsGameMode == 6) {
      // this is a survival match up and we only need two teams and the ratio is 
      //   four humans to one infected
      //   we use pbteam so we need to go by those rules.
      //   0 for humans and 1 for infected
      while (index < 64) {
        if (ListOfPlayers[index] == 1) {
          // there is a player playing in this index id
          if (SettingsGameMode == 5 || SettingsGameMode == 6) {
            // this is a survival match up and we only need two teams and the ratio is 
            //   four humans to one infected
            //   we use pbteam so we need to go by those rules.
            //   0 for humans and 1 for infected
            if (counter == 4) {
              // infected team member
              TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
              counter = 0;
              PlayersPerTeam[1]++;
            } else {
              // human team member
              TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
              counter++;
              PlayersPerTeam[0]++;
            }
          }
          if (SettingsGameMode == 3 || SettingsGameMode == 4) {
            // This is for Supremacy, the next least complicated but need to account for two team options: 2/3 teams total
            // Will Prioritize 0 Red and 1 Blue and 2 Green is the third option for three teams.
            if (TeamCount == 2) {
              // two teams
              if (counter == 1) {
                // infected team member
                TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
                PlayersPerTeam[counter]++;
                counter = 0;
              } else {
                // human team member
                TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
                PlayersPerTeam[counter]++;
                counter = 1;
              }
            } else {
              // three teams
              if (counter == 2) {
                // green team member
                TeamAssignmentString = "$TA," + String(index) + "," + "2,*";
                PlayersPerTeam[counter]++;
                Refresh = true;
              }
              if (counter == 1) {
                // blue team member
                TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (counter == 0) {
                // red team member
                TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (Refresh) {
                Refresh = false;
                counter = 0;
              }
            }
          }
          if (SettingsGameMode == 1 || SettingsGameMode == 2) {
            // this is for death match team assignments 1-4
            // note that this needs the tagger controller to send the team 2/3 after game start and before
            if (TeamCount == 2) {
              // two teams
              if (counter == 1) {
                // blue team member
                TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
                PlayersPerTeam[counter]++;
                counter = 0;
              } else {
                // red team member
                TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
                PlayersPerTeam[counter]++;
                counter = 1;
              }
            }
            if (TeamCount == 3) {
              // three teams
              if (counter == 2) {
                // green team member
                TeamAssignmentString = "$TA," + String(index) + "," + "2,*";
                PlayersPerTeam[counter]++;
                Refresh = true;
              }
              if (counter == 1) {
                // blue team member
                TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (counter == 0) {
                // red team member
                TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (Refresh) {
                Refresh = false;
                counter = 0;
              }
            }
            if (TeamCount == 4) {
              // three teams
              if (counter == 3) {
                // infected team member
                TeamAssignmentString = "$TA," + String(index) + "," + "3,*";
                PlayersPerTeam[counter]++;
                Refresh = true;
              }
              if (counter == 2) {
                // human team member
                TeamAssignmentString = "$TA," + String(index) + "," + "2,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (counter == 1) {
                // human team member
                TeamAssignmentString = "$TA," + String(index) + "," + "1,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (counter == 0) {
                // human team member
                TeamAssignmentString = "$TA," + String(index) + "," + "0,*";
                PlayersPerTeam[counter]++;
                counter++;
              }
              if (Refresh) {
                Refresh = false;
                counter = 0;
              }
            }
          }
          Serial1.println(TeamAssignmentString);
          index++;
          //delay(20);
          //Serial1.println(TeamAssignmentString);
          //delay(20);
        }
      }
    }
  }
}

void StartLoRaSerial() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa Serial Port");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
}
void FastLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  Serial1.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  //Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS=99\r\n");   //needs to be unique
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
}
void LongRangeLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //Serial1.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  //Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS=99\r\n");   //needs to be unique
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
}
void MidRangeLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //Serial1.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS=99\r\n");   //needs to be unique
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
}
// ************************** Second core Loop ******************************
void loop0(void *pvParameters) {
  Serial.print("Comms loop running on core ");
  Serial.println(xPortGetCoreID());
  int counter = 0;
  while (counter < 64) {
    PlayerUpdate[counter] = "null";
    counter++;
  }
  for(;;) { // starts the forever loop 
    // put all the serial activities in here for communication with the brx
    // LoRa Receiver Main Functions:
    ReceiveTransmission();
    long CurrentTime = millis();
    //if (CurrentTime - ClearCache > 2000) {
      //LastIncomingSerialData = "null";
    //}
    ws.cleanupClients();
    if (UPDATEWEBAPP) {
      UpdateWebApp0();
      UpdateWebApp1();
      UpdateWebApp2();
      UPDATEWEBAPP = false;
    }
    delay(1); // this has to be here or it will just continue to restart the esp32
  }
}
//************************* Setup ********************************************
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
  CommsSetting = EEPROM.read(101);
  if (CommsSetting == 255) {CommsSetting = 0;}
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
  //esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
  //esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  WiFi.softAP(ssid, password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  initWebSocket();
  // Route for root / web page
  server1.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  // Route for root / web page1
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
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
    Serial.println(inputMessage);
    Serial.print("OTA SSID = ");
    Serial.println(OTAssid);
    Serial.print("OTA Password = ");
    Serial.println(OTApassword);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  //server1.onNotFound(notFound);
  // Start server
  server.begin();
  server1.begin();

  //***********************************************************************
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing Serial");
  //Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  StartLoRaSerial();
  //***********************************************************************
  // initialize dual cores and dual loops
  Serial.print("Apps runninf on core: ");
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop0, "loop0", 10000, NULL, 1, NULL, 0);
}

//********************** Main Loop ***********************************************
void loop() {
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
  if (Serial.available() > 0) {
    String str = Serial.readString();
    Serial.println(str);
    Serial1.println(str);
    //delay(200);
    //Serial1.println(str);
  }
  /*
   * // THIS processes periodic player update for battle royale or assault type games
  if (CurrentCacheMillis - PreviousPlayerEval > 40000) {
    PreviousPlayerEval = millis();
    EvaluatePlayerUpdates();
  }
  */
  if(INGAME) {
    RunGameControls();
  }
  if (ASSIGNTEAMS) {
    MakeTeamAssignments();
  }
}
// end of loop

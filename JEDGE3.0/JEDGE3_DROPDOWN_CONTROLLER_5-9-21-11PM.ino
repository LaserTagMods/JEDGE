// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <Arduino_JSON.h>

// no need to change this
int GunID = 99; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
int aGunGeneration = 2; // change to gen 1, 2, 3
int TaggersOwned = 64; // how many taggers do you own or will play?

// Replace with your network credentials
const char* ssid = "GUN#99";
const char* password = "123456789";

int WebSocketData;
bool ledState = 0;
const int ledPin = 2;
int Menu[25]; // used for menu settings storage

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

bool SCORESYNC = false; // used for initializing score syncronization
int ScoreRequestCounter = 0; // counter used for score syncronization
String ScoreTokenStrings[73]; // used for disecting incoming score string/char
int PlayerDeaths[64]; // used for score accumilation
int PlayerKills[64]; // used for score accumilation
int PlayerObjectives[64]; // used for score accumilation
int TeamKills[4]; // used for score accumilation
int TeamObjectives[4]; // used for score accumilation
int TeamDeaths[4];
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
String datapacket4 = "null"; // used for score reporting only


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
  //incomingData4 = incomingReadings.DP4; // From
  Serial.println("DP1: " + String(incomingData1)); // INTENDED RECIPIENT
  Serial.println("DP2: " + String(incomingData2)); // FUNCTION/COMMAN
  Serial.println("DP3: " + String(incomingData3)); // From - device ID
  Serial.print("DP4: "); // used for scoring
  //Serial.println(incomingData4);
  Serial.write(incomingReadings.DP4);
  Serial.println();
  incomingData4 = String(incomingReadings.DP4);
  Serial.println(incomingData4);
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
  datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  datapacket2 = 32700; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  datapacket3 = GunID; // From - device ID
  datapacket4 = "null";
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
    <h1>JEDGE Host Controls</h1>
  </div>
  <div class="content">
    <div class="card">
      <p><button id="initializejedge" class="button">Lockout Players</button></p>
      <h2>Primary Weapon</h2>
      <p><select name="primary" id="primaryid">
        <option value="3">AMR</option>
        <option value="4">Assault Rifle</option>
        <option value="5">Bolt Rifle</option>
        <option value="6">Burst Rifle</option>
        <option value="7">Charge Rifle</option>
        <option value="8">Energy Launcher</option>
        <option value="9">Energy Rifle</option>
        <option value="10">Force Rifle</option>
        <option value="11">Ion Sniper</option>
        <option value="12">Laser Cannon</option>
        <option value="13">Plasma Sniper</option>
        <option value="14">Rail Gun</option>
        <option value="15">Rocket Launcher</option>
        <option value="16">Shotgun</option>
        <option value="17">SMG</option>
        <option value="18">Sniper Rifle</option>
        <option value="19">Stinger</option>
        <option value="20">Supressor Rifle</option>
        <option value="21">Pistol</option>
        <option value="1">Manual Selection</option>
        <option value="2">Unarmed</option>
        </select>
      </p>
      <h2>Secondary Weapon</h2>
      <p><select name="secondary" id="secondaryid">
        <option value="102">Unarmed</option>
        <option value="103">AMR</option>
        <option value="104">Assault Rifle</option>
        <option value="105">Bolt Rifle</option>
        <option value="106">Burst Rifle</option>
        <option value="107">Charge Rifle</option>
        <option value="108">Energy Launcher</option>
        <option value="109">Energy Rifle</option>
        <option value="110">Force Rifle</option>
        <option value="111">Ion Sniper</option>
        <option value="112">Laser Cannon</option>
        <option value="113">Plasma Sniper</option>
        <option value="114">Rail Gun</option>
        <option value="115">Rocket Launcher</option>
        <option value="116">Shotgun</option>
        <option value="117">SMG</option>
        <option value="118">Sniper Rifle</option>
        <option value="119">Stinger</option>
        <option value="120">Supressor Rifle</option>
        <option value="101">Manual Selection</option>
        </select>
      </p><h2>Ammunition Settings</h2>
      <p><select name="ammo" id="ammoid">
        <option value="1102">Unlimited Magazines</option>
        <option value="1103">Unlimited Rounds</option>
        <option value="1101">Limited Ammo</option>
        </select>
      </p>
      <h2>Melee Function</h2>
      <p><select name="MeleeSetting" id="meleesettingid">
        <option value="1700">Off</option>
        <option value="1701">Damage</option>
        <option value="1702">Disarm/Arm</option>
        </select>
      </p>
      <h2>Player Lives</h2>
      <p><select name="playerlives" id="playerlivesid">
        <option value="207">Unlimited</option>
        <option value="201">1</option>
        <option value="202">3</option>
        <option value="203">5</option>
        <option value="204">10</option>
        <option value="205">15</option>
        <option value="206">25</option>
        </select>
      </p>
      <h2>Game Time</h2>
      <p><select name="gametime" id="gametimeid">
        <option value="308">Unlimited</option>
        <option value="301">1 Minute</option>
        <option value="302">5 Minutes</option>
        <option value="303">10 Minutes</option>
        <option value="304">15 Minutes</option>
        <option value="305">20 Minutes</option>
        <option value="306">25 Minutes</option>
        <option value="307">30 Minutes</option>
        </select>
      </p>
      <h2>Ambience Setting</h2>
      <p><select name="ambience" id="ambienceid">
        <option value="401">Outdoor Mode</option>
        <option value="402">Indoor Mode</option>
        <option value="403">Stealth</option>
        </select>
      </p>
      <h2>Team Selections</h2>
      <p><select name="teamselection" id="teamselectionid">
        <option value="501">Free For All</option>
        <option value="502">Two Teams (Odds Vs Evens)</option>
        <option value="503">Three Teams (every third)</option>
        <option value="504">Four Teams (every fourth)</option>
        <option value="505">Manual Select</option>
        </select>
      </p>
      <h2>Preset Game Modes</h2>
      <p><select name="presetgamemodes" id="presetgamemodesid">
        <option value="601">Default</option>
        <option value="602">5min F4A</option>
        <option value="603">10min W/ Rndm Weapons</option>
        <option value="604">Battle Royale</option>
        <option value="605">Capture The Flag</option>
        <option value="606">Own The Zone</option>
        <option value="607">Survival-Infection</option>
        <option value="608">Assimilation</option>
        <option value="609">Gun Game</option>
        <option value="610">Team Death Match</option>
        <option value="611">Kids Mode</option>
        </select>
      </p>
      <h2>Respawn Mode</h2>
      <p><select name="respawn" id="respawnid">
        <option value="701">Automatic - Immediate</option>
        <option value="702">Automatic - 15 seconds</option>
        <option value="703">Automatic - 30 seconds</option>
        <option value="704">Automatic - 45 seconds</option>
        <option value="705">Automatic - 60 seconds</option>
        <option value="706">Automatic - 90 seconds</option>
        <option value="707">Automatic - 120 seconds</option>
        <option value="708">Automatic - 150 seconds</option>
        <option value="709">Manual - Respawn Station</option>
        </select>
      </p>
      <h2>Delayed Start Timer</h2>
      <p><select name="starttimer" id="starttimerid">
        <option value="801">None - Immediate</option>
        <option value="802">15 Seconds</option>
        <option value="803">30 Seconds</option>
        <option value="804">45 Seconds</option>
        <option value="805">60 Seconds</option>
        <option value="806">90 Seconds</option>
        <option value="807">5 Minutes</option>
        <option value="808">10 Minutes</option>
        <option value="809">15 Minutes</option>
        </select>
      </p>
      <h2>Friendly Fire</h2>
      <p><select name="friendlyfire" id="friendlyfireid">
        <option value="1201">On</option>
        <option value="1200">Off</option>
        </select>
      </p>
       <h2>Tagger Volume</h2>
      <p><select name="adjustvolume" id="adjustvolumeid">
        <option value="1301">1</option>
        <option value="1302">2</option>
        <option value="1303">3</option>
        <option value="1304">4</option>
        <option value="1305">5</option>
        </select>
      </p>
      <p><button id="gamestart" class="button">Start Game</button></p>
      <p><button id="gameend" class="button">End Game</button></p>
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
      <p><button id="initializeotaupdate" class="button">Enable OTA</button></p>
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
    document.getElementById('initializejedge').addEventListener('click', toggle15);
    document.getElementById('initializeotaupdate').addEventListener('click', toggle15D);
    document.getElementById('presetgamemodesid').addEventListener('change', handleGMode, false);
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('secondaryid').addEventListener('change', handlesecondary, false);
    document.getElementById('meleesettingid').addEventListener('change', handlemelee, false);
    document.getElementById('playerlivesid').addEventListener('change', handlelives, false);
    document.getElementById('gametimeid').addEventListener('change', handlegametime, false);
    document.getElementById('ambienceid').addEventListener('change', handleambience, false);
    document.getElementById('teamselectionid').addEventListener('change', handleteams, false);
    document.getElementById('respawnid').addEventListener('change', handlerespawn, false);
    document.getElementById('starttimerid').addEventListener('change', handlestartimer, false);
    document.getElementById('friendlyfireid').addEventListener('change', handlefriendlyfire, false);
    document.getElementById('ammoid').addEventListener('change', handleammo, false);
    document.getElementById('adjustvolumeid').addEventListener('change', handlevolumesetting, false);
    document.getElementById('gamestart').addEventListener('click', toggle14s);
    document.getElementById('gameend').addEventListener('click', toggle14e);
    document.getElementById('syncscores').addEventListener('click', toggle9);
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
  function toggle15D(){
    websocket.send('toggle15D');
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
      //WebSocketData = Menu[9];
      //notifyClients();
      Serial.println(" menu = " + String(Menu[9]));
      datapacket2 = 901;
      datapacket1 = 99;
      ClearScores();
      // BROADCASTESPNOW = true;
      SCORESYNC = true;
      Serial.println("Commencing Score Sync Process");
      ScoreRequestCounter = 0;
    }
    if (strcmp((char*)data, "toggle14s") == 0) { // game start
      Menu[14] = 1401;
      //WebSocketData = Menu[14];
      //notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle14e") == 0) { // game end
      Menu[14] = 1400;
      //WebSocketData = Menu[14];
      //notifyClients();
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
    if (strcmp((char*)data, "toggle15") == 0) { // initialize jedge
      Menu[15] = 1501;
      WebSocketData = Menu[15];
      //notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
    }
    if (strcmp((char*)data, "toggle15D") == 0) { // auto update
      Menu[15] = 1505;
      WebSocketData = Menu[15];
      //notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
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
    }
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
  if (ScoreRequestCounter < 63) {
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


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
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
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
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

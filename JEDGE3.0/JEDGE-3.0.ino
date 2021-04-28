/*
 * by Jay Burden
 * 
 * use pins 16 & 17 to connect to BLUETOOTH pins on tagger. 17 goes to RX of brx or TX of the bluetooth module, 16 goes to the TX of tagger or RX of bluetooth
 * optionally power from laser tag battery with a Dorhea MP1584EN mini buck converter or use available power supply port from tagger board
 * this devices uses the serial communication to set tagger and game configuration settings
 * Note the sections labeled *****IMPORTANT***** as it requires customization on your part
 *
 * Serial data sent to an LCD is also intended
 * 
 * For OTA Updates: be sure to download the latest python build and install with the option to add "python to path" this allows your pc to upload OTA
 * https://www.python.org/ftp/python/3.8.5/python-3.8.5.exe
 * Also make sure you have Bonjour installed and sometimes it is finicky so be prepared to restart your computer to make it work.
 * 
 * Also board selection for uploading this code, you can use whatever esp32 you want just be sure it has two cores, very rare to have one
 * here are the board settings:
 * Board: Wemos D1 Mini ESP32
 * Upload Speed: 921600
 * CPU Frequency: 240 MHZ (wifi/bt)
 * Flash frequency: 80 mhz
 * Parition scheme: minimal spiffs, large apps with OTA
 * Programmer: AVRISP mkII
 *  
 * update 4/2/2020 annotations added and copied over objects for setting up games
 * update 4/3/2020 Cleaned up for more compatibility to serial comms programing
 * update 4/3/2020 Was able to plug in set setting from serial for friendly fire and outdoor/indoor mode
 * update 4/4/2020 was able to get team settings and manual input team settings working as well as gender settings
 * updated 4/4/2020 included weapon selection assignment and notification to select in game settings manual options
 * updated 4/6/2020 separated many variables to not share writing abilities from both cores (trouble shooting)
 * upfsyrf 4/7/2020 finished isolating variables between cores to eliminate both cores writing to same variables
 * updated 4/7/2020 fixed the resetting of the BLE function so device stays paired to brx, also integrated the delay start with the send game settings to start the game
 * updated 4/8/2020 fixed some menu callout audio selections, reduced the delay time to verify what is best and still functions to avoid disconnects
 * updated 4/8/2020 fixed weapon slot 1 loading issue, was set to weapon slot 0 so guns were overwriting.
 * updated 4/9/2020 fixed team selection item number and set freindly fire on when free for all selection is made for team selection.
 * updated 4/9/2020 fixed audio announcement for teams for free for all to say free for all
 * updated 4/9/2020 fixed audio announcement for domination to read control point versus select a game
 * updated 4/9/2020 fixed limited ammo, wasnt allowing limited ammo due to incrorrect if then statement setting
 * updated 4/9/2020 fixed the manual team selection option to enable players to pick their own teams
 * updated 4/10/2020 enabled LCD data sending to esp8266, updated data to be sent to get lives, weapon and other correct indicators sent to the LCD
 * updated 4/13/2020 worked on more LCD debuging issues for sending correct data to LCD ESP8266
 * updated 4/14/2020 changed power output for the BLE antenna to try to minimize disconnects, was successful
 * updated 4/15/2020 fixed unlimited ammo when unarmed, was looping non stop for reloading because no ammo on unarmed
 * updated 4/15/2020 added additional ammunition options, limited, unlimited magazines, and unlimited rounds as options - ulimited rounds more for kids
 * updated 4/19/2020 improved team selection process, incorporated "End Game" selection (requires app update) disabled buttons/trigger/reload from making noises when pressed upon connection to avoid annoying sounds from players
 * updated 4/19/2020 enabled player selection for weapon slots 0/1. tested, debugged and ready to go for todays changes.
 * updated 4/22/2020 enabled serial communications to send weapon selection to ESP8266 so that it can be displayed what weapon is what if lCD is installed
 * updated 4/22/2020 enabled game timer to terminate a game for a player
 * updated 4/27/2020 enabled serial send of game score data to esp8266
 * updated 4/28/2020 adjusted volume settings to modify volume not at game start but whenever
 * updated 5/5/2020 modified the delayed start counter to work better and not stop the program as well as incorporate auditable countdown
 * updated 5/5/2020 incorporated respawn delay timers as well as respawn stations for manual respawn
 * updated 5/6/2020 fixed count down audio for respawn and delay start timers, also fixed lives assignment, was adding 100 to the lives selected. Note: all 5/5/2020 updates tested functional
 * updated 5/6/2020 fixed game timer repeat end, added two minute warning, one minute warning and ten second count down to end of game
 * updated 5/6/2020 re-instated three team selection and added four team auto selections
 * updated 5/7/2020 disabled player manual selections when triggered by blynk for a new option to be enabled or if game starts (cant give them players too much credit can we?)
 * updated 5/8/2020 fixed a bug with the count down game timer announcements
 * updated 5/22/2020 integrated custom weapon audio
 * updated 5/25/2020 disabled auto lockout of buttons upon esp32 pairing, enabled blynk enabled lockout of buttons instead by V18 or 1801 serial command This way we can control esp32 bluetooth activation to be enabled instead of automatic
 * updated 5/25/2020 added in deap sleep enabling if esp8266 sends a 1901 command
 * updated 8/11/2020 changed reporting score processes and timing to instant upon esp8266 request
 * updated 8/14/2020 fixed error for sending player scores, was missing player [0] and had a player[64] same for team[0] and team[6]
 * updated 8/16/2020 added an IR Debug mode that when a tag is recieved, it enables the data to be sent to the esp8266 and forwarded on to the app
 * updated 8/18/2020 added OTA updating capability to device, can be a bit finicky so be patient i guess.
 * updated 10/30/2020 reformatted code for easier legibility and clearl define all functions
 * updated 10/30/2020 added in manual respawns from nades/bases, also added in other IR based tag support, but it might be better to change the IR types later to not cause damage, or add in a healt boost with the feature
 * Updated 11/16/2020 fixed bugs with manual respawn. all good now
 * Updated 12/12/2020 started moving over blynk objects from esp8266 for making the esp32 a standalone device for controlling the brx
 * updated 12/12/2020 re did the dual core for my most up to date method that eliminates need for extensive delays between loop cycles
 * updated 12/12/2020 revised the method for sending the data to brx to use serial comms from Serial1 versus bluetooth
 * updated 12/12/2020 revised method for receiving data from brx to be serial instead of ble and created object to analyze the data received
 * updated 12/16/2020 bug checking, adding in some serial prints etc to see what it is doing, changed order and some declarations
 * updated 12/17/2020 fixed bugs where audio was not sounding for buttons pressed for confirmations, also fixed volume adjustments in app as well as code, fixed weapon selection bug for loading weapons
 * updated 12/17/2020 fixed serial print for debug for recieved data via serial from brx. improved print out for verification of received values
 * updated 12/17/2020 fixed primary weapon set when starting up, the respawn bullet type 15 is not being accepted from some reason. removed it for now for future fix
 * updated 12/17/2020 fixed looping weapon, team and weapon manual selection so that it actually exits, it was looping for some reason even after confirming
 * updated 12/17/2020 removed team selections under manual for teams 4 and 5 because it was previously discovered they dont work.
 * updated 12/23/2020 fixed server connection features, added automatic power down of esp if wifi or blynk not available to save energy on brx, added auto configurations based upon generation of gun input
 * updated 12/28/2020 fixed error for delayed starts, needed to add a vtask delay to the timer while loop to prevent dev board crashes
 * updated 12/28/2020 fixed lives call out and assignment as well as app
 * updated 12/29/2020 added in a reset function if wifi is present in game and the blynk app isnt connected after a good 10 seconds and a couple checks on it
 * updated 12/30/2020 added wifi reset for when error for connection to blynk server occurs. This will notify via audio of a connection lost and re established as well as enable blynk reconnection if more than 5 seconds pass with wifi present and blynk server disconnected.
 * updated 01/01/2021 updated score sync settings for sending but still needs love
 * updated 01/02/2021 fixed the network checks so that they do not spin out of control while in game and players move away from the wifi network
 * updated 01/07/2021 added player and team selection for custom settings based upon player id or team alignment
 * updated 01/08/2021 changed default options for ammunition to be unlimited magazines instead of limited ammo
 * updated 01/08/2021 added "lets move out" to start game sequence so players know when its time to go and they got the command to start the game and can "move out", no one knew when we could go spread out
 * updated 01/10/2021 added game mode integration for quick preset settings for games - work in progress still
 * updated 01/12/2021 added terminal integration for sending commands from terminal to push commands to taggers, also added blynking led on esp while awaiting OTA update
 * updated 01/13/2021 put back in the end of game announcements to do count down timers 2 minute warning, one minute and final countdown.
 * updated 01/23/2021 added in the integration of IR for picking up weapons and getting random perks by base activation.
 * updated 04/08/2021 updated for stealth and visual confirmations etc and gen3 specific needs
 * updated 04/08/2021 removed all blynk applications and prioritized interrupt with ESPNOW for applying settings
 *                    removed OTA update for now for simplicity, can add back in later.
 * updated 04/16/2021 added in all the webserver objects and declarations and started working on app-less controls
 *                    added some of the menu options for controls and tested between two esps
 * updated 04/18/2021 Fixed misc. bugs discovered by the guys testing out things on the set up and application. changed default esp command to 32700 so it doesnt make unusual kill confirmatinos                   
 *                    fixed some headset color issues, fixed some call out issues, nothing major in terms of changes, just debugging.
 * updated 04/19/2021 added back in automatic sleep function for when no controls are executed within 4 minutes of boot up.                  
 *                    Added OTA back in
 * UPDATED 04/20/2021 Fixed the bug for ending games, on host device/tagger
 *                    Added a toggle to allow for disabling Access point and web server
 * updated 04/26/2021 Fixed all scoring bugs (I hope) had it tested by Paul a bit and tested a bit on my own. Finalized how i want scoring to show on the app for totals - I should be all done with scoring
 *                    Added in syncing local scores if host device is a tagger.
 *                    
 *                    
 *                    
 *                    
 */

//*************************************************************************
//********************* LIBRARY INCLUSIONS - ALL **************************
//*************************************************************************
#include <HardwareSerial.h> // for assigining misc pins for tx/rx
#include <WiFi.h> // used for all kinds of wifi stuff
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <AsyncTCP.h> // used for web server
#include <ESPAsyncWebServer.h> // used for web server
#include <ESPmDNS.h> // needed for OTA updates
#include <WiFiUdp.h> // needed for OTA updates
#include <ArduinoOTA.h> // needed for OTA updates
#include <Arduino_JSON.h>
//****************************************************************

int sample[5];

#define SERIAL1_RXPIN 16 // TO BRX TX and BLUETOOTH RX
#define SERIAL1_TXPIN 17 // TO BRX RX and BLUETOOTH TX
bool ESPTimeout = true; // if true, this enables automatic deep sleep of esp32 device if wifi or blynk not available on boot
long TimeOutCurrentMillis = 0;
long TimeOutInterval = 240000;
int BaudRate = 57600; // 115200 is for GEN2/3, 57600 is for GEN1, this is set automatically based upon user input
bool RUNWEBSERVER = true; // this enables the esp to act as a web server with an access point to connect to
bool FAKESCORE = false;

//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//*********** YOU NEED TO CHANGE INFO IN HERE FOR EACH GUN!!!!!!***********
int GunID = 10; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
int GunGeneration = 2; // change to gen 1, 2, 3
const char GunName[] = "GUN#10"; // used for OTA id recognition on network and for AP for web server
const char* password = "123456789"; // Password for web server
const char* OTAssid = "maxipad"; // network name to update OTA
const char* OTApassword = "9165047812"; // Network password for OTA
int TaggersOwned = 64; // how many taggers do you own or will play?
bool ACTASHOST = true; // enables/disables the AP mode for the device so it cannot act as a host. Set to "true" if you want the device to act as a host
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//****************************************************************

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
String tokenStrings[100]; // once processed the data is broken up into this string array for use
char *ptr = NULL; // used for initializing and ending string break up.
// these are variables im using to create settings for the game mode and
// gun settings
int settingsallowed = 0; // trigger variable used to walk through steps for configuring gun(s) for serial core
int settingsallowed1 = 0; // trigger variable used to walk through steps for configuring gun(s) for BLE core
int SetSlotA=2; // this is for weapon slot 0, default is AMR
int SLOTA=100; // used when weapon selection is manual
int SetSlotB=1; // this is for weapon slot 1, default is unarmed
int SLOTB=100; // used when weapon selection is manual
int SetLives=32000; // used for configuring lives
int SetSlotC=0; // this is for weapon slot 3 Respawns Etc.
int SetSlotD = 0; // this is used for perk IR tag
int SetTeam=0; // used to configure team player settings, default is 0
long SetTime=2000000000; // used for in game timer functions on esp32 (future
int SetODMode=0; // used to set indoor and outdoor modes (default is on)
int SetGNDR=0; // used to change player to male 0/female 1, male is default 
int SetRSPNMode; // used to set auto or manual respawns from bases/ir (future)
long RespawnTimer = 0; // used to delay until respawn enabled
long RespawnTimerMax = 0; // max timer setting for incremental respawn timer (Ramp 90)
int SetObj=32000; // used to program objectives
int SetFF=1; // set game to friendly fire on/off (default is on)
int SetVol=80; // set tagger volume adjustment, default is 65
int CurrentWeapSlot; // used for indicating what weapon slot is being used, primarily for unlimited ammo
int ReloadType; // used for unlimited ammo... maybe 10 is for unlimited
int SwapBRXCounter = 0; // used for weapon swaps in game for all weapons
int totalweapons = 19; // counter used for weapon count, needs to be updated if i ever add more weapons
bool SWAPBRX = false; // used as trigger to enable/disable swapbrx mode
long SelectButtonTimer = 0; // used to disable option to swap weapons
int GetAmmoOdds = 3; // setting default odds as 1/3 for ammo reload

int Deaths = 0; // death counter
int Team=0; // team selection used when allowed for custom configuration
int MyKills = 0; // counter for this players kill count
int MaxKills = 32000; // setting limit on kill counts
int Objectives = 32000; // objective goals
int CompletedObjectives=0; // earned objectives by player
int PlayerLives = 32000; // setting max player lives
int MaxTeamLives = 32000; // setting maximum team lives
long GameTimer = 2000000000; // setting maximum game time
long GameStartTime=0; // used to set the start time when a match begins
int PlayerKillCount[64] = {0}; // so its players 0-63 as the player id.
int TeamKillCount[6] = {0}; // teams 0-6, Red-0, blue-1, yellow-2, green-3, purple-4, cyan-5
long DelayStart = 0; // set delay count down to 0 seconds for default
int GameMode=1; // for setting up general settings
int Special=0; // special settings
int AudioPlayCounter=0; // used to make sure audio is played only once (redundant check)
int UNLIMITEDAMMO = 2; // used to trigger ammo auto replenish if enabled
bool OutofAmmoA = false; // trigger for auto respawn of ammo weapon slot 0
bool OutofAmmoB = false; // trigger for auto respawn of ammo weapon slot 1
String AudioSelection; // used to play stored audio files with tagger FOR SERIAL CORE
String AudioSelection1; // used to play stored audio files with tagger FOR BLE CORE
int AudioDelay = 0; // used to delay an audio playback
long AudioPreviousMillis = 0;
String AudioPerk; // used to play stored audio files with tagger FOR BLE CORE
int AudioPerkTimer = 4000; // used to time the perk announcement
bool PERK = false; // used to trigger audio perk
String TerminalInput; // used for sending incoming terminal widget text to Tagger
bool TERMINALINPUT = false; // used to trigger sending terminal input to tagger
bool WRITETOTERMINAL = false; // used to write to terminal

int lastTaggedPlayer = -1;  // used to capture player id who last shot gun, for kill count attribution
int lastTaggedTeam = -1;  // used to captures last player team who shot gun, for kill count attribution
int lastTaggedBase = -1; // used to capture last base used for swapbrx etc.

// used to send to ESP8266 for LCD display
int ammo1 = 0;
int ammo2 = 0;
int weap = 0;
int health = 45;
int armor = 70;
int shield =70;
int SpecialWeapon = 0;
int PreviousSpecialWeapon = 0;
int ChangeMyColor = 99; // default, change value from 99 to execute tagger and headset change
int CurrentColor = 99; // default, change value from 99 to execute tagger and headset change
bool VOLUMEADJUST=false; // trigger for audio adjustment
bool RESPAWN = false; // trigger to enable auto respawn when killed in game
bool MANUALRESPAWN = false; // trigger to enable manual respawn from base stations when killed
bool PENDINGRESPAWNIR = false; // trigger to check for IR respawn signals
bool GAMEOVER = false; // used to trigger game over and boot a gun out of play mode
bool TAGGERUPDATE = false; // used to trigger sending data to ESP8266 used for BLE core
bool TAGGERUPDATE1 = false; // used to turn off BLE core lcd send data trigger
bool AUDIO = false; // used to trigger an audio on tagger FOR SERIAL CORE
bool AUDIO1 = false; // used to trigger an audio on tagger FOR BLE CORE
bool GAMESTART = false; // used to trigger game start
bool TurnOffAudio=false; // used to trigger audio off from serial core
bool GETTEAM=false; // used when configuring customized settings
bool STATUSCHANGE=false; // used to loop through selectable customized options
bool GETSLOT0=false; // used for configuring manual weapon selection
bool GETSLOT1=false; // used for configuring manual weapon selection
bool INGAME=false; // status check for game timer and other later for running certain checks if gun is in game.
bool COUNTDOWN1=false; // used for triggering a specic countdown
bool COUNTDOWN2=false; // used for triggering a specific countdown
bool COUNTDOWN3=false; // used for triggering a specific countdown
bool IRDEBUG = false; // used to enabling IR tag data send to esp8266
bool IRTOTRANSMIT = false; // used to enable transmitting of IR data to ESP8266
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool INITIALIZEOTA = false; // enables the object to disable BLE and enable WiFi
bool SPECIALWEAPON = false;
bool HASFLAG = false; // used for capture the flag
bool SELECTCONFIRM = false; // used for using select button to confirm an action
bool SPECIALWEAPONLOADOUT = false; // used for enabling special weapon loading
bool AMMOPOUCH = false; // used for enabling reload of a weapon
bool LOOT = false; // used to indicate a loot occured
bool STEALTH = false; // used to turn off gun led side lights
bool INITJEDGE = false;

long startScan = 0; // part of BLE enabling

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED
unsigned long ledpreviousMillis = 0;  // will store last time LED was updated
const long ledinterval = 1500;  // interval at which to blink (milliseconds)

bool WEAP = false; // not used anymore but was used to auto load gun settings on esp boot

bool SCORESYNC = false;
bool UPDATEWEBAPP = false;
long WebAppUpdaterTimer = 0;
int WebAppUpdaterProcessCounter = 0;

int ScoreRequestCounter = 0;
String ScoreTokenStrings[73];
int PlayerDeaths[64];
int PlayerKills[64];
int PlayerObjectives[64];
int TeamKills[4];
int TeamObjectives[4];
int TeamDeaths[4];
String ScoreString = "0";
long ScorePreviousMillis = 0;
bool SYNCLOCALSCORES = false;


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

//****************************************************8
// WebServer 
//****************************************************
int WebSocketData;
bool LEDState = 0;
const int ledPin = 2;
int Menu[25]; // used for menu settings storage

int id = 0;
//int tempteamscore;
//int tempplayerscore;
int teamscore[4];
int playerscore[64];
int pid = 0;

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
    <h1>JEDGE 3.0</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Initialize JEDGE</h2>
      <p class="state">Initialize JEDGE<span id="IJ">%JEDGE%</span></p>
      <p><button id="initializejedge" class="button">Lockout Players</button></p>
      <h2>Firmware-Upgrade</h2>
      <p class="state">Init OTA<span id="OTA">%UPDATE%</span></p>
      <p><button id="initializeotaupdate" class="button">OTA Mode</button></p>
    </div>
  </div>
  <div class="stopnav">
    <h3>Host Control Panel</h3>
  </div>
  <div class="content">  
    <div class="card">
      <h2>Primary Weapon</h2>
      <p class="state">Selected: <span id="W0">%WEAP0%</span></p>
      <p><button id="weapon0" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Secondary Weapon</h2>
      <p class="state">Selected: <span id="W1">%WEAP1%</span></p>
      <p><button id="weapon1" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Player Lives</h2>
      <p class="state">Selected: <span id="L0">%LVS0%</span></p>
      <p><button id="lives0" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Game Time/Length</h2>
      <p class="state">Selected: <span id="GT">%GTM%</span></p>
      <p><button id="gtime0" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Ambience/Lighting</h2>
      <p class="state">Selected: <span id="AMB">%OUTDR%</span></p>
      <p><button id="ambience" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Team Settings</h2>
      <p class="state">Selected: <span id="TM">%TEAM%</span></p>
      <p><button id="teams0" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Preset Game Modes</h2>
      <p class="state">Selected: <span id="GMD">%GMODE%</span></p>
      <p><button id="gamemode" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Respawn Setting</h2>
      <p class="state">Selected: <span id="RSPWN">%RSPWNMODE%</span></p>
      <p><button id="respawnmode" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Delayed Start Timer</h2>
      <p class="state">Selected: <span id="DST">%DLYSTM%</span></p>
      <p><button id="delaystarttimer" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Player Gender</h2>
      <p class="state">Selected: <span id="GND">%GENDER%</span></p>
      <p><button id="playergender" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Ammunitions Setting</h2>
      <p class="state">Selected: <span id="AMO">%AMMUNITION%</span></p>
      <p><button id="ammosetting" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Friendly Fire</h2>
      <p class="state">Selected: <span id="FF">%FRIENDLY%</span></p>
      <p><button id="friendlyfire" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Tagger Volume</h2>
      <p class="state">Selected: <span id="VOL">%VOLUME%</span></p>
      <p><button id="taggervolume" class="button">Toggle</button></p>
    </div>
    <div class="card">
      <h2>Start/End Game</h2>
      <p class="state">Selected: <span id="IG">%NGAME%</span></p>
      <p><button id="gamestart" class="button">Start</button></p>
      <p><button id="gameend" class="button">End</button></p>
    </div>
    <div class="card">
      <h2>Sync Scores</h2>
      <p class="state">Press <span id="SS">%SYNC%</span></p>
      <p><button id="syncscores" class="button">Sync</button></p>
    </div>
  </div>
  <div class="stopnav">
    <h3>Score Board</h3>
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

  document.getElementById("MO0").innerHTML = obj.ObjectivesLeader0;
  document.getElementById("MO10").innerHTML = obj.Leader0Objectives;
  document.getElementById("MO1").innerHTML = obj.ObjectivesLeader1;
  document.getElementById("MO11").innerHTML = obj.Leader1Objectives;
  document.getElementById("MO2").innerHTML = obj.ObjectivesLeader2;
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
    var OTA;
    var IG;
    var FF;
    var SS;
    var DST;
    var RSPWN;
    var GMD;
    var TM;
    var W0;
    var W1;
    var L0;
    var GT;
    var AMB;
    var GND;
    var AMO;
    var VOL;
    var IJ;
    var K0;
    var O0;
    if (event.data == "999") {
      K0 = TeamKills[0];
      document.getElementById('K0').innerHTML = K0;
    }
    if (event.data == "1505"){
      OTA = "OTA Enabled";
      document.getElementById('OTA').innerHTML = OTA;
    }
    if (event.data == "1"){
      W0 = "Manual Selection";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "2"){
      W0 = "Unarmed";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "3"){
      W0 = "AMR";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "4"){
      W0 = "Assault Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "5"){
      W0 = "Bolt Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "6"){
      W0 = "Burst Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "7"){
      W0 = "Charge Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "8"){
      W0 = "Energy Launcher";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "9"){
      W0 = "Energy Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "10"){
      W0 = "Force Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "11"){
      W0 = "Ion Sniper";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "12"){
      W0 = "Laser Cannon";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "13"){
      W0 = "Plasma Sniper";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "14"){
      W0 = "Rail Gun";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "15"){
      W0 = "Rocket Launcher";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "16"){
      W0 = "Shotgun";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "17"){
      W0 = "SMG";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "18"){
      W0 = "Sniper Rifle";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "19"){
      W0 = "Stinger";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "20"){
      W0 = "Suppressor";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "21"){
      W0 = "Pistol";
      document.getElementById('W0').innerHTML = W0;
    }
    if (event.data == "101"){
      W1 = "Manual Selection";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "102"){
      W1 = "Unarmed";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "103"){
      W1 = "AMR";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "104"){
      W1 = "Assault Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "105"){
      W1 = "Bolt Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "106"){
      W1 = "Burst Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "107"){
      W1 = "Charge Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "108"){
      W1 = "Energy Launcher";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "109"){
      W1 = "Energy Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "110"){
      W1 = "Force Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "111"){
      W1 = "Ion Sniper";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "112"){
      W1 = "Laser Cannon";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "113"){
      W1 = "Plasma Sniper";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "114"){
      W1 = "Rail Gun";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "115"){
      W1 = "Rocket Launcher";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "116"){
      W1 = "Shotgun";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "117"){
      W1 = "SMG";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "118"){
      W1 = "Sniper Rifle";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "119"){
      W1 = "Stinger";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "120"){
      W1 = "Suppressor";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "121"){
      W1 = "Pistol";
      document.getElementById('W1').innerHTML = W1;
    }
    if (event.data == "207"){
      L0 = "Unlimited";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "206"){
      L0 = "25";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "205"){
      L0 = "15";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "204"){
      L0 = "10";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "203"){
      L0 = "5";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "202"){
      L0 = "3";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "201"){
      L0 = "1";
      document.getElementById('L0').innerHTML = L0;
    }
    if (event.data == "301"){
      GT = "1 Minute";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "302"){
      GT = "5 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "303"){
      GT = "10 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "304"){
      GT = "15 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "305"){
      GT = "20 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "306"){
      GT = "25 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "307"){
      GT = "30 Minutes";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "308"){
      GT = "Unlimited";
      document.getElementById('GT').innerHTML = GT;
    }
    if (event.data == "401"){
      AMB = "Outdoor Mode";
      document.getElementById('AMB').innerHTML = AMB;
    }
    if (event.data == "402"){
      AMB = "Indoor Mode";
      document.getElementById('AMB').innerHTML = AMB;
    }
    if (event.data == "403"){
      AMB = "Stealth Mode";
      document.getElementById('AMB').innerHTML = AMB;
    }
    if (event.data == "501"){
      TM = "Free For All";
      document.getElementById('TM').innerHTML = TM;
    }
    if (event.data == "502"){
      TM = "Two Teams (odds/evens)";
      document.getElementById('TM').innerHTML = TM;
    }
    if (event.data == "503"){
      TM = "Three Teams (every 3rd)";
      document.getElementById('TM').innerHTML = TM;
    }
    if (event.data == "504"){
      TM = "Four Teams (every 4th)";
      document.getElementById('TM').innerHTML = TM;
    }
    if (event.data == "505"){
      TM = "Manual Selection";
      document.getElementById('TM').innerHTML = TM;
    }
    if (event.data == "601"){
      GMD = "Defaults";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "602"){
      GMD = "5min F4A W/ AR + Shotguns";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "603"){
      GMD = "10 W/ Rndm Weapons";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "604"){
      GMD = "Swap BRX (Battle Royale)";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "605"){
      GMD = "Capture The Flag";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "606"){
      GMD = "Own The Zone";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "607"){
      GMD = "Survival/Infection";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "608"){
      GMD = "Assimilation";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "609"){
      GMD = "Gun Game";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "610"){
      GMD = "Basic Team Domination";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "611"){
      GMD = "Trouble in Terrorist Town";
      document.getElementById('GMD').innerHTML = GMD;
    }
    if (event.data == "701"){
      RSPWN = "Auto - No Delay";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "702"){
      RSPWN = "Auto - 15 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "703"){
      RSPWN = "Auto - 30 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "704"){
      RSPWN = "Auto - 45 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "705"){
      RSPWN = "Auto - 60 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "706"){
      RSPWN = "Auto - 90 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "707"){
      RSPWN = "Auto - 120 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "708"){
      RSPWN = "Auto - 150 Seconds";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "709"){
      RSPWN = "Manual - Respawn Stations";
      document.getElementById('RSPWN').innerHTML = RSPWN;
    }
    if (event.data == "801"){
      DST = "Immediate";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "802"){
      DST = "15 Seconds";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "803"){
      DST = "30 Seconds";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "804"){
      DST = "45 Seconds";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "805"){
      DST = "60 Seconds";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "806"){
      DST = "90 Seconds";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "807"){
      DST = "5 Minutes";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "808"){
      DST = "10 Minutes";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "809"){
      DST = "15 Minutes";
      document.getElementById('DST').innerHTML = DST;
    }
    if (event.data == "901"){
      SS = "to Sync";
      document.getElementById('SS').innerHTML = SS;
    }
    if (event.data == "1000"){
      GND = "Male";
      document.getElementById('GND').innerHTML = GND;
    }
    if (event.data == "1001"){
      GND = "Female";
      document.getElementById('GND').innerHTML = GND;
    }
    if (event.data == "1101"){
      AMO = "Limited";
      document.getElementById('AMO').innerHTML = AMO;
    }
    if (event.data == "1102"){
      AMO = "Infinite Mags";
      document.getElementById('AMO').innerHTML = AMO;
    }
    if (event.data == "1103"){
      AMO = "Infinite Rounds";
      document.getElementById('AMO').innerHTML = AMO;
    }
    if (event.data == "1200"){
      FF = "OFF";
      document.getElementById('FF').innerHTML = FF;
    }
    if (event.data == "1201"){
      FF = "On";
      document.getElementById('FF').innerHTML = FF;
    }
    if (event.data == "1301"){
      VOL = "One";
      document.getElementById('VOL').innerHTML = VOL;
    }
    if (event.data == "1302"){
      VOL = "Two";
      document.getElementById('VOL').innerHTML = VOL;
    }
    if (event.data == "1303"){
      VOL = "Three";
      document.getElementById('VOL').innerHTML = VOL;
    }
    if (event.data == "1304"){
      VOL = "Four";
      document.getElementById('VOL').innerHTML = VOL;
    }
    if (event.data == "1305"){
      VOL = "Five";
      document.getElementById('VOL').innerHTML = VOL;
    }
    if (event.data == "1400"){
      IG = "Standby";
      document.getElementById('IG').innerHTML = IG;
    }
    if (event.data == "1401"){
      IG = "Game Started";
      document.getElementById('IG').innerHTML = IG;
    }
    if (event.data == "1501"){
      IJ = "JEDGE Initialized";
      document.getElementById('IJ').innerHTML = IJ;
    }
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('weapon0').addEventListener('click', toggle0);
    document.getElementById('weapon1').addEventListener('click', toggle1);
    document.getElementById('lives0').addEventListener('click', toggle2);
    document.getElementById('gtime0').addEventListener('click', toggle3);
    document.getElementById('ambience').addEventListener('click', toggle4);
    document.getElementById('teams0').addEventListener('click', toggle5);
    document.getElementById('gamemode').addEventListener('click', toggle6);
    document.getElementById('respawnmode').addEventListener('click', toggle7);
    document.getElementById('delaystarttimer').addEventListener('click', toggle8);
    document.getElementById('syncscores').addEventListener('click', toggle9);
    document.getElementById('playergender').addEventListener('click', toggle10);
    document.getElementById('ammosetting').addEventListener('click', toggle11);
    document.getElementById('friendlyfire').addEventListener('click', toggle12);
    document.getElementById('taggervolume').addEventListener('click', toggle13);
    document.getElementById('gamestart').addEventListener('click', toggle14s);
    document.getElementById('gameend').addEventListener('click', toggle14e);
    document.getElementById('initializejedge').addEventListener('click', toggle15);
    document.getElementById('initializeotaupdate').addEventListener('click', toggle15D);
  }
  function toggle0(){
    websocket.send('toggle0');
  }
  function toggle1(){
    websocket.send('toggle1');
  }
  function toggle2(){
    websocket.send('toggle2');
  }
  function toggle3(){
    websocket.send('toggle3');
  }
  function toggle4(){
    websocket.send('toggle4');
  }
  function toggle5(){
    websocket.send('toggle5');
  }
  function toggle6(){
    websocket.send('toggle6');
  }
  function toggle7(){
    websocket.send('toggle7');
  }
  function toggle8(){
    websocket.send('toggle8');
  }
  function toggle9(){
    websocket.send('toggle9');
  }
  function toggle10(){
    websocket.send('toggle10');
  }
  function toggle11(){
    websocket.send('toggle11');
  }
  function toggle12(){
    websocket.send('toggle12');
  }
  function toggle13(){
    websocket.send('toggle13');
  }
  function toggle14s(){
    websocket.send('toggle14s');
  }
  function toggle14e(){
    websocket.send('toggle14e');
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
  ws.textAll(String(WebSocketData));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle0") == 0) {
      if (Menu[0] > 20) {
        Menu[0] = 1;
      } else {
        Menu[0]++;
      }
      WebSocketData = Menu[0];
      notifyClients();
      Serial.println("Weapon 0= " + String(Menu[0]));
      datapacket2 = Menu[0];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle1") == 0) {
      if (Menu[1] > 119) {
        Menu[1] = 101;
      } else {
        Menu[1]++;
      }
      WebSocketData = Menu[1];
      notifyClients();
      Serial.println("Weapon 1 = " + String(Menu[1]));
      datapacket2 = Menu[1];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle2") == 0) {
      if (Menu[2] > 206) {
        Menu[2] = 201;
      } else {
        Menu[2]++;
      }
      WebSocketData = Menu[2];
      notifyClients();
      Serial.println("Lives Menu = " + String(Menu[2]));
      datapacket2 = Menu[2];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle3") == 0) {
      if (Menu[3] > 307) {
        Menu[3] = 301;
      } else {
        Menu[3]++;
      }
      WebSocketData = Menu[3];
      notifyClients();
      Serial.println("GAME TIME Menu = " + String(Menu[3]));
      datapacket2 = Menu[3];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle4") == 0) {
      if (Menu[4] > 402) {
        Menu[4] = 401;
      } else {
        Menu[4]++;
      }
      WebSocketData = Menu[4];
      notifyClients();
      Serial.println("AMBIENCE Menu = " + String(Menu[4]));
      datapacket2 = Menu[4];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle5") == 0) {
      if (Menu[5] > 504) {
        Menu[5] = 501;
      } else {
        Menu[5]++;
      }
      WebSocketData = Menu[5];
      notifyClients();
      Serial.println("Teams menu = " + String(Menu[5]));
      datapacket2 = Menu[5];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle6") == 0) {
      if (Menu[6] > 610) {
        Menu[6] = 601;
      } else {
        Menu[6]++;
      }
      WebSocketData = Menu[6];
      notifyClients();
      Serial.println("preset game mode menu = " + String(Menu[6]));
      datapacket2 = Menu[6];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle7") == 0) {
      if (Menu[7] > 708) {
        Menu[7] = 701;
      } else {
        Menu[7]++;
      }
      WebSocketData = Menu[7];
      notifyClients();
      Serial.println("respawn mode menu = " + String(Menu[7]));
      datapacket2 = Menu[7];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle8") == 0) {
      if (Menu[8] > 808) {
        Menu[8] = 801;
      } else {
        Menu[8]++;
      }
      WebSocketData = Menu[8];
      notifyClients();
      Serial.println("delay start timer menu = " + String(Menu[8]));
      datapacket2 = Menu[8];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle9") == 0) { // score sync
      Menu[9] = 901;
      WebSocketData = Menu[9];
      notifyClients();
      Serial.println(" menu = " + String(Menu[9]));
      // datapacket2 = 901;
      // datapacket1 = 99;
      // BROADCASTESPNOW = true;
      ClearScores();
      SCORESYNC = true;
      Serial.println("Commencing Score Sync Process");
      ScoreRequestCounter = 0;
    }
    if (strcmp((char*)data, "toggle10") == 0) {
      if (Menu[10] > 1000) {
        Menu[10] = 1000;
      } else {
        Menu[10] = 1001;
      }
      WebSocketData = Menu[10];
      notifyClients();
      Serial.println(" menu = " + String(Menu[10]));
      datapacket2 = Menu[10];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle11") == 0) {
      if (Menu[11] > 1102) {
        Menu[11] = 1101;
      } else {
        Menu[11]++;
      }
      WebSocketData = Menu[11];
      notifyClients();
      Serial.println(" = " + String(Menu[11]));
      datapacket2 = Menu[11];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle12") == 0) {
      if (Menu[12] == 1200) {
        Menu[12] = 1201;
      } else {
        Menu[12] = 1200;
      }
      WebSocketData = Menu[12];
      notifyClients();
      Serial.println("menu = " + String(Menu[12]));
      datapacket2 = Menu[12];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle13") == 0) {
      if (Menu[13] > 1304) {
        Menu[13] = 1301;
      } else {
        Menu[13]++;
      }
      WebSocketData = Menu[13];
      notifyClients();
      Serial.println(" menu = " + String(Menu[11]));
      datapacket2 = Menu[13];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle14s") == 0) {
      Menu[14] = 1401;
      WebSocketData = Menu[14];
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle14e") == 0) {
      Menu[14] = 1400;
      WebSocketData = Menu[14];
      notifyClients();
      Serial.println("menu = " + String(Menu[14]));
      datapacket2 = Menu[14];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle15") == 0) {
      Menu[15] = 1501;
      WebSocketData = Menu[15];
      notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
    if (strcmp((char*)data, "toggle15D") == 0) {
      Menu[15] = 1505;
      WebSocketData = Menu[15];
      notifyClients();
      Serial.println("menu = " + String(Menu[15]));
      datapacket2 = Menu[15];
      datapacket1 = 99;
      BROADCASTESPNOW = true;
      if (!INGAME) {
       incomingData1 = datapacket1;
       incomingData2 = datapacket2;
       ProcessIncomingCommands();      
      }
    }
  }
}
//**************************************************************
void ProcessIncomingCommands() {
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  if (ESPTimeout) {
    ESPTimeout = false;
  }
  int IncomingTeam = 99;
  if (incomingData1 > 199) {
    IncomingTeam = incomingData1 - 200;
  }
  if (incomingData1 == GunID || incomingData1 == 99 || IncomingTeam == SetTeam) { // data checked out to be intended for this player ID or for all players
    digitalWrite(2, HIGH);
    if (incomingData2 < 100) { // setting primary weapon or slot 0
      int b = incomingData2;
      if (INGAME==false){
        if (b == 1) {
          settingsallowed=1; 
          AudioSelection="VA5F";
          SetSlotA=100;
          Serial.println("Weapon Slot 0 set to Manual");
        }
        if (b > 1 && b < 50) {
          GETSLOT1=false;
          GETSLOT0=false;
          SetSlotA=b-1;
          Serial.println("Weapon Slot 0 set"); 
          if(SetSlotA < 10) {
            AudioSelection = ("GN0" + String(SetSlotA));
          }
          if (SetSlotA > 9) {
            AudioSelection = ("GN" + String(SetSlotA));
          }
          if(SetSlotA == 20) {
            AudioSelection = ("VA50");
          }
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 200 && incomingData2 > 100) { // setting seconczry weapon or slot 1
      int b = incomingData2 - 100;
      if (INGAME==false){
        if (b==1) {
          settingsallowed=2; 
          AudioSelection="VA5F"; 
          SetSlotB=100; 
          Serial.println("Weapon Slot 1 set to Manual");
        }
        if (b>1 && b < 50) {
          GETSLOT1=false;
          GETSLOT0=false;
          SetSlotB=b-1; 
          Serial.println("Weapon Slot 1 set"); 
          if(SetSlotB < 10) {
            AudioSelection = ("GN0" + String(SetSlotB));
          }
          if (SetSlotB > 9) {
            AudioSelection = ("GN" + String(SetSlotB));
          }
          if(SetSlotA == 20) {
            AudioSelection = ("VA50");
          }
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 300 && incomingData2 > 200) { // setting player lives
      int b = incomingData2 - 200;
      if (INGAME==false){
        if (b==7) {
          SetLives = 32000; 
          Serial.println("Lives is set to Unlimited"); 
          AudioSelection="VA6V";
        }
        if (b==6) {
          SetLives = 25; 
          Serial.println("Lives is set to 25"); 
          AudioSelection="VA0P";
        }
        if (b==5) {
          SetLives = 15; 
          Serial.println("Lives is set to 15"); 
          AudioSelection="VA0F";
        }
        if (b==4) {
          SetLives = 10; 
          Serial.println("Lives is set to 10"); 
          AudioSelection="VA0A";
        }
        if (b==3) {
          SetLives = 5; 
          Serial.println("Lives is set to 5"); 
          AudioSelection="VA05";
        }
        if (b==2) {
          SetLives = 3; 
          Serial.println("Lives is set to 3"); 
          AudioSelection="VA03";
        }
        if (b==1) {
          SetLives = 1; 
          Serial.println("Lives is set to 1"); 
          AudioSelection="VA01";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 400 && incomingData2 > 300) { // setting game time
      int b = incomingData2 - 300;
      if (INGAME==false){
        if (b==1) {
          SetTime=60000; 
          Serial.println("Game time set to 1 minute");
          AudioSelection="VA0V";
        }
        if (b==2) {
          SetTime=300000;
          Serial.println("Game time set to 5 minute");
          AudioSelection="VA2S";
        }
        if (b==3) {
          SetTime=600000; 
          Serial.println("Game time set to 10 minute"); 
          AudioSelection="VA6H";
        }
        if (b==4) {
          SetTime=900000;
          Serial.println("Game time set to 15 minute"); 
          AudioSelection="VA2P";
        }
        if (b==5) {
          SetTime=1200000; 
          Serial.println("Game time set to 20 minute");
          AudioSelection="VA6Q";
        }
        if (b==6) {
          SetTime=1500000;
          Serial.println("Game time set to 25 minute"); 
          AudioSelection="VA6P";
        }
        if (b==7) {
          SetTime=1800000; 
          Serial.println("Game time set to 30 minute");
          AudioSelection="VA0Q";
        }
        if (b==8) {
          SetTime=2000000000; 
          Serial.println("Game time set to Unlimited");
          AudioSelection="VA6V";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 500 && incomingData2 > 400) { // setting outdoor/indoor
      int b = incomingData2 - 400;
      if (INGAME==false){
        if (b==1) {
          SetODMode=0;
          Serial.println("Outdoor Mode On"); 
          AudioSelection="VA4W";
          STEALTH = false;
        }
        if (b==2) {
          SetODMode=1; 
          Serial.println("Indoor Mode On"); 
          AudioSelection="VA3W";
          STEALTH = false;
        }
        if (b==3) {
          SetODMode=1; 
          Serial.println("Stealth Mode On"); 
          AudioSelection="VA60";
          STEALTH = true;
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 600 && incomingData2 > 500) { // setting team modes
      int b = incomingData2 - 500;
      if (GETTEAM) {
        GETTEAM=false;
      }
      if (INGAME==false){
        int A = 0;
        int B = 1;
        int C = 2;
        int D = 3;
        if (b==1) {
          Serial.println("Free For All"); 
          SetTeam=0;
          SetFF=1;
          AudioSelection="VA30";
        }
        if (b==2) {
          Serial.println("Teams Mode Two Teams (odds/evens)");
          if (GunID % 2) {
            SetTeam=0; 
            AudioSelection="VA13";
          } else {
            SetTeam=1; 
            AudioSelection="VA1L";
          }
        }
        if (b==3) {
          Serial.println("Teams Mode Three Teams (every third player)");
          while (A < 64) {
            if (GunID == A) {
              SetTeam=0;
              AudioSelection="VA13";
            }
            if (GunID == B) {
              SetTeam=1;
              AudioSelection="VA1L";
            }
            if (GunID == C) {
              SetTeam=3;
              AudioSelection1="VA27";
            }
            A = A + 3;
            B = B + 3;
            C = C + 3;
            vTaskDelay(1);
          }
          A = 0;
          B = 1;
          C = 2;
        }    
        if (b==4) {
          Serial.println("Teams Mode Four Teams (every fourth player)");
          while (A < 64) {
            if (GunID == A) {
              SetTeam=0;
              AudioSelection="VA13";
            }
            if (GunID == B) {
              SetTeam=1;
              AudioSelection="VA1L";
            }
            if (GunID == C) {
              SetTeam=2;
              AudioSelection="VA1R";
            }
            if (GunID == D) {
              SetTeam=3;
              AudioSelection="VA27";
            }
            A = A + 4;
            B = B + 4;
            C = C + 4;
            D = D + 4;
            vTaskDelay(1);
          }
          A = 0;
          B = 1;
          C = 2;
          D = 3;
        }
        if (b==5) {
          Serial.println("Teams Mode Player's Choice"); 
          settingsallowed=3; 
          SetTeam=100; 
          AudioSelection="VA5E";
        } // this one allows for manual input of settings... each gun will need to select a team now
        AUDIO=true;
        ChangeMyColor = SetTeam; // triggers a gun/tagger color change
      }
    }
    if (incomingData2 < 700 && incomingData2 > 600) { // setting preset or quick game modes
      int b = incomingData2 - 600;
      if (INGAME==false){
        if (b==1) {
          GameMode=1; // Default Mode
          /*
           * Weapon 0 - AMR
           * Weapon 1 - Unarmed
           * Lives - Unlimited
           * Game Time - Unlimited
           * Delay Start - Off
           * Ammunitions - Unlimited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           * Volume - 3
           */
          SetSlotA = 2; // set weapon slot 0 as AMR
          Serial.println("Weapon slot 0 set to AMR");
          SetSlotB = 1; // set weapon slot 1 as unarmed
          Serial.println("Weapon slot 1 set to Unarmed");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=2000000000; // set game time to unlimited
          Serial.println("Game time set to Unlimited");
          DelayStart=0; // set delay start to immediate
          Serial.println("Delay Start Set to Immediate"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=1; // set respon mode to auto
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to Immediate"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          SetVol = 80;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to Defaults");
          AudioSelection="VA5Z";
        }
        if (b==2) { // 5 min F4A AR + ShotGun
          GameMode=2;
          /*
           * Weapon 0 - Assault Rifle
           * Weapon 1 - Shotgun
           * Lives - 5
           * Game Time - 5 minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - Free for all
           * Respawn - 15 seconds Automatic
           * Gender - Male
           * Friendly Fire - On
           * Volume - 5
           */
          SetSlotA = 3; // set weapon slot 0 as Assault Rifle
          Serial.println("Weapon slot 0 set to Assault Rifle");
          SetSlotB = 15; // set weapon slot 1 as Shotgun
          Serial.println("Weapon slot 1 set to Shotgun");
          SetLives = 5; // set lives to 5 minutes
          Serial.println("Lives is set to Unlimited");
          SetTime=300000; // set game time to 5 minutes
          Serial.println("Game time set to 5 minutes");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to 5Min F4A AR + Shotguns"); 
          AudioSelection="VA8I";
        }
        if (b==3) { // 10Min F4A Rndm Weap
          GameMode=3; 
          /*
           * Weapon 0 - Random
           * Weapon 1 - Random
           * Lives - 10
           * Game Time - 10 minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           * Volume - 5
           */
          SetSlotA = random(2, 19); // set weapon slot 0 as Random
          Serial.println("Weapon slot 0 set to Random");
          SetSlotB = random(2, 19); // set weapon slot 1 as random
          Serial.println("Weapon slot 1 set to random");
          SetLives = 10; // set lives to 10 
          Serial.println("Lives is set to 10");
          SetTime=600000; // set game time to 10 minutes
          Serial.println("Game time set to 10 minutes");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to 10 Min F4A Rndm Weapon");
          AudioSelection="VA8I";
        }
        if (b==4) { // SwapBRX
          GameMode=4;
          /*
           * Weapon 0 - PISTOL
           * Weapon 1 - Unarmed
           * Lives - 1
           * Game Time - Unlimited
           * Delay Start - 30 Seconds
           * Ammunitions - Limited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           * Volume - 5
           * 
           * integrate IR protocol for random weapon swap...
           */
          SetSlotA = 20; // set weapon slot 0 as pistol
          Serial.println("Weapon slot 0 set to PISTOL");
          SetSlotB = 1; // set weapon slot 1 as uarmed
          Serial.println("Weapon slot 1 set to unarmed");
          SetLives = 1; // set lives to 1 
          Serial.println("Lives is set to 1");
          SetTime=2000000000; // set game time to unlimited minutes
          Serial.println("Game time set to unlimited");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=1; // set ammunitions to limited mags
          Serial.println("Ammo set to limited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to Battle Royale or SwapBRX"); 
          AudioSelection="VA8J";
        }
        if (b==5) { // Capture the Flag
          GameMode=5; 
          /*
           * Weapon 0 - SMG
           * Weapon 1 - Sniper Rifle
           * Lives - Unlimited
           * Game Time - 15 Minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - Free For All
           * Respawn - Manual Respawn (station)
           * Gender - Male
           * Friendly Fire - Off
           * Volume - 5
           */
          SetSlotA = 16; // set weapon slot 0 as SMG
          Serial.println("Weapon slot 0 set to SMG");
          SetSlotB = 17; // set weapon slot 1 as sniper rifle
          Serial.println("Weapon slot 1 set to sniper rifle");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=900000;
          Serial.println("Game time set to 15 minute");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to Capture the Flag");
          AudioSelection="VA8P";
          AUDIO=true;
        }
        if (b==6) { // Own the Zone
          GameMode=6; 
          /*
           * Weapon 0 - Force Rifle
           * Weapon 1 - Ion Sniper
           * Lives - Unlimited
           * Game Time - 15 Minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Lighting/Ambience - Outdoor mode
           * Teams - No change
           * Respawn - Manual (station)
           * Gender - Male
           * Friendly Fire - Off
           * Volume - 5
           */
          SetSlotA = 9; // set weapon slot 0 as force rifle
          Serial.println("Weapon slot 0 set to force rifle");
          SetSlotB = 10; // set weapon slot 1 as ion sniper rifle
          Serial.println("Weapon slot 1 set to ion sniper rifle");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=900000;
          Serial.println("Game time set to 15 minute");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to Own the Zone");
          AudioSelection="VA93";
          AUDIO=true;
          vTaskDelay(3000);
          
        }
        if (b==7) { // Survival/Infection
          GameMode=7; 
          /*
           * Weapon 0 - Burst Rifle
           * Weapon 1 - medic heal dart with alt fire
           * Lives - Unlimited
           * Game Time - 10
           * Delay Start - 30 sec
           * Ammunitions - ulimited mags
           * Lighting/Ambience - outdoor
           * Teams - manual set (just red and green) human and infected
           * Respawn - Auto 15 seconds
           * Gender - Male
           * Friendly Fire - Off
           * Volume - 5
           */
          SetSlotA = 5; // set weapon slot 0 as burst rifle
          Serial.println("Weapon slot 0 set to burst rifle");
          // change game start sequence to make alt fire a health "perk" instead of "weapon selection/cycle"
          // SetSlotB = 10; // set weapon slot 1 as ion sniper rifle
          // Serial.println("Weapon slot 1 set to ion sniper rifle");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=600000;
          Serial.println("Game time set to 10 minute");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Outdoor Mode On");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds");
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
          Serial.println("Game mode set to Survival / Infection");
          AudioSelection="VA64";
          AUDIO=true;
          vTaskDelay(3000);
        }
        if (b==8) { // Assimilation
          /*
           * Weapon 0 - 
           * Weapon 1 - 
           * Lives - 
           * Game Time - 
           * Delay Start - 
           * Ammunitions - 
           * Lighting/Ambience - 
           * Teams - 
           * Respawn - 
           * Gender - 
           * Friendly Fire - 
           * Volume - 
           */
          GameMode=8; 
          Serial.println("Game mode set to Assimilation");
          AudioSelection="VA9P";
          SetSlotA = 3; // set weapon slot 0 as Assault Rifle
          Serial.println("Weapon slot 0 set to Assault Rifle");
          SetSlotB = 15; // set weapon slot 1 as Shotgun
          Serial.println("Weapon slot 1 set to Shotgun");
          SetLives = 32000; // set lives to 5 minutes
          Serial.println("Lives is set to Unlimited");
          SetTime=900000; // set game time to 15 minutes
          Serial.println("Game time set to 15 minutes");
          DelayStart=30000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetODMode=0; // outdoor mode set
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=0; // friendly fire set to off
          Serial.println("Friendly Fire Off");
          SetVol = 100;
          Serial.println("Volume set to" + String(SetVol));
          VOLUMEADJUST=true;
        }
        if (b==9) { // Gun Game
          GameMode=9;
          Serial.println("Game mode set to Gun Game");
          AudioSelection="VA9T";
        }
        if (b==10) {
          GameMode=10; 
          Serial.println("Game mode set to One Domination"); 
          AudioSelection="VA26";
        }
        if (b==11) {
          GameMode=11; 
          Serial.println("Game mode set to Battle Royale"); 
          AudioSelection = "VA8J";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 800 && incomingData2 > 700) { // setting respawn time
      int b = incomingData2 - 700;
      if (INGAME==false){
        if (b==1) {
          SetRSPNMode=1;
          RespawnTimer=10; 
          Serial.println("Respawn Set to Immediate"); 
          AudioSelection="VA54";
        }
        if (b==2) {
          SetRSPNMode=2; 
          RespawnTimer=15000;
          Serial.println("Respawn Set to 15 seconds"); 
          AudioSelection="VA2Q";
        }
        if (b==3) {
          SetRSPNMode=3;
          RespawnTimer=30000; 
          Serial.println("Respawn Set to 30 seconds"); 
          AudioSelection="VA0R";
        }
        if (b==4) {
          SetRSPNMode=4; 
          RespawnTimer=45000; 
          Serial.println("Respawn Set to 45 seconds"); 
          AudioSelection="VA0T";
        }
        if (b==5) {
         SetRSPNMode=5; 
          RespawnTimer=60000;
          Serial.println("Respawn Set to 60 seconds");
          AudioSelection="VA0V";
        }
        if (b==6) {
          SetRSPNMode=6;
          RespawnTimer=90000;
          Serial.println("Respawn Set to 90 seconds"); 
          AudioSelection="VA0X";
        }
        if (b==7) {
          SetRSPNMode=7; 
          RespawnTimer=120000;
          Serial.println("Respawn Set to 120 seconds");
          AudioSelection="VA0S";
        }
        if (b==8) {
          SetRSPNMode=8; 
          RespawnTimer=150000;
          Serial.println("Respawn Set to 150 seconds");
          AudioSelection="VA0W";
        }
        if (b==9) {
          SetRSPNMode=9;
          RespawnTimer=10; 
          Serial.println("Respawn Set to Manual/Respawn Station");
          AudioSelection="VA9H";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 900 && incomingData2 > 800) { // setting delay start
      int b = incomingData2 - 800;
      if (INGAME==false){
        if (b==1) {
          DelayStart=0;
          Serial.println("Delay Start Set to Immediate"); 
          AudioSelection="VA4T";
        }
        if (b==2) {
          DelayStart=15000;
          Serial.println("Delay Start Set to 15 seconds"); 
          AudioSelection="VA2Q";
        }
        if (b==3) {
          DelayStart=30000; 
          Serial.println("Delay Start Set to 30 seconds");
          AudioSelection="VA0R";
        }
        if (b==4) {
          DelayStart=45000; 
          Serial.println("Delay Start Set to 45 seconds");
          AudioSelection="VA0T";
        }
        if (b==5) {
          DelayStart=60000;
          Serial.println("Delay Start Set to 60 seconds"); 
          AudioSelection="VA0V";
        }
        if (b==6) {        
          DelayStart=90000;
          Serial.println("Delay Start Set to 90 seconds");
          AudioSelection="VA0X";
        }
        if (b==7) {
          DelayStart=300000; 
          Serial.println("Delay Start Set to 5 minutes"); 
          AudioSelection="VA2S";
        }
        if (b==8) {
          DelayStart=600000; 
          Serial.println("Delay Start Set to 10 minutes"); 
          AudioSelection="VA6H";
        }
        if (b==9) {
          DelayStart=900000;
          Serial.println("Delay Start Set to 15 minutes"); 
          AudioSelection="VA2P";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 1000 && incomingData2 > 900) { // Syncing scores
      int b = incomingData2 - 900;
      if (b == 1) {
        Serial.println("Request Recieved to Sync Scoring");
        SyncScores();
        AudioSelection="VA91";
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
      if (b == 2) { // this is an incoming score from a player!
        AccumulateIncomingScores();
      }
    }
    if (incomingData2 < 1100 && incomingData2 > 999) { // Setting Gender
      int b = incomingData2 - 1000;
      if (INGAME==false){
        if (b == 0) {
          SetGNDR=0; 
          Serial.println("Gender set to Male");
          AudioSelection="V3I";
        }
        if (b == 1) {
          SetGNDR=1;
          Serial.println("Gender set to Female");
          AudioSelection="VBI";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 1200 && incomingData2 > 1100) { // Setting ammo
      int b = incomingData2 - 1100;
      if (INGAME==false){
        if (b==3) {
          UNLIMITEDAMMO=3; 
          Serial.println("Ammo set to unlimited rounds"); 
          AudioSelection="VA6V";
        }
        if (b==2) {
          UNLIMITEDAMMO=2;
          Serial.println("Ammo set to unlimited magazies"); 
          AudioSelection="VA6V";
        }
        if (b==1) {
          UNLIMITEDAMMO=1; 
          Serial.println("Ammo set to limited"); 
          AudioSelection="VA14";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 1300 && incomingData2 > 1199) { // Setting friendly fire
      int b = incomingData2 - 1200;
      if (INGAME==false){
        if (b==0) {
          SetFF=0; 
          Serial.println("Friendly Fire Off");
          AudioSelection="VA31";
        }
        if (b==1) {
          SetFF=1;
          Serial.println("Friendly Fire On"); 
          AudioSelection="VA32";
        }
        AUDIO=true;
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 < 1400 && incomingData2 > 1299) { // Setting volume
      int b = incomingData2 - 1300;
      if (INGAME==false){
       if (b == 1) {
         SetVol = 60;
         AudioSelection="VA01";
         Serial.println("Volume set to" + String(SetVol));
       }
       if (b == 2) {
         SetVol = 70;
         AudioSelection="VA02";
         Serial.println("Volume set to" + String(SetVol));
       }
       if (b == 3) {
         SetVol = 80;
         AudioSelection="VA03";
         Serial.println("Volume set to" + String(SetVol));
       }
       if (b == 4) {
         SetVol = 90;
         AudioSelection="VA04";
         Serial.println("Volume set to" + String(SetVol));
       }
       if (b == 5) {
         SetVol = 100;
         AudioSelection="VA05";
         Serial.println("Volume set to" + String(SetVol));
       }
       if (b == 6) {
         SetVol = 20;
         AudioSelection="VA05";
         Serial.println("Volume set to" + String(SetVol));
       }
       VOLUMEADJUST=true;
       AUDIO=true;
       if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
     }
   }
   if (incomingData2 < 1500 && incomingData2 > 1399) { // Starting/Ending Game
     int b = incomingData2 - 1400;
     if (b==0) {
       if (INGAME){
         GAMEOVER=true; 
         Serial.println("ending game");
       }
     }
     if (b==1) {
       if (!INGAME){
         GAMESTART=true; 
         Serial.println("starting game");
         if (SetTeam == 100) {
           SetTeam=Team;
         }
       }
     }
   }
   if (incomingData2 < 1600 && incomingData2 > 1499) { // Misc. Debug
     int b = incomingData2 - 1500;
     if (b==1) {
       INITJEDGE = true;
       //InitializeJEDGE();
       ChangeMyColor = SetTeam; // triggers a gun/tagger color change
     }
     if (b==2) {
       ESP.restart();
     }
     if (b==3) {
       int fakescorecounter = 0;
       while (fakescorecounter < 64) {
         PlayerKillCount[fakescorecounter] = random(25);
         Serial.println("Player " + String(fakescorecounter) + " kills: " + String(PlayerKillCount[fakescorecounter]));
         fakescorecounter++;
         vTaskDelay(1);
       }
       fakescorecounter = 0;
       while (fakescorecounter < 4) {
         TeamKillCount[fakescorecounter] = random(100);
         Serial.println("Team " + String(fakescorecounter) + " kills: " + String(TeamKillCount[fakescorecounter]));
         fakescorecounter++;
       }
       if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
     }
     if (b==4) {
       Serial.println("received request to report in, waiting in line...");
       //int terminaldelay = GunID * 1000;
       //delay(terminaldelay);
       //terminal.print(GunName);
       //terminal.println(" is connected to the JEDGE server");
       //terminal.flush();
       Serial.println("Reported to Server");
       if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
     }
     if (b==5) { //1505
       Serial.println("OTA Update Mode");
       INITIALIZEOTA = true;
       if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
     }
   }
   if (incomingData2 < 1700 && incomingData2 > 1599) { // Player-Team Selector
     int b = incomingData2 - 1600;
     int teamselector = 99;
     if (b == 68) {
       INGAME = false;
       ChangeMyColor = 7; // triggers a gun/tagger color change
     } else {
      if (b > 63) {
        teamselector = b - 64;
      }
      if (teamselector < 99) {
        if (teamselector == SetTeam) {
          INGAME = false;
          ChangeMyColor = 7; // triggers a gun/tagger color change
        } else {
          INGAME = true;
          ChangeMyColor = 9; // triggers a gun/tagger color change
        }
      } else {
        if (b == GunID) {
          INGAME = false;
          ChangeMyColor = 7; // triggers a gun/tagger color change
        } else {
          INGAME = true;
          ChangeMyColor = 9; // triggers a gun/tagger color change
        }
      }
     }
    }    
    if (incomingData2 == 32000) { // if true, this is a kill confirmation
      MyKills++; // accumulate one kill count
      AudioSelection="VN8"; // set audio play for "kill confirmed
      AUDIO=true;
      AudioDelay = 2000;
      AudioPreviousMillis = millis();
      if (GameMode == 4) {
        // apply weapon pick up from kill  
        SpecialWeapon = incomingData3;
        // Enable special weapon load by select button
        SELECTCONFIRM = true; // enables trigger for select button
        SPECIALWEAPONLOADOUT = true;
        SelectButtonTimer = millis();
        AudioSelection1="VA9B"; // set audio playback to "press select button"   
      }
    }
    digitalWrite(2, LOW);
  }  
}
//******************************************************************************************
void InitializeJEDGE() {
  Serial.println();
  Serial.println();
  Serial.println("*****************************************************");
  Serial.println("************* Initializing JEDGE 2.0 ****************");
  Serial.println("*****************************************************");
  Serial.println();
  Serial.println("Waiting for BRX to Boot...");
  /*
  int forfun = 5;
  while (forfun > 0) { 
    delay(1000);
    Serial.print(String(forfun) + ", ");
    forfun--;
  }
  */
  //sendString("$_BAT,4162,*"); // test for gen3
  //sendString("$PLAYX,0,*"); // test for gen3
  //sendString("$VERSION,*"); // test for gen3
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  Serial.println();
  Serial.println("JEDGE is taking over the BRX");
  //sendString("$CLEAR,*"); // clears any brx settings
  sendString("$STOP,*"); // starts the callsign mode up
  //delay(300);
  //sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  sendString("$PLAY,VA20,4,6,,,,,*"); // says "connection established"
  sendString("$HLED,0,0,,,10,,*"); // test for gen3
  sendString("$GLED,0,0,0,0,10,,*"); // test for gen3
}
//**********************************************************************************************
void SyncScores() {
  if (FAKESCORE) { // CHECK IF WE ARE DOING A TEST ONLY FOR DATA SENDING
     CompletedObjectives = random(25);
     int playercounter = 0;
     while (playercounter < 64) {
      PlayerKillCount[playercounter] = random(20);
      playercounter++;
     }
     PlayerKillCount[GunID] = 0;
     TeamKillCount[0] = PlayerKillCount[0] + PlayerKillCount[1] + PlayerKillCount[2];
     TeamKillCount[1] = PlayerKillCount[3] + PlayerKillCount[4] + PlayerKillCount[5];
     TeamKillCount[2] = PlayerKillCount[6] + PlayerKillCount[7] + PlayerKillCount[8];
     TeamKillCount[3] = PlayerKillCount[9] + PlayerKillCount[10] + PlayerKillCount[11];
     if (GunID < 3) {
      SetTeam = 0;
     }
     if (GunID < 6 && GunID > 2) {
       SetTeam = 1;
     }
     if (GunID < 9 && GunID > 5) {
      SetTeam = 2;
     } 
     if (GunID > 8) {
      SetTeam = 3;
     }
  }
  // create a string that looks like this: 
  // (Player ID, token 0), (Player Team, token 1), (Player Objective Score, token 2) (Team scores, tokens 3-8), (player kill counts, tokens 9-72 
  String ScoreData = String(GunID)+","+String(SetTeam)+","+String(CompletedObjectives)+","+String(TeamKillCount[0])+","+String(TeamKillCount[1])+","+String(TeamKillCount[2])+","+String(TeamKillCount[3])+","+String(TeamKillCount[4])+","+String(TeamKillCount[5])+","+String(PlayerKillCount[0])+","+String(PlayerKillCount[1])+","+String(PlayerKillCount[2])+","+String(PlayerKillCount[3])+","+String(PlayerKillCount[4])+","+String(PlayerKillCount[5])+","+String(PlayerKillCount[6])+","+String(PlayerKillCount[7])+","+String(PlayerKillCount[8])+","+String(PlayerKillCount[9])+","+String(PlayerKillCount[10])+","+String(PlayerKillCount[11])+","+String(PlayerKillCount[12])+","+String(PlayerKillCount[13])+","+String(PlayerKillCount[14])+","+String(PlayerKillCount[15])+","+String(PlayerKillCount[16])+","+String(PlayerKillCount[17])+","+String(PlayerKillCount[18])+","+String(PlayerKillCount[19])+","+String(PlayerKillCount[20])+","+String(PlayerKillCount[21])+","+String(PlayerKillCount[22])+","+String(PlayerKillCount[23])+","+String(PlayerKillCount[24])+","+String(PlayerKillCount[25])+","+String(PlayerKillCount[26])+","+String(PlayerKillCount[27])+","+String(PlayerKillCount[28])+","+String(PlayerKillCount[29])+","+String(PlayerKillCount[30])+","+String(PlayerKillCount[31])+","+String(PlayerKillCount[32])+","+String(PlayerKillCount[33])+","+String(PlayerKillCount[34])+","+String(PlayerKillCount[35])+","+String(PlayerKillCount[36])+","+String(PlayerKillCount[37])+","+String(PlayerKillCount[38])+","+String(PlayerKillCount[39])+","+String(PlayerKillCount[40])+","+String(PlayerKillCount[41])+","+String(PlayerKillCount[42])+","+String(PlayerKillCount[43])+","+String(PlayerKillCount[44])+","+String(PlayerKillCount[45])+","+String(PlayerKillCount[46])+","+String(PlayerKillCount[47])+","+String(PlayerKillCount[48])+","+String(PlayerKillCount[49])+","+String(PlayerKillCount[50])+","+String(PlayerKillCount[51])+","+String(PlayerKillCount[52])+","+String(PlayerKillCount[53])+","+String(PlayerKillCount[54])+","+String(PlayerKillCount[55])+","+String(PlayerKillCount[56])+","+String(PlayerKillCount[57])+","+String(PlayerKillCount[58])+","+String(PlayerKillCount[59])+","+String(PlayerKillCount[60])+","+String(PlayerKillCount[61])+","+String(PlayerKillCount[62])+","+String(PlayerKillCount[63]);
  Serial.println("Sending the following Score Data to Server");
  Serial.println(ScoreData);
  datapacket1 = incomingData3;
  datapacket4 = ScoreData;
  datapacket2 = 902;
  BROADCASTESPNOW = true;
  //bridge1.virtualWrite(V100, ScoreData); // sending the whole string from esp32
  Serial.println("Sent Score data to Server");
  //reset sent scores:
  //Serial.println("resetting all scores");
  //CompletedObjectives = 0;
  //int teamcounter = 0;
  //while (teamcounter < 6) {
    //TeamKillCount[teamcounter] = 0;
    //teamcounter++;
    //vTaskDelay(1);
  //}
  //int playercounter = 0;
  //while (playercounter < 64) {
    //PlayerKillCount[playercounter] = 0;
    //playercounter++;
    //vTaskDelay(1);
  //}
  //Serial.println("Scores Reset");  
}
//******************************************************************************************

// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
void sendString(String value) {
  const char * c_string = value.c_str();
  Serial.print("sending: ");
  if (GunGeneration == 1) {
    for (int i = 0; i < value.length() / 1; i++) { // creates a for loop
      Serial1.println(c_string[i]); // sends one value at a time to gen1 brx
      delay(5); // this is an added delay needed for the way the brx gen1 receives data (brute force method because i could not match the comm settings exactly)
      Serial.print(c_string[i]);
    }
    Serial.println();
  }
  if (GunGeneration > 1) {
    int matchingdelay = value.length() * 5; // this just creates a temp value for how long the string is multiplied by 5 milliseconds
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    Serial1.println(c_string); // sending the string to brx gen 2
    Serial.println(c_string);
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
  if(var == "WEAP0"){
    if (SetSlotA == 0){
      return "Manual Selection";
    }
    if (SetSlotA == 1) {
      return "Unarmed";
    }
    if (SetSlotA > 1) {
      return "AMR";
    }
  }
  if(var == "WEAP1"){
    if (SetSlotB == 0){
      return "Manual Selection";
    }
    if (SetSlotB == 1) {
      return "Unarmed";
    }
    if (SetSlotB > 1) {
      return "AMR";
    }
  }
  if(var == "LVS0"){
    if (SetLives == 32000){
      return "Unlimited";
    }
    if (SetLives == 1) {
      return "1";
    }
    if (SetLives == 5) {
      return "5";
    }
  }
  if(var == "GTM"){
    if (SetTime == 2000000000){
      return "Unlimited";
    }
  }
  if(var == "OUTDR"){
    if (SetODMode == 0){
      return "Outdoor Mode";
    }
  }
  if(var == "TEAM"){
    if (Menu[5] == 501){
      return "Free For All";
    }
  }
  if(var == "GMODE"){
    if (Menu[6] == 601){
      return "Defaults";
    }
  }
  if(var == "RSPWNMODE"){
    if (Menu[7] == 701){
      return "Auto - No Delay";
    }
  }
  if(var == "DLYSTM"){
    if (Menu[8] == 801){
      return "Immediate";
    }
  }
  if(var == "SYNC"){
    if (Menu[9] == 901){
      return "to Sync";
    }
  }
  if(var == "GENDER"){
    if (Menu[10] == 1000){
      return "Male";
    } else { return "Female";}
  }
  if(var == "AMMUNITION"){
    if (Menu[11] == 1002){
      return "Infinite Mags";
    }
  }
  if(var == "GENDER"){
    if (Menu[12] == 1201){
      return "On";
    }
  }
  if(var == "VOLUME"){
    if (Menu[13] == 1303){
      return "Three";
    }
  }
  if(var == "NGAME"){
    if (Menu[14] == 1400){
      return "Standby";
    } else { return "In Game";}
  }
  if(var == "JEDGE"){
    if (Menu[15] == 1500){
      return "Standby";
    } else { return "Initialized";}
  }
}

void InitAP() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(GunName, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void InitOTA() {
  Serial.println("Starting OTA Update");
  WiFi.softAPdisconnect (true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(OTAssid, OTApassword);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(2000);
  }
  Serial.println("connected to OTA Wifi");
  ArduinoOTA.setHostname(GunName);
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ENABLEOTAUPDATE = true;
  Serial.println("OTA setup complete");
}


//**********************************************************************************************
// ****************************************************************************************
/*Analyzing incoming tag to determine if a perk action is needed
        space 2 is for he type of tag recieved/shot. almost all are 0 and im thinking other types are medic etc.
        space 3 is the player id, in this case it was player 0 hitting player 1.
        space 4 is the team id, this was team 0 or red team
        space 5 is the damage dealt to the player, this took 13 hit points off the player 1
        space 6 is "is critical" if critical a damage multiplier would apply, rare.
        space 7 is "power", not sure what that does.*/
void TagPerks() {
  Serial.println("IR Direction: "+String(tokenStrings[1]));
  Serial.println("Bullet Type: "+String(tokenStrings[2]));
  Serial.println("Player ID: "+String(tokenStrings[3]));
  Serial.println("Team: "+String(tokenStrings[4]));
  Serial.println("Damage: "+String(tokenStrings[5]));
  Serial.println("Is Critical: "+String(tokenStrings[6]));
  Serial.println("Power: "+String(tokenStrings[7]));
  //  Checking for special tags
  if (tokenStrings[2] == "15" && tokenStrings[6] == "1" && tokenStrings[7] == "0") { // we just determined that it is a respawn tag
      Serial.println("Received a respawn tag");
      if (tokenStrings[4].toInt() == SetTeam) {
        Serial.println("Respawn Tag Team is a Match");
        if (PENDINGRESPAWNIR) { // checks if we are awaiting a respawn signal
          RESPAWN = true; // triggers a respawn
          PENDINGRESPAWNIR = false; // closing the process of checking for a respawining tag and enables all other normal in game functions
        }
      } else {
        Serial.println("Respawn Tag Team does not match");        
      }
  }
  if (tokenStrings[2] == "1" && tokenStrings[7] == "1") { // we determined that it is a custom tag for game interaction
    Serial.println("received a game interaction tag");
    // now that we know weve got a special tag, what to do with it...
    // respawning tag
    if (tokenStrings[5] == "1") { // We have a random weapon selection tag
      Serial.println("SWAPBRX Tag Received, Randomizing Weapon/Perk");
      //if (tokenStrings[3].toInt() == GunID) {
        // player ID is a match, therefore this tagger deserves the loot...
        swapbrx(); // Running SwapBRX (Battle Royale Loot Box object)
      //}
    }
    /*
    // Own The Zone Tag received
    if (tokenStrings[7] == "1") { // just determined that this is an objective tag - is critical, bullet 0, power 1, damage 1
      Serial.println("Received an Objective Tag");
      if (GameMode == 2) { // we're playing capture the flag
        if (tokenStrings[4].toInt() == SetTeam) {
          Serial.println("Objective Tag Team is a Match");
          AudioSelection1="VA2K"; // setting audio play to notify we captured the flag
          AUDIO1 = true; // enabling play back of audio
          HASFLAG = true; // set condition that we have flag to true
          SpecialWeapon = 99; // loads a new weapon that will deposit flag into base
          SPECIALWEAPON = true; // enables program to load the new weapon
        } else {
          Serial.println("Objective tag team does not match");
        }
        } else {
          CompletedObjectives++; // added one point to player objectives
          AudioSelection1="VAR"; // set an announcement "good job team"
          AUDIO1=true; // enabling BLE Audio Announcement Send
        }
    }
    */
  }
}
//******************************************************************************************
// sets and sends game settings based upon the stored settings
void SetFFOutdoor() {
  // token one of the following command is free for all, 0 is off and 1 is on
  if(SetODMode == 0 && SetFF == 1) {
    sendString("$GSET,1,0,1,0,1,0,50,1,*");
    }
  if(SetODMode == 1 && SetFF == 1) {
    sendString("$GSET,1,1,1,0,1,0,50,1,*");
    }
  if(SetODMode == 1 && SetFF == 0) {
    sendString("$GSET,0,1,1,0,1,0,50,1,*");
    }
  if(SetODMode == 0 && SetFF == 0) {
    sendString("$GSET,0,0,1,0,1,0,50,1,*");
    }
}
//******************************************************************************************
// sets and sends gun type to slot 0 based upon stored settings
void weaponsettingsA() {
  if (SLOTA != 100) {
    SetSlotA=SLOTA; 
    SLOTA=100;
  }
  if (UNLIMITEDAMMO==3){ // unlimited rounds
    if(SetSlotA == 1) {
      Serial.println("Weapon 0 set to Unarmed"); 
      sendString("$WEAP,0,*");
    } // cleared out weapon 0
    if(SetSlotA == 2) {
      Serial.println("Weapon 0 set to AMR"); 
      sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,0,1400,10,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
    if(SetSlotA == 3) {
      Serial.println("Weapon 0 set to Assault Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,0,1400,10,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
    if(SetSlotA == 4) {
      Serial.println("Weapon 0 set to Bolt Rifle"); 
      sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,0,2000,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
    if(SetSlotA == 5) {
      Serial.println("Weapon 0 set to BurstRifle"); 
      sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,0,1700,10,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
    if(SetSlotA == 6) {
      Serial.println("Weapon 0 set to ChargeRifle"); 
      sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,0,2500,10,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
    if(SetSlotA == 7) {
      Serial.println("Weapon 0 set to Energy Launcher");
      sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,0,1400,10,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
    if(SetSlotA == 8) {
      Serial.println("Weapon 0 set to Energy Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,0,2400,10,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
    if(SetSlotA == 9) {
      Serial.println("Weapon 0 set to Force Rifle");
      sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,0,1700,10,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
    if(SetSlotA == 10) {
      Serial.println("Weapon 0 set to Ion Sniper"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,0,2000,10,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
    if(SetSlotA == 11) {
      Serial.println("Weapon 0 set to Laser Cannon"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,0,2000,10,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
    if(SetSlotA == 12) {
      Serial.println("Weapon 0 set to Plasma Sniper");
      sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,0,2000,10,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
    if(SetSlotA == 13) {
      Serial.println("Weapon 0 set to Rail Gun");
      sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,0,2400,10,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
    if(SetSlotA == 14) {
      Serial.println("Weapon 0 set to Rocket Launcher");
      sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,0,1200,10,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
    if(SetSlotA == 15) {
      Serial.println("Weapon 0 set to Shotgun");
      sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,0,400,10,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
    if(SetSlotA == 16) {
      Serial.println("Weapon 0 set to SMG");
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,0,2500,10,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
    if(SetSlotA == 17) {
      Serial.println("Weapon 0 set to Sniper Rifle");
      sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,0,1700,10,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
    if(SetSlotA == 18) {
      Serial.println("Weapon 0 set to Stinger");
      sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,0,1700,10,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
    if(SetSlotA == 19) {
      Serial.println("Weapon 0 set to Suppressor"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,0,2000,10,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
    if(SetSlotA == 20) {
      Serial.println("Weapon 0 set to Pistol"); 
      sendString("$WEAP,0,,35,0,0,8,0,,,,,,,,225,850,9,27,400,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,27,20,,*");
    }
  }
  if (UNLIMITEDAMMO==2) { // unlimited mags
    if(SetSlotA == 1) {
      Serial.println("Weapon 0 set to Unarmed");
      sendString("$WEAP,0,*");
    } // cleared out weapon 0
    if(SetSlotA == 2) {
      Serial.println("Weapon 0 set to AMR"); 
      sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,32768,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,9999999,75,,*");
    }
    if(SetSlotA == 3) {
      Serial.println("Weapon 0 set to Assault Rifle");
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*");
    }
    if(SetSlotA == 4) {
      Serial.println("Weapon 0 set to Bolt Rifle");
      sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,32768,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,9999999,75,,*");
    }
    if(SetSlotA == 5) {
      Serial.println("Weapon 0 set to BurstRifle"); 
      sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,32768,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,9999999,75,,*");
    }
    if(SetSlotA == 6) {
      Serial.println("Weapon 0 set to ChargeRifle"); 
      sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*");
    }
    if(SetSlotA == 7) {
      Serial.println("Weapon 0 set to Energy Launcher");
      sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,32768,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,9999999,75,,*");
    }
    if(SetSlotA == 8) {
      Serial.println("Weapon 0 set to Energy Rifle"); 
      sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,32768,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,9999999,75,,*");
    }
    if(SetSlotA == 9) {
      Serial.println("Weapon 0 set to Force Rifle"); 
      sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,32768,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,9999999,75,,*");
    }
    if(SetSlotA == 10) {
      Serial.println("Weapon 0 set to Ion Sniper"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*");
    }
    if(SetSlotA == 11) {
      Serial.println("Weapon 0 set to Laser Cannon"); 
      sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,32768,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,9999999,75,,*");
    }
    if(SetSlotA == 12) {
      Serial.println("Weapon 0 set to Plasma Sniper"); 
      sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,32768,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,9999999,75,40,*");
    }
    if(SetSlotA == 13) {
      Serial.println("Weapon 0 set to Rail Gun"); 
      sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,32768,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,9999999,75,,*");
    }
    if(SetSlotA == 14) {
      Serial.println("Weapon 0 set to Rocket Launcher"); 
      sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,75,30,*");
    }
    if(SetSlotA == 15) {
      Serial.println("Weapon 0 set to Shotgun"); 
      sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,32768,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,9999999,75,30,*");
    }
    if(SetSlotA == 16) {
      Serial.println("Weapon 0 set to SMG"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,32768,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,9999999,75,,*");
    }
    if(SetSlotA == 17) {
      Serial.println("Weapon 0 set to Sniper Rifle");
      sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,32768,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,9999999,75,,*");
    }
    if(SetSlotA == 18) {
      Serial.println("Weapon 0 set to Stinger");
      sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,32768,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,9999999,75,,*");
    }
    if(SetSlotA == 19) {
      Serial.println("Weapon 0 set to Suppressor"); 
      sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,32768,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,9999999,75,,*");
    }
    if(SetSlotA == 20) {
      Serial.println("Weapon 0 set to Pistol"); 
      sendString("$WEAP,0,,35,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,20,,*");
    }
  }
  if (UNLIMITEDAMMO==1) { // limited mags
      if(SetSlotA == 1) {
        Serial.println("Weapon 0 set to Unarmed");
        sendString("$WEAP,0,*");
        } // cleared out weapon 0
      if(SetSlotA == 2) {
        Serial.println("Weapon 0 set to AMR");
        sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
        }
      if(SetSlotA == 3) {
        Serial.println("Weapon 0 set to Assault Rifle"); 
        sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
        }
      if(SetSlotA == 4) {
        Serial.println("Weapon 0 set to Bolt Rifle"); 
        sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
        }
      if(SetSlotA == 5) {
        Serial.println("Weapon 0 set to BurstRifle"); 
        sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
        }
      if(SetSlotA == 6) {
        Serial.println("Weapon 0 set to ChargeRifle");
        sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
        }
      if(SetSlotA == 7) {
        Serial.println("Weapon 0 set to Energy Launcher"); 
        sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
        }
      if(SetSlotA == 8) {
        Serial.println("Weapon 0 set to Energy Rifle"); 
        sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
        }
      if(SetSlotA == 9) {
        Serial.println("Weapon 0 set to Force Rifle"); 
        sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
        }
      if(SetSlotA == 10) {
        Serial.println("Weapon 0 set to Ion Sniper");
        sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
        }
      if(SetSlotA == 11) {
        Serial.println("Weapon 0 set to Laser Cannon"); 
        sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
        }
      if(SetSlotA == 12) {
        Serial.println("Weapon 0 set to Plasma Sniper");
        sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
        }
      if(SetSlotA == 13) {
        Serial.println("Weapon 0 set to Rail Gun");
        sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
        }
      if(SetSlotA == 14) {
        Serial.println("Weapon 0 set to Rocket Launcher"); 
        sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
        }
      if(SetSlotA == 15) {
        Serial.println("Weapon 0 set to Shotgun"); 
        sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
        }
      if(SetSlotA == 16) {
        Serial.println("Weapon 0 set to SMG"); 
        sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
        }
      if(SetSlotA == 17) {
        Serial.println("Weapon 0 set to Sniper Rifle"); 
        sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
        }
      if(SetSlotA == 18) {
        Serial.println("Weapon 0 set to Stinger");
        sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
        }
      if(SetSlotA == 19) {
        Serial.println("Weapon 0 set to Suppressor"); 
        sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
      }
      if(SetSlotA == 20) {
        Serial.println("Weapon 0 set to Pistol"); 
        sendString("$WEAP,0,,35,0,0,8,0,,,,,,,,225,850,9,27,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,27,20,,*");
      }
  }
}
//*****************************************************************************************
// In Game Weapon Upgrade
void LoadSpecialWeapon() {
  
  if(SpecialWeapon == 1) {
    Serial.println("Weapon 1 set to Unarmed"); 
    sendString("$WEAP,1,*");
    } // cleared out weapon 0
  if(SpecialWeapon == 2) {
    Serial.println("Weapon 1 set to AMR"); 
    sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
  if(SpecialWeapon == 3) {
    Serial.println("Weapon 1 set to Assault Rifle");
    sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
  if(SpecialWeapon == 4) {
    Serial.println("Weapon 1 set to Bolt Rifle"); 
    sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
  if(SpecialWeapon == 5) {
    Serial.println("Weapon 1 set to BurstRifle");
    sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
  if(SpecialWeapon == 6) {
    Serial.println("Weapon 1 set to ChargeRifle");
    sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
  if(SpecialWeapon == 7) {
    Serial.println("Weapon 1 set to Energy Launcher"); 
    sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 8) {
    Serial.println("Weapon 1 set to Energy Rifle");
    sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
  if(SpecialWeapon == 9) {
    Serial.println("Weapon 1 set to Force Rifle");
    sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
  if(SpecialWeapon == 10) {
    Serial.println("Weapon 1 set to Ion Sniper"); 
    sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
  if(SpecialWeapon == 11) {
    Serial.println("Weapon 1 set to Laser Cannon"); 
    sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
  if(SpecialWeapon == 12) {
    Serial.println("Weapon 1 set to Plasma Sniper"); 
    sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
  if(SpecialWeapon == 13) {
    Serial.println("Weapon 1 set to Rail Gun"); 
    sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 14) {
    Serial.println("Weapon 1 set to Rocket Launcher"); 
    sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
  if(SpecialWeapon == 15) {
    Serial.println("Weapon 1 set to Shotgun"); 
    sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
  if(SpecialWeapon == 16) {
    Serial.println("Weapon 1 set to SMG"); 
    sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
  if(SpecialWeapon == 17) {
    Serial.println("Weapon 1 set to Sniper Rifle"); 
    sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
  if(SpecialWeapon == 18) {
    Serial.println("Weapon 1 set to Stinger"); 
    sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
  if(SpecialWeapon == 19) {
    Serial.println("Weapon 1 set to Suppressor"); 
    sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  if(SpecialWeapon == 99) {
    Serial.println("Flag Capture, Gun becomes the flag"); 
    sendString("$WEAP,1,*"); 
    sendString("$WEAP,1,*");
    sendString("$WEAP,1,*"); 
    sendString("$BMAP,3,2,,,,,*");
    sendString("$WEAP,3,,100,2,0,0,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,32768,75,,*");
  }
  PreviousSpecialWeapon = SpecialWeapon;
  SpecialWeapon == 0; // reset the special weapon to zero after use     
}

//******************************************************************************************

// sets and sends gun for slot 0 based upon stored settings
void weaponsettingsB() {
  if (SLOTB != 100) {
    SetSlotB=SLOTB; 
    SLOTB=100;
  }
  if (UNLIMITEDAMMO==3){
    if(SetSlotB == 1) {
      Serial.println("Weapon 1 set to Unarmed"); 
      sendString("$WEAP,1,*");
    } // cleared out weapon 0
    if(SetSlotB == 2) {
      Serial.println("Weapon 1 set to AMR"); 
      sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,0,1400,10,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
    if(SetSlotB == 3) {
      Serial.println("Weapon 1 set to Assault Rifle"); 
      sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,0,1400,10,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
    if(SetSlotB == 4) {
      Serial.println("Weapon 1 set to Bolt Rifle"); 
      sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,0,2000,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
    if(SetSlotB == 5) {
      Serial.println("Weapon 1 set to BurstRifle");
      sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,0,1700,10,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
    if(SetSlotB == 6) {
      Serial.println("Weapon 1 set to ChargeRifle"); 
      sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,0,2500,10,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
    if(SetSlotB == 7) {
      Serial.println("Weapon 1 set to Energy Launcher"); 
      sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,0,1400,10,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
    if(SetSlotB == 8) {
      Serial.println("Weapon 1 set to Energy Rifle"); 
      sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,0,2400,10,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
    if(SetSlotB == 9) {
      Serial.println("Weapon 1 set to Force Rifle"); 
      sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,0,1700,10,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
    if(SetSlotB == 10) {
      Serial.println("Weapon 1 set to Ion Sniper"); 
      sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,0,2000,10,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
    if(SetSlotB == 11) {
      Serial.println("Weapon 1 set to Laser Cannon"); 
      sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,0,2000,10,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
    if(SetSlotB == 12) {
      Serial.println("Weapon 1 set to Plasma Sniper");
      sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,0,2000,10,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
    if(SetSlotB == 13) {
      Serial.println("Weapon 1 set to Rail Gun"); 
      sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,0,2400,10,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
    if(SetSlotB == 14) {
      Serial.println("Weapon 1 set to Rocket Launcher");
      sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,0,1200,10,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
    if(SetSlotB == 15) {
      Serial.println("Weapon 1 set to Shotgun");
      sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,0,400,10,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
    if(SetSlotB == 16) {
      Serial.println("Weapon 1 set to SMG"); 
      sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,0,2500,10,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
    if(SetSlotB == 17) {
      Serial.println("Weapon 1 set to Sniper Rifle"); 
      sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,0,1700,10,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
    if(SetSlotB == 18) {
      Serial.println("Weapon 1 set to Stinger"); 
      sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,0,1700,10,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
    if(SetSlotB == 19) {
      Serial.println("Weapon 1 set to Suppressor"); 
      sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,0,2000,10,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  }  
  if (UNLIMITEDAMMO==2) {
      if(SetSlotB == 1) {
        Serial.println("Weapon 1 set to Unarmed"); 
        sendString("$WEAP,1,*");
        } // cleared out weapon 0
      if(SetSlotB == 2) {
        Serial.println("Weapon 1 set to AMR"); 
        sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,32768,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,9999999,75,,*");
        }
      if(SetSlotB == 3) {
        Serial.println("Weapon 1 set to Assault Rifle");
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*");
        }
      if(SetSlotB == 4) {
        Serial.println("Weapon 1 set to Bolt Rifle"); 
        sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,32768,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,9999999,75,,*");
        }
      if(SetSlotB == 5) {
        Serial.println("Weapon 1 set to BurstRifle"); 
        sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,32768,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,9999999,75,,*");
        }
      if(SetSlotB == 6) {
        Serial.println("Weapon 1 set to ChargeRifle"); 
        sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*");
        }
      if(SetSlotB == 7) {
        Serial.println("Weapon 1 set to Energy Launcher"); 
        sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,32768,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,9999999,75,,*");
        }
      if(SetSlotB == 8) {
        Serial.println("Weapon 1 set to Energy Rifle");
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,32768,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,9999999,75,,*");
        }
      if(SetSlotB == 9) {
        Serial.println("Weapon 1 set to Force Rifle");
        sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,32768,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,9999999,75,,*");
        }
      if(SetSlotB == 10) {
        Serial.println("Weapon 1 set to Ion Sniper"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*");
        }
      if(SetSlotB == 11) {
        Serial.println("Weapon 1 set to Laser Cannon");
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,32768,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,9999999,75,,*");
        }
      if(SetSlotB == 12) {
        Serial.println("Weapon 1 set to Plasma Sniper");
        sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,32768,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,9999999,75,40,*");
        }
      if(SetSlotB == 13) {
        Serial.println("Weapon 1 set to Rail Gun");
        sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,32768,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,9999999,75,,*");
        }
      if(SetSlotB == 14) {
        Serial.println("Weapon 1 set to Rocket Launcher"); 
        sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,75,30,*");
        }
      if(SetSlotB == 15) {
        Serial.println("Weapon 1 set to Shotgun"); 
        sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,32768,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,9999999,75,30,*");
        }
      if(SetSlotB == 16) {
        Serial.println("Weapon 1 set to SMG"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,32768,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,9999999,75,,*");
        }
      if(SetSlotB == 17) {
        Serial.println("Weapon 1 set to Sniper Rifle"); 
        sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,32768,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,9999999,75,,*");
        }
      if(SetSlotB == 18) {
        Serial.println("Weapon 1 set to Stinger");
        sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,32768,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,9999999,75,,*");
        }
      if(SetSlotB == 19) {
        Serial.println("Weapon 1 set to Suppressor"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,32768,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,9999999,75,,*");
        }
    }
    if (UNLIMITEDAMMO==1) {
      if(SetSlotB == 1) {
        Serial.println("Weapon 1 set to Unarmed");
        sendString("$WEAP,1,*");
        } // cleared out weapon 0
      if(SetSlotB == 2) {
        Serial.println("Weapon 1 set to AMR"); 
        sendString("$WEAP,1,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
        }
      if(SetSlotB == 3) {
        Serial.println("Weapon 1 set to Assault Rifle"); 
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
        }
      if(SetSlotB == 4) {
        Serial.println("Weapon 1 set to Bolt Rifle");
        sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
        }
      if(SetSlotB == 5) {
        Serial.println("Weapon 1 set to BurstRifle"); 
        sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
        }
      if(SetSlotB == 6) {
        Serial.println("Weapon 1 set to ChargeRifle");
        sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
        }
      if(SetSlotB == 7) {
        Serial.println("Weapon 1 set to Energy Launcher");
        sendString("$WEAP,1,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
        }
      if(SetSlotB == 8) {
        Serial.println("Weapon 1 set to Energy Rifle"); 
        sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
        }
      if(SetSlotB == 9) {
        Serial.println("Weapon 1 set to Force Rifle"); 
        sendString("$WEAP,1,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
        }
      if(SetSlotB == 10) {
        Serial.println("Weapon 1 set to Ion Sniper"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
        }
      if(SetSlotB == 11) {
        Serial.println("Weapon 1 set to Laser Cannon"); 
        sendString("$WEAP,1,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
        }
      if(SetSlotB == 12) {
        Serial.println("Weapon 1 set to Plasma Sniper"); 
        sendString("$WEAP,1,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
        }
      if(SetSlotB == 13) {
        Serial.println("Weapon 1 set to Rail Gun"); 
        sendString("$WEAP,1,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
        }
      if(SetSlotB == 14) {
        Serial.println("Weapon 1 set to Rocket Launcher"); 
        sendString("$WEAP,1,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
        }
      if(SetSlotB == 15) {
        Serial.println("Weapon 1 set to Shotgun"); 
        sendString("$WEAP,1,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
        }
      if(SetSlotB == 16) {
        Serial.println("Weapon 1 set to SMG"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
        }
      if(SetSlotB == 17) {
        Serial.println("Weapon 1 set to Sniper Rifle"); 
        sendString("$WEAP,1,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
        }
      if(SetSlotB == 18) {
        Serial.println("Weapon 1 set to Stinger"); 
        sendString("$WEAP,1,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
        }
      if(SetSlotB == 19) {
        Serial.println("Weapon 1 set to Suppressor"); 
        sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
        }
    }
}

void weaponsettingsC() {
  if(SetSlotC == 0){
    Serial.println("Weapon 3 set to Unarmed"); 
    sendString("$WEAP,3,*"); // Set weapon 3 as unarmed
  }
  if(SetSlotC == 1){
    Serial.println("Weapon 3 set to Respawn"); 
    sendString("$WEAP,3,1,90,15,0,6,100,,,,,,,,1000,100,1,0,0,10,15,100,100,,0,0,,,,,,,,,,,,,,1,0,20,,*"); // Set weapon 3 as respawn
  }
  if(SetSlotD == 0){
    Serial.println("Weapon 5 set to Unarmed"); 
    sendString("$WEAP,5,*"); // Set weapon 5 as unarmed
  }
  if(SetSlotD == 1) {
    Serial.println("Weapon 5 set to Medic");
    sendString("$WEAP,5,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 2) {
    Serial.println("Weapon 5 set to Sheilds");
    sendString("$WEAP,5,1,90,2,1,70,0,,,,,,,,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 3) {
    Serial.println("Weapon 5 set to Armor");
    sendString("$WEAP,5,1,90,3,0,70,0,,,,,,,,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 4) {
    Serial.println("Weapon 5 set to Tear Gas");
    sendString("$WEAP,5,2,100,11,1,1,0,,,,,,1,80,1400,50,10,0,0,10,11,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,10,9999999,75,30,*");
  }
  if(SetSlotD == 5) {
    Serial.println("Weapon 5 set to Medic at Range");
    sendString("$WEAP,5,2,100,1,0,20,0,,,,,,20,80,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 6) {
    Serial.println("Weapon 5 set to Armor at Range");
    sendString("$WEAP,5,2,100,2,1,30,0,,,,,,40,80,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 7) {
    Serial.println("Weapon 5 set to Sheilds at range");
    sendString("$WEAP,5,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
}
//****************************************************************************************
//************** This sends Settings to Tagger *******************************************
//****************************************************************************************
// loads all the game configuration settings into the gun
void gameconfigurator() {
  Serial.println("Running Game Configurator based upon recieved inputs");
  sendString("$CLEAR,*");
  sendString("$START,*");
  SetFFOutdoor();
  playersettings();
  /* SIR commands to configure the incoming hits recognition
   *  $SIR,0,0,,1,0,0,1,,*  Assault Rifle Energy Rifle  Ion Sniper  Laser Cannon  Plasma Sniper Shotgun SMG Stinger Suppressor
   *  $SIR,0,1,,36,0,0,1,,* Force Rifle Sniper Rifle         
   *  $SIR,0,3,,37,0,0,1,,* AMR Bolt Rifle  BurstRifle              
   *  $SIR,1,0,H29,10,0,0,1,,*  Respawn and Add HP                  
   *  $SIR,2,1,VA8C,11,0,0,1,,* Add Shields                      
   *  $SIR,3,0,VA16,13,0,0,1,,* Add Armor                
   *  $SIR,6,0,H02,1,0,90,1,40,*  Rail Gun                    
   *  $SIR,8,0,,38,0,0,1,,* ChargeRifle               
   *  $SIR,9,3,,24,10,0,,,* Energy Launcher               
   *  $SIR,10,0,X13,1,0,100,2,60,*  Rocket Launcher               
   *  $SIR,11,0,VA2,28,0,0,1,,* Tear Gas - not working yet                        
   *  $SIR,13,0,H50,1,0,0,1,,*  Energy Blade 
   *  $SIR,13,1,H57,1,0,0,1,,*  Rifle Bash                  
   *  $SIR,13,3,H49,1,0,100,0,60,*  WarHammer               
   *  $SIR,15,0,H29,10,0,0,1,,* Add HP Or Respawns Player
   */
  sendString("$SIR,0,0,,1,0,0,1,,*"); // standard weapon, damages shields then armor then HP
  sendString("$SIR,0,1,,36,0,0,1,,*"); // force rifle, sniper rifle ?pass through damage?
  sendString("$SIR,0,3,,37,0,0,1,,*"); // bolt rifle, burst rifle, AMR
  sendString("$SIR,1,0,,10,0,0,1,,*"); // adds HP with this function
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*"); // adds shields
  sendString("$SIR,3,0,VA16,13,0,0,1,,*"); // adds armor
  sendString("$SIR,6,0,H02,1,0,90,1,40,*"); // rail gun
  sendString("$SIR,8,0,,38,0,0,1,,*"); // charge rifle
  sendString("$SIR,9,3,,24,10,0,,,*"); // energy launcher
  sendString("$SIR,10,0,X13,1,0,100,2,60,*"); // rocket launcher
  sendString("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
  sendString("$SIR,13,1,H57,1,0,0,1,,*"); // energy blade
  sendString("$SIR,13,0,H50,1,0,0,1,,*"); // rifle bash
  sendString("$SIR,13,3,H49,1,0,100,0,60,*"); // war hammer
  sendString("$SIR,1,1,,8,0,100,0,60,*"); // no damage just vibe on impact - use for SWAPBRX, and other game features based upon player ID 0-63
  // protocol for SWAPBRX: bullet type: 1, power: 1, damage: 1, player: 0, team: 2, critical: 0; just look for bullet, power, player for trigger weapon swap
  //sendString("$SIR,15,1,,50,0,0,1,,*");
  //sendString("$SIR,14,0,,14,0,0,1,,*");
  // The $SIR functions above can be changed to incorporate more in game IR based functions (health boosts, armor, shields) or customized over BLE to support game functions/modes
  //delay(500);
  sendString("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
  sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
  sendString("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
  sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5
  sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4
  sendString("$BMAP,5,3,,,,,*"); // Sets the right button as weapon 3, using for perks/respawns etc. 
  sendString("$BMAP,8,4,,,,,*"); // sets the gyro as weapon 4
  Serial.println("Finished Game Configuration set up");
}

//****************************************************************************************
//************************ This starts a game *******************************************
//****************************************************************************************
// this starts a game
void delaystart() {
  Serial.println("Starting Delayed Game Start");
  sendString("$HLED,,6,,,,,*"); // changes headset to end of game
  // this portion creates a hang up in the program to delay until the time is up
  bool RunInitialDelay = true;
  long actualdelay = 0; // used to count the actual delay versus desired delay
  long delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
  long initialdelay = DelayStart - 3000; // this will be used to track current time in milliseconds and compared to the start of the delay
  int audibletrigger = 0; // used as a trigger once we get to 10 seconds left
  if (DelayStart > 10) {
    Serial.println("Delayed Start timer = " + String(DelayStart));
    while(initialdelay > actualdelay) {
      actualdelay = millis() - delaybeginning;
      Serial.println("Actual Delay Counter = " + String(actualdelay));
      vTaskDelay(1);
    }
    sendString("$PLAY,VA80,4,6,,,,,*");
    Serial.println("Running 3 second countdown");
    actualdelay = 0;
    delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
    while(3000 > actualdelay) {
      actualdelay = millis() - delaybeginning;
      Serial.println("Actual Delay Counter = " + String(actualdelay));
      vTaskDelay(1);
    }
  }
  sendString("$PLAY,VA81,4,6,,,,,*"); // plays the .. nevermind
  sendString("$PLAYX,0,*");
  sendString("$SPAWN,,*");
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  Serial.println("Delayed Start Complete, should be in game play mode now");
  GameStartTime=millis();
  GAMEOVER=false;
  INGAME=true;
  sendString("$PLAY,VA1A,4,6,,,,,*"); // plays the .. let the battle begin
  //reset sent scores:
  Serial.println("resetting all scores");
  CompletedObjectives = 0;
  int teamcounter = 0;
  while (teamcounter < 6) {
    TeamKillCount[teamcounter] = 0;
    teamcounter++;
    vTaskDelay(1);
  }
  int playercounter = 0;
  while (playercounter < 64) {
    PlayerKillCount[playercounter] = 0;
    playercounter++;
    vTaskDelay(1);
  }
  Serial.println("Scores Reset");
}


//******************************************************************************************
// sets and sends player settings to gun based upon configuration settings
void playersettings() {
  // token 2 is player id or gun id other tokens were interested in modification
  // are 2 for team ID, 3 for max HP, 4 for max Armor, 5 for Max Shields, 
  // 6 is critical damage bonus
  // We really are only messing with Gender and Team though
  // Gender is determined by the audio call outs listed, tokens 9 and on
  // male is default as 0, female is 1
  // health = 45; armor = 70; shield =70;
  if (GameMode == 7 && Team == 3) { // infected team
    sendString("$PSET,"+String(GunID)+",3,67,105,105,50,,H44,JAD,V33,V3I,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
  } else {
    if(SetGNDR == 0) {
      sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,V33,V3I,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
    else {
      sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,VB3,VBI,VBC,VBG,VBE,VB7,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
  }
}
//******************************************************************************************
//******************************************************************************************
// ends game... thats all
void gameover() {
  Serial.println("disabling bool triggers");
  GAMEOVER = false;
  INGAME=false;
  COUNTDOWN1=false;
  COUNTDOWN2=false;
  COUNTDOWN3=false;
  SwapBRXCounter = 0;
  SWAPBRX = false;
  SELECTCONFIRM = false;
  SPECIALWEAPONLOADOUT = false;
  PreviousSpecialWeapon = 0;
  Serial.println("sending game exit commands");
  sendString("$STOP,*"); // stops everything going on
  sendString("$CLEAR,*"); // clears out anything stored for game settings
  sendString("$PLAY,VS6,4,6,,,,,*"); // says game over
  Serial.println("Game Over Object Complete");
}
//******************************************************************************************
// as the name says... respawn a player!
void respawnplayer() {
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  sendString("$HLOOP,2,1200,*"); // flashes the headset lights in a loop
  // this portion creates a hang up in the program to delay until the time is up
  if (SetRSPNMode == 9) {
    gameconfigurator();
  }
  if (RespawnTimer < RespawnTimerMax) {
    RespawnTimer = 5000 * Deaths;
  }
  long actualdelay = 0; // used to count the actual delay versus desired delay
  long delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
  long delaycounter = millis(); // this will be used to track current time in milliseconds and compared to the start of the delay
  int audibletrigger = 0; // used as a trigger once we get to 10 seconds left
  if (RespawnTimer > 10) {
  while (RespawnTimer > actualdelay) { // this creates a sub loop in the object to keep doing the following steps until this condition is met... actual delay is the same as planned delay
    delaycounter = millis(); // sets the delay clock to the current progam timer
    actualdelay = delaycounter - delaybeginning; // calculates how long weve been delaying the program/start
    if ((RespawnTimer-actualdelay) < 3000) {
      audibletrigger++;
      } // a check to start adding value to the audible trigger
    if (audibletrigger == 1) {
      sendString("$PLAY,VA80,4,6,,,,,*"); 
      Serial.println("playing 3 second countdown");
    } // this can only happen once so it doesnt keep looping in the program we only play it when trigger is equal to 1
    vTaskDelay(1);
  }
  }
  Serial.println("Respawning Player");
  //sendString("$WEAP,0,*"); // cleared out weapon 0
  //sendString("$WEAP,1,*"); // cleared out weapon 1
  //sendString("$WEAP,4,*"); // cleared out melee weapon
  //sendString("$WEAP,3,*"); // cleared out melee weapon
  int hloopreset = 0;
  while (hloopreset < 10) {
    sendString("$HLOOP,0,0,*"); // stops headset flashing
    delay(200);
    hloopreset++;
  }
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  sendString("$GLED,,,,5,,,*"); // changes headset to tagged out color
  sendString("$SPAWN,,*"); // respawns player back in game
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  Serial.println("Player Respawned");
  RESPAWN = false;
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  if (STEALTH){
      sendString("$GLED,,,,5,,,"); // TURNS OFF SIDE LIGHTS
  }
}
//****************************************************************************************
// this function will be used when a player is eliminated and needs to respawn off of a base
// or player signal to respawn them... a lot to think about still on this and im using auto respawn 
// for now untill this is further thought out and developed
void ManualRespawnMode() {
  delay(4000);
  sendString("$VOL,0,0,*"); // adjust volume to default
  sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,,,,,,,,,,,,,,,,,*");
  sendString("$STOP,*"); // this is essentially ending the game for player... need to rerun configurator or use a different command
  sendString("$SPAWN,,*"); // this is playing "get some as part of a respawn
  sendString("$HLOOP,2,1200,*"); // this puts the headset in a loop for flashing
  delay(1500);
  sendString("$GLED,,,,,,,*"); // changes headset to tagged out color
  //AudioSelection1 = "VA54"; // audio that says respawn time
  //AUDIO1 = true;
  PENDINGRESPAWNIR = true;
  MANUALRESPAWN = false;
}
//****************************************************************************
void Audio() {
  if (AudioSelection1=="VA20") {
    sendString("$CLEAR,*");
    sendString("$START,*");
  }
  if (AUDIO) {
    if (AudioDelay > 1) {
      if (millis() - AudioPreviousMillis > AudioDelay) {
        sendString("$PLAY,"+AudioSelection+",4,6,,,,,*");
        AUDIO = false;
        AudioSelection  = "NULL";
        AudioDelay = 0; // reset the delay for another application
      }
    } else {
      sendString("$PLAY,"+AudioSelection+",4,6,,,,,*");
      AUDIO = false;
      AudioSelection  = "NULL";
    }
  }
  if (AUDIO1) {
    sendString("$PLAY,"+AudioSelection1+",4,6,,,,,*");
    AUDIO1=false; 
    AudioSelection1 = "NULL";
    }
}

//********************************************************************************************
// This main game object
void MainGame() {
  if (INITJEDGE) {
    INITJEDGE = false;
    InitializeJEDGE();
  }
  if (ChangeMyColor != CurrentColor) { // need to change colors
    ChangeColors();
  }
  if (AUDIO) {
    Audio();
  }
  if (AUDIO1) {
    Audio();
  }
  if (GAMEOVER) { // checks if something triggered game over (out of lives, objective met)
    gameover(); // runs object to kick player out of game
    sendString("$HLOOP,0,0,*"); // stops headset flashing
  }
  if (GAMESTART) {
    // need to turn off the trigger audible selections if a player didnt press alt fire to confirm
    sendString("$PLAY,VAQ,4,6,,,,,*"); // says "lets move out"
    GETSLOT0=false; 
    GETSLOT1=false; 
    GETTEAM=false;
    gameconfigurator(); // send all game settings, weapons etc. 
    delaystart(); // enable the start based upon delay selected
    GAMESTART = false; // disables the trigger so this doesnt loop/
    PlayerLives=SetLives; // sets configured lives from settings received
    GameTimer=SetTime; // sets timer from input of esp8266
    MyKills = 0;
    if (GameTimer > 60000) {
      COUNTDOWN1 = true;
      COUNTDOWN2 = false;
      COUNTDOWN3 = false;
    } else {
      COUNTDOWN3 = true;
      COUNTDOWN1 = false;
      COUNTDOWN2 = false;
    }
    Deaths = 0;
    if (STEALTH){
      sendString("$GLED,,,,5,,,"); // TURNS OFF SIDE LIGHTS
    }
  }
  if (!INGAME) {
    if (settingsallowed1==3) {
      Serial.println("Team Settings requested"); 
      delay(250);
      GETTEAM=true;
      GETSLOT0=false;
      GETSLOT1=false; 
      settingsallowed1=0;
    }
    if (settingsallowed1==1) {
        Serial.println("Weapon Slot 0 Requested"); 
        delay(250);
        GETSLOT0=true; 
        GETSLOT1=false;
        GETTEAM=false; 
        settingsallowed1=0;
        }
    if (settingsallowed1==2) {
      Serial.println("Weapon Slot 1 Requested"); 
      delay(250); 
      GETSLOT1=true;
      GETSLOT0=false;
      GETTEAM=false; 
      settingsallowed1=0;
    }
    if (settingsallowed>0) {
      Serial.println("manual settings requested"); 
      settingsallowed1=settingsallowed;
      settingsallowed = 0;
    } // this is triggered if a manual option is required for game settings    
    if (VOLUMEADJUST) {
      VOLUMEADJUST=false;
      sendString("$VOL,"+String(SetVol)+",0,*"); // sets max volume on gun 0-100 feet distance
    }    
  }
  // In game checks and balances to execute actions for in game functions
  if (INGAME) {
    long ActualGameTime = millis() - GameStartTime;
    long GameTimeRemaining = GameTimer - ActualGameTime;
    if (SELECTCONFIRM) {
      Serial.println("SELECTCONFIRM = TRUE");
      if(millis() - SelectButtonTimer > 1500) {
        Serial.println("select button timer is less than actual game time minus 1500 and audio selection is VA9B");
        if (AudioSelection1 == "VA9B") {
          Serial.print("enabling audio1 trigger");
          AUDIO1=true; 
        }
      }
      if(millis() - SelectButtonTimer >= 11500) {
        SELECTCONFIRM = false; // out of time, deactivate ability to confirm
        Serial.println("SELECTCONFIRM is now false because actual ActualGameTime minus 10000 is greater than SelectButtonTimer");
        SPECIALWEAPONLOADOUT = false; // disabling ability to load the special weapon
      }
    }
    if (PERK) {
      if (millis() - SelectButtonTimer >= AudioPerkTimer) {
        PERK = false;
        sendString("$PLAY,"+AudioPerk+",4,6,,,,,*");
      }
    }
    if (COUNTDOWN1) {
      if (GameTimeRemaining < 120001) {
        AudioSelection1="VSD";
        AUDIO1=true; 
        COUNTDOWN1=false; 
        COUNTDOWN2=true;
        Serial.println("2 minutes remaining");
      } // two minute warning
    }
    if (COUNTDOWN2) {
      if (GameTimeRemaining < 60001) {
        AudioSelection1="VSA"; 
        AUDIO1=true;
        COUNTDOWN2=false; 
        COUNTDOWN3=true; 
        Serial.println("1 minute remaining");
      } // one minute warning
    }
    if (COUNTDOWN3) {
      if (GameTimeRemaining < 10001) {
        AudioSelection1="VA83"; 
        AUDIO1=true; 
        COUNTDOWN3=false;
        Serial.println("ten seconds remaining");
      } // ten second count down
    }
    if (ActualGameTime > GameTimer) {
      GAMEOVER=true; 
      Serial.println("game time expired");
    }
    if (RESPAWN) { // checks if auto respawn was triggered to respawn a player
      respawnplayer(); // respawns player
    }
    if (MANUALRESPAWN) { // checks if manual respawn was triggered to respawn a player
      ManualRespawnMode();
    }
    if (LOOT) {
      LOOT = false;
      swapbrx();
    }
  }
}
//**************************************************************************
// run SWAPTX style weapon swap as from my youtube funny video:
void swapbrx() {
  // need to check if going to restock ammor or not:
  int getammo = random(1,GetAmmoOdds);
  if (getammo == GetAmmoOdds) { // current special weapon restock of ammo enabled
    SpecialWeapon = PreviousSpecialWeapon; // makes special weapon equal to whatever the last special weapon was set to
    LoadSpecialWeapon(); // reloads the last special weapon again to refresh ammo to original maximums
    sendString("$PLAY,VA14,4,6,,,,,*");
  }
  // need to randomize the weapon pick up
  SpecialWeapon = random(2, totalweapons); // randomizing between all weapons with total weapons count
  Serial.println("Special Weapon Load Out = " + String(SpecialWeapon));
  // need to add to counter to increase power for pistol
  if (SwapBRXCounter < 100) {
    SwapBRXCounter = SwapBRXCounter + 10; // this is used to increase the critical chance strike and improve your weapon power.
    Serial.println("Set SwapBRXCounter to : " + String(SwapBRXCounter));
    sendString("$WEAP,0,,35,0,0,8," + String(SwapBRXCounter) + ",,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,20,,*");
  }
  Serial.println("Weapon Slot 0 set"); 
  if(SpecialWeapon < 10) {
    AudioSelection = ("GN0" + String(SpecialWeapon));
  }
  if (SpecialWeapon > 9) {
    AudioSelection = ("GN" + String(SpecialWeapon));
  }
  sendString("$PLAY,"+AudioSelection+",4,6,,,,,*"); // plays the gun id callout
  AudioSelection = "NULL";
  // Enable special weapon load by select button
  SELECTCONFIRM = true; // enables trigger for select button
  SPECIALWEAPONLOADOUT = true;
  SelectButtonTimer = millis();
  AudioSelection1="VA9B"; // set audio playback to "press select button"
  // randomize perk pick up
  int statboost = random(1,10);
  Serial.println("Random Perk Statboost = " + String(statboost));
  if (statboost == 10) { // full health restore plus shields 10% chance
    sendString("$LIFE,70,70,45,1,*");
    AudioPerk = "VA1Q";
    PERK = true;
  }
  if (statboost == 8 || statboost == 9) { // get shields only 20% chance
    sendString("$LIFE,70,,,1,*");
    AudioPerk = "VA5K";
    PERK = true;
  }
  if (statboost == 6 || statboost == 7) { // Armor replenish 20% chance
    sendString("$LIFE,,70,,,*");
    AudioPerk = "VA16";
    PERK = true;
  }
}
//**************************************************************
// this bad boy captures any ble data from gun and analyzes it based upon the
// protocol predecessor, a lot going on and very important as we are able to 
// use the ble data from gun to decode brx protocol with the gun, get data from 
// guns health status and ammo reserve etc.
void ProcessBRXData() {
  // lets process the data just received from the BRX:
  char *ptr = strtok((char*)readStr.c_str(), ",");
  int index = 0;
  while (ptr != NULL) {
    tokenStrings[index] = ptr;
    index++;
    ptr = strtok(NULL, ",");  // takes a list of delimiters
  }
  Serial.println("We received: " + readStr);   
  Serial.println("We have found " + String(index ) + " tokens from the received data");
  // trouble shooting Gen3 Taggers
  WRITETOTERMINAL = true;
  TerminalInput = readStr;
  int stringnumber = 0;
  while (stringnumber < index) {
    Serial.print("Token " + String(stringnumber) + ": ");
    Serial.println(tokenStrings[stringnumber]);
    stringnumber++;
  }
  // If gen3, we get some differing starting variable. to fix, lets see if it exists
  // then correct gen 3 nonsense to look like gen1/2 comms
  if (tokenStrings[0] == "$!DFP") { // if this is the case (gen three data received)
    Serial.println("found gen3 data");
    int fixgen3data0 = 1;
    int fixgen3data1 = 2;
    tokenStrings[0] = ("$" + tokenStrings[1]);
    while(stringnumber > fixgen3data1) {
      tokenStrings[fixgen3data0] = tokenStrings[fixgen3data1];
      fixgen3data0++;
      fixgen3data1++;
    }
    Serial.println("changed serial data from brx to match gen1/2 data packets");
    TerminalInput = "Modified Gen3 Data to Match Gen 1/2";
    WRITETOTERMINAL = true;
  }
    /* 
     *  Mapping of the BLE buttons pressed
     *  Trigger pulled: $BUT,0,1,*
     *  Tirgger Released: $BUT,0,0,*
     *  Alt fire pressed:  $BUT,1,1,*
     *  Alt fire released:  $BUT,1,0,*
     *  Reload pulled:  $BUT,2,1,*
     *  Reload released:  $BUT,2,0,*
     *  select button pressed:  $BUT,3,1,*
     *  select button released:  $BUT,3,0,*
     *  left button pressed:  $BUT,4,1,*
     *  left button released:  $BUT,4,0,*
     *  right button pressed:  $BUT,5,1,*
     *  right button released:  $BUT,5,0,*
     */
    if (tokenStrings[0] == "$BUT") {
      if (tokenStrings[1] == "0") {
        if (tokenStrings[2] == "1") {
          Serial.println("Trigger pulled"); // as indicated this is the trigger
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Trigger Released"); // goes without sayin... you let go of the trigger
          // upon release of a trigger, team settings can be changed if the proper allowance is in place
          if (GETTEAM){ // used for configuring manual team selection
            if (GameMode == 7) { // survival teams
              if (Team==0 && STATUSCHANGE == false) {
                Team=3; 
                STATUSCHANGE=true; 
                AudioSelection1="VA3Y"; 
                AUDIO1=true;
                Serial.println("team changed to Infected");
                }          
              if (Team==3 && STATUSCHANGE == false) {
                Team=0; 
                AudioSelection1="VA3T"; 
                AUDIO1=true;
                Serial.println("team changed to Human");
                }
               STATUSCHANGE = false;
              } else {
                if (Team==5) {
                  Team=0; 
                  STATUSCHANGE=true; 
                  AudioSelection1="VA13"; 
                  AUDIO1=true;
                  Serial.println("team changed from 5 to 0");
                }          
                if (Team==4) {
                  Team=5; 
                  AudioSelection1="VA2Y"; 
                  AUDIO1=true; 
                  Serial.println("team changed from 4 to 5");
                } // foxtrot team
                if (Team==3) {
                  Team=0; 
                  STATUSCHANGE=true; 
                  AudioSelection1="VA13"; 
                  AUDIO1=true; 
                  Serial.println("team changed from 3 to 0");
                } // echo team
                if (Team==2) {
                  Team=3; 
                  AudioSelection1="VA27"; 
                  AUDIO1=true; 
                  Serial.println("team changed from 2 to 3");
                  } // delta team
                if (Team==1) {
                  Team=2; 
                  AudioSelection1="VA1R"; 
                  AUDIO1=true; 
                  Serial.println("team changed from 1 to 2");
                  } // charlie team
                if (Team==0 && STATUSCHANGE==false) {
                  Team=1; 
                  AudioSelection1="VA1L"; 
                  AUDIO1=true; Serial.println("team changed from 0 to 1");
                } // bravo team        
                STATUSCHANGE=false;
                ChangeMyColor = Team; // triggers a gun/tagger color change
              }
          }
          if (GETSLOT0){ // used for configuring manual team selection
            if (SLOTA==19) {
              SLOTA=1; 
              STATUSCHANGE=true; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon changed from 19 to 1");
              }          
            if (SLOTA==18) {
              SLOTA=19; 
              AudioSelection1="GN19"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 18 to 19");
              } // 
            if (SLOTA==17) {
              SLOTA=18; 
              AudioSelection1="GN18"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 17 to 18");
              } // 
            if (SLOTA==16) {
              SLOTA=17; 
              AudioSelection1="GN17"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 16 to 17");
              } // 
            if (SLOTA==15) {
              SLOTA=16; 
              AudioSelection1="GN16"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 15 to 16");
              } //        
            if (SLOTA==14) {
              SLOTA=15; 
              AudioSelection1="GN15"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 14 to 15");
              } // 
            if (SLOTA==13) {
              SLOTA=14; 
              AudioSelection1="GN14"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 13 to 14");
              } // 
            if (SLOTA==12) {
              SLOTA=13; 
              AudioSelection1="GN13"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 12 to 13");
              } // 
            if (SLOTA==11) {
              SLOTA=12; 
              AudioSelection1="GN12"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 11 to 12");
              } //         
            if (SLOTA==10) {
              SLOTA=11; 
              AudioSelection1="GN11"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 10 to 11");
              } // 
            if (SLOTA==9) {
              SLOTA=10; 
              AudioSelection1="GN10"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 9 to 10");
              } // 
            if (SLOTA==8) {
              SLOTA=9; 
              AudioSelection1="GN09"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 8 to 9");
              } // 
            if (SLOTA==7) {
              SLOTA=8; 
              AudioSelection1="GN08"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 7 to 8");
              } // 
            if (SLOTA==6) {
              SLOTA=7; 
              AudioSelection1="GN07"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 6 to 7");
              } // 
            if (SLOTA==5) {
              SLOTA=6; 
              AudioSelection1="GN06"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 5 to 6");
              } // 
            if (SLOTA==4) {
              SLOTA=5; 
              AudioSelection1="GN05"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 4 to 5");
              } //         
            if (SLOTA==3) {
              SLOTA=4; 
              AudioSelection1="GN04"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 3 to 4");
              } // 
            if (SLOTA==2) {
              SLOTA=3; 
              AudioSelection1="GN03"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 2 to 3");
              } // 
            if (SLOTA==1 && STATUSCHANGE==false) {
              SLOTA=2; 
              AudioSelection1="GN02"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 1 to 2");
              } // 
            if (SLOTA==100) {
              SLOTA=1; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon 0 changed from 0 to 1");
              } //        
            STATUSCHANGE=false;
            TAGGERUPDATE=true;
          }
          if (GETSLOT1){ // used for configuring manual team selection
            if (SLOTB==19) {
              SLOTB=1; 
              STATUSCHANGE=true; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon changed from 19 to 1");
              }          
            if (SLOTB==18) {
              SLOTB=19; 
              AudioSelection1="GN19";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 18 to 19");
              } // 
            if (SLOTB==17) {
              SLOTB=18; 
              AudioSelection1="GN18"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 17 to 18");
              } // 
            if (SLOTB==16) {
              SLOTB=17; 
              AudioSelection1="GN17";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 16 to 17");
              } // 
            if (SLOTB==15) {
              SLOTB=16; 
              AudioSelection1="GN16"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 15 to 16");
              } //        
            if (SLOTB==14) {
              SLOTB=15; 
              AudioSelection1="GN15"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 14 to 15");
              } // 
            if (SLOTB==13) {
              SLOTB=14; 
              AudioSelection1="GN14"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 13 to 14");
              } // 
            if (SLOTB==12) {
              SLOTB=13; 
              AudioSelection1="GN13"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 12 to 13");
              } // 
            if (SLOTB==11) {
              SLOTB=12; 
              AudioSelection1="GN12"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 11 to 12");
              } //         
            if (SLOTB==10) {
              SLOTB=11; 
              AudioSelection1="GN11"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 10 to 11");
              } // 
            if (SLOTB==9) {
              SLOTB=10; 
              AudioSelection1="GN10"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 9 to 10");
              } // 
            if (SLOTB==8) {
              SLOTB=9; 
              AudioSelection1="GN09";
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 8 to 9");
              } // 
            if (SLOTB==7) {
              SLOTB=8; 
              AudioSelection1="GN08"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 7 to 8");
              } // 
            if (SLOTB==6) {
              SLOTB=7; 
              AudioSelection1="GN07"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 6 to 7");
              } // 
            if (SLOTB==5) {
              SLOTB=6; 
              AudioSelection1="GN06"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 5 to 6");
              } // 
            if (SLOTB==4) {
              SLOTB=5; 
              AudioSelection1="GN05"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 4 to 5");
              } //         
            if (SLOTB==3) {
              SLOTB=4; 
              AudioSelection1="GN04"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 3 to 4");
              } // 
            if (SLOTB==2) {
              SLOTB=3; 
              AudioSelection1="GN03"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 2 to 3");
              } // 
            if (SLOTB==1 && STATUSCHANGE==false) {
              SLOTB=2; 
              AudioSelection1="GN02"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 1 to 2");
              } // 
            if (SLOTB==100) {
              SLOTB=1; 
              AudioSelection1="GN01"; 
              AUDIO1=true; 
              Serial.println("Weapon 1 changed from 0 to 1");
              } //        
            STATUSCHANGE=false;
            TAGGERUPDATE=true;
          }
        }
      }
      // alt fire button pressed and released section
      if (tokenStrings[1] == "1") {
        if (tokenStrings[2] == "1") {
          Serial.println("Alt fire pulled"); // yeah.. you pushed the red/yellow button
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Alt fire Released"); // now you released the button
          if (GETTEAM) {
            GETTEAM=false; 
            AudioSelection1="VAO"; 
            AUDIO1=true;
            }
          if (GETSLOT0) {
            GETSLOT0=false; 
            AudioSelection1="VAO"; 
            AUDIO1=true;
            }
          if (GETSLOT1) {
            GETSLOT1=false;
            AudioSelection1="VAO"; 
            AUDIO1=true;
            }
        }
      }
      // charge or reload handle pulled and released section
      if (tokenStrings[1] == "2") {
        if (tokenStrings[2] == "1") {
          Serial.println("charge handle pulled"); // as indicated this is the handle
        }
        if (tokenStrings[2] == "0") {
          Serial.println("Charge handle Released"); // goes without sayin... you let go of the handle
          // upon release of a handle, following steps occur
          //if(UNLIMITEDAMMO==2) { // checking if unlimited ammo is on or not
            //if (CurrentWeapSlot == 0) {
              //OutofAmmoA=true; // here is the variable for weapon slot 0
              //Serial.println("Weapon Slot 0 is out of ammo, enabling weapon reload for weapon slot 0");
            //}
            //if (CurrentWeapSlot == 1) {
               //OutofAmmoB=true; // trigger variable for weapon slot 1
               //Serial.println("Weapon Slot 1 is out of ammo, enabling weapon reload for weapon slot 1");
            //}
          //}
          }
      }
      // select button pressed
      if (tokenStrings[1] == "3") {
        if (tokenStrings[2] == "0") {
          Serial.println("select button depressed"); // goes without sayin... you let go of the handle
          // upon release of the button, following steps occur
          if(SELECTCONFIRM) { // checking if a process is awaiting the select button
            SELECTCONFIRM = false; // disables this trigger
            if(SPECIALWEAPONLOADOUT) {
              SPECIALWEAPONLOADOUT = false;
              LoadSpecialWeapon();
            }
          }
          }
      }
      // we can do more for left right select but yeah... maybe another time 
    }
    
    // here is where we use the incoming ir to the guns sensors to our benefit in many ways
    // we can use it to id the player landing a tag and every aspect of that tag
    // but why stop there? we can use fabricated tags to set variables on the esp32 for programming game modes!
    if (tokenStrings[0] == "$HIR") {
      /*space 1 is the direction the tag came from or what sensor was hit, in above example sensor 4
        space 2 is for he type of tag recieved/shot. this is 0-15 for bullet type
        space 3 is the player id of the person sending the tag
        space 4 is the team id, 
        space 5 is the damage of the tag
        space 6 is "is critical" if critical a damage multiplier would apply, rare.
        space 7 is "power", not sure what that does.
      */
      //been tagged
      // run analysis on tag to determine if it carry's special instructions:
      if (tokenStrings[2] == "15" || tokenStrings[2] == "1") {
        TagPerks();
      }
      lastTaggedPlayer = tokenStrings[3].toInt();
      lastTaggedTeam = tokenStrings[4].toInt();
      Serial.println("Just tagged by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
    }
    
    // this is a cool pop up... lots of info provided and broken out below.
    // this pops up everytime you pull the trigger and deplete your ammo.hmmm you
    // can guess what im using that for... see more below
// ****** note i still need to assign the ammo to the lcd outputs********
    if (tokenStrings[0] == "$ALCD") {
      /* ammunition status update occured 
       *  space 0 is ALCD indicator
       *  space 1 is rounds remaining in clip
       *  space 2 is accuracy of weapon
       *  space 3 is slot of weapon
       *  space 4 is remaining ammo outside clip
       *  space 5 is the overheating feature if aplicable
       *  
       *  more can be done with this, like using the ammo info to post to lcd
       */
       ammo1 = tokenStrings[1].toInt(); // sets the variable to be sent to esp8266 for ammo in clip
       ammo2 = tokenStrings[4].toInt(); // sets the total magazine capacity to be sent to 8266
       if (tokenStrings[3].toInt() == 0) {
        weap=SetSlotA; 
        CurrentWeapSlot=0;
        } // if the weapon slot 0 is loaded up, sets the variable to notify weapon type loaded to the right weapon
       if (tokenStrings[3].toInt() == 1) {
        weap=SetSlotB; 
        CurrentWeapSlot=1;
        } // same thing but for weapon slot 1
       if (tokenStrings[3].toInt() == 2) {
        CurrentWeapSlot=4;
        }   
    }
    
    // this guy is recieved from brx every time your health changes... usually after
    // getting shot.. from the HIR indicator above. but this can tell us our health status
    // and we can send it to an LCD or use it to know when were dead and award points to the 
    // player who loast tagged the gun... hence the nomenclature "lasttaggedPlayer" etc
    // add in some kill counts and we can deplete lives and use it to enable or disable respawn functions
    if (tokenStrings[0] == "$HP") {
      /*health status update occured
       * can be used for updates on health as well as death occurance
       */
      health = tokenStrings[1].toInt(); // setting variable to be sent to esp8266
      armor = tokenStrings[2].toInt(); // setting variable to be sent to esp8266
      shield = tokenStrings[3].toInt(); // setting variable to be sent to esp8266
      if ((tokenStrings[1] == "0") && (tokenStrings[2] == "0") && (tokenStrings[3] == "0")) { // player is dead
        if (HASFLAG) { HASFLAG = false; AudioSelection1 = "VA2I"; AUDIO1 = true;}
        PlayerKillCount[lastTaggedPlayer]++; // adding a point to the last player who killed us
        datapacket1 = lastTaggedPlayer; // setting an identifier for who recieves the tag
        datapacket2 = 32000; // identifier to tell player they got a kill
        if (GameMode == 4) {
          datapacket3 = PreviousSpecialWeapon; // current special weapon equiped
        } else {
          datapacket3 = GunID; // null action to send
        }
        BROADCASTESPNOW = true;
        TeamKillCount[lastTaggedTeam]++; // adding a point to the team who caused the last kill
        PlayerLives--; // taking our preset lives and subtracting one life then talking about it on the monitor
        Deaths++;
        //AudioSelection1="KBP"+String(lastTaggedPlayer);
        //AUDIO1=true;
        Serial.println("Lives Remaining = " + String(PlayerLives));
        Serial.println("Killed by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
        Serial.println("Team: " + String(lastTaggedTeam) + "Score: " + String(TeamKillCount[lastTaggedTeam]));
        Serial.println("Player: " + String(lastTaggedPlayer) + " Score: " + String(PlayerKillCount[lastTaggedPlayer]));
        Serial.println("Death Count: " + String(Deaths));
        if (GameMode != 8) {
          if (PlayerLives > 0 && SetRSPNMode < 9) { // doing a check if we still have lives left after dying
            RESPAWN = true;
            Serial.println("Auto respawn enabled");
          }
          if (PlayerLives > 0 && SetRSPNMode == 9) { // doing a check if we still have lives left after dying
            MANUALRESPAWN = true;
            Serial.println("Manual respawn enabled");
          }
          if (PlayerLives == 0) {
            //GAMEOVER=true;
            TerminalInput = "Lives Depleted"; // storing the string for sending to Tagger, elswhere
            WRITETOTERMINAL = true;
            delay(3000);
            sendString("$HLOOP,2,1200,*"); // this puts the headset in a loop for flashing
            sendString("$PLAY,VA46,4,6,,,,,*"); // says lives depleted
            Serial.println("lives depleted");
          }
        }
        if (GameMode == 8) {
          RESPAWN = true;
          SetTeam = lastTaggedTeam;
          if (SetTeam == 0) {sendString("$PLAY,VA5G,4,6,,,,,*");}
          if (SetTeam == 1) {sendString("$PLAY,VA1L,4,6,,,,,*");}
          if (SetTeam == 2) {sendString("$PLAY,VA1R,4,6,,,,,*");}
          if (SetTeam == 3) {sendString("$PLAY,VA27,4,6,,,,,*");}
        }
    }
  }
}
//******************************************************************************************
//******************************************************************************************
void SoftWareReset() {
  ESP.restart();
}
//******************************************************************************************
void ChangeColors() {
  String colorstring = ("$GLED," + String(ChangeMyColor) + "," + String(ChangeMyColor) + "," + String(ChangeMyColor) + ",0,10,,*");
  sendString(colorstring);
  colorstring = ("$HLED," + String(ChangeMyColor) + ",0,,,10,,*");
  sendString(colorstring);
  //ChangeMyColor = 99;
  CurrentColor = ChangeMyColor;
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
    SYNCLOCALSCORES = true;
    WebAppUpdaterTimer = millis();
  }
  Serial.print("cOMMS loop running on core ");
  Serial.println(xPortGetCoreID());
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
    //UpdateWebApp();
}


//******************************************************************************************
// ***********************************  DIRTY LOOP  ****************************************
// *****************************************************************************************
void loop1(void *pvParameters) {
  Serial.print("Dirty loop running on core ");
  Serial.println(xPortGetCoreID());   
  //auto control brx start up:
  //InitializeJEDGE();
  while(1) { // starts the forever loop
    // put all the serial activities in here for communication with the brx
    if (Serial1.available()) {
      readStr = Serial1.readStringUntil('\n');
      ProcessBRXData();
    }
    MainGame(); 
    delay(1); // this has to be here or it will just continue to restart the esp32
  }
}
// *****************************************************************************************
// **************************************  Comms LOOP  *************************************
// *****************************************************************************************
void loop2(void *pvParameters) {
  Serial.print("cOMMS loop running on core ");
  Serial.println(xPortGetCoreID());
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
  if (!ACTASHOST) {
    RUNWEBSERVER = false;
    WiFi.softAPdisconnect (true);
    Serial.println("Device is not intended to act as WebServer / Host. Shutting down AP and WebServer");
  }
  while (1) { // starts the forever loop
    if (RUNWEBSERVER) {
      ws.cleanupClients();
      static unsigned long lastEventTime = millis();
      static const unsigned long EVENT_INTERVAL_MS = 1000;
      if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
        events.send("ping",NULL,millis());
        lastEventTime = millis();
        //UpdateWebApp();   
      }
    }
    if (BROADCASTESPNOW) {
      BROADCASTESPNOW = false;
      getReadings();
      BroadcastData(); // sending data via ESPNOW
      Serial.println("Sent Data Via ESPNOW");
      ResetReadings();
    }
    if (ESPTimeout) {
      TimeOutCurrentMillis = millis();
      if (TimeOutCurrentMillis > TimeOutInterval) {
        Serial.println("Enabling Deep Sleep");
        delay(500);
        esp_deep_sleep_start();
      }
    }
    if (INITIALIZEOTA) {
      INITIALIZEOTA = false;
      InitOTA();
      
    }
    if (ENABLEOTAUPDATE) {
      ArduinoOTA.handle();
      if (WiFi.status() == WL_CONNECTED) {
        // blink the onboard led
        if (millis() > previousMillis + interval) {
          previousMillis = millis();
          if (ledState == LOW) {
            ledState = HIGH; 
          } else {
            ledState = LOW;
          } 
          digitalWrite(led, ledState);  
        }
      }
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
        if (SYNCLOCALSCORES) {
          SYNCLOCALSCORES = false;
          // create a string that looks like this: 
          // (Player ID, token 0), (Player Team, token 1), (Player Objective Score, token 2) (Team scores, tokens 3-8), (player kill counts, tokens 9-72 
          String LocalScoreData = String(GunID)+","+String(SetTeam)+","+String(CompletedObjectives)+","+String(TeamKillCount[0])+","+String(TeamKillCount[1])+","+String(TeamKillCount[2])+","+String(TeamKillCount[3])+","+String(TeamKillCount[4])+","+String(TeamKillCount[5])+","+String(PlayerKillCount[0])+","+String(PlayerKillCount[1])+","+String(PlayerKillCount[2])+","+String(PlayerKillCount[3])+","+String(PlayerKillCount[4])+","+String(PlayerKillCount[5])+","+String(PlayerKillCount[6])+","+String(PlayerKillCount[7])+","+String(PlayerKillCount[8])+","+String(PlayerKillCount[9])+","+String(PlayerKillCount[10])+","+String(PlayerKillCount[11])+","+String(PlayerKillCount[12])+","+String(PlayerKillCount[13])+","+String(PlayerKillCount[14])+","+String(PlayerKillCount[15])+","+String(PlayerKillCount[16])+","+String(PlayerKillCount[17])+","+String(PlayerKillCount[18])+","+String(PlayerKillCount[19])+","+String(PlayerKillCount[20])+","+String(PlayerKillCount[21])+","+String(PlayerKillCount[22])+","+String(PlayerKillCount[23])+","+String(PlayerKillCount[24])+","+String(PlayerKillCount[25])+","+String(PlayerKillCount[26])+","+String(PlayerKillCount[27])+","+String(PlayerKillCount[28])+","+String(PlayerKillCount[29])+","+String(PlayerKillCount[30])+","+String(PlayerKillCount[31])+","+String(PlayerKillCount[32])+","+String(PlayerKillCount[33])+","+String(PlayerKillCount[34])+","+String(PlayerKillCount[35])+","+String(PlayerKillCount[36])+","+String(PlayerKillCount[37])+","+String(PlayerKillCount[38])+","+String(PlayerKillCount[39])+","+String(PlayerKillCount[40])+","+String(PlayerKillCount[41])+","+String(PlayerKillCount[42])+","+String(PlayerKillCount[43])+","+String(PlayerKillCount[44])+","+String(PlayerKillCount[45])+","+String(PlayerKillCount[46])+","+String(PlayerKillCount[47])+","+String(PlayerKillCount[48])+","+String(PlayerKillCount[49])+","+String(PlayerKillCount[50])+","+String(PlayerKillCount[51])+","+String(PlayerKillCount[52])+","+String(PlayerKillCount[53])+","+String(PlayerKillCount[54])+","+String(PlayerKillCount[55])+","+String(PlayerKillCount[56])+","+String(PlayerKillCount[57])+","+String(PlayerKillCount[58])+","+String(PlayerKillCount[59])+","+String(PlayerKillCount[60])+","+String(PlayerKillCount[61])+","+String(PlayerKillCount[62])+","+String(PlayerKillCount[63]);
          Serial.println("Sending the following Score Data to Server");
          Serial.println(LocalScoreData);
          incomingData4 = LocalScoreData;
          AccumulateIncomingScores();
        }
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
    delay(1); // this has to be here or the esp32 will just keep rebooting
  }
}
//******************************************************************************************
//*************************************** SET UP *******************************************
//******************************************************************************************
void setup() {
  Serial.begin(115200); // set serial monitor to match this baud rate
  Serial.println("Initializing serial output settings, Tagger Generation set to Gen: " + String(GunGeneration));
  if (GunGeneration > 1) {
    BaudRate = 115200;
  }
  Serial.println("Serial Buad Rate set for: " + String(BaudRate));
  Serial1.begin(BaudRate, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the serial pins for sending data to BRX
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.print("setup() running on core ");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  if (RUNWEBSERVER) {
    InitAP();
    initWebSocket();
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
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
  }
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
} // End of setup.
// **********************
// ****  EMPTY LOOP  ****
// **********************
void loop() {
  //empty loop
}

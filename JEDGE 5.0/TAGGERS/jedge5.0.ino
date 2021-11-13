/*
 * This Program is intended to provide the capability to modify brx tagger settins as desired. It
 * is recommended the device be powered by the tagger or ideally a secondary power source to not
 * drain tagger battery. This will require a secondary device as a controller to mass configure 
 * multiple taggers or individual taggers. 
 * 
 * Log: 
 * 10/4/2021  put together BLE coms from previous configurator
 * 10/5/2021  confirmed coms with a tagger that ble client objects are successful
 *            removed unnecessary portions of code to minimize size of sket and organization
 *            and preparing for espnow introduction to run in background of ble comms
 *            added in espnow objects
 * 10/6/2021  Added in the webserver for settings adjustments from default
 *            added in web based update and tested for performance
 *            added in capacitive touch for verification of if web app is to run or not
 *            added in eeprom for storing wifi credentials for web updates
 *            cleared out unnecessary variables and code lines from copied/pasted code from other sketches
 * 10/7/2021  planning on option implementations:
 *              Difficulty: easy, normal, hard - to adjust armor and currenthealth levels
 *              Weapon slots one, two and three as options for misc weapons depending on how complicated you want it to get for boss player
 *              add in a melee weapon selection for close range instant explosive death
 *              add in a left button for stun/disable players temporarily
 *              add in a right button for gasing players
 * 11/1/2021  I've integrated all the functionality of JEDGE on top of the web server auto start functions. I need to do some testing now
 *              Both BLE and Bluetooth and Serial comms should function to integrate all JEDGE 4.0 functionality
 *              I also added back in the score reporting for each tagger by pressing the select button (needs testing)
 * 
 * 
 * Desired Implementations:
 *              HUD for stats
 *              Randomized guns and health boosts on death/respawn
 *              Send kill confirmations
 *              Arcade mode
 *              
 *            
 */

//****************************************************************
// libraries to include:
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "BLEDevice.h" // needed for bluetooth low energy client set up and pairing to brx
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
#include <WiFi.h> // used for espnow and web server status
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <HardwareSerial.h>
HardwareSerial SerialBT(1);
#define SERIAL1_RXPIN 16 // TO HC-05 TX
#define SERIAL1_TXPIN 17 // TO HC-05 RX

//****************************************************************
//****  OLED DISPLAY DEFINITIONS *********

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 200

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
  
//******************* IMPORTANT *********************
//*********** YOU NEED TO CHANGE INFO IN HERE FOR EACH GUN!!!!!!***********
#define BLE_SERVER_SERVICE_NAME "jayeburden" // CHANGE ME!!!! (case sensitive)
String BLEName = "jayeburden";
// this is important it is how we filter
// out unwanted guns or other devices that use UART BLE characteristics you need to change 
// this to the name of the gun ble server
byte GunID = 63; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
// These should be modified as applicable for your needs
String ssid = "BossApp63";
const char* password = "123456789";
byte GunGeneration = 2;
//******************* IMPORTANT *********************

//****************************************************************

// The remote service we wish to connect to, all guns should be the same as
// this is an uart comms set up
static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
// these uuids are used to send and recieve data
static BLEUUID    charRXUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    charTXUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// variables used for ble set up
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteRXCharacteristic;
static BLERemoteCharacteristic* pRemoteTXCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEClient*  pClient;

// variables used to provide notifications of pairing device 
// as well as connection status and recieving data from gun
char notifyData[100];
int notifyDataIndex = 0;
String tokenStrings[100];
char *ptr = NULL;

int TouchCounter = 0; // 0 = BLE for tagger, 1 = webserver, 2 = OTA mode
int GearMod = 0;

bool ENABLEBLE = false; // used to enable or disable BLE device
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool ENABLEWEBSERVER = false; // enables the web server for settings/configuration
bool BLUETOOTHCLASSIC = false; // enables bluetooth classic control
bool UPDATEOLED = false; // used to trigger an upudate to OLED screen

byte SetPlayerMode = 0;
bool INGAME = false;

long startScan = 0; // part of BLE enabling

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long PreviousMillis0 = 0;  // will store last time LED was updated
unsigned long PreviousMillis2 = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED

//*************************************************************************
//******************** OTA UPDATES ****************************************
//*************************************************************************
String FirmwareVer = {"1.0"};
#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/bossapp_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/bossappfw.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"

bool OTAMODE = false;
// text inputs
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";

void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();
bool UPTODATE = false;
byte updatenotification;

unsigned long previousMillis_2 = 0;
const long otainterval = 1000;
const long mini_interval = 1000;

String OTAssid = "dontchangeme"; // network name to update OTA
String OTApassword = "dontchangeme"; // Network password for OTA

//*************************************************************************
//******************** VARIABLE DECLARATIONS ******************************
//*************************************************************************
int WebSocketData;
//bool ledState = 0;
const int ledPin = 2;

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
//char *ptr = NULL; // used for initializing and ending string break up.

//variabls for blinking an LED with Millis
//const int led = 2; // ESP32 Pin to which onboard LED is connected
//unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long ledpreviousMillis = 0;  // will store last time LED was updated
int ledinterval = 1500;  // interval at which to blink (milliseconds)
unsigned long currentMillis0 = 0; // used for core 0 counter/timer.
unsigned long currentMillis1 = 0; // used for core 0 counter/timer.
unsigned long previousMillis1 = 0;  // will store last time LED was updated
unsigned long LastScoreUpdate = 0; // for score updates

//************************************* JEDGE VARIABLES ***********************************
bool BACKGROUNDMUSIC = false;
bool PBLOCKOUT = false;
byte SetVol = 70;
byte BootCounter = 0;
byte TeamID = 0;
String LastStringProcessed = "null";
byte CompletedObjectives = 0;
byte lastTaggedPlayer = 65;
byte lastTaggedTeam = 5;
bool REQUESTQUERY = false;
int currenthealth = 0;
int armor = 0;
byte PlayerKillCount[64];
byte TeamKillCount[4];
byte Deaths;
byte PlayerLives = 255;
long LastDeathTime = 0;
int lastincomingData2 = 0;
byte ChangeMyColor = 99;
byte pbgame = 0;
byte pbteam = 0;
byte pbweap = 0;
byte pbperk = 0;
byte pblives = 0;
byte pbtime = 0;
byte pbspawn = 0;
byte pbindoor = 0;
String AudioSelection = "null";
String MusicSelection = "null";
long MusicRestart = 0;
long MusicPreviousMillis = 0;
bool CurrentDominationLeader = 99;
byte ClosingVictory[0];
int AudioDelay = 0; // used to delay an audio playback
long AudioPreviousMillis = 0;
byte AudioChannel = 3;
byte AnounceScore = 0;
bool STEALTH = false;
bool PLAYERDIED = false;
byte RebootWhileInGame = 0;
String ScoreTokenStrings[73];
byte PlayerDeaths[64];
byte PlayerKills[64];
byte PlayerObjectives[64];
byte TeamKills[4];
byte TeamObjectives[4];
byte TeamDeaths[4];
String ScoreString = "0";
byte GameMode = 0;
byte MyKills = 0; // counter for this players kill count
byte PreviousSpecialWeapon = 0;
bool SELECTCONFIRM = false;
long SelectButtonTimer = 0;
bool SPECIALWEAPONLOADOUT = false;
byte SpecialWeapon = 0;
byte SwapBRXCounter = 0;
byte SwapPistolLevel = 0;
bool SWAPBRX = false;
bool LOOT = false;
bool SPECIALWEAPONPICKUP = false;
bool SCORECHECK = false;
long LastScoreCheck = 0;
String SwapBRXPistol = "$WEAP,0,,50,0,0,10,0,,,,,,,,225,850,9,32768,400,0,7,70,70,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,30,,*";
String SwapBRXPistol1 = "$WEAP,0,,55,0,0,15,2,,,,,,,,225,850,10,32768,400,0,7,70,70,,0,,,R13,,,,D04,D03,D02,D18,,,,,10,9999999,35,,*";
String SwapBRXPistol2 = "$WEAP,0,,60,0,0,20,4,,,,,,,,225,850,11,32768,400,0,7,80,80,,0,,,R14,,,,D04,D03,D02,D18,,,,,11,9999999,40,,*";
String SwapBRXPistol3 = "$WEAP,0,,65,0,0,25,6,,,,,,,,225,850,12,32768,400,0,7,80,80,,0,,,R15,,,,D04,D03,D02,D18,,,,,12,9999999,45,,*";
String SwapBRXPistol4 = "$WEAP,0,,70,0,0,30,8,,,,,,,,225,850,13,32768,400,0,7,90,90,,0,,,R16,,,,D04,D03,D02,D18,,,,,13,9999999,50,,*";
String SwapBRXPistol5 = "$WEAP,0,,75,0,0,35,10,,,,,,,,225,850,14,32768,400,0,7,90,90,,0,,,R17,,,,D04,D03,D02,D18,,,,,14,9999999,55,,*";
String SwapBRXPistol6 = "$WEAP,0,,80,0,0,40,12,,,,,,,,225,850,15,32768,400,0,7,90,90,,0,,,R18,,,,D04,D03,D02,D18,,,,,15,9999999,60,,*";
String SwapBRXPistol7 = "$WEAP,0,,85,0,0,45,14,,,,,,,,225,850,16,32768,400,0,7,90,90,,0,,,R19,,,,D04,D03,D02,D18,,,,,16,9999999,65,,*";
String SwapBRXPistol8 = "$WEAP,0,,90,0,0,50,16,,,,,,,,225,850,17,32768,400,0,7,90,90,,0,,,R20,,,,D04,D03,D02,D18,,,,,17,9999999,70,,*";
String SwapBRXPistol9 = "$WEAP,0,2,100,0,0,60,18,,,,,,100,30,225,850,20,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,20,9999999,75,15,*";
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
byte datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
int datapacket2 = 32700; // ///COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
byte datapacket3 = GunID; // From - device ID
String datapacket4 = "null"; // used for score reporting only


// Define variables to store incoming readings
byte incomingData1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - up to 255
int incomingData2; // ///COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
byte incomingData3; // From - device ID - up to 255
String incomingData4; // used for score reporting only

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    byte DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
    int DP2; // ///COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
    byte DP3; // From - device ID
    char DP4[200]; // used for score reporting
} struct_message;

// Create a struct_message called DataToBroadcast to hold sensor readings
struct_message DataToBroadcast;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

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
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingData1 = incomingReadings.DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  incomingData2 = incomingReadings.DP2; // ///COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  incomingData3 = incomingReadings.DP3; // From
  //incomingData4 = incomingReadings.DP4; // From
  Serial.println("DP1: " + String(incomingData1)); // INTENDED RECIPIENT
  Serial.println("DP2: " + String(incomingData2)); // ///COMMAN
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
  Serial.println(datapacket1);
  Serial.println(datapacket2);
  Serial.println(datapacket3);
  Serial.println(datapacket4);
}

void ResetReadings() {
  //datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  datapacket2 = 32700; // ///COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
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
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  
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
  
  // Register for a callback // that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void ProcessTeamsAssignmnet() {
          if (TeamID == 0) {
            Serial.println("team0"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection = "VS9"; 
              pbteam = 0;
            } 
            if (pbgame > 4) {
              AudioSelection = "VA3T"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection = "VA13"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          if (TeamID == 1) {
            Serial.println("team1"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection = "VA4N"; 
              pbteam = 1;
            }
            if (pbgame > 4) {
              AudioSelection = "VA1L"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection = "VA1L"; 
              pbteam = 1;
            }
            Serial.println("pbteam = " + String(pbteam));
          }
          if (TeamID == 2) {
            Serial.println("team2"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection = "VA71"; 
              pbteam = 2;
            } 
            if (pbgame > 4) {
              AudioSelection = "VA3Y"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection = "VA27"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          if (TeamID == 3) {
            Serial.println("team3"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection = "VA1R"; 
              pbteam = 2;
            } 
            if (pbgame > 4) {
              AudioSelection = "VA1R"; 
              pbteam = 1;
            } 
            if (pbgame < 3) {
              AudioSelection = "VA1R"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          EEPROM.write(5, pbteam);
          EEPROM.write(3, TeamID);
          EEPROM.commit();
          Audio();
          ChangeMyColor = TeamID;
}
//**************************************************************
void OLEDStart() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...

  testdrawline();      // Draw many lines

  testdrawrect();      // Draw rectangles (outlines)

  testfillrect();      // Draw rectangles (filled)

  testdrawcircle();    // Draw circles (outlines)

  testfillcircle();    // Draw circles (filled)

  testdrawroundrect(); // Draw rounded rectangles (outlines)

  testfillroundrect(); // Draw rounded rectangles (filled)

  testdrawtriangle();  // Draw triangles (outlines)

  testfilltriangle();  // Draw triangles (filled)

  testdrawchar();      // Draw characters of the default font

  testdrawstyles();    // Draw 'stylized' characters

  testscrolltext();    // Draw scrolling text

  testdrawbitmap();    // Draw a small bitmap image

  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);

  testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
}
void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}

void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}

void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }

  delay(2000);
}

void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  int8_t f, icons[NUMFLAKES][3];

  // Initialize 'snowflake' positions
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }
  /*
  for(;;) { // Loop forever...
    display.clearDisplay(); // Clear the display buffer

    // Draw each snowflake:
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }

    display.display(); // Show the display buffer on the screen
    delay(200);        // Pause for 1/10 second

    // Then update coordinates of each flake...
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      // If snowflake is off the bottom of the screen...
      if (icons[f][YPOS] >= display.height()) {
        // Reinitialize to a random position, just off the top
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
  */
}
//**************************************************************
void Audio() {
    if (AudioDelay > 1) {
      if (millis() - AudioPreviousMillis > AudioDelay) {
        if (AudioChannel == 3) {
          sendString("$PLAY,"+AudioSelection+",3,10,,,,,*");
        } else {
          sendString("$PLAY,"+AudioSelection+"," + AudioChannel + ",10,,,,,*");
          AudioChannel = 3;
        }
        //
        AudioSelection  = "NULL";
        AudioDelay = 0; // reset the delay for another application
      }
    } else {
      sendString("$PLAY,"+AudioSelection+",3,10,,,,,*");
      //
      AudioSelection  = "NULL";
    }
    if (AnounceScore == 1) {
      AnounceScore = 2;
      AudioPreviousMillis = millis();
      AudioDelay = 1500;
      AudioSelection="VNA"; // "Kill Confirmed"
      Audio();
    }
}
void recvMsg(uint8_t *data, size_t len){
  /*
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  Serial.print("Received the following data via webserver: ");
  Serial.println(d);
  if (d == "ON"){
    digitalWrite(led, HIGH);
    Serial.println("led turned high");
  }
  if (d=="OFF"){
    digitalWrite(led, LOW);
    Serial.println("led turned low");
  }
  */
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
void SyncScores() {
  // create a string that looks like this: 
  // (Player ID, token 0), (Player Team, token 1), (Player Objective Score, token 2) (Team scores, tokens 3-8), (player kill counts, tokens 9-72 
  String ScoreData = String(GunID)+","+String(TeamID)+","+String(CompletedObjectives)+","+String(TeamKillCount[0])+","+String(TeamKillCount[1])+","+String(TeamKillCount[2])+","+String(TeamKillCount[3])+","+String(TeamKillCount[4])+","+String(TeamKillCount[5])+","+String(PlayerKillCount[0])+","+String(PlayerKillCount[1])+","+String(PlayerKillCount[2])+","+String(PlayerKillCount[3])+","+String(PlayerKillCount[4])+","+String(PlayerKillCount[5])+","+String(PlayerKillCount[6])+","+String(PlayerKillCount[7])+","+String(PlayerKillCount[8])+","+String(PlayerKillCount[9])+","+String(PlayerKillCount[10])+","+String(PlayerKillCount[11])+","+String(PlayerKillCount[12])+","+String(PlayerKillCount[13])+","+String(PlayerKillCount[14])+","+String(PlayerKillCount[15])+","+String(PlayerKillCount[16])+","+String(PlayerKillCount[17])+","+String(PlayerKillCount[18])+","+String(PlayerKillCount[19])+","+String(PlayerKillCount[20])+","+String(PlayerKillCount[21])+","+String(PlayerKillCount[22])+","+String(PlayerKillCount[23])+","+String(PlayerKillCount[24])+","+String(PlayerKillCount[25])+","+String(PlayerKillCount[26])+","+String(PlayerKillCount[27])+","+String(PlayerKillCount[28])+","+String(PlayerKillCount[29])+","+String(PlayerKillCount[30])+","+String(PlayerKillCount[31])+","+String(PlayerKillCount[32])+","+String(PlayerKillCount[33])+","+String(PlayerKillCount[34])+","+String(PlayerKillCount[35])+","+String(PlayerKillCount[36])+","+String(PlayerKillCount[37])+","+String(PlayerKillCount[38])+","+String(PlayerKillCount[39])+","+String(PlayerKillCount[40])+","+String(PlayerKillCount[41])+","+String(PlayerKillCount[42])+","+String(PlayerKillCount[43])+","+String(PlayerKillCount[44])+","+String(PlayerKillCount[45])+","+String(PlayerKillCount[46])+","+String(PlayerKillCount[47])+","+String(PlayerKillCount[48])+","+String(PlayerKillCount[49])+","+String(PlayerKillCount[50])+","+String(PlayerKillCount[51])+","+String(PlayerKillCount[52])+","+String(PlayerKillCount[53])+","+String(PlayerKillCount[54])+","+String(PlayerKillCount[55])+","+String(PlayerKillCount[56])+","+String(PlayerKillCount[57])+","+String(PlayerKillCount[58])+","+String(PlayerKillCount[59])+","+String(PlayerKillCount[60])+","+String(PlayerKillCount[61])+","+String(PlayerKillCount[62])+","+String(PlayerKillCount[63]);
  Serial.println("Sending the following Score Data to Server");
  Serial.println(ScoreData);
  datapacket1 = incomingData3;
  datapacket4 = ScoreData;
  datapacket2 = 902;
  BROADCASTESPNOW = true;
  Serial.println("Sent Score data to Server");  
}
void ProcessIncomingCommands() {
  ledState = !ledState;
  if (SetPlayerMode == 4) {
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  if (lastincomingData2 != incomingData2) {
  byte IncomingTeam = 99;
  if (incomingData1 > 199) {
    IncomingTeam = incomingData1 - 200;
  }
  if (incomingData2 == 904) { // this is a wifi credential
    Serial.println("received SSID from controller");
    OTAssid = incomingData4;
    //Serial.print("OTApassword = ");
    //Serial.println(OTApassword);
    Serial.print("OTAssid = ");
    Serial.println(OTAssid);
    // clear eeprom
    Serial.println("clearing eeprom");
    for (int i = 11; i < 44; ++i) {
      EEPROM.write(i, 0);
    }
    Serial.println("writing eeprom ssid:");
    int j = 11;
    for (int i = 0; i < OTAssid.length(); ++i)
    {
      EEPROM.write(j, OTAssid[i]);
      Serial.print("Wrote: ");
      Serial.println(OTAssid[i]);
      j++;
    }
    EEPROM.commit();
  }
  if (incomingData2 == 905) { // this is a wifi credential
    Serial.println("received Wifi Password from controller");
    OTApassword = incomingData4;
    Serial.print("OTApassword = ");
    Serial.println(OTApassword);
    //Serial.print("OTAssid = ");
    //Serial.println(OTAssid);
    Serial.println("clearing eeprom");
    for (int i = 44; i < 100; ++i) {
      EEPROM.write(i, 0);
    }
    Serial.println("writing eeprom Password:");
    int k = 44;
    for (int i = 0; i < OTApassword.length(); ++i)
    {
      EEPROM.write(k, OTApassword[i]);
      Serial.print("Wrote: ");
      Serial.println(OTApassword[i]);
      k++;
    }
    EEPROM.commit();
  }
  if (incomingData1 == GunID || incomingData1 == 99 || IncomingTeam == TeamID) { // data checked out to be intended for this player ID or for all players
    lastincomingData2 = incomingData2;
    digitalWrite(2, HIGH);
    if (incomingData2 < 600 && incomingData2 > 500) { // setting team modes
      int b = incomingData2 - 500;
      
      if (INGAME==false){
        byte A = 0;
        byte B = 1;
        byte C = 2;
        byte D = 3;
        if (b==1) {
          Serial.println("Free For All"); 
          TeamID=0;
          ProcessTeamsAssignmnet();
          //Audio();
          //AudioSelection="VA30";
        }
        if (b==2) {
          Serial.println("Teams Mode Two Teams (odds/evens)");
          if (GunID % 2) {
            TeamID=0; 
            //AudioSelection="VA13";
          } else {
            TeamID=1; 
            //AudioSelection="VA1L";
          }
          ProcessTeamsAssignmnet();
          //Audio();
        }
        if (b==3) {
          Serial.println("Teams Mode Three Teams (every third player)");
          while (A < 64) {
            if (GunID == A) {
              TeamID=0;
              //AudioSelection="VA13";
            }
            if (GunID == B) {
              TeamID=1;
              //AudioSelection="VA1L";
            }
            if (GunID == C) {
              TeamID=3;
              //AudioSelection="VA27";
            }
            A = A + 3;
            B = B + 3;
            C = C + 3;
            delay(1);
          }
          A = 0;
          B = 1;
          C = 2;
          ProcessTeamsAssignmnet();
          //Audio();
        }
        if (b==4) {
          Serial.println("Teams Mode Four Teams (every fourth player)");
          while (A < 64) {
            if (GunID == A) {
              TeamID=0;
              //AudioSelection="VA13";
            }
            if (GunID == B) {
              TeamID=1;
              //AudioSelection="VA1L";
            }
            if (GunID == C) {
              TeamID=2;
              //AudioSelection="VA1R";
            }
            if (GunID == D) {
              TeamID=3;
              //AudioSelection="VA27";
            }
            A = A + 4;
            B = B + 4;
            C = C + 4;
            D = D + 4;
            delay(1);
          }
          A = 0;
          B = 1;
          C = 2;
          D = 3;
          ProcessTeamsAssignmnet();
          //Audio();
        }
        // this one allows for manual input of settings... each gun will need to select a team now
        ChangeMyColor = TeamID; // triggers a gun/tagger color change
      }
    }
    if (incomingData2 < 1000 && incomingData2 > 900) { // Syncing scores
      int b = incomingData2 - 900;
      if (b == 1) {
        Serial.println("Request Recieved to Sync Scoring");
        SyncScores();
        AudioSelection="OP15";
        Audio();
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
      }
    }
    if (incomingData2 == 902) {
      AccumulateIncomingScores();
      MyKills = PlayerKills[GunID];
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
       sendString("$VOL,"+String(SetVol)+",0,*"); // sets max volume on gun 0-100 feet distance
       Audio();
       if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
       } else { 
         ChangeMyColor++;
       }
       EEPROM.write(195, SetVol);
     }
   }
   if (incomingData2 < 1500 && incomingData2 > 1399) { // Starting/Ending Game
     int b = incomingData2 - 1400;
     if (b==0) {
       if (INGAME){
         PBEndGame(); 
         Serial.println("ending game");
         datapacket2 = 1400;
         datapacket1 = 99;
         // BROADCASTESPNOW = true;
       }
     }
     if (b > 9 && b < 20) { // this is a game winning end game... you might be a winner
       if (INGAME){
         PBEndGame(); 
         Serial.println("ending game");
         datapacket2 = 1410 + b;
         datapacket1 = 99;
         //BROADCASTESPNOW = true;
         int winnercheck = b - 10;
         if (winnercheck == TeamID) { // your team won
          delay(3000);
          AudioSelection = "VSF";
          Audio();
          sendString("$HLED,4,2,,,10,100,*");
         }
       }
     }
     if (b > 19 && b < 30) { // this is change in leaders
       if (INGAME){
         Serial.println("change in leaders detected");
         int leadercheck = b - 20;
         if (leadercheck != CurrentDominationLeader) { 
           // you have not received this notification yet
           CurrentDominationLeader = leadercheck;
           if (leadercheck == 0) {
             AudioSelection = "VB17";
           }
           if (leadercheck == 1) {
             AudioSelection = "VB07";
           }
           if (leadercheck == 2) {
             AudioSelection = "VB1J";
           }
           if (leadercheck == 3) {
             AudioSelection = "VB0L";
           }
           Audio();
           datapacket2 = 1420 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
       }
     }
     if (b > 29 && b < 40) { // this is a game winning end game... you might be a winner
       if (INGAME){
         int leadercheck = b - 30;
         Serial.println("change in leaders detected");
         if (leadercheck == 0 && ClosingVictory[0] == 0) {
           AudioSelection = "VB13";
           ClosingVictory[0] = 1;
           Audio();
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 1 && ClosingVictory[1] == 0) {
           AudioSelection = "VB03";
           ClosingVictory[1] = 1;
           Audio();
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 2 && ClosingVictory[2] == 0) {
           AudioSelection = "VB1F";
           ClosingVictory[2] = 1;
           Audio();
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 3 && ClosingVictory[3] == 0) {
           AudioSelection = "VB0H";
           ClosingVictory[3] = 1;
           Audio();
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
       }
     }
   }
   if (incomingData2 < 1600 && incomingData2 > 1499) { // Misc. Debug
     int b = incomingData2 - 1500;
     if (b==0) {
      Serial.println("unlocking all hidden menu items");
      int dlccounter = 0;
      while (dlccounter < 12) {
        sendString("$DLC,"+String(dlccounter)+",1,*");
        dlccounter++;
        delay(150);
      }
      sendString("$PLAY,OP61,3,10,,,,,*");
     }
     if (b==2) {
       ESP.restart();
     }
     if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
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
       sendString("$BLINK,1,500,1000,100,*");
       delay(200);
       sendString("$PLAY,OP62,3,10,,,,,*");
       if (ChangeMyColor > 8) {
         ChangeMyColor = 4; // triggers a gun/tagger color change
       } else { 
        ChangeMyColor++;
       }
       Serial.println("OTA Update Mode");
       EEPROM.write(0, 0); // sets OTA update mode
       EEPROM.commit(); // Storar in Flash
       ESP.restart();
     }
   }
   if (incomingData2 < 1700 && incomingData2 > 1599) { // Player-Team Selector
     int b = incomingData2 - 1600;
     int teamselector = 99;
     if (b == 68) {
       INGAME = false;
          Serial.println("listening to commands");
       ChangeMyColor = 7; // triggers a gun/tagger color change
     } else {
      if (b > 63) {
        teamselector = b - 64;
      }
      if (teamselector < 99) {
        if (teamselector == TeamID) {
          INGAME = false;
          Serial.println("listening to commands");
          ChangeMyColor = 7; // triggers a gun/tagger color change
        } else {
          INGAME = true;
          ChangeMyColor = 9; // triggers a gun/tagger color change
          Serial.println("Not listening any more");
        }
      } else {
        if (b == GunID) {
          INGAME = false;
          ChangeMyColor = 7; // triggers a gun/tagger color change
          Serial.println("listening to commands");
        } else {
          INGAME = true;
          ChangeMyColor = 9; // triggers a gun/tagger color change
          Serial.println("Not listening any more");
        }
      }
     }
    }
    // 2100s used for offline games
    if (incomingData2 > 2099 && incomingData2 < 3000) {
      Serial.println("received a 2100 series command");
      if (!INGAME) {
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
        if (incomingData2 == 2100) { pbgame = 0; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA30"; Serial.println("Free For All");} // f4A
        if (incomingData2 == 2101) { pbgame = 1; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA26"; Serial.println("Death Match");} // DMATCH
        if (incomingData2 == 2102) { pbgame = 2; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA36"; Serial.println("Generals");} // GENERALS
        if (incomingData2 == 2103) { pbgame = 3; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA62"; Serial.println("supremacy");} // SUPREM
        if (incomingData2 == 2104) { pbgame = 4; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA1Y"; Serial.println("comander");} // COMMNDR
        if (incomingData2 == 2105) { pbgame = 5; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA64"; Serial.println("survival");} // SURVIVAL
        if (incomingData2 == 2106) { pbgame = 6; EEPROM.write(4, pbgame); EEPROM.commit(); AudioSelection="VA6J"; Serial.println("swarm");} // SWARM
        if (incomingData2 == 2107) { TeamID = 0; Serial.println("team0"); if (pbgame == 3 || pbgame == 4) {AudioSelection = "VS9"; pbteam = 0;} if (pbgame > 4) {AudioSelection = "VA3T"; pbteam = 0;} if (pbgame < 3) {AudioSelection = "VA13"; pbteam = 0;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = TeamID; EEPROM.write(5, pbteam); EEPROM.commit();}
        if (incomingData2 == 2108) { TeamID = 1; Serial.println("team1"); if (pbgame == 3 || pbgame == 4) {AudioSelection = "VA4N"; pbteam = 1;} if (pbgame > 4) {AudioSelection = "VA1L"; pbteam = 0;} if (pbgame < 3) {AudioSelection = "VA1L"; pbteam = 1;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = TeamID; EEPROM.write(5, pbteam); EEPROM.commit();}
        if (incomingData2 == 2109) { TeamID = 3; Serial.println("team3"); if (pbgame == 3 || pbgame == 4) {AudioSelection = "VA71"; pbteam = 2;} if (pbgame > 4) {AudioSelection = "VA3Y"; pbteam = 1;} if (pbgame < 3) {AudioSelection = "VA27"; pbteam = 0;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = TeamID; EEPROM.write(5, pbteam); EEPROM.commit();}
        if (incomingData2 == 2143) { TeamID = 2; Serial.println("team2"); if (pbgame == 3 || pbgame == 4) {AudioSelection = "VA1R"; pbteam = 1;} if (pbgame > 4) {AudioSelection = "VA1R"; pbteam = 0;} if (pbgame < 3) {AudioSelection = "VA1R"; pbteam = 1;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = TeamID; EEPROM.write(5, pbteam); EEPROM.commit();}
        if (incomingData2 == 2110) { pbweap = 0; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap0"); weaponalignment();}
        if (incomingData2 == 2111) { pbweap = 1; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap1"); weaponalignment();}
        if (incomingData2 == 2112) { pbweap = 2; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap2"); weaponalignment();}
        if (incomingData2 == 2113) { pbweap = 3; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap3"); weaponalignment();}
        if (incomingData2 == 2114) { pbweap = 4; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap4"); weaponalignment();}
        if (incomingData2 == 2115) { pbweap = 5; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap5"); weaponalignment();}
        if (incomingData2 == 2116) { pbweap = 6; EEPROM.write(6, pbweap); EEPROM.commit(); Serial.println("weap6"); weaponalignment();}
        if (incomingData2 == 2117) { pbperk = 0; EEPROM.write(7, pbperk); EEPROM.commit(); Serial.println("perk0"); AudioSelection = "VA38";}
        if (incomingData2 == 2118) { pbperk = 1; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA4D"; Serial.println("perk1");}
        if (incomingData2 == 2119) { pbperk = 2; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA1G"; Serial.println("perk2");}
        if (incomingData2 == 2120) { pbperk = 3; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA2N"; Serial.println("perk3");}
        if (incomingData2 == 2121) { pbperk = 4; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA1Z"; Serial.println("perk4");}
        if (incomingData2 == 2122) { pbperk = 5; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA24"; Serial.println("perk5");}
        if (incomingData2 == 2157) { pbperk = 6; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA2W"; Serial.println("perk6");}
        if (incomingData2 == 2158) { pbperk = 7; EEPROM.write(7, pbperk); EEPROM.commit(); AudioSelection="VA4T"; Serial.println("perkoff");}
        if (incomingData2 == 2123) { pblives = 0; PlayerLives = 1; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA01"; Serial.println("lives0");}
        if (incomingData2 == 2124) { pblives = 1; PlayerLives = 3; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA03"; Serial.println("lives1");}
        if (incomingData2 == 2125) { pblives = 2; PlayerLives = 5; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA05"; Serial.println("lives2");}
        if (incomingData2 == 2126) { pblives = 3; PlayerLives = 10; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA0A"; Serial.println("lives3");}
        if (incomingData2 == 2127) { pblives = 4; PlayerLives = 15; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA0F"; Serial.println("lives4");}
        if (incomingData2 == 2128) { pblives = 5; PlayerLives = 32000; EEPROM.write(8, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection="VA6V"; Serial.println("lives5");}
        if (incomingData2 == 2129) { pbtime = 0; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA2S"; Serial.println("time0");}
        if (incomingData2 == 2130) { pbtime = 1; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA6H"; Serial.println("time1");}
        if (incomingData2 == 2131) { pbtime = 2; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA2P"; Serial.println("time2");}
        if (incomingData2 == 2132) { pbtime = 3; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA6Q"; Serial.println("time3");}
        if (incomingData2 == 2133) { pbtime = 4; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA0Q"; Serial.println("time4");}
        if (incomingData2 == 2134) { pbtime = 5; EEPROM.write(9, pbtime); EEPROM.commit(); AudioSelection="VA6V"; Serial.println("time5");}
        if (incomingData2 == 2135) { pbspawn = 0; EEPROM.write(9, pbspawn); EEPROM.commit(); AudioSelection="VA4T"; Serial.println("spawn0");}
        if (incomingData2 == 2136) { pbspawn = 1; EEPROM.write(9, pbspawn); EEPROM.commit(); AudioSelection="VA2Q"; Serial.println("spawn1");}
        if (incomingData2 == 2137) { pbspawn = 2; EEPROM.write(189, pbspawn); EEPROM.commit(); AudioSelection="VA0R"; Serial.println("spawn2");}
        if (incomingData2 == 2138) { pbspawn = 3; EEPROM.write(189, pbspawn); EEPROM.commit(); AudioSelection="VA0V"; Serial.println("spawn3");}
        if (incomingData2 == 2139) { pbspawn = 4; EEPROM.write(189, pbspawn); EEPROM.commit(); AudioSelection="VA0S"; Serial.println("spawn4");}
        if (incomingData2 == 2140) { pbspawn = 5; EEPROM.write(189, pbspawn); EEPROM.commit(); AudioSelection="VA0W"; Serial.println("spawn5");}
        if (incomingData2 == 2141) { pbindoor = 0; EEPROM.write(190, 0); EEPROM.commit(); AudioSelection="VA4W"; Serial.println("outdoor");}
        if (incomingData2 == 2142) { pbindoor = 1; EEPROM.write(190, 1); EEPROM.commit(); AudioSelection="VA3W"; Serial.println("indoor");}
        if (incomingData2 == 2151) { pbindoor = 0; EEPROM.write(190, 2); EEPROM.commit(); STEALTH = true; AudioSelection="VA60"; Serial.println("outdoorS");}
        if (incomingData2 == 2152) { pbindoor = 1; EEPROM.write(190, 3); EEPROM.commit(); STEALTH = true; AudioSelection="VA60"; Serial.println("indoorS");}
        if (incomingData2 == 2144) { GameMode = 0; EEPROM.write(191, GameMode); EEPROM.commit(); AudioSelection="VA5Z"; BACKGROUNDMUSIC = false; Serial.println("nogamemod"); sendString("$PLAYX,4,*");} // standard - defaul
        if (incomingData2 == 2145) { GameMode = 4; EEPROM.write(191, GameMode); EEPROM.commit(); AudioSelection="VA8J"; BACKGROUNDMUSIC = false; Serial.println("royalegamemod"); sendString("$PLAYX,4,*");} // battle royale
        if (incomingData2 == 2146) { GameMode = 8; EEPROM.write(191, GameMode); EEPROM.commit(); AudioSelection="OP01"; BACKGROUNDMUSIC = false; Serial.println("assimilationgamemod"); sendString("$PLAYX,4,*");} // assimilation
        if (incomingData2 == 2147) { // JEDGE SUPREMACY 5V5
          GameMode = 9;  
          EEPROM.write(191, GameMode);
          pbgame = 4; 
          EEPROM.write(4, pbgame);
          SetVol = 100;
          pbindoor = 0; 
          EEPROM.write(190, pbindoor);
          pbgame = 4; 
          EEPROM.write(4, pbgame);
          pblives = 5; 
          EEPROM.write(8, pblives);
          pbtime = 4; 
          EEPROM.write(9, pbtime);
          pbperk = 7; 
          EEPROM.write(7, pbperk);
          EEPROM.commit();
          if (GunID == 0 || GunID == 5 || GunID == 10 || GunID == 15) { // Soldier
            pbweap = 1;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 1 || GunID == 6 || GunID == 11 || GunID == 16) { // heavy
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 2 || GunID == 7 || GunID == 12 || GunID == 17) { // Mercenary
            pbweap = 4;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 3 || GunID == 8 || GunID == 13 || GunID == 18) { // Sniper
            pbweap = 1;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 4 || GunID == 9 || GunID == 14 || GunID == 19) { // Wraith
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID < 5) {
            TeamID = 0;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 10 && GunID > 4) {
            TeamID = 1;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 15 && GunID > 9) {
            TeamID = 2;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 20 && GunID > 14) {
            TeamID = 3;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          Serial.println("Volume set to" + String(SetVol));
          AudioSelection="VA62";
          Serial.println("SUPREMACY 5V5"); 
          sendString("$PLAYX,4,*");
          sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
        }
        if (incomingData2 == 2147) { // JEDGE SUPREMACY 5V5
          pbgame = 4; 
          EEPROM.write(4, pbgame);
          GameMode = 10;  
          EEPROM.write(191, GameMode);
          SetVol = 100;
          pbindoor = 0; 
          EEPROM.write(190, pbindoor);
          pbgame = 4; 
          EEPROM.write(4, pbgame);
          pblives = 5;
          EEPROM.write(8, pblives);
          pbtime = 4; 
          EEPROM.write(9, pbtime);
          pbperk = 7; 
          EEPROM.write(7, pbperk);
          EEPROM.commit();
          if (GunID == 0 || GunID == 6 || GunID == 12 || GunID == 18) { // Soldier
            pbweap = 1;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 1 || GunID == 7 || GunID == 13 || GunID == 19) { // heavy
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 2 || GunID == 8 || GunID == 14 || GunID == 20) { // Mercenary
            pbweap = 4;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 3 || GunID == 9 || GunID == 15 || GunID == 21) { // Sniper
            pbweap = 1;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 4 || GunID == 10 || GunID == 16 || GunID == 22) { // Wraith
            pbweap = 0;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 5 || GunID == 11 || GunID == 17 || GunID == 23) { // Medic/Commander
            pbweap = 5;
            pbteam = 0;
            pbspawn = 2;
            pblives = 1;
            EEPROM.write(8, pblives);
            EEPROM.write(5, pbteam);
            EEPROM.write(6, pbweap);
            EEPROM.write(189, pbspawn);
            EEPROM.commit();
          }
          if (GunID < 6) {
            TeamID = 0;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 12 && GunID > 5) {
            TeamID = 1;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 18 && GunID > 11) {
            TeamID = 2;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          if (GunID < 24 && GunID > 17) {
            TeamID = 3;
            EEPROM.write(192, TeamID);
            EEPROM.commit();
          }
          Serial.println("Volume set to" + String(SetVol));
          AudioSelection="VA62";
          Serial.println("SUPREMACY 6V6"); 
          sendString("$PLAYX,4,*");
          sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
        }
        if (incomingData2 == 2148) { GameMode = 12; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection="CC07"; Serial.println("contra"); sendString("$PLAYX,4,*");} // contra
        if (incomingData2 == 2149) { GameMode = 13; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection="SW00"; Serial.println("starwars"); sendString("$PLAYX,4,*");} // starwars
        if (incomingData2 == 2150) { GameMode = 14; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection="JA5"; Serial.println("halo"); sendString("$PLAYX,4,*");} // halo
        if (incomingData2 == 2153) {
          TeamID = random(2);
          ProcessTeamsAssignmnet();
        }  
        if (incomingData2 == 2154) {
          TeamID = random(3);
          ProcessTeamsAssignmnet();
        }
        if (incomingData2 == 2155) {
          TeamID = random(4);
          ProcessTeamsAssignmnet();
        }
        if (incomingData2 == 2159) {
          // Trouble in Terrorist Town mode
        }
        if (incomingData2 == 2199 && incomingData1 == 99) {
          //ApplyGameSettings();
          Serial.println("start game");
          PBGameStart();
          ChangeMyColor = 9; // turns off led
        }
        if (incomingData2 == 2198 && incomingData1 == 99) {
          //ApplyGameSettings();
          Serial.println("delay start");
          AudioSelection = "OP04"; // says game count down initiated
          Audio(); 
          delay(13000);
          PBGameStart();
        }
      }
      if (incomingData2 == 2197) { 
        StandbyMode();
        INGAME = false;
        sendString("$PLAY,OP47,3,10,,,,,*"); // says "jedge init."
      }
      if (incomingData2 == 2196) { 
        PBEndGame();
      }
      Audio();
    }

    // 10000s used for JBOX
    
    if (incomingData2 == 31100) { // Zone Base Captured
      // check to see if friendly zone
        CompletedObjectives++; // added one point to player objectives
        EEPROM.write(193, CompletedObjectives);
        EEPROM.commit();
        AudioSelection="VAR"; // set an announcement "good job team"
        Audio(); // enabling BLE Audio Announcement Send
        Serial.println("Zone Base Captured");
        lastincomingData2 = 0;
    }    
    if (incomingData2 == 31000) { 
      if (INGAME) {
        LOOT = true;    
        Serial.println("Loot box found");
        lastincomingData2 = 0;
      }
    }
    // 31101 - currenthealth boost
    // 31102 - armor boost
    // 31103 - shield boost
    // 31104 - ammo boost
    // 31110 - team 0 spawn
    // 31111 - team 1 spawn
    // 31112 - team 2 spawn
    // 31113 - team 3 spawn
    if (incomingData2 == 31110) { 
      if (INGAME) {
        if (TeamID == 0) {
          // respawn if dead
          if (currenthealth == 0) {
            sendString("$SPAWN,*");
          } else {
            // in spawn area give away location
            AudioSelection = "N05"; // chirp
            Audio(); 
          }
        } else {
          // Notify player that player is close to enemy base - silently
          sendString("VIB,*");
        }
        lastincomingData2 = 0;
      }
    }
    if (incomingData2 == 31111) { 
      if (INGAME) {
        if (TeamID == 1) {
          // respawn if dead
          if (currenthealth == 0) {
            sendString("$SPAWN,*");
          } else {
            // in spawn area give away location
            AudioSelection = "N05"; // chirp
            Audio(); 
          }
        } else {
          // Notify player that player is close to enemy base - silently
          sendString("VIB,*");
        }
        lastincomingData2 = 0;
      }
    }
    if (incomingData2 == 31112) { 
      if (INGAME) {
        if (TeamID == 2) {
          // respawn if dead
          if (currenthealth == 0) {
            sendString("$SPAWN,*");
          } else {
            // in spawn area give away location
            AudioSelection = "N05"; // chirp
            Audio(); 
          }
        } else {
          // Notify player that player is close to enemy base - silently
          sendString("VIB,*");
        }
        lastincomingData2 = 0;
      }
    }
    if (incomingData2 == 31113) { 
      if (INGAME) {
        if (TeamID == 3) {
          // respawn if dead
          if (currenthealth == 0) {
            sendString("$SPAWN,*");
          } else {
            // in spawn area give away location
            AudioSelection = "N05"; // chirp
            Audio(); 
          }
        } else {
          // Notify player that player is close to enemy base - silently
          sendString("VIB,*");
        }
        lastincomingData2 = 0;
      }
    }
    if (31300 > incomingData2 > 31199) {
      // notification of a flag captured
      lastincomingData2 = 0;
      Serial.println("a flag was captured");
      int capture = incomingData2 - 31200;
      if (capture == TeamID) {
        // We captured the flag!
        AudioSelection="VA2K"; // setting audio play to notify we captured the flag
        Audio(); // enabling play back of audio
      }
      else {
        // enemey has our flag
        AudioSelection="VA2J"; // setting audio play to notify we captured the flag
        Audio(); // enabling play back of audio
      }
    }
  }
  }
  
  if (31600 > incomingData2 > 31500) {
      lastincomingData2 = 0;
      AudioSelection = "VA20";
      Audio();
      SpecialWeapon = incomingData2 - 31500;
      Serial.println("weapon Picked up");
      if (SpecialWeapon == 99) {
        AudioSelection="VA2K"; // setting audio play to notify we captured the flag
        Audio(); // enabling play back of audio
        // HASFLAG = true; // set condition that we have flag to true
        SpecialWeapon = 0; // loads a new weapon that will deposit flag into base
        Serial.println("Flag Capture, Gun becomes the flag"); 
        sendString("$WEAP,1,*");
        sendString("$WEAP,0,1,90,4,0,90,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*");
        sendString("$WEAP,4,*");
        sendString("$WEAP,3,*");
        sendString("$WEAP,5,*");
        datapacket2 = 31200 + TeamID;
        datapacket1 = 99;
        //BROADCASTESPNOW = true;
      }
      else {
        SPECIALWEAPONPICKUP = true;
      }
  }
  // repeat incoming command if coming from controller for reaching all taggers
  if (incomingData1 == 99 && incomingData3 > 97) { // this is a general broadcast command for all taggers for a setting and coming from the controller device
    datapacket1 = incomingData1;
    datapacket2 = incomingData2;
    datapacket4 = incomingData4;
    BROADCASTESPNOW = true;
  }
  digitalWrite(2, LOW);
}
}
void weaponalignment() {
  if (pbgame < 3) {// F4A, Death, Generals
    if (pbweap == 0) {
     AudioSelection = "VA4B";
    }
    if (pbweap == 1) {
     AudioSelection = "VA5Y";
    }
    if (pbweap == 2) {
     AudioSelection = "VA5R";
    }
    if (pbweap == 3) {
     AudioSelection = "VA6A";
    }
    if (pbweap == 4) {
     AudioSelection = "VA4F";
    }
    if (pbweap == 5) {
     AudioSelection = "VA6B";
    }
    if (pbweap == 6) {
     AudioSelection = "VA9O";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 0) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection = "V3M";
    }
    if (pbweap == 1) {
     AudioSelection = "VEM";
    }
    if (pbweap == 2) {
     AudioSelection = "V8M";
    }
    if (pbweap == 3) {
     AudioSelection = "VHM";
    }
    if (pbweap == 4) {
     AudioSelection = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection = "VS1";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 1) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection = "VCM";
    }
    if (pbweap == 1) {
     AudioSelection = "V7M";
    }
    if (pbweap == 2) {
     AudioSelection = "V2M";
    }
    if (pbweap == 3) {
     AudioSelection = "V1M";
    }
    if (pbweap == 4) {
     AudioSelection = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection = "VS1";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 2) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection = "VKM";
    }
    if (pbweap == 1) {
     AudioSelection = "VDM";
    }
    if (pbweap == 2) {
     AudioSelection = "VGM";
    }
    if (pbweap == 3) {
     AudioSelection = "VJM";
    }
    if (pbweap == 4) {
     AudioSelection = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection = "VS1";
    }
  }
  if (pbgame > 4 && pbteam == 0) {// survival, swarm
    if (pbweap == 0) {
     AudioSelection = "VA4B";
    }
    if (pbweap == 1) {
     AudioSelection = "VA5Y";
    }
    if (pbweap == 2) {
     AudioSelection = "VA5R";
    }
    if (pbweap == 3) {
     AudioSelection = "VA6A";
    }
    if (pbweap == 4) {
     AudioSelection = "VA4F";
    }
    if (pbweap == 5) {
     AudioSelection = "VA6B";
    }
    if (pbweap == 6) {
     AudioSelection = "VA9O";
    }
  }
  Audio();
}
void ClearScores() {
  Serial.println("resetting all scores");
  CompletedObjectives = 0;
  int teamcounter = 0;
  int eepromcounter = 184;
  int playercounter = 0;
  while (teamcounter < 4) {
    TeamKillCount[teamcounter] = 0;
    EEPROM.write(eepromcounter, TeamKillCount[teamcounter]);
    EEPROM.commit();
    teamcounter++;
    eepromcounter++;
    delay(1);
  }
  eepromcounter = 120;
  while (playercounter < 64) {
    PlayerKillCount[playercounter] = 0;
    EEPROM.write(eepromcounter, PlayerKillCount[playercounter]);
    EEPROM.commit();
    playercounter++;
    eepromcounter++;
    delay(1);
  }
  EEPROM.write(193, CompletedObjectives);
  Deaths = 0;
  EEPROM.write(188, Deaths);
  EEPROM.commit();
  Serial.println("Scores Reset");
}
void PBGameStart() {
          sendString("$VOL,0,0,*"); // adjust volume to default
          sendString("$PBLOCK,*");
          sendString("$PLAYX,0,*");
          sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
          sendString("$PLAY,VAQ,3,10,,,,,*"); // says let battle begin  
          if (!INGAME) {
            ClearScores();
            INGAME = true;
            EEPROM.write(194, 1);
            EEPROM.commit();
          }
          sendString("$PBINDOOR," + String(pbindoor) + ",*");
          sendString("$PBGAME," + String(pbgame) + ",*");
          sendString("$PBLIVES," + String(pblives) + ",*");
          sendString("$PBTIME," + String(pbtime) + ",*");
          sendString("$PBSPAWN," + String(pbspawn) + ",*");
          sendString("$PBWEAP," + String(pbweap) + ",*");
          if (pbperk < 7 && pbgame < 3) {
            sendString("$PBPERK," + String(pbperk) + ",*");
          }
          delay(2000);
          sendString("$PBSTART,*");
          sendString("$PID," + String(GunID) + ",*");
          sendString("$TID," + String(TeamID) + ",*");
          sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4****
          sendString("$BMAP,3,98,,,,,*"); // sets the select button as Weapon 5****
          if (GameMode == 15) {
            sendString("$IRTX,200,15,63,1,6,0,0,100,5,500,0,*"); // sends respawn command to get everyone in respawn mode
          }
          if (GameMode == 4) { // battle royale
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5****
            sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5****
            sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,40,,*");
            sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,40,,*");
            sendString("$WEAP,3,1,90,15,0,6,0,,,,,,,,1400,50,1,32768,0,10,13,100,100,,0,0,,H29,,,,,,,D18,,,0,,1,9999999,20,,*"); // Set weapon 3 as respawn
            sendString("$WEAP,3,1,90,15,0,6,0,,,,,,,,1400,50,1,32768,0,10,13,100,100,,0,0,,H29,,,,,,,D18,,,0,,1,9999999,20,,*"); // Set weapon 3 as respawn
            sendString("$WEAP,1,*");
            sendString("$WEAP,1,*");        
          }  
          if (GameMode == 8) { // assimilation
            // NOTHING? JUST TRIGGER ADDED WHEN DEAD 
          }
          if (GameMode == 12) { // contra
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$PSET,"+String(GunID)+","+String(TeamID)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            sendString("$PSET,"+String(GunID)+","+String(TeamID)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
            MusicSelection = "$PLAY,CC08,4,10,,,,,*"; 
            BACKGROUNDMUSIC = true; 
            MusicRestart = 107000; 
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            Audio();;
          }
          if (GameMode == 13) { // starwars
            sendString("$PSET,"+String(GunID)+","+String(TeamID)+",45,50,,H44,JAD,SW05,SW01,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
            sendString("$PSET,"+String(GunID)+","+String(TeamID)+",45,50,,H44,JAD,SW05,SW01,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
            sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
            sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
            sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
            sendString("$WEAP,4,1,100,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
            sendString("$WEAP,4,1,100,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
            if (TeamID == 0 || TeamID == 2) {
              MusicSelection = "$PLAY,SW31,4,10,,,,,*";
              MusicRestart = 60000;
            } else {
              MusicSelection = "$PLAY,SW04,4,10,,,,,*";
              MusicRestart = 92000;
            }
            BACKGROUNDMUSIC = true;
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            Audio();;
          }
          if (GameMode == 14) { // halo
            if (TeamID == 0 || TeamID == 2) {
              MusicSelection = "$PLAY,JAN,4,10,,,,,*";
              MusicRestart = 59000;
            }
            if (TeamID == 1) {
              MusicSelection = "$PLAY,JAO,4,10,,,,,*";
              MusicRestart = 59000;
            }
            if (TeamID == 3) {
              MusicSelection = "$PLAY,JAP,4,10,,,,,*";
              MusicRestart = 59000;
            }
            BACKGROUNDMUSIC = true;
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            Audio();
          }
}
//****************************************************
// WebServer 
//****************************************************
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

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
    <h1>JEDGE 5.0 Config Menu</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Set Player Mode</h2>
      <p><select name="primary" id="primaryid">
        <option value="99">Select a Mode</option>
        <option value="4">Standard JEDGE Player</option> 
        <option value="0">Auto Start Player - Strong</option>
        <option value="1">Auto Start Player - Stronger</option>
        <option value="2">Auto Start Player - Strongest</option>  
        <option value="3">Auto Start Player - Ridiculous</option>       
        </select>
      </p>
      <h2>Tagger Generation</h2>
      <p><select name="Gen" id="GenID">
        <option value="99">Select a Tagger Gen</option>
        <option value="12">Gen 2/3</option>
        <option value="11">Gen 1</option>
        <option value="13">Gen 1 - hardwired</option>
        <option value="14">Gen 2/3 - hardwired</option>   
        </select>
      </p>
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
      <p><button id="otaupdate" class="button">OTA Firmware Refresh</button></p>   
      <form action="/get">
        wifi ssid: <input type="text" name="input1">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        wifi pass: <input type="text" name="input2">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        BLE Server Name: <input type="text" name="input3">
        <input type="submit" value="Submit">
      </form><br>
      <h2>Gen1 format: "1234,ab,abcdef"</h2>
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
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('otaupdate').addEventListener('click', toggle14o);
    document.getElementById('playerassignmentid').addEventListener('change', handleplayerassignment, false);
    document.getElementById('GenID').addEventListener('change', handleGen, false);
  }
  function toggle14o(){
    websocket.send('toggle14o');
  }
  function handleGen() {
    var xb = document.getElementById("GenID").value;
    websocket.send(xb);
  }
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handleplayerassignment() {
    var xp = document.getElementById("playerassignmentid").value;
    websocket.send(xp);
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
  Serial.println("handling websocket message");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    ledState = !ledState;
    if (strcmp((char*)data, "toggle14o") == 0) { // update firmware
      Serial.println("OTA Update Mode");
      EEPROM.write(0, 1); // reset OTA attempts counter
      EEPROM.commit(); // Store in Flash
      delay(2000);
      ESP.restart();
    }
    if (strcmp((char*)data, "0") == 0) {
      Serial.println("Easy Boss Mode");
      SetPlayerMode = 0;
      EEPROM.write(1,SetPlayerMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1") == 0) {
      Serial.println("Normal Boss Mode");
      SetPlayerMode = 1;
      EEPROM.write(1,SetPlayerMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "2") == 0) {
      Serial.println("Hard Boss Mode");
      SetPlayerMode = 2;
      EEPROM.write(1,SetPlayerMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "3") == 0) {
      Serial.println("Ridiculous Boss Mode");
      SetPlayerMode = 3;
      EEPROM.write(1,SetPlayerMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "4") == 0) {
      Serial.println("Standard Player Mode");
      SetPlayerMode = 4;
      EEPROM.write(1,SetPlayerMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "11") == 0) {
      Serial.println("Tagger is a Gen 1");
      GunGeneration = 1;
      EEPROM.write(196,GunGeneration);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "12") == 0) {
      Serial.println("Tagger is a Gen 2/3");
      GunGeneration = 2;
      EEPROM.write(196,GunGeneration);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "13") == 0) {
      Serial.println("Tagger is a hardwired gen 1");
      GunGeneration = 11;
      EEPROM.write(196,GunGeneration);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "14") == 0) {
      Serial.println("Tagger is a hard wired Gen 2/3");
      GunGeneration = 12;
      EEPROM.write(196,GunGeneration);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1900") == 0) {
      GunID = 0;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //00;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1901") == 0) {
      GunID = 1;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //01;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1902") == 0) {
      GunID = 2;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //02;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1903") == 0) {
      GunID = 3;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //03;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1904") == 0) {
      GunID = 4;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //04;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1905") == 0) {
      GunID = 5;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //05;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1906") == 0) {
      GunID = 6;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //06;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1907") == 0) {
      GunID = 7;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //07;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1908") == 0) {
      GunID = 8;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //08;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1909") == 0) {
      GunID = 9;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //09;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1910") == 0) {
      GunID = 10;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //10;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1911") == 0) {
      GunID = 11;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //11;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1912") == 0) {
      GunID = 12;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //12;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1913") == 0) {
      GunID = 13;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //13;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1914") == 0) {
      GunID = 14;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //14;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1915") == 0) {
      GunID = 15;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //15;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1916") == 0) {
      GunID = 16;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //16;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1917") == 0) {
      GunID = 17;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //17;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1918") == 0) {
      GunID = 18;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //18;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1919") == 0) {
      GunID = 19;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //19;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1920") == 0) {
      GunID = 20;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //20;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1921") == 0) {
      GunID = 21;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //21;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1922") == 0) {
      GunID = 22;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //22;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1923") == 0) {
      GunID = 23;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //23;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1924") == 0) {
      GunID = 24;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //24;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1925") == 0) {
      GunID = 25;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //25;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1926") == 0) {
      GunID = 26;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //26;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1927") == 0) {
      GunID = 27;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //27;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1928") == 0) {
      GunID = 28;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //28;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1929") == 0) {
      GunID = 29;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //29;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1930") == 0) {
      GunID = 30;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //30;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1931") == 0) {
      GunID = 31;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //31;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1932") == 0) {
      GunID = 32;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //32;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1933") == 0) {
      GunID = 33;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //33;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1934") == 0) {
      GunID = 34;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //34;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1935") == 0) {
      GunID = 35;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //35;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1936") == 0) {
      GunID = 36;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //36;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1937") == 0) {
      GunID = 37;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //37;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1938") == 0) {
      GunID = 38;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //38;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1939") == 0) {
      GunID = 39;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //39;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1940") == 0) {
      GunID = 40;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //40;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1941") == 0) {
      GunID = 41;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //41;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1942") == 0) {
      GunID = 42;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //42;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1943") == 0) {
      GunID = 43;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //43;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1944") == 0) {
      GunID = 44;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //44;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1945") == 0) {
      GunID = 45;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //45;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1946") == 0) {
      GunID = 46;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //46;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1947") == 0) {
      GunID = 47;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //47;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1948") == 0) {
      GunID = 48;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //48;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1949") == 0) {
      GunID = 49;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //49;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1950") == 0) {
      GunID = 50;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //50;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1951") == 0) {
      GunID = 51;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //51;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1952") == 0) {
      GunID = 52;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //52;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1953") == 0) {
      GunID = 53;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //53;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1954") == 0) {
      GunID = 54;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //54;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1955") == 0) {
      GunID = 55;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //55;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1956") == 0) {
      GunID = 56;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //56;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1957") == 0) {
      GunID = 57;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //57;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1958") == 0) {
      GunID = 58;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //58;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1959") == 0) {
      GunID = 59;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //59;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1960") == 0) {
      GunID = 60;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //60;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1961") == 0) {
      GunID = 61;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //61;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1962") == 0) {
      GunID = 62;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //62;
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1963") == 0) {
      GunID = 63;
      Serial.println("Gun ID changed to: " + String(GunID));
      EEPROM.write(10, GunID); //63;
      EEPROM.commit();
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

//**************************************************************
//**************************************************************
// this bad boy captures any ble data from gun and analyzes it based upon the
// protocol predecessor, a lot going on and very important as we are able to 
// use the ble data from gun to decode brx protocol with the gun, get data from 
// guns currenthealth status and ammo reserve etc.

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);

  memcpy(notifyData + notifyDataIndex, pData, length);
  notifyDataIndex = notifyDataIndex + length;
  if (notifyData[notifyDataIndex - 1] == '*') { // complete receviing
    notifyData[notifyDataIndex] = '\0';
    notifyDataIndex = 0; // reset index
    //Lets tokenize by ","
    String receData = notifyData;
    byte index = 0;
    ptr = strtok((char*)receData.c_str(), ",");  // takes a list of delimiters
    while (ptr != NULL){
      tokenStrings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    Serial.println("We have found " + String(index ) + " tokens");
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
    //TerminalInput = "Modified Gen3 Data to Match Gen 1/2";
    }
    //*******************************************************
    ProcessBRXData();
  }
}

//******************************************************************************************************
// BLE client stuff, cant say i understan all of it but yeah... had some help with this
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      doConnect = true;
      doScan = true; // enables scaning for designated BLE server (tagger)
      // WEAP = false; used to trigger sending game start commands to tagger
      Serial.println("onDisconnect");

    }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  Serial.println(" - Created client");
  // Connect to the remove BLE Server.
  if (pClient->connect(myDevice)) { // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteRXCharacteristic = pRemoteService->getCharacteristic(charRXUUID);
    if (pRemoteRXCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charRXUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our RX characteristic");
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteTXCharacteristic = pRemoteService->getCharacteristic(charTXUUID);
    if (pRemoteTXCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charTXUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our TX characteristic");
    // Read the value of the characteristic.
    if (pRemoteRXCharacteristic->canRead()) {
      std::string value = pRemoteRXCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    if (pRemoteTXCharacteristic->canRead()) {
      std::string value = pRemoteTXCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    if (pRemoteTXCharacteristic->canNotify())
      pRemoteTXCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    return true;
  }
  return false;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      // We have found a device, let us now see if it contains the service we are looking for.
      //if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      if (advertisedDevice.getName() == BLEName.c_str()) {
      //if (advertisedDevice.getName() == BLE_SERVER_SERVICE_NAME) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        Serial.println("Found the correct BLE Device, Pairing in 2 seconds");
        delay(2000);
        doConnect = true;
        doScan = false;
      } // Found our server
      else {
        doScan = true;
      }
    } // onResult
}; // MyAdvertisedDeviceCallbacks
//******************************************************************************************
void BluetoothSetup() {
  Serial.println("Setting Up Bluetooth Classic");
  SerialBT.begin(38400, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up serial communication with ESP8266 on pins 16 as RX/17 as Tx w baud rate of 9600
  pinMode(4, INPUT); // Status Pin on HC-05, used to determine if paired with a tagger
}
void ListenToBluetoothClassic() {
  if (SerialBT.available()) {
    String receData = SerialBT.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(receData); // this is printing... 
    byte index = 0;
    ptr = strtok((char*)receData.c_str(), ",");  // takes a list of delimiters
    while (ptr != NULL)
    {
      tokenStrings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    Serial.println("We have found " + String(index ) + " tokens"); // this isnt breaking out the characters...
    for(int i=0;i<index;i++){
      Serial.print(tokenStrings[i]);
      Serial.print(':');
    }
    Serial.println();    
  }
  ProcessBRXData();
}
void BLESetup(){
BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.

  /**
 * notes from possible ways to increase output power
 * 
 * bledevice::setPower(Powerlevel);
 * @brief Set the transmission power.
 * The power level can be one of:
 * * ESP_PWR_LVL_N14
 * * ESP_PWR_LVL_N11
 * * ESP_PWR_LVL_N8
 * * ESP_PWR_LVL_N5
 * * ESP_PWR_LVL_N2
 * * ESP_PWR_LVL_P1
 * * ESP_PWR_LVL_P4
 * * ESP_PWR_LVL_P7
 * @param [in] powerLevel.
 * esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P9);
 * esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
 * esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_P9); 
 */

  esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_P7); // updated version 4/14/2020
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P7); // updated version 4/14/2020
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_P7); // updated version 4/14/2020
  BLEDevice::setPower(ESP_PWR_LVL_P7);
  //BLEDevice::setPower(ESP_PWR_LVL_N14); // old version
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10, true);

  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  }
//**************************************************************************
// run SWAPTX style weapon swap as from my youtube funny video:
void swapbrx() {
  // Lets Roll for a perk!
  int LuckyRoll = random(100);
  /*  lets talk perks!
   *  0. pick up body armor = add defense                Chance: 10%
   *  1. pick up a med kit - restore health              Chance: 10%
   *  2. pick up a full restore of health and armor      Chance: 10%
   *  3. pick up a shotgun                               Chance: 10%
   *  4. pick up an SMG                                  Chance: 10%
   *  5. pick up a sniper rifle                          Chance: 10%
   *  6. pick up an assault rifle                        Chance: 10%
   *  7. pick up a rocket launcher                       Chance: 10%
   *  8. pick up ammo for your current weapon            Chance: 10%
   *  9. pick up a weapon upgrade for your stock pistol  Chance: 10%
   */
  // apply the perk!
  if (LuckyRoll > 0 && LuckyRoll < 10) {
    // body armor picked up, Adding full body armor restore:
    AudioSelection = "OP42";
    sendString("$BUMP,70,0,1,0,*");
    //sendString("$LIFE,,70,,1,*"); // $BUMP,5,1,1,0,*  hp, armor, shields
  }
  if (LuckyRoll > 9 && LuckyRoll < 20) {
    // MedKit picked up, Adding full health restore:
    AudioSelection = "OP34";
    sendString("$BUMP,45,1,0,0,*");
    //sendString("$LIFE,,,45,1,*");
  }
  if (LuckyRoll > 19 && LuckyRoll < 30) {
    // Full health/armor restore picked up:
    AudioSelection = "OP44";
    sendString("$BUMP,70,0,1,0,*");
    sendString("$BUMP,45,1,,0,*");
    //sendString("$LIFE,,70,45,1,*");
  }
  if (LuckyRoll > 29 && LuckyRoll < 40) {
    // shotgun picked up:
    AudioSelection = "OP37";
    SpecialWeapon = 15;
    SELECTCONFIRM = true; // enables trigger for select button
    SPECIALWEAPONLOADOUT = true;
    SelectButtonTimer = millis();
    //AudioSelection1="VA9B"; // set audio playback to "press select button"
  }
  if (LuckyRoll > 39 && LuckyRoll < 50) {
    // SMG picked up:
    AudioSelection = "OP40";
    SpecialWeapon = 16;
    SELECTCONFIRM = true; // enables trigger for select button
    SPECIALWEAPONLOADOUT = true;
    SelectButtonTimer = millis();
    //AudioSelection1="VA9B"; // set audio playback to "press select button"
  }
  if (LuckyRoll > 49 && LuckyRoll < 60) {
    // sniper picked up:
    AudioSelection = "OP38";
    SpecialWeapon = 17;
    SELECTCONFIRM = true; // enables trigger for select button
    SPECIALWEAPONLOADOUT = true;
    SelectButtonTimer = millis();
    //AudioSelection1="VA9B"; // set audio playback to "press select button"
  }
  if (LuckyRoll > 59 && LuckyRoll < 70) {
    // assault rifle picked up:
    AudioSelection = "OP39";
    SpecialWeapon = 3;
    SELECTCONFIRM = true; // enables trigger for select button
    SPECIALWEAPONLOADOUT = true;
    SelectButtonTimer = millis();
    //AudioSelection1="VA9B"; // set audio playback to "press select button"
  }
  if (LuckyRoll > 69 && LuckyRoll < 80) {
    // rocket picked up:
    AudioSelection = "OP36";
    SpecialWeapon = 14;
    SELECTCONFIRM = true; // enables trigger for select button
    SPECIALWEAPONLOADOUT = true;
    SelectButtonTimer = millis();
    //AudioSelection1="VA9B"; // set audio playback to "press select button"
  }
  if (LuckyRoll > 79 && LuckyRoll < 90) {
    // Ammo picked up:
    AudioSelection = "OP41";
    SpecialWeapon = PreviousSpecialWeapon; // makes special weapon equal to whatever the last special weapon was set to
    LoadSpecialWeapon(); // reloads the last special weapon again to refresh ammo to original maximums
  }
  if (LuckyRoll > 99 && LuckyRoll < 100) {
    // Pistol upgrade picked up:
    AudioSelection = "VA6W";
    // increase pistol level 0-9; 
    if (SwapPistolLevel < 9) {
      SwapPistolLevel++;
      if (SwapPistolLevel == 1) {
        sendString(SwapBRXPistol1);
      }
      if (SwapPistolLevel == 2) {
        sendString(SwapBRXPistol2);
      }
      if (SwapPistolLevel == 3) {
        sendString(SwapBRXPistol3);
      }
      if (SwapPistolLevel == 4) {
        sendString(SwapBRXPistol4);
      }
      if (SwapPistolLevel == 5) {
        sendString(SwapBRXPistol5);
      }
      if (SwapPistolLevel == 6) {
        sendString(SwapBRXPistol6);
      }
      if (SwapPistolLevel == 7) {
        sendString(SwapBRXPistol7);
      }
      if (SwapPistolLevel == 8) {
        sendString(SwapBRXPistol8);
      }
      if (SwapPistolLevel == 9) {
        sendString(SwapBRXPistol9);
      }
    }
  }
  Audio();
}
//******************************************************************************************
void ProcessBRXData() {
  // apply conditional statements for when coms com from tagger
    
    // TIME REMAINING IN OFFLINE GAME MODE OR PB GAMES 
    if (tokenStrings[0] == "$TIME") {
      if (tokenStrings[1] == "0") {
        // THIS INDICATES THAT TIME IS UP IN GAME
        INGAME = false;
        EEPROM.write(2, 1);
        EEPROM.commit();
        delay(3000);
        StandbyMode();
      }
    }
    if (tokenStrings[0] == "$BUT") { // a button was pressed
      if (tokenStrings[1] == "3") { // select button was pressed
        if (tokenStrings[2] == "0") { // select button was depressed
          long CurrentScoreCheck = millis();
          if (CurrentScoreCheck - LastScoreCheck < 2000) {
              // lets do a score announcement
              String tempaudio = "$PLAY,KL" + String(MyKills) + ",3,10,,,,,*";
              sendString(tempaudio);
              delay(2000);
              tempaudio = "$PLAY,DT" + String(Deaths) + ",3,10,,,,,*";
              sendString(tempaudio);
          } else {
            LastScoreCheck = millis();
          }
        }
      }
    }
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
    if (tokenStrings[2] == "15" && tokenStrings[5] == "16" && tokenStrings[4] == String(TeamID)) { // this is a KOTH tag
      // add objective point for being in the radius of a hill
      CompletedObjectives++;
      EEPROM.write(193, CompletedObjectives);
      EEPROM.commit();
    }
    lastTaggedPlayer = tokenStrings[3].toInt();
    lastTaggedTeam = tokenStrings[4].toInt();
    //Serial.println("Just tagged by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
  }
  if (tokenStrings[0] == "$LCD") {
    /* SPAWN STATS UPDATE status update occured 
     *  space 0 is LCD indicator
     *  space 1 is currenthealth
     *  space 2 is ARMOR
     *  space 3 is SHIELD
     *  space 4 is WEAPON SLOT
     *  space 5 is CLIPS
     *  SPACE 6 IS AMMO
     *  
     *  more can be done with this, like using the ammo info to post to lcd
    */
    if (REQUESTQUERY) {
      REQUESTQUERY = false;
    }
    currenthealth = tokenStrings[1].toInt(); // 
      if (GameMode > 11) { // we are playing a music game
        BACKGROUNDMUSIC = true;
      }
      if (GameMode == 13) { // starwars, need to respawn the weapons!
        sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
        sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
        sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
        sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
        sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
        sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
        sendString("$WEAP,4,1,90,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
        sendString("$WEAP,4,1,90,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
      }
      if (GameMode == 12) { // contra mode, bring back weapons
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            //sendString("$PSET,"+String(GunID)+","+String(TeamID)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            //sendString("$PSET,"+String(GunID)+","+String(TeamID)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
      }
      if (GameMode == 4) { // battle royale
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,3,3,,,,,*"); // sets the select button as Weapon 3****
            sendString("$BMAP,3,3,,,,,*"); // sets the select button as Weapon 3****
            // reinstall last pistol upgrade?
            if (SwapPistolLevel == 1) {
              sendString(SwapBRXPistol1);
              sendString(SwapBRXPistol1);
            }
            if (SwapPistolLevel == 2) {
              sendString(SwapBRXPistol2);
              sendString(SwapBRXPistol2);
            }
            if (SwapPistolLevel == 3) {
              sendString(SwapBRXPistol3);
              sendString(SwapBRXPistol3);
            }
            if (SwapPistolLevel == 4) {
              sendString(SwapBRXPistol4);
              sendString(SwapBRXPistol4);
            }
            if (SwapPistolLevel == 5) {
              sendString(SwapBRXPistol5);
              sendString(SwapBRXPistol5);
            }
            if (SwapPistolLevel == 6) {
              sendString(SwapBRXPistol6);
              sendString(SwapBRXPistol6);
            }
            if (SwapPistolLevel == 7) {
              sendString(SwapBRXPistol7);
              sendString(SwapBRXPistol7);
            }
            if (SwapPistolLevel == 8) {
              sendString(SwapBRXPistol8);
              sendString(SwapBRXPistol8);
            }
            if (SwapPistolLevel == 9) {
              sendString(SwapBRXPistol9);
              sendString(SwapBRXPistol9);
            }
            if (SwapPistolLevel == 0) {
              sendString(SwapBRXPistol);
              sendString(SwapBRXPistol);
            }
            sendString("$WEAP,3,1,90,15,0,6,0,,,,,,,,1400,50,1,32768,0,10,13,100,100,,0,0,,H29,,,,,,,D18,,,0,,1,9999999,20,,*"); // Set weapon 3 as respawn
            sendString("$WEAP,3,1,90,15,0,6,0,,,,,,,,1400,50,1,32768,0,10,13,100,100,,0,0,,H29,,,,,,,D18,,,0,,1,9999999,20,,*"); // Set weapon 3 as respawn
            sendString("$WEAP,1,*");
            sendString("$WEAP,1,*");        
          }
      // need to add in battle royale...
  }
  if (tokenStrings[0] == "$HP") {
    /*currenthealth status update occured
     * can be used for updates on currenthealth as well as death occurance
     */
    currenthealth = tokenStrings[1].toInt(); // setting variable to be sent to esp8266
    armor = tokenStrings[2].toInt(); // setting variable to be sent to esp8266
    if ((tokenStrings[1] == "0") && (tokenStrings[2] == "0") && (tokenStrings[3] == "0")) { // player is dead
        long CurrentDeathTime = millis();
        if (CurrentDeathTime - LastDeathTime > 2000) {
          LastDeathTime = millis();
          if (GameMode > 11) {
            BACKGROUNDMUSIC = false;
          }
          if (GameMode == 8) {
            // playersettings();
            if (lastTaggedTeam == 0) {sendString("$PLAY,VA13,3,10,,,,,*"); delay(300); sendString("$TID,0,*"); sendString("$TID,0,*");}
            if (lastTaggedTeam == 1) {sendString("$PLAY,VA1L,3,10,,,,,*"); delay(300); sendString("$TID,1,*"); sendString("$TID,1,*");}
            if (lastTaggedTeam == 2) {sendString("$PLAY,VA1R,3,10,,,,,*"); delay(300); sendString("$TID,2,*"); sendString("$TID,2,*");}
            if (lastTaggedTeam == 3) {sendString("$PLAY,VA27,3,10,,,,,*"); delay(300); sendString("$TID,3,*");sendString("$TID,3,*");}
          }
          PlayerKillCount[lastTaggedPlayer]++; // adding a point to the last player who killed us
          TeamKillCount[lastTaggedTeam]++; // adding a point to the team who caused the last kill
          Deaths++;
          byte peepromid = lastTaggedPlayer + 120;
          byte teepromid = lastTaggedTeam + 184;
          EEPROM.write(peepromid, PlayerKillCount[lastTaggedPlayer]);
          EEPROM.write(teepromid, TeamKillCount[lastTaggedTeam]);
          EEPROM.write(188, Deaths);
          EEPROM.commit();
          //   call audio to announce player's killer
          delay(5000);
          sendString("$PLAY,KB"+String(lastTaggedPlayer)+"1,10,,,,,*");
          PLAYERDIED = true;
        }
      }
    }
}

void StartBossPlayer() {
  sendString("$CLEAR,*");
  sendString("$START,*");
  sendString("$VOL,100,0,*"); // adjust volume to default
  // token one of the following command is free for all, 0 is off and 1 is on
  sendString("$GSET,0,0,1,0,1,0,50,1,*");
  String currenthealth = "null";
  if (SetPlayerMode == 0) {
    currenthealth = "500,250,150";
  }
  if (SetPlayerMode == 1) {
    currenthealth = "1000,500,250";
  }
  if (SetPlayerMode == 2) {
    currenthealth = "2000,1000,500";
  }
  if (SetPlayerMode == 3) {
    currenthealth = "6000,3000,500";
  }
  sendString("$PSET,63,2,"+String(currenthealth)+",50,,H44,JAD,V33,M05,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
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
  sendString("$SIR,4,0,,1,0,0,1,,*"); // standard damgat but is actually the captured flag pole
  sendString("$SIR,0,0,,1,0,0,1,,*"); // standard weapon, damages shields then armor then HP
  sendString("$SIR,0,1,,36,0,0,1,,*"); // force rifle, sniper rifle ?pass through damage?
  sendString("$SIR,0,3,,37,0,0,1,,*"); // bolt rifle, burst rifle, AMR
  sendString("$SIR,1,0,H29,10,0,0,1,,*"); // adds HP with this function********
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*"); // adds shields*******
  sendString("$SIR,3,0,VA16,13,0,0,1,,*"); // adds armor*********
  sendString("$SIR,6,0,H02,1,0,90,1,40,*"); // rail gun
  sendString("$SIR,8,0,,38,0,0,1,,*"); // charge rifle
  sendString("$SIR,9,3,,24,10,0,,,*"); // energy launcher
  sendString("$SIR,10,0,X13,1,0,100,2,60,*"); // rocket launcher
  sendString("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
  sendString("$SIR,13,1,H57,1,0,0,1,,*"); // energy blade
  sendString("$SIR,13,0,H50,1,0,0,1,,*"); // rifle bash
  sendString("$SIR,13,3,H49,1,0,100,0,60,*"); // war hammer
  sendString("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
  sendString("$BMAP,1,100,0,1,2,6,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
  sendString("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
  sendString("$BMAP,3,3,,,,,*"); // sets the select button as Weapon 3
  sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4
  sendString("$BMAP,5,5,,,,,*"); // Sets the right button as weapon 5
  sendString("$BMAP,8,4,,,,,*"); // sets the gyro as weapon 4
  sendString("$SPAWN,,*");
  String Weap0 = "$WEAP,0,,100,0,0,24,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*";
  String Weap1 = "$WEAP,1,,100,8,0,150,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*";
  String Weap2 = "$WEAP,2,,100,0,0,150,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*";
  String Weap3 = "$WEAP,3,1,90,11,1,1,0,,,,,,1,80,1400,50,10,0,0,10,11,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,10,9999999,30,30,*";
  String Weap4 = "$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,30,,*";
  String Weap5 = "$WEAP,5,1,90,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,30,20,*";
  String Weap6 = "$WEAP,6,,100,11,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*";
  sendString(Weap0); // Assault Rifle
  sendString(Weap4); // Melee
  if (SetPlayerMode == 1) {
    sendString(Weap1); // Charge Rifle
    sendString(Weap3); // Gas players melee
  }
  if (SetPlayerMode == 2) {
    sendString(Weap1); // Charge Rifle
    sendString(Weap2); // Ion Sniper
    sendString(Weap3); // Gas players melee
    sendString(Weap5); // Explosion melee
  }
  if (SetPlayerMode == 3) {
    sendString(Weap1); // Charge Rifle
    sendString(Weap2); // Ion Sniper
    sendString(Weap3); // Gas players melee
    sendString(Weap5); // Explosion melee
    sendString(Weap6); // Starwars Stun Gun
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
    sendString("$WEAP,1,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,80,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
  if(SpecialWeapon == 4) {
    Serial.println("Weapon 1 set to Bolt Rifle"); 
    sendString("$WEAP,1,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
  if(SpecialWeapon == 5) {
    Serial.println("Weapon 1 set to BurstRifle");
    sendString("$WEAP,1,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,90,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
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
    sendString("$WEAP,1,2,80,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,50,30,*");
    }
  if(SpecialWeapon == 16) {
    Serial.println("Weapon 1 set to SMG"); 
    sendString("$WEAP,1,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,80,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
  if(SpecialWeapon == 17) {
    Serial.println("Weapon 1 set to Sniper Rifle"); 
    sendString("$WEAP,1,,100,0,1,115,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
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
// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
// had some major help from Sri Lanka Guy!
void sendString(String value) {
  int genonedelay = 6;
  int matchingdelay = value.length() * genonedelay ; // this just creates a temp value for how long the string is multiplied by 5 milliseconds
  const char * c_string = value.c_str();
  uint8_t buf[21] = {0};
  int sentSize = 0;
  Serial.println("sending ");
  Serial.println(value);
  if (GunGeneration == 2) { // blue tooth gen 2/3
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    if (value.length() > 20) {
      for (int i = 0; i < value.length() / 20; i++) {
        memcpy(buf, c_string + i * 20, 20);
        Serial.print((char*)buf);
        pRemoteRXCharacteristic->writeValue(buf, 20, true);
        sentSize += 20;
      }
      int remaining = value.length() - sentSize;
      memcpy(buf, c_string + sentSize, remaining);
      pRemoteRXCharacteristic->writeValue(buf, remaining, true);
      for (int i = 0; i < remaining; i++)
        Serial.print((char)buf[i]);
      Serial.println();
    }
    else {
      pRemoteRXCharacteristic->writeValue((uint8_t*)value.c_str(), value.length(), true);
    }
  }
  if (GunGeneration == 1) { // bluetooth gen 1
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    SerialBT.print(value);
  }
  if (GunGeneration == 12) { // serial gen 2/3
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    SerialBT.println(value); // sending the string to brx gen 2
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    SerialBT.println(value);
  }
  if (GunGeneration == 11) { // Serial Gen1
    for (int i = 0; i < value.length() / 1; i++) { // creates a for loop
      SerialBT.println(c_string[i]); // sends one value at a time to gen1 brx
      delay(genonedelay); // this is an added delay needed for the way the brx gen1 receives data (brute force method because i could not match the comm settings exactly)
    }
    for (int j = 0; j < value.length() / 1; j++) { // creates a for loop
      SerialBT.println(c_string[j]); // sends one value at a time to gen1 brx
      delay(genonedelay); // this is an added delay needed for the way the brx gen1 receives data (brute force method because i could not match the comm settings exactly)
    }
  }
}

//******************************************************************************************

void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(OTAssid.c_str(), OTApassword.c_str());
  int attemptcounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attemptcounter++;
    if (attemptcounter > 20) {
      ESP.restart();
    }
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
      EEPROM.write(0, 0); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      return 0;} 
    else 
    {
      EEPROM.write(0, 0); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      Serial.println(payload);
      Serial.println("New firmware detected");
      
      return 1;
    }
  } 
  return 0;  
}
void PBEndGame() {
  sendString("$VOL,0,0,*"); // adjust volume to default
  sendString("$PBLOCK,*");
  sendString("$PLAYX,0,*");
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  INGAME = false;
  EEPROM.write(194, 0);
  EEPROM.commit();
  Serial.println("sending game exit commands");
  BACKGROUNDMUSIC = false;
  SwapBRXCounter = 0;
  SWAPBRX = false;
  SELECTCONFIRM = false;
  SPECIALWEAPONLOADOUT = false;
  PreviousSpecialWeapon = 0;
  Serial.println("sending game exit commands");
  delay(500);
  
  AudioSelection = "VA33"; // "game over"
  Audio();
  delay(1500);
  AudioSelection="VAP"; // "head back to base"
  Audio();
  Serial.println("Game Over Object Complete");
  if (GameMode == 12) {
    sendString("$PLAY,CC09,3,10,,,,,*"); // says game over
  } else {
    if (GameMode == 13) {
      sendString("$PLAY,SW03,4,6,,,,,*");
    } else {
      sendString("$PLAY,VS6,3,10,,,,,*"); // says game over
    }
  }
  Serial.println("Game Over Object Complete");
  CurrentDominationLeader = 99; // resetting the domination game status
  ClosingVictory[0] = 0;
  ClosingVictory[1] = 0;
  ClosingVictory[2] = 0;
  ClosingVictory[3] = 0;
  StandbyMode();
}
void StandbyMode() {
  // NOW WE START A GAME THAT DOESNT ALLOW TAGGERS TO DO ANYTHING SO WE CAN MONITOR BUTTON PRESSES:
  sendString("$VOL,0,0,*"); // adjust volume to ZERO
  Serial.println();
  Serial.println("JEDGE is taking over the BRX");
  sendString("$PHONE,*"); // ENABLES BACK AND FORTH COMS WITH BRX
  sendString("$PBLOCK,*"); // ENDS EXISTING GAME WITH BRX - IF ANY
  sendString("$PLAYX,0,*"); // SILENCES AUDIO CHANNELS
  sendString("$PBINDOOR,1,*");
  sendString("$PBGAME,1,*");
  sendString("$PBLIVES,5,*");
  sendString("$PBTIME,5,*");
  sendString("$PBSPAWN,0,*");
  sendString("$PBWEAP,6,*");
  sendString("$PBSTART,*");
  sendString("$BMAP,1,98,,,,,*"); 
  sendString("$BMAP,2,98,,,,,*");
  sendString("$BMAP,3,98,,,,,*");
  sendString("$BMAP,4,98,,,,,*");
  sendString("$BMAP,5,98,,,,,*"); 
  sendString("$BMAP,8,98,,,,,*");
  sendString("$WEAP,0,*");
  sendString("$WEAP,3,*");
  sendString("$WEAP,1,*");
  sendString("$WEAP,2,*");   
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to CURRENT SETTINGS
}
void StartWebServer() {
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //***********************************************************************
  
  // initialize web server
  initWebSocket();
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
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
      for (int i = 11; i < 44; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom ssid:");
      int j = 11;
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
      for (int i = 44; i < 100; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom Password:");
      int k = 44;
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
      BLEName = inputMessage;
      Serial.println("clearing eeprom");
      for (int i = 100; i < 120; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom Password:");
      int l = 100;
      for (int i = 0; i < BLEName.length(); ++i)
      {
        EEPROM.write(l, BLEName[i]);
        Serial.print("Wrote: ");
        Serial.println(BLEName[i]);
        l++;
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
    Serial.print("BLE Server Name = ");
    Serial.println(BLEName);
    if (GunGeneration == 1) {
      // need to run AT scripts via serial to set up the HC-05
      SetUpHC05();
    }
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  // Start server
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  //WebSerial.begin(&server);
  //WebSerial.msgCallback(recvMsg);
  server.begin();
}
void SetUpHC05() {
  sendString("AT\r\n");
  delay(100);
  sendString("AT+ORGL\r\n");
  delay(100);
  sendString("AT+RMAAD\r\n");
  delay(100);
  sendString("AT+PSWD=0001\r\n");
  delay(100);
  sendString("AT+BIND="+BLEName+"\r\n");
  delay(100);
  sendString("AT+ROLE=1\r\n");
  delay(100);
  sendString("AT+RESET\r\n");  
}
void RunESPNOW() {
  
    if (BROADCASTESPNOW) {
      BROADCASTESPNOW = false;
      getReadings();
      BroadcastData(); // sending data via ESPNOW
      Serial.println("Sent Data Via ESPNOW");
      ResetReadings();
    }
}
void ManageTagger() {
  digitalWrite(ledPin, ledState);
  currentMillis1 = millis(); // sets the timer for timed operations
  unsigned long CurrentMillis = millis();
      if (INGAME == false && SetPlayerMode < 4) { // runs upon paired ble to tagger from start up
        if (CurrentMillis - previousMillis > 2000) { 
          previousMillis = CurrentMillis;
          Serial.print("tokenstrings 0 = ");
          Serial.println(tokenStrings[0]);
          if (tokenStrings[0] == "$PONG") {
            INGAME = true;
            StartBossPlayer();
          } else {
            sendString("$PING,*");
            BootCounter++;
            if (BootCounter == 10) {
              //Clearly the tagger is ignoring us now...
              ESP.restart();
            }
          }
        }
      }
      if (INGAME == false && PBLOCKOUT == false && SetPlayerMode == 4) { // runs if ingame is triggered and running as a normal player
        if (CurrentMillis - previousMillis > 2000) { 
          previousMillis = CurrentMillis;
          if (tokenStrings[0] == "$PONG") {
            sendString("$VOL,0,0,*"); // adjust volume to default
            Serial.println();
            Serial.println("JEDGE is taking over the BRX");
            sendString("$PHONE,*");
            sendString("$PLAYX,0,*");
            sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
            sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
            INGAME = false;
            sendString("$PLAY,OP47,3,10,,,,,*"); // says "jedge init."
            PBLOCKOUT = true;
            if (updatenotification == 1) {
              sendString("$PLAY,VA6W,3,10,,,,,*");
              updatenotification = 0;
            }
            if (updatenotification == 2) {
              sendString("$PLAY,OP32,3,10,,,,,*"); 
              updatenotification = 0;
            }
          } else {
            sendString("$PING,*");
            BootCounter++;
            if (BootCounter == 10) {
              ESP.restart();
            }
          }
        }
      }
      if (INGAME) {
        if (PLAYERDIED) {
          if (CurrentMillis - previousMillis > 1000) { 
            previousMillis = CurrentMillis;
            if (currenthealth > 0) {
              PLAYERDIED = false;
              sendString("$PID," + String(GunID) + ",*");
            }
          }
        }
        if (RebootWhileInGame > 0) {
          if (RebootWhileInGame == 1) {
            if (CurrentMillis - previousMillis > 1000) { 
              previousMillis = CurrentMillis;
              sendString("$PING,*");
              if (tokenStrings[0] == "$PONG") {
                RebootWhileInGame = 2;
              } 
            }
          }
          if (RebootWhileInGame == 2) {
            RebootWhileInGame = 0;
            sendString("$VOL,0,0,*"); // adjust volume to default
            Serial.println();
            Serial.println("JEDGE is taking over the BRX");
            sendString("$PHONE,*");
            sendString("$PLAYX,0,*");
            sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
            sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
            sendString("$PLAY,OP47,3,10,,,,,*"); // says "jedge init."
            PBLOCKOUT = true;
            PBGameStart();
          }
        }        
        if (BACKGROUNDMUSIC) {
          unsigned long CurrentMusicMillis = millis();
          if (CurrentMusicMillis - MusicPreviousMillis > MusicRestart) {
            MusicPreviousMillis = CurrentMusicMillis;
            if (GameMode == 13) {
              int randomcallout = random(11);
              randomcallout = randomcallout + 40;
              sendString("$PLAY,SW" + String(randomcallout) + ",3,10,,,,,*");
              AudioDelay = 10000;
              AudioPreviousMillis = millis();
            }
            if (GameMode == 14) {
              int randomcallout = random(7);
              sendString("$PLAY,K0" + String(randomcallout) + ",3,10,,,,,*");
              AudioDelay = 10000;
              AudioPreviousMillis = millis();
            }
            AudioSelection = MusicSelection;
            Audio();
          }    
        }    
        if (SELECTCONFIRM) {
          Serial.println("SELECTCONFIRM = TRUE");
          if(millis() - SelectButtonTimer > 1500) {
            Serial.println("select button timer is less than actual game time minus 1500 and audio selection is VA9B");
            if (AudioSelection == "VA9B") {
              Serial.print("enabling Audio(); trigger");
              Audio(); 
            }
          }
          if(millis() - SelectButtonTimer >= 11500) {
            SELECTCONFIRM = false; // out of time, deactivate ability to confirm
            Serial.println("SELECTCONFIRM is now false because actual ActualGameTime minus 10000 is greater than SelectButtonTimer");
            SPECIALWEAPONLOADOUT = false; // disabling ability to load the special weapon
          }    
        }
        if (LOOT) {
          LOOT = false;
          swapbrx();
        }
        if (SPECIALWEAPONPICKUP) {
          SPECIALWEAPONPICKUP = false;
          LoadSpecialWeapon();
        }
      }
}
//********************************** Second Core Loop **********************************
// loop used for OLED SCREEN
void loop0(void *pvParameters) {
  // set up section that will run once only:
  OLEDStart();
  delay(2000);
  if (ENABLEOTAUPDATE) {
    // display to indicate that the device is starting update process
  }
  if (ENABLEWEBSERVER) {
    // display to indicate that were connecting to BLE tagger
  }
  if (ENABLEBLE) {
    // display that it is connecting to bluetooth classic
  }
  if (BLUETOOTHCLASSIC) {
    // display indicator that we are taking over Gen1 Player #
  }
  while(1) {
    // main loop for repeating display notifications
    long CurrentMillis0 = millis();
    if (INGAME) {
      if (CurrentMillis0 - PreviousMillis0 > 2000) {
        PreviousMillis0 = CurrentMillis0;
      }
    }
    if (UPDATEOLED) {
      // update the LCD Screen
    }
    delay(1);
  }
}
void setup() {
// initialize serial port for debug
  Serial.begin(115200);
  Serial.println("Starting Arduino JEDGE application...");
  pinMode(led, OUTPUT);
  digitalWrite(led,  ledState);
// initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  GunID = EEPROM.read(10);
  GunGeneration = EEPROM.read(196);
  if (GunGeneration == 255) {
    GunGeneration = 2;
  }
  Serial.println("Player ID: " + String(GunID));
  Serial.println("Gun Generation: " + String(GunGeneration));
  ssid = "Player#" + String(GunID);
  if (EEPROM.read(194) == 1) {
    INGAME = true;
    Serial.println("Device Reset While In Game, ... Loading Previous Game Settings");
  }
  if (EEPROM.read(194) == 2) {
    INGAME = false;
    updatenotification = 1;
    EEPROM.write(194, 0);
  }
  if (EEPROM.read(194) == 3) {
    INGAME = false;
    updatenotification = 2;
    EEPROM.write(194, 0);
  }
  int bootstatus = EEPROM.read(0);
  Serial.print("boot status = ");
  Serial.println(bootstatus);
  if (bootstatus > 0 && bootstatus < 4) {
    Serial.println("Enabling OTA Update Mode");
    bootstatus++;
    if (bootstatus == 4) {
      bootstatus = 0;
    }
    EEPROM.write(0, bootstatus);
    EEPROM.commit();
    ENABLEOTAUPDATE = true;
  }
  // setting up eeprom based SSID:
  String esid;
  for (int i = 11; i < 43; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  // Setting up EEPROM Password
  String epass = "";
  for (int i = 44; i < 99; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  // Setting up EEPROM BLE Server Name
  String ename = "";
  for (int i = 100; i < 120; ++i)
  {
    ename += char(EEPROM.read(i));
  }
  Serial.print("BLE Name: ");
  Serial.println(ename);
  // now updating the OTA credentials to match
  OTAssid = esid;
  OTApassword = epass;
  BLEName = ename;
  int settingcheck = EEPROM.read(1);
  Serial.println("Player mode = " + String(settingcheck));
  if (settingcheck > 4) {
    SetPlayerMode = 0;
  } else {
    SetPlayerMode = settingcheck;
  }
  if (ENABLEOTAUPDATE) {
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
    pinMode(LED_BUILTIN, OUTPUT);
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.println("Starting ESPNOW");
    Serial.println("wifi credentials");
    Serial.println(OTAssid);
    Serial.println(OTApassword);
    connect_wifi();
  } else {
    // look for indicator for boot mode:
    int touchstate = 0;
    touchstate = touchRead(4);
    if (touchstate < 70) { // indicates capacitive touch is being touched
      delay(1000);
      touchstate = touchRead(4);
      if (touchstate < 70) { // cap.touch button still pressed
        Serial.println("touchstate = " + String(touchstate));
        StartWebServer();
        ENABLEWEBSERVER = true;
        Serial.println("WebServer Activated");
        digitalWrite(led,HIGH);
        if (GunGeneration == 1) {
          BluetoothSetup();
        }
      }
    }
  }
  if (!ENABLEWEBSERVER && !ENABLEOTAUPDATE) {
    // load all stored settings from eeprom:
    if (INGAME) { 
      // there was a reset during play - need to see if tagger is in game or not
      // if not, then we need to auto start the last game mode
      RebootWhileInGame = 1;
      TeamID = EEPROM.read(3);
      pbgame = EEPROM.read(4);
      pbteam = EEPROM.read(5);
      pbweap = EEPROM.read(6);
      pbperk = EEPROM.read(7);
      pblives = EEPROM.read(8);
      pbtime = EEPROM.read(9);
      pbspawn = EEPROM.read(189);
      pbindoor = EEPROM.read(190);
      SetVol = EEPROM.read(195);
      // lets reload our current scores:
      Serial.println("loading player's previous scores");
      int scorecounter = 120;
      int playercounter = 0;
      while (scorecounter < 184) {
        PlayerKillCount[playercounter] = EEPROM.read(scorecounter);
        Serial.println("Player " + String(playercounter) + "has a score of: " + String(PlayerKillCount[playercounter]));
        playercounter++;
        scorecounter++;
      }
      Serial.println("Loading Previous Team Scores");
      TeamKillCount[0] = EEPROM.read(184);
      TeamKillCount[1] = EEPROM.read(185);
      TeamKillCount[2] = EEPROM.read(186);
      TeamKillCount[3] = EEPROM.read(187);
      Serial.println("Loading Objectives and Deaths");
      Deaths = EEPROM.read(188);
      CompletedObjectives = EEPROM.read(193);
    }
    // initialize ble cient connection to brx
    
    if (GunGeneration == 2) {
      ENABLEBLE = true;
      Serial.println("booting in BLE mode");
      BLESetup();
    } else {
      BLUETOOTHCLASSIC = true;
      Serial.println("booting in bluetooth classic mode");
      BluetoothSetup();
    }
    // Start ESP Now
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.println("Starting ESPNOW");
    IntializeESPNOW();
  }
  xTaskCreatePinnedToCore(loop0, "loop0", 4096, NULL, 1, NULL, 0);
}

//******************************************************************************************
//******************************************************************************************
//******************************************************************************************

// This is the Arduino main loop // for the BLE Core.
void loop() {
  while (ENABLEOTAUPDATE) {
    digitalWrite(ledPin, ledState);
  // //s if trying to update
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
      Serial.println("all good, up to date, lets restart");
      ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
    }
  }
  while (ENABLEWEBSERVER) {
    if (GunGeneration == 1) {
      ListenToBluetoothClassic();
    }
    digitalWrite(ledPin, ledState);
    ws.cleanupClients();
    // currentMillis1 = millis(); // sets the timer for timed operations
    /*
    if (currentMillis1 - previousMillis > 2000) {
      previousMillis = currentMillis1;
      WebSerial.println("Hello!");
    }
    */
  }
  while (ENABLEBLE) {
    // the main loop for BLE activity is here, it is devided in three sections....
    // sections are for when connected, when not connected and to connect again
//***********************************************************************************
    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be true.
    if (doConnect == true) {
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Server.");
        doConnect = false; // stop trying to make the connection.
      } else {
        Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      }
    }
//*************************************************************************************
    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected) {
      ManageTagger();
    } else if (doScan) {
      BLESetup();
    }
    RunESPNOW();
  }
  while (BLUETOOTHCLASSIC) {
    ListenToBluetoothClassic();
    ManageTagger();
    RunESPNOW();
  }
} // End of loop

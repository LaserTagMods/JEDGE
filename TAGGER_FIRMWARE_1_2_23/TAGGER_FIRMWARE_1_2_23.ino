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
 * 10/7/2021  planning on option implementations FOR BOSS MODE:
 *              Difficulty: easy, normal, hard - to adjust armor and health levels
 *              Weapon slots one, two and three as options for misc weapons depending on how complicated you want it to get for boss player
 *              add in a melee weapon selection for close range instant explosive death
 *              add in a left button for stun/disable players temporarily
 *              add in a right button for gasing players
 * 8/5/2022   Revisited and cleaned up extra lines, reorganized main loop to use object based functions for distinguishing applications depending on preset variables
 *            changed out the espnow comms type for compatibility with swaptx and brx new hosting methods
 *            espnow lr added and functional
 * 8/7/2022   Added in specifics for BRX firmwar 4.26, including the AS, SP, and UP commands, tested out for AS to ensure functionality
 *            Added in ability to calculate unique ID number based on mac address in set up, to compare to $Giv command for ID assignments
 * 8/10/22    Success Integrating ESPNOW Commands and starting and stopping tagger with host controller
 * 
 *
 *
 *            
 *            
 */
//****************************************************************
// libraries to include:
#include <HardwareSerial.h> // for assigining misc pins for tx/rx
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
#include "BluetoothSerial.h" // used for serial bluethooth or classic
// ***********************************************************
// DEFINE PINS FOR HARDWARE SERIAL
HardwareSerial SerialHW(1);
#define SerialHW_RXPIN 16 // TO HC-05 TX
#define SerialHW_TXPIN 17 // TO HC-05 RX
HardwareSerial SerialRY(2);
#define SerialRY_RXPIN 23 // TO RYLR896 TX
#define SerialRY_TXPIN 22 // TO RYLR896 RX
int BaudRate = 57600; // 115200 is for GEN2/3, 57600 is for GEN1, this is set automatically based upon user input
// ***********************************************************
// create a serial object for serial bluetooth
BluetoothSerial SerialBT;
String name = "LTPhead-R0522"; // this is the device name we need to pair to
char *pin = "0001"; // Pin needed to connect to the device
bool connected; // staus indicator to tell if paired or not
bool BLEMODE = true;
bool SPPMODE = false;
bool HC05MODE = false;
//****************************************************************
// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 200
// 0-10 for variable storing, 11-120 for ble, wifi, password
// 0 for boot mode
// 1 for device mode (boss, player)
// 2 for gun ID 1-63 max
// 3 for bluetooth mode
// 4 for Wireless Comms Mode 0-espnow, 1-LoRa
//***********************************************************************************
#define BLE_SERVER_SERVICE_NAME "jayeburden" // CHANGE ME!!!! (case sensitive)
String BLEName = "jayeburden"; // set temporarily and will change from eeprom in setup
// this is important it is how we filter
// out unwanted guns or other devices that use UART BLE characteristics you need to change 
// this to the name of the gun ble server
int GunID = 1; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
int TeamID = 0; // used for game controls and reporting to host
byte CommsSetting = 0;
//*****************************************************************************************
// BLE DECLARATIONS:
//****************************************************************
// The remote service we wish to connect to, all guns should be the same as
// this is an uart comms set up
long DisconnectTime;
static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
// The characteristic of the remote service we are interested in.
// these uuids are used to send and recieve data
static BLEUUID    charRXUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID    charTXUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
// variables used for ble set up
static boolean doConnect = false;
// static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteRXCharacteristic;
static BLERemoteCharacteristic* pRemoteTXCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEClient*  pClient;
// variables used to provide notifications of pairing device 
// as well as connection status and recieving data from gun
char notifyData[100];
int notifyDataIndex = 0;
char *ptr = NULL;
int TouchCounter = 0; // 0 = BLE for tagger, 1 = webserver, 2 = OTA mode
bool ENABLEBLE = false; // used to enable or disable BLE device
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool ENABLEWEBSERVER = false; // enables the web server for settings/configuration
bool FirstConnect = false; // used to indicate the first time the device pairs with tagger
bool RunInit = false; // used to run initial bluetooth set up on tagger
int PresetGameMode = 0;
byte LifeStatus = 1; // default indicator that player is dead
bool REGISTERED = true;
bool INGAME = false;
String DataToTagger = "null";
String DataToESPNOWBroadcast = "null";
String SendToTaggerLater1 = "null";
String SendToTaggerLater2 = "null";
int WaitTime = 0;
long WaitStart = 0;
String Setup = "null";
long startScan = 0; // part of BLE enabling
//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
int ledState = LOW;  // ledState used to set the LED
bool DEVICEHOSTED = false;
byte UnHostedCounter = 0;
bool ALLOWTEAMSELECTION = false;
bool ALLOWPERKSELECTION = false;
bool ALLOWWEAPONSELECTION = false;
bool FAKEHEADSET = false;
int DuplicateKillCheck[10];
byte killcheckcounter = 0;
bool READYFORNEXTGAME = true;
//*****************************************************************************************
// ESP Now DECLARATIONS:
//*****************************************************************************************
// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09}; 
// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
//uint8_t ESPNOWBroadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};
uint8_t ESPNOWBroadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// I SET ALL DEVICES TO THE SAME MAC ADDRESS SO ALL RECEIVE THE ESPNOWBroadcast, SAME AS ESPNOWBroadcastING TO ALL
// data to process espnow data
String TokenStrings[40];
String LastIncomingData;
String IdentifierValue;
long ClearCache = 0;
long PreviousHostUpdate;
long PreviousHeadsetPing;
//********************************************************************************************
//*************************************************************************
//******************** OTA UPDATES ****************************************
//*************************************************************************
String FirmwareVer = {"5.0"};
#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/JEDGE5VER.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/JEDGE5FIRM.bin"
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
int updatenotification;
unsigned long previousMillis_2 = 0;
const long otainterval = 1000;
const long mini_interval = 1000;
String OTAssid = "dontchangeme"; // network name to update OTA
String OTApassword = "dontchangeme"; // Network password for OTA
//*************************************************************************
//******************** DEVICE SPECIFIC DEFINITIONS ************************
//*************************************************************************
// These should be modified as applicable for your needs
String ssid = "SettingsAP";
const char* password = "123456789";
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
byte lastTaggedPlayer;
byte lastTaggedTeam;
byte SettingsGameMode = 0;
byte SettingsGameRules = 0;
int PBWeap = 0;
int PBPerk = 0;
bool DebugComms = false;
byte MyKillCounter = 0;
String killnotificationstring = "null";
bool HOSTKILLCONFIRM = false;
bool PLAYERKILLCONFIRM = false;
long LastKillConfirmationSend = 0;
byte KillConfirmationAttemptCounter = 0;
byte HostKillConfirmationAttemptCounter = 0;
String LoRaDataReceived = "null";
String LoRaDataToSend = "null";
bool INITBLUETOOTH = false;
long PresetGameStart = 0;
long PresetGameEnd = 600000;
// **************************************************************
void StartHC05Serial() {
  Serial.println("Serial Buad Rate set for: " + String(BaudRate));
  SerialHW.begin(9600, SERIAL_8N1, SerialHW_RXPIN, SerialHW_TXPIN); // setting up serial communication with ESP8266 on pins 16 as RX/17 as Tx w baud rate of 9600
}
void SetUpHC05() {
  // need to hold the button on hc-05 for this to work
  // end current serialhw settings
  SerialHW.end();
  // set up serialhw 
  SerialHW.begin(38400, SERIAL_8N1, SerialHW_RXPIN, SerialHW_TXPIN); // setting up serial communication with ESP8266 on pins 16 as RX/17 as Tx w baud rate of 9600
  // modify the ble name to have "" on the name
  String tempdata = "\"" + BLEName + "\"";
  Serial.println("BLEName converted to: " + tempdata);
  delay(100);
  BLESend("AT\r\n");
  delay(100);
  //("AT+ORGL\r\n");
  //delay(100);
  //BLESend("AT+RMAAD\r\n");
  //delay(100);
  BLESend("AT+PSWD=0001\r\n");
  delay(100);
  BLESend("AT+BIND="+tempdata+"\r\n");
  delay(100);
  BLESend("AT+ROLE=1\r\n");
  delay(100);
  BLESend("AT+RESET\r\n");
  SerialHW.end();
  StartHC05Serial();
}
void ListenToHardwareSerial() {
  if (SerialHW.available()) {
    String receData = SerialHW.readStringUntil('\n');
    int datalength = receData.length();
    Serial.print("Received: ");
    Serial.println(receData); // this is printing... 
    Serial.println("Data Length: " + String(datalength) + " characters");
    byte index = 0;
    ptr = strtok((char*)receData.c_str(), ",");  // takes a list of delimiters
    while (ptr != NULL)
    {
      TokenStrings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    Serial.println("We have found " + String(index ) + " tokens"); // this isnt breaking out the characters...
    for(int i=0;i<index;i++){
      if (DebugComms) {
        Serial.print("tokenString(" + String(i) +"): (");
        Serial.print(TokenStrings[i]);
        Serial.println(")");
      }
    }
    //Serial.println(); 
    ProcessBRXData();   
  }
}
void StartLoRaSerial() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa Serial Port");
  SerialRY.begin(115200, SERIAL_8N1, SerialRY_RXPIN, SerialRY_TXPIN); // setting up the LoRa pins
}
void FastLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  SerialRY.begin(115200, SERIAL_8N1, SerialRY_RXPIN, SerialRY_TXPIN); // setting up the LoRa pins
  SerialRY.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  SerialRY.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  //SerialRY.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //SerialRY.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  SerialRY.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS="+String(GunID)+"\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
  SerialRY.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial.println("LoRa Module Initialized");
}
void LongRangeLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  SerialRY.begin(115200, SERIAL_8N1, SerialRY_RXPIN, SerialRY_TXPIN); // setting up the LoRa pins
  SerialRY.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //SerialRY.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  //SerialRY.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  SerialRY.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  SerialRY.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS="+String(GunID)+"\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
  SerialRY.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial.println("LoRa Module Initialized");
}
void MidRangeLoraSetup() {
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  SerialRY.begin(115200, SERIAL_8N1, SerialRY_RXPIN, SerialRY_TXPIN); // setting up the LoRa pins
  SerialRY.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //SerialRY.print("AT+PARAMETER=6,9,1,4\r\n");    //FOR TESTING FOR SPEED
  SerialRY.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //SerialRY.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  SerialRY.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS="+String(GunID)+"\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
  SerialRY.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  SerialRY.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  SerialRY.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial.println("LoRa Module Initialized");
}
void TransmitLoRa() {
  // LETS SEND SOMETHING:
  // delay(5000);
  // String LoRaDataToSend = (String(valueA)+String(valueB)+String(valueC)+String(valueD)+String(valueE)); 
  // If needed to send a response to the sender, this function builds the needed string
  Serial.println("Transmitting Via LoRa"); // printing to serial monitor
  SerialRY.print("AT+SEND=99,"+String(LoRaDataToSend.length())+","+LoRaDataToSend+"\r\n"); // used to send a data packet
}
void ReceiveLoRaTransmission() {
  //Serial.println("scanning");
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(SerialRY.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    DataToTagger = SerialRY.readString(); // stores the incoming data to the pre set string
    Serial.print("LoRaDataReceived: "); // printing data received
    Serial.println(DataToTagger); // printing data received
    if(DataToTagger.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)DataToTagger.c_str(), ",");
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
      ProcessWirelessComms();
    }
  }
}
//******************************************************************
void SetupBluetoothSPP() {
  SerialBT.setPin(pin);
  SerialBT.begin("ESP32test", true); 
  SerialBT.setPin(pin);
  Serial.println("The device started SPP Mode!");
  // connect to device
  connected = SerialBT.connect(name);
  if(connected) {
    Serial.println("Connected Succesfully!");
  } else {
    if(!SerialBT.connected(1000)) {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app."); 
    }
  }
}
// ************************************************************
//************************************************************************
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}
void ESPNOWBroadcast(const String &message)
// Emulates a ESPNOWBroadcast
{
  // ESPNOWBroadcast a message to every device in range
  uint8_t ESPNOWBroadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, ESPNOWBroadcastAddress, 6);
  if (!esp_now_is_peer_exist(ESPNOWBroadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(ESPNOWBroadcastAddress, (const uint8_t *)message.c_str(), message.length()); 
  // Print results to serial monitor
  if (result == ESP_OK)
  {
    Serial.println("ESPNOWBroadcast message success");
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
// OBJECT FOR RECEIVING ESPNOW DATA PACKETS AND PROCESSING THEM
void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  // Called when data is received
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
  DataToTagger = IncomingData;
  if (IncomingData != LastIncomingData) {
    
    if(DataToTagger.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)DataToTagger.c_str(), ",");
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
        if (DebugComms) {
          Serial.print("Token " + String(stringnumber) + ": ");
          Serial.println(TokenStrings[stringnumber]);
        }
        stringnumber++;
      }
    }
    ProcessWirelessComms();
  }
}
void ProcessWirelessComms() {
    if (!ENABLEBLE) {
      INITBLUETOOTH = true;
    }
    // used for BLE Sending
    // SECTION FOR ANALYZING ESPNOW INCOMING DATA PACKETS
    // all packets with $AS, $SP, $UP should be forwarded on if it has a 99 and applied to this tagger
    // if it carrys a designation of 0 then it needs to only be applied to this tagger
    if (TokenStrings[0] == "$AS") { // used for game play/control, 0-11 tokens, 8 is applicator token
      // this is a game setting, see if we send it on or only use it locally
      // we apply this to our tagger
      SettingsGameMode = TokenStrings[2].toInt();
      SettingsGameRules = TokenStrings[3].toInt();
      if (TokenStrings[1] == "2") {
        // Perk Select
        BLESend("$AS,4,0,0,0,0,0,75,*");
        ALLOWPERKSELECTION = true;
        delay(300);
        Serial.println("enabling PERK Selection");
        // tell gun to say select a team:
        SendToTaggerLater1 = "$PLAY,VA5D,3,10,,,,,*"; 
        READYFORNEXTGAME = true;
      }
      if (TokenStrings[1] == "3") {
        // Team Select
        BLESend("$AS,4,0,0,0,0,0,75,*");
        ALLOWTEAMSELECTION = true;
        delay(300);
        Serial.println("enabling Team Selection");
        // tell gun to say select a team:
        SendToTaggerLater1 = "$PLAY,VA5E,3,10,,,,,*"; 
        READYFORNEXTGAME = true;
      }
      if (TokenStrings[1] == "5") {
        //Weapon Select
        BLESend("$AS,4,0,0,0,0,0,75,*");
        ALLOWWEAPONSELECTION = true;
        delay(300);
        Serial.println("enabling wEAPON Selection");
        // tell gun to say select a team:
        SendToTaggerLater1 = "$PLAY,VA5F,3,10,,,,,*"; 
        READYFORNEXTGAME = true;
      }
      if (TokenStrings[1] == "4") {
        DataToTagger = TokenStrings[0] + "," + TokenStrings[1] + "," + TokenStrings[2] + "," + TokenStrings[3] + "," + TokenStrings[4] + "," + TokenStrings[5] + "," + TokenStrings[6] + "," + TokenStrings[7] + "," + TokenStrings[8];
        BLESend(DataToTagger);
        READYFORNEXTGAME = true;
      }
      if (TokenStrings[1] == "1") {
        Serial.println("***** Game Starting ******");
        if (!READYFORNEXTGAME) {
          BLESend("$AS,4,0,0,0,0,0,75,*");
          delay(300);
        }
        if (SettingsGameMode > 0) {
          Setup = "$PBTEAM," + String(TeamID) + ",*";
          BLESend(Setup);
          delay(300);
        }
        if (SettingsGameMode == 1 || SettingsGameMode == 2) {
          Setup = "$PBPERK," + String(PBPerk) + ",*";
          BLESend(Setup);
          delay(200);
          BLESend(Setup);
          delay(300);
        }
        Setup = "$PBWEAP," + String(PBWeap) + ",*";
        BLESend(Setup);
        delay(200);
        BLESend(Setup);
        delay(300);
        DataToTagger = TokenStrings[0] + "," + TokenStrings[1] + "," + TokenStrings[2] + "," + TokenStrings[3] + "," + TokenStrings[4] + "," + TokenStrings[5] + "," + TokenStrings[6] + "," + TokenStrings[7] + "," + TokenStrings[8];
        BLESend(DataToTagger);
        // game start command, need to let host know that we are good to go and in game periodically
        LifeStatus = 0; // indicates player is alive
        // this tells the host that he is not alone and that this gun is alive or in play
        INGAME = true;
        ALLOWTEAMSELECTION = false;
        ALLOWPERKSELECTION = false;
        ALLOWWEAPONSELECTION = false;
        if (SettingsGameMode > 0) {
          SendToTaggerLater1 = "$TID," + String(TeamID) + ",*";
          WaitTime = 2000;
          WaitStart = millis();
        }
      }
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      //}
      READYFORNEXTGAME = false;    
    }
    if (TokenStrings[0] == "$SP") { // 0-5 TOKENS FOR END OF GAME
      DataToTagger = TokenStrings[0] + "," + TokenStrings[1] + "," + TokenStrings[2];
      BLESend(DataToTagger);
      INGAME = false;
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      //}
    }
    if (TokenStrings[0] == "$UP") { // 0-6 TOKENS FOR updates/status
      DataToTagger = TokenStrings[0] + "," + TokenStrings[1] + "," + TokenStrings[2] + "," + TokenStrings[3] + "," + TokenStrings[4];
      BLESend(DataToTagger);
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      //}
    }
    // $DD,(PlayerIDThatKilled),(teamIDthatkilled),(DeadPlayerID),(randomnumber),*
    if (TokenStrings[0] == "$DD") {
      int IntendedPlayer = TokenStrings[1].toInt();
      Serial.println("Received Kill Confirmation, Checking if it is my kill");
      // this is a kill confirmation, need to assess if this is for this player or for someone else
      if (IntendedPlayer == GunID) {
        // yes it is intended for this tagger
        Serial.println("confirmed this is my kill");
        // check that it is not a duplicate
        int IncomingRandomNumber = TokenStrings[4].toInt();
        byte j = 0;
        bool ISMYKILL = true;
        while (j < 10) {
          if (DuplicateKillCheck[j] == IncomingRandomNumber) {
            j = 10;
            Serial.println("duplicate kill received");
            ISMYKILL = false;
          }
          j++;
        }
        if (ISMYKILL) {
          MyKillCounter++;
          String killconfirmation = "$KK," + String(MyKillCounter) + ",*";
          BLESend(killconfirmation); // gives the this player a tag/point
          DuplicateKillCheck[killcheckcounter] = IncomingRandomNumber;
          if (killcheckcounter < 9) {
            killcheckcounter++;
          } else {
            killcheckcounter = 0;
          }
          String KillConfirmationReceived = "$PKC," + TokenStrings[3] + ",*";
            ESPNOWBroadcast(KillConfirmationReceived);
            //delay(5);
            //ESPNOWBroadcast(KillConfirmationReceived);
            delay(5);
            ESPNOWBroadcast(KillConfirmationReceived);
        }
      } else {
        //ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      }
    }
    // $PKC,Intended Player ID,*
    if (TokenStrings[0] == "$PKC") {
      if (PLAYERKILLCONFIRM) {
        if (TokenStrings[1].toInt() == GunID) {
          PLAYERKILLCONFIRM = false;
        }
      } else {
        //ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      }
    }
    // $HKC,Intended Player ID,*
    if (TokenStrings[0] == "$HKC") {
      if (HOSTKILLCONFIRM) {
        if (TokenStrings[1].toInt() == GunID) {
          HOSTKILLCONFIRM = false;
        }
      } else {  
        //ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
      }
    }
    // $TA,PlayerID,TeamID,*
    if (TokenStrings[0] == "$TA") {
      // received a team assignment, check to see if matches player ID
      if (TokenStrings[1].toInt() == GunID) {
        // this assignment belongs to this player
        TeamID = TokenStrings[2].toInt();
      } else {
        //if (CommsSetting == 0) {
          ESPNOWBroadcast(DataToTagger); // rebroadcasting to get to others
        //}
      }
    }
    // registration with host
    if (TokenStrings[0] == "$PT") {
      if (TokenStrings[1].toInt() == GunID) {
        REGISTERED = true;
      } else {
        // Send Data On For Others As Well
        //if (CommsSetting == 0) {
          ESPNOWBroadcast(DataToTagger); 
        //} 
      }
    }
    // Unregister with host
    if (TokenStrings[0] == "$UR") {
        REGISTERED = false;
        // Send Data On For Others As Well
        //if (CommsSetting == 0) {
          ESPNOWBroadcast(DataToTagger);  
        //}
    }
    // Respawn Player Packet
    // $RP,GunID,LivesRemaining,*
    if (TokenStrings[0] == "$RP") {
      // respawn packet received Send Revived to Tagger
      if (GunID == TokenStrings[1].toInt()) {
        // This applies to this gun
        String Revive = "$RV," + TokenStrings[1] + ",0," + TokenStrings[2] + ",*";
        BLESend("$SPAWN,*");
      } else {
        // Send Data On For Others As Well
        //if (CommsSetting == 0) {
          ESPNOWBroadcast(DataToTagger);  
        //}
      }
    }
    // weapon and perk assignments
    if (TokenStrings[0] == "$PERK") {
      PBPerk = TokenStrings[1].toInt();
      // Send Data On For Others As Well
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger);
      //}  
    }
    if (TokenStrings[0] == "$WEAP") {
      PBWeap = TokenStrings[1].toInt();
      // Send Data On For Others As Well
      Serial.println("weapon now: " + String(PBWeap));
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger);
      //}
    }
    if (TokenStrings[0] == "$HS") {
      if(TokenStrings[1] == "0") {
        Serial.println("Disable Faux Headset");
        FAKEHEADSET = false;
        SendToTaggerLater1 = "$PLAY,VA9E,3,10,,,,,*";
      } else {
        Serial.println("Enable Fauz Headset");
        FAKEHEADSET = true;
        SendToTaggerLater1 = "$PLAY,VA9N,3,10,,,,,*";
      }
      // Send Data On For Others As Well
      Serial.println("weapon now: " + String(PBWeap));
      //if (CommsSetting == 0) {
        ESPNOWBroadcast(DataToTagger);
      //}
    }
    // dATA PACKET HAS BEEN PROCESSED, SAVE CURRENT ESPNOW PACKET FOR REFERENCE TO NOT REPEAT SEND TO TAGGER
    LastIncomingData = DataToTagger;
    ClearCache = millis();
}
// object for ESPNOWBroadcasting the data packets
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
// OBJECT USED TO CHANGE THE MAC ADDRESS FOR DEVICE
void ChangeMACaddress() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
  esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
  //esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  //esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  //  Run to change device mac address
  //  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  /* used to convert mac address to a variable to compare to the $Giv esp packet
  String WiFiMacString = WiFi.macAddress();
  Serial.println("MAC Address as String = " + WiFiMacString); 
  String MacTokens[6];
  byte MACValue[6];
  char *ptr = strtok((char*)WiFiMacString.c_str(), ":"); // looks for commas as breaks to split up the string
  int index = 0;
  while (ptr != NULL) {
    MacTokens[index] = ptr; // saves the individual characters divided by commas as individual strings
    index++;
    ptr = strtok(NULL, ":");  // takes a list of delimiters
  }
  Serial.println("We have found " + String(index ) + " tokens");
  int stringnumber = 0;
  while (stringnumber < index) {
    Serial.print("Token " + String(stringnumber) + ": ");
    Serial.println(MacTokens[stringnumber]);
    if (MacTokens[stringnumber] == "00") { MACValue[stringnumber] = 0;}
    if (MacTokens[stringnumber] == "01") { MACValue[stringnumber] = 1;}
    if (MacTokens[stringnumber] == "02") { MACValue[stringnumber] = 2;}
    if (MacTokens[stringnumber] == "03") { MACValue[stringnumber] = 3;}
    if (MacTokens[stringnumber] == "04") { MACValue[stringnumber] = 4;}
    if (MacTokens[stringnumber] == "05") { MACValue[stringnumber] = 5;}
    if (MacTokens[stringnumber] == "06") { MACValue[stringnumber] = 6;}
    if (MacTokens[stringnumber] == "07") { MACValue[stringnumber] = 7;}
    if (MacTokens[stringnumber] == "08") { MACValue[stringnumber] = 8;}
    if (MacTokens[stringnumber] == "09") { MACValue[stringnumber] = 9;}
    if (MacTokens[stringnumber] == "0A") { MACValue[stringnumber] = 10;}
    if (MacTokens[stringnumber] == "0B") { MACValue[stringnumber] = 11;}
    if (MacTokens[stringnumber] == "0C") { MACValue[stringnumber] = 12;}
    if (MacTokens[stringnumber] == "0D") { MACValue[stringnumber] = 13;}
    if (MacTokens[stringnumber] == "0E") { MACValue[stringnumber] = 14;}
    if (MacTokens[stringnumber] == "0F") { MACValue[stringnumber] = 15;}
    if (MacTokens[stringnumber] == "10") { MACValue[stringnumber] = 16;}
    if (MacTokens[stringnumber] == "11") { MACValue[stringnumber] = 17;}
    if (MacTokens[stringnumber] == "12") { MACValue[stringnumber] = 18;}
    if (MacTokens[stringnumber] == "13") { MACValue[stringnumber] = 19;}
    if (MacTokens[stringnumber] == "14") { MACValue[stringnumber] = 20;}
    if (MacTokens[stringnumber] == "15") { MACValue[stringnumber] = 21;}
    if (MacTokens[stringnumber] == "16") { MACValue[stringnumber] = 22;}
    if (MacTokens[stringnumber] == "17") { MACValue[stringnumber] = 23;}
    if (MacTokens[stringnumber] == "18") { MACValue[stringnumber] = 24;}
    if (MacTokens[stringnumber] == "19") { MACValue[stringnumber] = 25;}
    if (MacTokens[stringnumber] == "1A") { MACValue[stringnumber] = 26;}
    if (MacTokens[stringnumber] == "1B") { MACValue[stringnumber] = 27;}
    if (MacTokens[stringnumber] == "1C") { MACValue[stringnumber] = 28;}
    if (MacTokens[stringnumber] == "1D") { MACValue[stringnumber] = 29;}
    if (MacTokens[stringnumber] == "1E") { MACValue[stringnumber] = 30;}
    if (MacTokens[stringnumber] == "1F") { MACValue[stringnumber] = 31;}
    if (MacTokens[stringnumber] == "20") { MACValue[stringnumber] = 32;}
    if (MacTokens[stringnumber] == "21") { MACValue[stringnumber] = 33;}
    if (MacTokens[stringnumber] == "22") { MACValue[stringnumber] = 34;}
    if (MacTokens[stringnumber] == "23") { MACValue[stringnumber] = 35;}
    if (MacTokens[stringnumber] == "24") { MACValue[stringnumber] = 36;}
    if (MacTokens[stringnumber] == "25") { MACValue[stringnumber] = 37;}
    if (MacTokens[stringnumber] == "26") { MACValue[stringnumber] = 38;}
    if (MacTokens[stringnumber] == "27") { MACValue[stringnumber] = 39;}
    if (MacTokens[stringnumber] == "28") { MACValue[stringnumber] = 40;}
    if (MacTokens[stringnumber] == "29") { MACValue[stringnumber] = 41;}
    if (MacTokens[stringnumber] == "2A") { MACValue[stringnumber] = 42;}
    if (MacTokens[stringnumber] == "2B") { MACValue[stringnumber] = 43;}
    if (MacTokens[stringnumber] == "2C") { MACValue[stringnumber] = 44;}
    if (MacTokens[stringnumber] == "2D") { MACValue[stringnumber] = 45;}
    if (MacTokens[stringnumber] == "2E") { MACValue[stringnumber] = 46;}
    if (MacTokens[stringnumber] == "2F") { MACValue[stringnumber] = 47;}
    if (MacTokens[stringnumber] == "30") { MACValue[stringnumber] = 48;}
    if (MacTokens[stringnumber] == "31") { MACValue[stringnumber] = 49;}
    if (MacTokens[stringnumber] == "32") { MACValue[stringnumber] = 50;}
    if (MacTokens[stringnumber] == "33") { MACValue[stringnumber] = 51;}
    if (MacTokens[stringnumber] == "34") { MACValue[stringnumber] = 52;}
    if (MacTokens[stringnumber] == "35") { MACValue[stringnumber] = 53;}
    if (MacTokens[stringnumber] == "36") { MACValue[stringnumber] = 54;}
    if (MacTokens[stringnumber] == "37") { MACValue[stringnumber] = 55;}
    if (MacTokens[stringnumber] == "38") { MACValue[stringnumber] = 56;}
    if (MacTokens[stringnumber] == "39") { MACValue[stringnumber] = 57;}
    if (MacTokens[stringnumber] == "3A") { MACValue[stringnumber] = 58;}
    if (MacTokens[stringnumber] == "3B") { MACValue[stringnumber] = 59;}
    if (MacTokens[stringnumber] == "3C") { MACValue[stringnumber] = 60;}
    if (MacTokens[stringnumber] == "3D") { MACValue[stringnumber] = 61;}
    if (MacTokens[stringnumber] == "3E") { MACValue[stringnumber] = 62;}
    if (MacTokens[stringnumber] == "3F") { MACValue[stringnumber] = 63;}
    if (MacTokens[stringnumber] == "40") { MACValue[stringnumber] = 64;}
    if (MacTokens[stringnumber] == "41") { MACValue[stringnumber] = 65;}
    if (MacTokens[stringnumber] == "42") { MACValue[stringnumber] = 66;}
    if (MacTokens[stringnumber] == "43") { MACValue[stringnumber] = 67;}
    if (MacTokens[stringnumber] == "44") { MACValue[stringnumber] = 68;}
    if (MacTokens[stringnumber] == "45") { MACValue[stringnumber] = 69;}
    if (MacTokens[stringnumber] == "46") { MACValue[stringnumber] = 70;}
    if (MacTokens[stringnumber] == "47") { MACValue[stringnumber] = 71;}
    if (MacTokens[stringnumber] == "48") { MACValue[stringnumber] = 72;}
    if (MacTokens[stringnumber] == "49") { MACValue[stringnumber] = 73;}
    if (MacTokens[stringnumber] == "4A") { MACValue[stringnumber] = 74;}
    if (MacTokens[stringnumber] == "4B") { MACValue[stringnumber] = 75;}
    if (MacTokens[stringnumber] == "4C") { MACValue[stringnumber] = 76;}
    if (MacTokens[stringnumber] == "4D") { MACValue[stringnumber] = 77;}
    if (MacTokens[stringnumber] == "4E") { MACValue[stringnumber] = 78;}
    if (MacTokens[stringnumber] == "4F") { MACValue[stringnumber] = 79;}
    if (MacTokens[stringnumber] == "50") { MACValue[stringnumber] = 80;}
    if (MacTokens[stringnumber] == "51") { MACValue[stringnumber] = 81;}
    if (MacTokens[stringnumber] == "52") { MACValue[stringnumber] = 82;}
    if (MacTokens[stringnumber] == "53") { MACValue[stringnumber] = 83;}
    if (MacTokens[stringnumber] == "54") { MACValue[stringnumber] = 84;}
    if (MacTokens[stringnumber] == "55") { MACValue[stringnumber] = 85;}
    if (MacTokens[stringnumber] == "56") { MACValue[stringnumber] = 86;}
    if (MacTokens[stringnumber] == "57") { MACValue[stringnumber] = 87;}
    if (MacTokens[stringnumber] == "58") { MACValue[stringnumber] = 88;}
    if (MacTokens[stringnumber] == "59") { MACValue[stringnumber] = 89;}
    if (MacTokens[stringnumber] == "5A") { MACValue[stringnumber] = 90;}
    if (MacTokens[stringnumber] == "5B") { MACValue[stringnumber] = 91;}
    if (MacTokens[stringnumber] == "5C") { MACValue[stringnumber] = 92;}
    if (MacTokens[stringnumber] == "5D") { MACValue[stringnumber] = 93;}
    if (MacTokens[stringnumber] == "5E") { MACValue[stringnumber] = 94;}
    if (MacTokens[stringnumber] == "5F") { MACValue[stringnumber] = 95;}
    if (MacTokens[stringnumber] == "60") { MACValue[stringnumber] = 96;}
    if (MacTokens[stringnumber] == "61") { MACValue[stringnumber] = 97;}
    if (MacTokens[stringnumber] == "62") { MACValue[stringnumber] = 98;}
    if (MacTokens[stringnumber] == "63") { MACValue[stringnumber] = 99;}
    if (MacTokens[stringnumber] == "64") { MACValue[stringnumber] = 100;}
    if (MacTokens[stringnumber] == "65") { MACValue[stringnumber] = 101;}
    if (MacTokens[stringnumber] == "66") { MACValue[stringnumber] = 102;}
    if (MacTokens[stringnumber] == "67") { MACValue[stringnumber] = 103;}
    if (MacTokens[stringnumber] == "68") { MACValue[stringnumber] = 104;}
    if (MacTokens[stringnumber] == "69") { MACValue[stringnumber] = 105;}
    if (MacTokens[stringnumber] == "6A") { MACValue[stringnumber] = 106;}
    if (MacTokens[stringnumber] == "6B") { MACValue[stringnumber] = 107;}
    if (MacTokens[stringnumber] == "6C") { MACValue[stringnumber] = 108;}
    if (MacTokens[stringnumber] == "6D") { MACValue[stringnumber] = 109;}
    if (MacTokens[stringnumber] == "6E") { MACValue[stringnumber] = 110;}
    if (MacTokens[stringnumber] == "6F") { MACValue[stringnumber] = 111;}
    if (MacTokens[stringnumber] == "70") { MACValue[stringnumber] = 112;}
    if (MacTokens[stringnumber] == "71") { MACValue[stringnumber] = 113;}
    if (MacTokens[stringnumber] == "72") { MACValue[stringnumber] = 114;}
    if (MacTokens[stringnumber] == "73") { MACValue[stringnumber] = 115;}
    if (MacTokens[stringnumber] == "74") { MACValue[stringnumber] = 116;}
    if (MacTokens[stringnumber] == "75") { MACValue[stringnumber] = 117;}
    if (MacTokens[stringnumber] == "76") { MACValue[stringnumber] = 118;}
    if (MacTokens[stringnumber] == "77") { MACValue[stringnumber] = 119;}
    if (MacTokens[stringnumber] == "78") { MACValue[stringnumber] = 120;}
    if (MacTokens[stringnumber] == "79") { MACValue[stringnumber] = 121;}
    if (MacTokens[stringnumber] == "7A") { MACValue[stringnumber] = 122;}
    if (MacTokens[stringnumber] == "7B") { MACValue[stringnumber] = 123;}
    if (MacTokens[stringnumber] == "7C") { MACValue[stringnumber] = 124;}
    if (MacTokens[stringnumber] == "7D") { MACValue[stringnumber] = 125;}
    if (MacTokens[stringnumber] == "7E") { MACValue[stringnumber] = 126;}
    if (MacTokens[stringnumber] == "7F") { MACValue[stringnumber] = 127;}
    if (MacTokens[stringnumber] == "80") { MACValue[stringnumber] = 128;}
    if (MacTokens[stringnumber] == "81") { MACValue[stringnumber] = 129;}
    if (MacTokens[stringnumber] == "82") { MACValue[stringnumber] = 130;}
    if (MacTokens[stringnumber] == "83") { MACValue[stringnumber] = 131;}
    if (MacTokens[stringnumber] == "84") { MACValue[stringnumber] = 132;}
    if (MacTokens[stringnumber] == "85") { MACValue[stringnumber] = 133;}
    if (MacTokens[stringnumber] == "86") { MACValue[stringnumber] = 134;}
    if (MacTokens[stringnumber] == "87") { MACValue[stringnumber] = 135;}
    if (MacTokens[stringnumber] == "88") { MACValue[stringnumber] = 136;}
    if (MacTokens[stringnumber] == "89") { MACValue[stringnumber] = 137;}
    if (MacTokens[stringnumber] == "8A") { MACValue[stringnumber] = 138;}
    if (MacTokens[stringnumber] == "8B") { MACValue[stringnumber] = 139;}
    if (MacTokens[stringnumber] == "8C") { MACValue[stringnumber] = 140;}
    if (MacTokens[stringnumber] == "8D") { MACValue[stringnumber] = 141;}
    if (MacTokens[stringnumber] == "8E") { MACValue[stringnumber] = 142;}
    if (MacTokens[stringnumber] == "8F") { MACValue[stringnumber] = 143;}
    if (MacTokens[stringnumber] == "90") { MACValue[stringnumber] = 144;}
    if (MacTokens[stringnumber] == "91") { MACValue[stringnumber] = 145;}
    if (MacTokens[stringnumber] == "92") { MACValue[stringnumber] = 146;}
    if (MacTokens[stringnumber] == "93") { MACValue[stringnumber] = 147;}
    if (MacTokens[stringnumber] == "94") { MACValue[stringnumber] = 148;}
    if (MacTokens[stringnumber] == "95") { MACValue[stringnumber] = 149;}
    if (MacTokens[stringnumber] == "96") { MACValue[stringnumber] = 150;}
    if (MacTokens[stringnumber] == "97") { MACValue[stringnumber] = 151;}
    if (MacTokens[stringnumber] == "98") { MACValue[stringnumber] = 152;}
    if (MacTokens[stringnumber] == "99") { MACValue[stringnumber] = 153;}
    if (MacTokens[stringnumber] == "9A") { MACValue[stringnumber] = 154;}
    if (MacTokens[stringnumber] == "9B") { MACValue[stringnumber] = 155;}
    if (MacTokens[stringnumber] == "9C") { MACValue[stringnumber] = 156;}
    if (MacTokens[stringnumber] == "9D") { MACValue[stringnumber] = 157;}
    if (MacTokens[stringnumber] == "9E") { MACValue[stringnumber] = 158;}
    if (MacTokens[stringnumber] == "9F") { MACValue[stringnumber] = 159;}
    if (MacTokens[stringnumber] == "A0") { MACValue[stringnumber] = 160;}
    if (MacTokens[stringnumber] == "A1") { MACValue[stringnumber] = 161;}
    if (MacTokens[stringnumber] == "A2") { MACValue[stringnumber] = 162;}
    if (MacTokens[stringnumber] == "A3") { MACValue[stringnumber] = 163;}
    if (MacTokens[stringnumber] == "A4") { MACValue[stringnumber] = 164;}
    if (MacTokens[stringnumber] == "A5") { MACValue[stringnumber] = 165;}
    if (MacTokens[stringnumber] == "A6") { MACValue[stringnumber] = 166;}
    if (MacTokens[stringnumber] == "A7") { MACValue[stringnumber] = 167;}
    if (MacTokens[stringnumber] == "A8") { MACValue[stringnumber] = 168;}
    if (MacTokens[stringnumber] == "A9") { MACValue[stringnumber] = 169;}
    if (MacTokens[stringnumber] == "AA") { MACValue[stringnumber] = 170;}
    if (MacTokens[stringnumber] == "AB") { MACValue[stringnumber] = 171;}
    if (MacTokens[stringnumber] == "AC") { MACValue[stringnumber] = 172;}
    if (MacTokens[stringnumber] == "AD") { MACValue[stringnumber] = 173;}
    if (MacTokens[stringnumber] == "AE") { MACValue[stringnumber] = 174;}
    if (MacTokens[stringnumber] == "AF") { MACValue[stringnumber] = 175;}
    if (MacTokens[stringnumber] == "B0") { MACValue[stringnumber] = 176;}
    if (MacTokens[stringnumber] == "B1") { MACValue[stringnumber] = 177;}
    if (MacTokens[stringnumber] == "B2") { MACValue[stringnumber] = 178;}
    if (MacTokens[stringnumber] == "B3") { MACValue[stringnumber] = 179;}
    if (MacTokens[stringnumber] == "B4") { MACValue[stringnumber] = 180;}
    if (MacTokens[stringnumber] == "B5") { MACValue[stringnumber] = 181;}
    if (MacTokens[stringnumber] == "B6") { MACValue[stringnumber] = 182;}
    if (MacTokens[stringnumber] == "B7") { MACValue[stringnumber] = 183;}
    if (MacTokens[stringnumber] == "B8") { MACValue[stringnumber] = 184;}
    if (MacTokens[stringnumber] == "B9") { MACValue[stringnumber] = 185;}
    if (MacTokens[stringnumber] == "BA") { MACValue[stringnumber] = 186;}
    if (MacTokens[stringnumber] == "BB") { MACValue[stringnumber] = 187;}
    if (MacTokens[stringnumber] == "BC") { MACValue[stringnumber] = 188;}
    if (MacTokens[stringnumber] == "BD") { MACValue[stringnumber] = 189;}
    if (MacTokens[stringnumber] == "BE") { MACValue[stringnumber] = 190;}
    if (MacTokens[stringnumber] == "BF") { MACValue[stringnumber] = 191;}
    if (MacTokens[stringnumber] == "C0") { MACValue[stringnumber] = 192;}
    if (MacTokens[stringnumber] == "C1") { MACValue[stringnumber] = 193;}
    if (MacTokens[stringnumber] == "C2") { MACValue[stringnumber] = 194;}
    if (MacTokens[stringnumber] == "C3") { MACValue[stringnumber] = 195;}
    if (MacTokens[stringnumber] == "C4") { MACValue[stringnumber] = 196;}
    if (MacTokens[stringnumber] == "C5") { MACValue[stringnumber] = 197;}
    if (MacTokens[stringnumber] == "C6") { MACValue[stringnumber] = 198;}
    if (MacTokens[stringnumber] == "C7") { MACValue[stringnumber] = 199;}
    if (MacTokens[stringnumber] == "C8") { MACValue[stringnumber] = 200;}
    if (MacTokens[stringnumber] == "C9") { MACValue[stringnumber] = 201;}
    if (MacTokens[stringnumber] == "CA") { MACValue[stringnumber] = 202;}
    if (MacTokens[stringnumber] == "CB") { MACValue[stringnumber] = 203;}
    if (MacTokens[stringnumber] == "CC") { MACValue[stringnumber] = 204;}
    if (MacTokens[stringnumber] == "CD") { MACValue[stringnumber] = 205;}
    if (MacTokens[stringnumber] == "CE") { MACValue[stringnumber] = 206;}
    if (MacTokens[stringnumber] == "CF") { MACValue[stringnumber] = 207;}
    if (MacTokens[stringnumber] == "D0") { MACValue[stringnumber] = 208;}
    if (MacTokens[stringnumber] == "D1") { MACValue[stringnumber] = 209;}
    if (MacTokens[stringnumber] == "D2") { MACValue[stringnumber] = 210;}
    if (MacTokens[stringnumber] == "D3") { MACValue[stringnumber] = 211;}
    if (MacTokens[stringnumber] == "D4") { MACValue[stringnumber] = 212;}
    if (MacTokens[stringnumber] == "D5") { MACValue[stringnumber] = 213;}
    if (MacTokens[stringnumber] == "D6") { MACValue[stringnumber] = 214;}
    if (MacTokens[stringnumber] == "D7") { MACValue[stringnumber] = 215;}
    if (MacTokens[stringnumber] == "D8") { MACValue[stringnumber] = 216;}
    if (MacTokens[stringnumber] == "D9") { MACValue[stringnumber] = 217;}
    if (MacTokens[stringnumber] == "DA") { MACValue[stringnumber] = 218;}
    if (MacTokens[stringnumber] == "DB") { MACValue[stringnumber] = 219;}
    if (MacTokens[stringnumber] == "DC") { MACValue[stringnumber] = 220;}
    if (MacTokens[stringnumber] == "DD") { MACValue[stringnumber] = 221;}
    if (MacTokens[stringnumber] == "DE") { MACValue[stringnumber] = 222;}
    if (MacTokens[stringnumber] == "DF") { MACValue[stringnumber] = 223;}
    if (MacTokens[stringnumber] == "E0") { MACValue[stringnumber] = 224;}
    if (MacTokens[stringnumber] == "E1") { MACValue[stringnumber] = 225;}
    if (MacTokens[stringnumber] == "E2") { MACValue[stringnumber] = 226;}
    if (MacTokens[stringnumber] == "E3") { MACValue[stringnumber] = 227;}
    if (MacTokens[stringnumber] == "E4") { MACValue[stringnumber] = 228;}
    if (MacTokens[stringnumber] == "E5") { MACValue[stringnumber] = 229;}
    if (MacTokens[stringnumber] == "E6") { MACValue[stringnumber] = 230;}
    if (MacTokens[stringnumber] == "E7") { MACValue[stringnumber] = 231;}
    if (MacTokens[stringnumber] == "E8") { MACValue[stringnumber] = 232;}
    if (MacTokens[stringnumber] == "E9") { MACValue[stringnumber] = 233;}
    if (MacTokens[stringnumber] == "EA") { MACValue[stringnumber] = 234;}
    if (MacTokens[stringnumber] == "EB") { MACValue[stringnumber] = 235;}
    if (MacTokens[stringnumber] == "EC") { MACValue[stringnumber] = 236;}
    if (MacTokens[stringnumber] == "ED") { MACValue[stringnumber] = 237;}
    if (MacTokens[stringnumber] == "EE") { MACValue[stringnumber] = 238;}
    if (MacTokens[stringnumber] == "EF") { MACValue[stringnumber] = 239;}
    if (MacTokens[stringnumber] == "F0") { MACValue[stringnumber] = 240;}
    if (MacTokens[stringnumber] == "F1") { MACValue[stringnumber] = 241;}
    if (MacTokens[stringnumber] == "F2") { MACValue[stringnumber] = 242;}
    if (MacTokens[stringnumber] == "F3") { MACValue[stringnumber] = 243;}
    if (MacTokens[stringnumber] == "F4") { MACValue[stringnumber] = 244;}
    if (MacTokens[stringnumber] == "F5") { MACValue[stringnumber] = 245;}
    if (MacTokens[stringnumber] == "F6") { MACValue[stringnumber] = 246;}
    if (MacTokens[stringnumber] == "F7") { MACValue[stringnumber] = 247;}
    if (MacTokens[stringnumber] == "F8") { MACValue[stringnumber] = 248;}
    if (MacTokens[stringnumber] == "F9") { MACValue[stringnumber] = 249;}
    if (MacTokens[stringnumber] == "FA") { MACValue[stringnumber] = 250;}
    if (MacTokens[stringnumber] == "FB") { MACValue[stringnumber] = 251;}
    if (MacTokens[stringnumber] == "FC") { MACValue[stringnumber] = 252;}
    if (MacTokens[stringnumber] == "FD") { MACValue[stringnumber] = 253;}
    if (MacTokens[stringnumber] == "FE") { MACValue[stringnumber] = 254;}
    if (MacTokens[stringnumber] == "FF") { MACValue[stringnumber] = 255;}
    stringnumber++;
  }
  IdentifierValue = String(MACValue[0]) + String(MACValue[1]) + String(MACValue[2]) + String(MACValue[3]) + String(MACValue[4]) + String(MACValue[5]);
  Serial.println("MAC Address as Decimal: " + IdentifierValue);
  */
}
void IntializeESPNOW() {
  // Set up the onboard LED
  pinMode(2, OUTPUT);
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  // Init ESP-NOW
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
    <h1>JEDGE DEVICE CONFIG</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Set Tagger Mode</h2>
      <p><select name="primary" id="primaryid">
        <option value="4">Select a Mode</option>
        <option value="0">Easy Boss</option>
        <option value="1">Normal Boss</option>
        <option value="2">Hard Boss</option>
        <option value="3">Standard JEDGE Player</option>
        <option value="ArcadeModeFFA">Arcade Mode</option>
        </select>
      </p>
      <h2>Set Wireless Comms Mode</h2>
      <p><select name="Comms" id="Commsid">
        <option value="4">Select a Mode</option>
        <option value="ESPNOW">ESPNOW LR</option>
        <option value="LoRaFast">Fast LoRa</option>
        <option value="LoRaMid">Mid Range LoRa</option>
        <option value="LoRaLR">Long Range LoRa</option>
        </select>
      </p>
      <h2>Set Tagger ID</h2>
      <p><select name="tagger" id="taggerid">
        <option value="x">Select A Player ID</option>
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
      <h2>Select Bluetooth Mode</h2>
      <p><select name="bluetooth" id="bluetoothid">
        <option value="4">Select a Mode</option>
        <option value="BLE">Bluetooth Low Energy</option>
        <option value="SPP">Bluetooth SPP</option>
        <option value="HC05">Bluetooth HC05</option>
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
    document.getElementById('taggerid').addEventListener('change', handletagger, false);
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('Commsid').addEventListener('change', handleComms, false);
    document.getElementById('otaupdate').addEventListener('click', toggle14o);
  }
  function toggle14o(){
    websocket.send('toggle14o');
  }
  function handleComms() {
    var xa = document.getElementById("Commsid").value;
    websocket.send(xa);
  }
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handletagger() {
    var xa = document.getElementById("taggerid").value;
    websocket.send(xa);
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
    if (strcmp((char*)data, "toggle14o") == 0) { // update firmware
      Serial.println("OTA Update Mode");
      EEPROM.write(0, 1); // reset OTA attempts counter
      EEPROM.commit(); // Store in Flash
      delay(2000);
      ESP.restart();
    }
    if (strcmp((char*)data, "0") == 0) {
      Serial.println("Easy Mode");
      PresetGameMode = 0;
      EEPROM.write(1,PresetGameMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "1") == 0) {
      Serial.println("Normal Mode");
      PresetGameMode = 1;
      EEPROM.write(1,PresetGameMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "2") == 0) {
      Serial.println("Hard Mode");
      PresetGameMode = 2;
      EEPROM.write(1,PresetGameMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "3") == 0) {
      Serial.println("Standard JEDGE Player");
      PresetGameMode = 3;
      EEPROM.write(1,PresetGameMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "ArcadeModeFFA") == 0) {
      Serial.println("ARCADE MODE - Host Not Required - Free For All");
      PresetGameMode = 4;
      EEPROM.write(1,PresetGameMode);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "ESPNOW") == 0) {
      Serial.println("Comms Set to ESPNOW");
      CommsSetting = 0;
      EEPROM.write(4,CommsSetting);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "LoRaFast") == 0) {
      Serial.println("Comms Set to LoRa");
      CommsSetting = 1;
      EEPROM.write(4,CommsSetting);
      EEPROM.commit();
      FastLoraSetup();
    }
    if (strcmp((char*)data, "LoRaMid") == 0) {
      Serial.println("Comms Set to LoRa");
      CommsSetting = 1;
      EEPROM.write(4,CommsSetting);
      EEPROM.commit();
      MidRangeLoraSetup();
    }
    if (strcmp((char*)data, "LoRaLR") == 0) {
      Serial.println("Comms Set to LoRa");
      CommsSetting = 1;
      EEPROM.write(4,CommsSetting);
      EEPROM.commit();
      LongRangeLoraSetup();
    }
    if (strcmp((char*)data, "1901") == 0) {
      GunID = 1;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1902") == 0) {
      GunID = 2;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1903") == 0) {
      GunID = 3;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1904") == 0) {
      GunID = 4;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1905") == 0) {
      GunID = 5;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1906") == 0) {
      GunID = 6;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1907") == 0) {
      GunID = 7;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1908") == 0) {
      GunID = 8;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1909") == 0) {
      GunID = 9;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1910") == 0) {
      GunID = 10;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1911") == 0) {
      GunID = 11;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1912") == 0) {
      GunID = 12;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1913") == 0) {
      GunID = 13;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1914") == 0) {
      GunID = 14;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1915") == 0) {
      GunID = 15;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1916") == 0) {
      GunID = 16;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1917") == 0) {
      GunID = 17;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1918") == 0) {
      GunID = 18;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1919") == 0) {
      GunID = 19;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1920") == 0) {
      GunID = 20;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1921") == 0) {
      GunID = 21;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1922") == 0) {
      GunID = 22;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1923") == 0) {
      GunID = 23;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1924") == 0) {
      GunID = 24;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1925") == 0) {
      GunID = 25;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1926") == 0) {
      GunID = 26;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1927") == 0) {
      GunID = 27;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1928") == 0) {
      GunID = 28;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1929") == 0) {
      GunID = 29;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1930") == 0) {
      GunID = 30;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1931") == 0) {
      GunID = 31;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1932") == 0) {
      GunID = 32;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1933") == 0) {
      GunID = 33;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1934") == 0) {
      GunID = 34;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1935") == 0) {
      GunID = 35;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1936") == 0) {
      GunID = 36;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1937") == 0) {
      GunID = 37;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1938") == 0) {
      GunID = 38;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1939") == 0) {
      GunID = 39;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1940") == 0) {
      GunID = 40;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1941") == 0) {
      GunID = 41;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1942") == 0) {
      GunID = 42;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1943") == 0) {
      GunID = 43;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1944") == 0) {
      GunID = 44;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1945") == 0) {
      GunID = 45;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1946") == 0) {
      GunID = 46;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1947") == 0) {
      GunID = 47;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1948") == 0) {
      GunID = 48;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1949") == 0) {
      GunID = 49;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1950") == 0) {
      GunID = 50;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1951") == 0) {
      GunID = 51;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1952") == 0) {
      GunID = 52;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1953") == 0) {
      GunID = 53;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1954") == 0) {
      GunID = 54;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1955") == 0) {
      GunID = 55;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1956") == 0) {
      GunID = 56;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1957") == 0) {
      GunID = 57;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1958") == 0) {
      GunID = 58;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1959") == 0) {
      GunID = 59;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1960") == 0) {
      GunID = 60;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1961") == 0) {
      GunID = 61;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "1962") == 0) {
      GunID = 62;
      EEPROM.write(2,GunID);
      EEPROM.commit();
      Serial.println("Gun ID set to: " + String(GunID));
    }
    if (strcmp((char*)data, "SPP") == 0) {
      BLEMODE = false;
      SPPMODE = true;
      HC05MODE = false;
      EEPROM.write(3,0);
      EEPROM.commit();
      Serial.println("Bluetooth Set to SPP");
    }
    if (strcmp((char*)data, "BLE") == 0) {
      BLEMODE = true;
      SPPMODE = false;
      HC05MODE = false;
      EEPROM.write(3,1); // 1 is on
      EEPROM.commit();
      Serial.println("Bluetooth Set to BLE");
    }
    if (strcmp((char*)data, "HC05") == 0) {
      BLEMODE = false;
      SPPMODE = false;
      HC05MODE = true;
      EEPROM.write(3,2); // 1 is on
      EEPROM.commit();
      Serial.println("Bluetooth Set to HC05");
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
// guns health status and ammo reserve etc.
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  //Serial.print("Notify callback for characteristic ");
  //Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  //Serial.print(" of data length ");
  //Serial.println(length);
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
    while (ptr != NULL)
    {
      TokenStrings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
      if (DebugComms) {
        Serial.println("Token " + String(index) + ": " + TokenStrings[index]);
      }
    }
    Serial.println("Token 0: " + TokenStrings[0]);
    Serial.println("We have found " + String(index ) + " tokens");
    if (!FirstConnect) {
      FirstConnect = true;
      RunInit = true;
    }
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
      Serial.println("onDisconnect");
      Serial.println("Creating Delay for Reconnect");
      DisconnectTime = millis();

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
    if (!FirstConnect) {
      FirstConnect = true;
      RunInit = true;
    }
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
        doConnect = true;
        doScan = false;
      } // Found our server
      else {
        doScan = true;
      }
    } // onResult
}; // MyAdvertisedDeviceCallbacks
//******************************************************************************************
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
//******************************************************************************************
void ProcessBRXData() {
  // apply conditional statements for when coms com from tagger
  if (TokenStrings[0] == "$PONG") { // CONFIRMING CONNECTION
    connected = true;
    Serial.println("Connection to Tagger Verified");
  }
  if (TokenStrings[0] == "$BUT") { // a button was pressed
    if (TokenStrings[1] == "0" && TokenStrings[2] == "0") { // Trigger pulled
      Serial.println("trigger pulled");
      // IF IN ASSAULT MODE, TAGGER NEEDS TO SEE IF THERE ARE ANY SPAWNS LEFT FOR TEAM
      // BY ASKING HOST FOR A SPAWN IF AVAILABLE
      if (LifeStatus == 1) {
        if (SettingsGameRules == 2) {
          // we have a dead player playing assault and pulling the trigger to spawn
          // RespawnRequest,TeamID,GunID,*
          String RespawnRequest = "$RR," + String(TeamID) + "," + String(GunID) + ",*";
          if (CommsSetting == 0) {
            ESPNOWBroadcast(RespawnRequest);
            delay(5);
            ESPNOWBroadcast(RespawnRequest);
          } else {
            LoRaDataToSend = RespawnRequest;
            TransmitLoRa();
          }
        }
      }
    }
    if (ALLOWPERKSELECTION) {
      if (TokenStrings[1] == "1" && TokenStrings[2] == "0") {
        // ALT PRESSED, CHANGE WEAPON SELECTION
        if (PBPerk == 6) {
          PBPerk = 0;
        } else {
          PBPerk++;
        }
        if (PBPerk == 0) { 
          SendToTaggerLater1 = "$PLAY,VA38,3,10,,,,,*";
        }
        if (PBPerk == 1) { 
          SendToTaggerLater1 = "$PLAY,VA4D,3,10,,,,,*";
        }
        if (PBPerk == 2) { 
          SendToTaggerLater1 = "$PLAY,VA1G,3,10,,,,,*";
        }
        if (PBPerk == 3) { 
          SendToTaggerLater1 = "$PLAY,VA2N,3,10,,,,,*";
        }
        if (PBPerk == 4) { 
          SendToTaggerLater1 = "$PLAY,VA1Z,3,10,,,,,*";
        }
        if (PBPerk == 5) { 
          SendToTaggerLater1 = "$PLAY,VA24,3,10,,,,,*";
        }
        if (PBPerk == 6) { 
          SendToTaggerLater1 = "$PLAY,VA2W,3,10,,,,,*";
        }
        Serial.println("changed PERK id to: " + String(PBPerk));
      }
    }
    if (ALLOWWEAPONSELECTION) {
      if (TokenStrings[1] == "0" && TokenStrings[2] == "0") {
        // TRIGGER PULLED, CHANGE WEAPON SELECTION
        if (PBWeap == 6) {
          PBWeap = 0;
        } else {
          PBWeap++;
        }
        if (SettingsGameMode < 3) {
          if (PBWeap == 0) { 
            SendToTaggerLater1 = "$PLAY,VA4B,3,10,,,,,*";
          }
          if (PBWeap == 1) { 
            SendToTaggerLater1 = "$PLAY,VA5Y,3,10,,,,,*";
          }
          if (PBWeap == 2) { 
            SendToTaggerLater1 = "$PLAY,VA5R,3,10,,,,,*";
          }
          if (PBWeap == 3) { 
            SendToTaggerLater1 = "$PLAY,VA6A,3,10,,,,,*";
          }
          if (PBWeap == 4) { 
            SendToTaggerLater1 = "$PLAY,VA4F,3,10,,,,,*";
          }
          if (PBWeap == 5) { 
            SendToTaggerLater1 = "$PLAY,VA6B,3,10,,,,,*";
          }
          if (PBWeap == 6) { 
            SendToTaggerLater1 = "$PLAY,VA9O,3,10,,,,,*";
          }
        }
        if (SettingsGameMode > 2 && SettingsGameMode < 5) {
          if (TeamID == 0) {
            // resistance
            if (PBWeap == 0) { 
              SendToTaggerLater1 = "$PLAY,V3M,3,10,,,,,*";
            }
            if (PBWeap == 1) { 
              SendToTaggerLater1 = "$PLAY,VEM,3,10,,,,,*";
            }
            if (PBWeap == 2) { 
              SendToTaggerLater1 = "$PLAY,V8M,3,10,,,,,*";
            }
            if (PBWeap == 3) { 
              SendToTaggerLater1 = "$PLAY,VHM,3,10,,,,,*";
            }
            if (PBWeap == 4) { 
              SendToTaggerLater1 = "$PLAY,VNM,3,10,,,,,*";
            }
            if (PBWeap == 5) { 
              SendToTaggerLater1 = "$PLAY,VA35,3,10,,,,,*";
            }
          }
          if (TeamID == 1) {
            // NEXUS
            if (PBWeap == 0) { 
              SendToTaggerLater1 = "$PLAY,VCM,3,10,,,,,*";
            }
            if (PBWeap == 1) { 
              SendToTaggerLater1 = "$PLAY,V7M,3,10,,,,,*";
            }
            if (PBWeap == 2) { 
              SendToTaggerLater1 = "$PLAY,V2M,3,10,,,,,*";
            }
            if (PBWeap == 3) { 
              SendToTaggerLater1 = "$PLAY,V1M,3,10,,,,,*";
            }
            if (PBWeap == 4) { 
              SendToTaggerLater1 = "$PLAY,VNM,3,10,,,,,*";
            }
            if (PBWeap == 5) { 
              SendToTaggerLater1 = "$PLAY,VQ1,3,10,,,,,*";
            }
          }
          if (TeamID == 3) {
            // VANGUARD
            if (PBWeap == 0) { 
              SendToTaggerLater1 = "$PLAY,VKM,3,10,,,,,*";
            }
            if (PBWeap == 1) { 
              SendToTaggerLater1 = "$PLAY,VDM,3,10,,,,,*";
            }
            if (PBWeap == 2) { 
              SendToTaggerLater1 = "$PLAY,VGM,3,10,,,,,*";
            }
            if (PBWeap == 3) { 
              SendToTaggerLater1 = "$PLAY,VJM,3,10,,,,,*";
            }
            if (PBWeap == 4) { 
              SendToTaggerLater1 = "$PLAY,VNM,3,10,,,,,*";
            }
            if (PBWeap == 5) { 
              SendToTaggerLater1 = "$PLAY,VR1,3,10,,,,,*";
            }
          }
        }
        Serial.println("changed Weapon id to: " + String(PBWeap));
      }
    }
    if (TokenStrings[1] == "5" && TokenStrings[2] == "0") {
      // select pressed and released
      if (ALLOWTEAMSELECTION) {
        // CHANGE PLAYER TEAM
        if (TeamID == 3) {
          TeamID = 0;
        } else {
          TeamID++;
        }
        if (TeamID == 0) { 
          SendToTaggerLater1 = "$PLAY,VA13,3,10,,,,,*"; 
          SendToTaggerLater2 = "$GLED,0,0,0,0,10,,*";
        }
        if (TeamID == 1) { 
          SendToTaggerLater1 = "$PLAY,VA1L,3,10,,,,,*"; 
          SendToTaggerLater2 = "$GLED,1,1,1,0,10,,*";
        }
        if (TeamID == 2) { 
          SendToTaggerLater1 = "$PLAY,VA1R,3,10,,,,,*"; 
          SendToTaggerLater2 = "$GLED,2,2,2,0,10,,*";
        }
        if (TeamID == 3) { 
          SendToTaggerLater1 = "$PLAY,VA27,3,10,,,,,*"; 
          SendToTaggerLater2 = "$GLED,3,3,3,0,10,,*";
        }
        Serial.println("changed team id to: " + String(TeamID));
      }
    }
    if (TokenStrings[1] == "2" && TokenStrings[2] == "0") {
      // RELOAD PULLED
      if (ALLOWTEAMSELECTION) {
        ALLOWTEAMSELECTION = false;
        SendToTaggerLater1 = "$PLAY,VAO,3,10,,,,,*";
      }
      if (ALLOWPERKSELECTION) {
        ALLOWPERKSELECTION = false;
        SendToTaggerLater1 = "$PLAY,VAO,3,10,,,,,*";
      }
      if (ALLOWWEAPONSELECTION) {
        ALLOWWEAPONSELECTION = false;
        SendToTaggerLater1 = "$PLAY,VAO,3,10,,,,,*";
      }
    }
  }
  if (TokenStrings[0] == "$HIR") {
    // tagged by another player, store who and what team
    lastTaggedPlayer = TokenStrings[3].toInt();
    lastTaggedTeam = TokenStrings[4].toInt();
  }
  if (TokenStrings[0] == "$HP") { 
    // this is an update for current health
    if (TokenStrings[1] == "0") {
      // player is dead or turned zombie
      if (LifeStatus == 0) {
        // player was previously alive
        Serial.println("Player Just Died or turned zombie if in survival");
        LifeStatus = 1; // indicating player dead
        int RandomNumber = random(0,9999);
        // $DD,(PlayerIDThatKilled),(teamIDthatkilled),(deadplayerID),*
        killnotificationstring = "$DD," + String(lastTaggedPlayer) + "," + String(lastTaggedTeam) + "," + String(GunID) + "," + String(RandomNumber) +",*";
        HOSTKILLCONFIRM = true;
        PLAYERKILLCONFIRM = true;
        LastKillConfirmationSend = millis() - (6 * GunID);
        KillConfirmationAttemptCounter = 0;
        HostKillConfirmationAttemptCounter = 0;
      }
    } else {
      if (LifeStatus == 1 && SettingsGameMode < 5) {
        // player was dead and now is alive and we are not playing survival
        LifeStatus = 0;
        Serial.println("Player Revived");
      }
    }  
  }
}
void StartPresetGameMode() {
  FAKEHEADSET = true;
  if (PresetGameMode < 3) {
    BLESend("$CLEAR,*");
    BLESend("$START,*");
    // token one of the following command is free for all, 0 is off and 1 is on
    BLESend("$GSET,0,0,1,0,1,0,50,1,*");
    String Health = "null";
    if (PresetGameMode == 0) {
      Health = "500,250,150";
    }
    if (PresetGameMode == 1) {
      Health = "1000,500,250";
    }
    if (PresetGameMode == 2) {
      Health = "2000,1000,500";
    }
    BLESend("$PSET,63,2,"+String(Health)+",50,,H44,JAD,V33,M05,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
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
    BLESend("$SIR,4,0,,1,0,0,1,,*"); // standard damgat but is actually the captured flag pole
    BLESend("$SIR,0,0,,1,0,0,1,,*"); // standard weapon, damages shields then armor then HP
    BLESend("$SIR,0,1,,36,0,0,1,,*"); // force rifle, sniper rifle ?pass through damage?
    BLESend("$SIR,0,3,,37,0,0,1,,*"); // bolt rifle, burst rifle, AMR
    BLESend("$SIR,1,0,H29,10,0,0,1,,*"); // adds HP with this function********
    BLESend("$SIR,2,1,VA8C,11,0,0,1,,*"); // adds shields*******
    BLESend("$SIR,3,0,VA16,13,0,0,1,,*"); // adds armor*********
    BLESend("$SIR,6,0,H02,1,0,90,1,40,*"); // rail gun
    BLESend("$SIR,8,0,,38,0,0,1,,*"); // charge rifle
    BLESend("$SIR,9,3,,24,10,0,,,*"); // energy launcher
    BLESend("$SIR,10,0,X13,1,0,100,2,60,*"); // rocket launcher
    BLESend("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
    BLESend("$SIR,13,1,H57,1,0,0,1,,*"); // energy blade
    BLESend("$SIR,13,0,H50,1,0,0,1,,*"); // rifle bash
    BLESend("$SIR,13,3,H49,1,0,100,0,60,*"); // war hammer
    BLESend("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
    BLESend("$BMAP,1,100,0,1,2,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
    BLESend("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
    BLESend("$BMAP,3,3,,,,,*"); // sets the select button as Weapon 3
    BLESend("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4
    BLESend("$BMAP,5,5,,,,,*"); // Sets the right button as weapon 5
    BLESend("$BMAP,8,4,,,,,*"); // sets the gyro as weapon 4
    BLESend("$SPAWN,,*");
    String Weap0 = "$WEAP,0,,100,0,0,24,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*";
    String Weap1 = "$WEAP,1,,100,8,0,150,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*";
    String Weap2 = "$WEAP,2,,100,0,0,150,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*";
    String Weap3 = "$WEAP,3,1,90,11,1,1,0,,,,,,1,80,1400,50,10,0,0,10,11,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,10,9999999,30,30,*";
    String Weap4 = "$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,30,,*";
    String Weap5 = "$WEAP,5,1,90,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,30,20,*";
    BLESend(Weap0); // Assault Rifle
    BLESend(Weap4); // Melee
    if (PresetGameMode == 1) {
      BLESend(Weap1); // Charge Rifle
      BLESend(Weap3); // Gas players melee
    }
    if (PresetGameMode == 2) {
      BLESend(Weap1); // Charge Rifle
      BLESend(Weap2); // Ion Sniper
      BLESend(Weap3); // Gas players melee
      BLESend(Weap5); // Explosion melee
    }
  }
  if (PresetGameMode == 4) {
    // start basic game that allows live scoring without a host
    Serial.println("***** Game Starting ******");
    DataToTagger = "$AS,1,0,4,0,10,0,95,*";
    Setup = "$PBWEAP,3,*";
    BLESend(Setup);
    delay(200);
    BLESend(DataToTagger);
    // game start command, need to let host know that we are good to go and in game periodically
    LifeStatus = 0; // indicates player is alive
    // this tells the host that he is not alone and that this gun is alive or in play
    INGAME = true;
    ALLOWTEAMSELECTION = false;
    ALLOWPERKSELECTION = false;
    ALLOWWEAPONSELECTION = false;
    PresetGameStart = millis();
    PresetGameEnd = PresetGameStart + 600000;
  }
}
// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
// had some major help from Sri Lanka Guy!
void BLESend(String value) {
  if (BLEMODE) {
    const char * c_string = value.c_str();
    uint8_t buf[21] = {0};
    int sentSize = 0;
    Serial.println("sending ");
    Serial.println(value);
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
      for (int i = 0; i < remaining; i++) {
        Serial.print((char)buf[i]);
        Serial.println();
      }
    } else {
      pRemoteRXCharacteristic->writeValue((uint8_t*)value.c_str(), value.length(), true);
    }
  } else {
    if (SPPMODE) {
      SerialBT.print(value);
    }
    if (HC05MODE) {
      SerialHW.print(value);
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
void StartWebServer() {
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  //esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
  //esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
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
      name = BLEName;
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
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  // Start server
  server.begin();
}
void ManageOTAUpdate() {
    // if trying to update
    static int num=0;
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= otainterval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      //if (FirmwareVersionCheck()) {
        firmwareUpdate();
      //}
    }
    if (UPTODATE) {
      Serial.println("all good, up to date, lets restart");
      ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
    }
}
void TakeOverTagger() {
  RunInit = false;
  // enable tagger bluetooth data to phone chip
  BLESend("$CONNECT,*");
  delay(500);
  // enable tagger bluetooth data to phone chip
  BLESend("$INIT,*");
  delay(500);
  // enable tagger bluetooth data to phone chip
  BLESend("$PHONE,*");
  delay(500);
  // set player ID and lock out player
  DataToTagger = "$IT,"+String(GunID)+",4,0,0,0,0,0,75,0,*";
  BLESend(DataToTagger);
}
void ManageBluetooth() {
  currentMillis1 = millis(); // sets the timer for timed operations
  if (currentMillis1 - ClearCache > 2000) {
    LastIncomingData = "null";
    ClearCache = millis();
  }
  // the main loop for BLE activity is here, it is devided in three sections....
  // sections are for when connected, when not connected and to connect again
  //***********************************************************************************
  if (BLEMODE) {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (currentMillis1 - DisconnectTime > 400) {
      if (connectToServer()) {
        Serial.println("We are now connected to the BLE Server.");
        doConnect = false; // stop trying to make the connection.
      } else {
        Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      }
    }
  }
  }
  //*************************************************************************************
  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (HC05MODE) {
    ListenToHardwareSerial();
  }
  if (connected) {
    unsigned long CurrentMillis = millis();
    // CHECK SERIAL MONITOR FOR INPUT TO SEND TO TAGGER
    if (Serial.available() > 0) {
        String str = Serial.readString();
        BLESend(str);
    }
    if (HC05MODE) {
      ListenToHardwareSerial();
    }
    if (FAKEHEADSET) {
      if (CurrentMillis - PreviousHeadsetPing > 4000) {
        // time to update the TAGGGER THAT IT IS PAIRED
        PreviousHeadsetPing = CurrentMillis;
        BLESend("$RADSK,*");
      }
    }
    if (!REGISTERED) {
      //this sends to host player status alive and their team
      if (CurrentMillis - PreviousHostUpdate > 20000) {
        // time to update the host on status
        PreviousHostUpdate = millis();
        byte ingamestatus = 0; // 0 for not in game, 1 for in game
        if (INGAME) {ingamestatus = 1;}
        String StatusUpdate = "$PH," + String(GunID) + "," + String(TeamID) + "," + String(ingamestatus) + "," + String(LifeStatus) + ",*";
        if (CommsSetting == 0) {
          ESPNOWBroadcast(StatusUpdate);
        }
        BLESend("$PING,*");
      }
    }
    if (RunInit) {
      TakeOverTagger();
    }
    if (PresetGameMode != 3) {
      if (!INGAME) {
        // here we put in processes to run based upon conditions to make a boss, only runs if not in game
        if (CurrentMillis > 20000) { 
          INGAME = true;
          StartPresetGameMode();
        }
      }
    } 
    if (PresetGameMode > 3) {
      if (INGAME) {
        if (CurrentMillis > PresetGameEnd) {
          DataToTagger = "$SP,99,*";
          BLESend(DataToTagger);
          INGAME = false;
        }
      }
    }
    if (SendToTaggerLater1 != "null") {
      if (CurrentMillis - WaitStart > WaitTime) {
        BLESend(SendToTaggerLater1);
        WaitTime = 0;
        SendToTaggerLater1 = "null";
        Serial.println("Sent SendToTaggerLater1");
        if (SendToTaggerLater2 != "null") {
          delay(250);
        }
      }
    }
    if (SendToTaggerLater2 != "null") {
      if (CurrentMillis - WaitStart > WaitTime) {
        BLESend(SendToTaggerLater2);
        WaitTime = 0;
        SendToTaggerLater2 = "null";
        Serial.println("Sent SendToTaggerLater1");
      }
    }
    if (PLAYERKILLCONFIRM == true) {
      if (CurrentMillis - LastKillConfirmationSend > 750) {
        LastKillConfirmationSend = CurrentMillis;
        ESPNOWBroadcast(killnotificationstring);
        KillConfirmationAttemptCounter++;
        if (KillConfirmationAttemptCounter > 6) {
          PLAYERKILLCONFIRM = false;
          KillConfirmationAttemptCounter = 0;
          Serial.println("Failed to receive confirmation from host and killing player");
        }
      }
    }
    if (HOSTKILLCONFIRM == true) {
      if (CurrentMillis - LastKillConfirmationSend > 9000) {
        LastKillConfirmationSend = CurrentMillis;
        if (CommsSetting == 1) {
          LoRaDataToSend = killnotificationstring;
          TransmitLoRa();
        } else {
          ESPNOWBroadcast(killnotificationstring);
        }
        HostKillConfirmationAttemptCounter++;
        if (HostKillConfirmationAttemptCounter > 5) {
          HOSTKILLCONFIRM = false;
          HostKillConfirmationAttemptCounter = 0;
          Serial.println("Failed to receive confirmation from host and killing player");
        }
      }
    }
  } else {
    Serial.println("No paired device found");
    if (BLEMODE) {
      if (doScan) {
        BLESetup();
      }
    } else {
      if (SPPMODE) {
        if(!connected) {
          Serial.println("trying to connect");
          connected = SerialBT.connect(name);
          Serial.println("Attempt Complete");
        }
      }
      if (HC05MODE) {
        if (!connected) {
          Serial.println("trying to connect");
          BLESend("$PING,*");
        }
      }
    }
  }
}
//************************************************************************************
void setup() {
// initialize serial port for debug
  Serial.begin(115200);
  Serial.println("Starting JEDGE Client Application...");
  pinMode(led, OUTPUT);
  digitalWrite(led,  ledState);
// initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  CommsSetting = EEPROM.read(4);
  if (CommsSetting == 255) {CommsSetting = 0;}
  GunID = EEPROM.read(2);
  Serial.println("Player ID: " + String(GunID));
  ssid = "JEDGE-TAGGER#" + String(GunID);
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
  byte datacheck = EEPROM.read(3);
  Serial.println("bluetooth data check: " + String(datacheck));
  if (datacheck == 0) {
    BLEMODE = false;
    SPPMODE = true;
    HC05MODE = false;
    Serial.println("Device in SPP Mode");
  } else {
    if (datacheck == 1) {
      BLEMODE = true;
      SPPMODE = false;
      HC05MODE = false;
      Serial.println("Device in BLE Mode");
    }
    if (datacheck == 2) {
      BLEMODE = false;
      SPPMODE = false;
      HC05MODE = true;
      Serial.println("Device in HC05 Mode");
    }
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
  name = BLEName;
  int settingcheck = EEPROM.read(1);
  if (settingcheck == 255) {
    PresetGameMode = 4;
  } else {
    PresetGameMode = settingcheck;
  }
  if (PresetGameMode == 0) {Serial.println("Boss Mode - Easy");}
  if (PresetGameMode == 1) {Serial.println("Boss Mode - Medium");}
  if (PresetGameMode == 2) {Serial.println("Boss Mode - Hard");}
  if (PresetGameMode == 3) {Serial.println("Standard JEDGE Player");}
  if (PresetGameMode == 4) {Serial.println("JEDGE Arcade Mode");}
  if (ENABLEOTAUPDATE) {
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
    pinMode(2, OUTPUT);
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.mode(WIFI_AP_STA);
    // SET THE RIGHT WIRELESS PROTOCOL
    //esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
    //esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
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
    if (touchstate < 50) { // indicates capacitive touch is being touched
      Serial.println("TouchState: " + String(touchstate));
      delay(1000);
      touchstate = touchRead(4);
      Serial.println("TouchState: " + String(touchstate));
      if (touchstate < 50) { // cap.touch button still pressed
        StartWebServer();
        ENABLEWEBSERVER = true;
        Serial.println("WebServer Activated");
        digitalWrite(led,HIGH);
      } else {Serial.println("touch contact lost");}
    } else {Serial.println("No Capacitive touch detected, starting preset boot modes");}
  }
  if (!ENABLEWEBSERVER && !ENABLEOTAUPDATE) {
    // delay for boot time
    delay(7000);
    // initialize ble cient connection to brx
    // Start Wireless Comms Now
    IntializeESPNOW();
    StartLoRaSerial();
    if (PresetGameMode != 3) {
      INITBLUETOOTH = true;
    }
  }
}
// End of Setup
//******************************************************************************************
//******************************************************************************************
//******************************************************************************************
// This is the Arduino main loop // for the BLE Core.
void loop() {
  // THE DEVICE RUNS IN THREE MODES: UPDATE, JEDGE, CONFIG.
  // THESE WHILE LOOPS MANAGE THOSE PROCESSES, CAN BE IF STATEMENTS AS WELL, DOESNT SEEM TO MATTER
  if (CommsSetting == 1) {
    ReceiveLoRaTransmission();
  }
  if (ENABLEOTAUPDATE) {
    ManageOTAUpdate();
  }
  if (ENABLEWEBSERVER) {
    ws.cleanupClients();
  }
  if (ENABLEBLE) {
    ManageBluetooth();
  } else {
    if (PresetGameMode == 3) {
      //this sends to host player status alive and their team
      currentMillis0 = millis();
      if (currentMillis0 - previousMillis1 > 10000) {
        // time to update the host on status
        previousMillis1 = millis();
        byte ingamestatus = 0; // 0 for not in game, 1 for in game
        if (INGAME) {ingamestatus = 1;}
        String StatusUpdate = "$PH," + String(GunID) + "," + String(TeamID) + "," + String(ingamestatus) + "," + String(LifeStatus) + ",*";
        if (CommsSetting == 0) {
          ESPNOWBroadcast(StatusUpdate);
        } else {
          LoRaDataToSend = StatusUpdate;
          TransmitLoRa();
        }
        if (currentMillis0 > 300000) {
          Serial.println("No Host Detected, Power Saving Initialized");
          esp_deep_sleep_start();
        }
      }
    }
  }
  if (INITBLUETOOTH) {
    INITBLUETOOTH = false;
    Serial.println("booting bluetooth");
    ENABLEBLE = true;
    // Check if we are in legacy tagger mode or not
    if (BLEMODE) {
      BLESetup();
    }
    if (SPPMODE) {
     SetupBluetoothSPP();
    }
    if (HC05MODE) {
      StartHC05Serial();
    }
  }
}
// End of loop

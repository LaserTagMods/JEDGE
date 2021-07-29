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
 * updated 02/15/2021 fixed the stealth option to turn of gun leds in game play
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
 * updated 05/01/2021 found bug for assimilation - hopefully fixed by adding in player settings object
 *                    added in menu option for doing variations of melee.
 *                    finished gun game mode
 * updated 05/01/2021 tweaked a bit more on melee disarm function                   
 *                    finished up main settings for infection mode.
 * updated 05/12/2021 added some jbox espnow actions in                   
 * updated 05/15/2021 added in a dead but not out timer for battle royale
 *                    added in perk selections, finally, for weapon 5 right button
 * updated 05/16.2021 added notifications for when a new leadr captures  domination base                   
 * updated 05/19/2021 added in capture the flag gameplay interactino with jbox                  
 * updated 06/06/2021 added in feature for resent settings from controller so that taggers who received it do not react but those who missed it, do react
 *                    fixed the battle royale pistol so that it has unlimited ammo instead of limited ammo
 * updated 06/07/2021 added in forwarding of kill confirmations to other players. unique kill confirmations to be forwarded once and then reset after 4 seconds pass to alow forwarding of the sae one again, later.
 *                    added hidden menu unlock ability
 * updated 06/12/2021 added repeat of settings commands from controller to rebroadcast to help to ensure that all taggers get the command sent from controller
 * updated 07/04/2021 added in score reporting in game by each tagger by pressing a button (left button - melee)               
 *                    enabled capturing of other taggers score reporting to accumulate score on this device to report accurate score info at end of game on tagger.
 *                    fixed bug for receiving kill confirmations that are repetitive or back to back so that it works regardless.
 * updated 07/10/2021 Added in web based updating and tagger ID reasignment                   
 * updated 07/11/2021 Added in data packet 5 for espnow to request and receive wifi credentials from controller
 * updated 07/12/2021 removed packet 5 and went with strings for web connection and added in gun gen selector for eeprom
 * updated 07/20/2021 Added callout functions provided by paul for some settings, adjusted kill confirmation callouts but still not 100%
 *                    Added tagger/headset color changes upon esp32 boot/reboot, for identification of reboot/boot
 *                    Added in audio confirmations for update processes
 * updated 07/24/2021 Fixed gen3 false starts with work around by multiple sendstring repetitions                   
 *                    Fixed Gen3 looping green headsets after respawn
 * updated 07/26/2021 fixed all kinds of audio integration issues, got starwars up and running, contra up and running. fixed tons of bugs               
 *                    possibly fixed assimilation... needs testing but hopefull *crosses fingers*
 * updated 07/29/2021 Changed the score announcements for players taggers to select button instead of left dpad button (melee interference)                   
 *                    fixed the stealth outdoor/indoor call out and selection
 *                    tweaked the headset random flashing problem - hopfully
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
#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <Arduino_JSON.h> // needed for auto updates on web server
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
//****************************************************************

int sample[5];

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 5 // use 0 for OTA and 1 for Tagger ID
// if eeprom 0 is 1, it is OTA mode, if 0, regular mode.

#define SERIAL1_RXPIN 16 // TO BRX TX and BLUETOOTH RX
#define SERIAL1_TXPIN 17 // TO BRX RX and BLUETOOTH TX
bool ESPTimeout = true; // if true, this enables automatic deep sleep of esp32 device if wifi or blynk not available on boot
long TimeOutCurrentMillis = 0;
long TimeOutInterval = 480000;
int BaudRate = 57600; // 115200 is for GEN2/3, 57600 is for GEN1, this is set automatically based upon user input
bool RUNWEBSERVER = true; // this enables the esp to act as a web server with an access point to connect to
bool FAKESCORE = false;

//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//*********** YOU NEED TO CHANGE INFO IN HERE FOR EACH GUN!!!!!!***********
int GunID = 3; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
const char GunName[] = "GUN#3"; // used for OTA id recognition on network and for AP for web server
int GunGeneration = 2; // change to gen 1, 2, 3
const char* password = "123456789"; // Password for web server
String OTAssid = "dontchangeme"; // network name to update OTA
String OTApassword = "dontchangeme"; // Network password for OTA
int TaggersOwned = 64; // how many taggers do you own or will play?
bool ACTASHOST = false; // enables/disables the AP mode for the device so it cannot act as a host. Set to "true" if you want the device to act as a host
String FirmwareVer = {"3.33"};
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//****************************************************************

// OTA variables

#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/fw.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"

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
int Melee = 0; // default melee to off
int SetSlotB=1; // this is for weapon slot 1, default is unarmed
int SLOTB=100; // used when weapon selection is manual
int SetLives=32000; // used for configuring lives
int SetHealth = 0; // used for player health settings
int SetSlotC=0; // this is for weapon slot 3 Respawns Etc.
int SetSlotD = 0; // this is used for perk IR tag
int SetTeam=0; // used to configure team player settings, default is 0
long SetTime=2000000000; // used for in game timer functions on esp32 (future
int SetODMode=0; // used to set indoor and outdoor modes (default is on)
int SetGNDR=0; // used to change player to male 0/female 1, male is default 
int SetRSPNMode; // used to set auto or manual respawns from bases/ir (future)
long RespawnTimer = 0; // used to delay until respawn enabled
long LastRespawnTime = 0;
int RespawnStatus = 0;
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
int AnounceScore = 99; // used for button activated score announcement
long AudioPreviousMillis = 0;
String AudioPerk; // used to play stored audio files with tagger FOR BLE CORE
int AudioPerkTimer = 4000; // used to time the perk announcement
bool PERK = false; // used to trigger audio perk
String TerminalInput; // used for sending incoming terminal widget text to Tagger
bool TERMINALINPUT = false; // used to trigger sending terminal input to tagger
int CurrentDominationLeader = 99;
int ClosingVictory[4];

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
long RespawnStartTimer = 0; // used for dead but not out scenario for battle royale
int DeadButNotOutTimer = 45000; // used as the maximum time for allowing a respawn 
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
bool STATBOOST = false; // used to enable a stat boost
int PlayerStatBoost = 0; // used for implementing a stat boost
bool SPECIALWEAPONPICKUP = false; // used to trigger a weapon loadout
bool STEALTH = false; // used to turn off gun led side lights
bool INITJEDGE = false;
bool STRINGSENDER = false;
String StringSender = "Null";
bool DISARMED = false;
bool UNLOCKHIDDENMENU = false;

// Special Game MODE VARIABLES
bool BACKGROUNDMUSIC = false;
unsigned long MusicPreviousMillis = 0;
long MusicRestart = 0;
String MusicSelection = "null"; 

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

int lastincomingData2 = 0; // used for repeated data sends
int lastkillconfirmationorigin = 9999;
int lastkillconfirmationdestination = 9999;
long killconfirmationresettimestamp = 0;
bool KILLCONFIRMATIONRESETNEEDED = false;
int lastkilledplayer = 9999;
bool LASTKILLEDPLAYERRESETNEEDED = false;
long lastkilledplayertimestamp = 0;

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
  Serial.println();
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
    
  document.getElementById("to3").innerHTML = obj.temporaryteamobjectives3;
  
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
    
    var O0;
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
    document.getElementById('presetgamemodesid').addEventListener('change', handleGMode, false);
  }
  function handleGMode() {
    var xx = document.getElementById("presetgamemodesid").value;
    websocket.send(xx);
  }
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
    
    if (strcmp((char*)data, "toggle16") == 0) {
      if (Menu[17] == 1702) {
        Menu[17] = 1700;
      } else {
        Menu[17]++;
      }
      WebSocketData = Menu[17];
      notifyClients();
      Serial.println(" menu = " + String(Menu[17]));
      datapacket2 = Menu[17];
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
//**************************************************************
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
void ProcessIncomingCommands() {
  ledState = !ledState;
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  if (incomingData2 == 902) {
    // this is a score report from another tagger, lets get our own score up to date. 
    // tokens 9-72 are player kills to each player being reported to the controller
    // we will capture this data as well and see how many kills this tagger got and 
    // determine a winner.
    AccumulateIncomingScores();
    MyKills = PlayerKills[GunID];
  }
  if (incomingData2 == 904) { // this is a wifi credential
    Serial.println("received SSID from controller");
    OTAssid = incomingData4;
    Serial.print("OTApassword = ");
    Serial.println(OTApassword);
    Serial.print("OTAssid = ");
    Serial.println(OTAssid);
  }
  if (incomingData2 == 905) { // this is a wifi credential
    Serial.println("received Wifi Password from controller");
    OTApassword = incomingData4;
    Serial.print("OTApassword = ");
    Serial.println(OTApassword);
    Serial.print("OTAssid = ");
    Serial.println(OTAssid);
  }
  if (ESPTimeout) {
    ESPTimeout = false;
  }
  int IncomingTeam = 99;
  if (incomingData1 > 199) {
    IncomingTeam = incomingData1 - 200;
  }
  if (incomingData1 == GunID || incomingData1 == 99 || IncomingTeam == SetTeam) { // data checked out to be intended for this player ID or for all players
    digitalWrite(2, HIGH);
   if (incomingData2 != lastincomingData2 || incomingData2 == 32000) {
    lastincomingData2 = incomingData2;
    if (incomingData2 < 100) { // setting primary weapon or slot 0
      int b = incomingData2;
      if (INGAME==false){
        if (b == 1) {
          settingsallowed=1; 
          AudioSelection="OP10";
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
          AudioSelection="OP11"; 
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
        if (b == 10) { // Player Health set to Standard
          Serial.println("Health Set to Standard"); 
          AudioSelection="OP58";
          SetHealth = 0;
        }
        if (b == 11) { // Player Health set to Weak
          Serial.println("Player Health = weak"); 
          AudioSelection="OP09";
          SetHealth = 1;
        }
        if (b == 12) { // Player Health set to Jugernaught
          Serial.println("Health Set to Jugernaugh"); 
          AudioSelection="A100";
          SetHealth = 2;
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
          Serial.println("Stealth Mode On - Indoor"); 
          AudioSelection="OP59";
          STEALTH = true;
        }
        if (b==4) {
          SetODMode=0; 
          Serial.println("Stealth Mode On - Outdoor"); 
          AudioSelection="OP60";
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
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           */
          BACKGROUNDMUSIC = false;
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
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=1; // set respon mode to auto
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to Immediate"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          Serial.println("Game mode set to Defaults");
          AudioSelection="OP30";
          SetSlotD = 0;
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
           * Teams - Free for all
           * Respawn - 15 seconds Automatic
           * Gender - Male
           * Friendly Fire - On
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 3; // set weapon slot 0 as Assault Rifle
          Serial.println("Weapon slot 0 set to Assault Rifle");
          SetSlotB = 15; // set weapon slot 1 as Shotgun
          Serial.println("Weapon slot 1 set to Shotgun");
          SetLives = 5; // set lives to 5 minutes
          Serial.println("Lives is set to Unlimited");
          SetTime=300000; // set game time to 5 minutes
          Serial.println("Game time set to 5 minutes");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          Serial.println("Game mode set to 5Min F4A AR + Shotguns"); 
          AudioSelection="OP21";
          SetSlotD = 0;
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
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = random(2, 19); // set weapon slot 0 as Random
          Serial.println("Weapon slot 0 set to Random");
          SetSlotB = random(2, 19); // set weapon slot 1 as random
          Serial.println("Weapon slot 1 set to random");
          SetLives = 10; // set lives to 10 
          Serial.println("Lives is set to 10");
          SetTime=600000; // set game time to 10 minutes
          Serial.println("Game time set to 10 minutes");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          Serial.println("Game mode set to 10 Min F4A Rndm Weapon");
          AudioSelection="OP16";
          SetSlotD = 0;
        }
        if (b==4) { // Battle Royal (SwapBRX)
          GameMode=4;
          /*
           * Weapon 0 - PISTOL
           * Weapon 1 - Unarmed
           * Lives - Unlimited
           * Game Time - Unlimited
           * Delay Start - 30 Seconds
           * Ammunitions - Limited magazines
           * Teams - Free for all
           * Respawn - Respawn Station
           * Gender - Male
           * Friendly Fire - On
           * integrate IR protocol for random weapon swap...
           * integrate dead but not out 45 second timer
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 20; // set weapon slot 0 as pistol
          Serial.println("Weapon slot 0 set to PISTOL");
          SetSlotB = 1; // set weapon slot 1 as uarmed
          Serial.println("Weapon slot 1 set to unarmed");
          SetSlotD = 8; // set respawn station as weapon slot 3
          Serial.println("Weapons Slot 3 set as respawn station");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=2000000000; // set game time to unlimited minutes
          Serial.println("Game time set to unlimited");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=1; // set ammunitions to limited mags
          Serial.println("Ammo set to limited magazies"); 
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
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
           * Teams - Free For All
           * Respawn - Manual Respawn (station)
           * Gender - Male
           * Friendly Fire - Off
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 16; // set weapon slot 0 as SMG
          Serial.println("Weapon slot 0 set to SMG");
          SetSlotB = 17; // set weapon slot 1 as sniper rifle
          Serial.println("Weapon slot 1 set to sniper rifle");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=900000;
          Serial.println("Game time set to 15 minute");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to Capture the Flag");
          AudioSelection="VA8P";
          AUDIO=true;
          SetSlotD = 0;
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
           * Teams - No change
           * Respawn - Manual (station)
           * Gender - Male
           * Friendly Fire - Off
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 9; // set weapon slot 0 as force rifle
          Serial.println("Weapon slot 0 set to force rifle");
          SetSlotB = 10; // set weapon slot 1 as ion sniper rifle
          Serial.println("Weapon slot 1 set to ion sniper rifle");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=900000;
          Serial.println("Game time set to 15 minute");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to Own the Zone");
          AudioSelection="OP56";
          AUDIO=true;
          //vTaskDelay(3000);
          SetSlotD = 0;
          
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
           * Teams - manual set (just red and green) human and infected
           * Respawn - Auto 15 seconds
           * Gender - Male
           * Friendly Fire - Off
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 5; // set weapon slot 0 as burst rifle
          Serial.println("Weapon slot 0 set to burst rifle");
          SetSlotB = 21; // set weapon slot 1 as ion sniper rifle
          Serial.println("Weapon slot 1 set to Med Kit");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=600000;
          Serial.println("Game time set to 10 minute");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds");
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to Survival / Infection");
          AudioSelection="VA64";
          AUDIO=true;
          //vTaskDelay(3000);
          SetSlotD = 0;
        }
        if (b==8) { // Assimilation
          /*
           * Weapon 0 - Assault Rifle
           * Weapon 1 - Shotgun
           * Lives - Unlimited
           * Game Time - 15 Minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited Gags
           * Teams - Free for All
           * Respawn - 15 sec auto
           * Gender - male
           * Friendly Fire - off
           */
          BACKGROUNDMUSIC = false;
          GameMode=8; 
          Serial.println("Game mode set to Assimilation");
          AudioSelection="OP01";
          SetSlotA = 3; // set weapon slot 0 as Assault Rifle
          Serial.println("Weapon slot 0 set to Assault Rifle");
          SetSlotB = 15; // set weapon slot 1 as Shotgun
          Serial.println("Weapon slot 1 set to Shotgun");
          SetLives = 32000; // set lives to 5 minutes
          Serial.println("Lives is set to Unlimited");
          SetTime=900000; // set game time to 15 minutes
          Serial.println("Game time set to 15 minutes");
          DelayStart=5000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=5000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=0; // friendly fire set to off
          Serial.println("Friendly Fire Off");
          SetSlotD = 0;
        }
        if (b==9) { // Gun Game
          GameMode=9;
          /*
           * Weapon 0 - AMR
           * Weapon 1 - Unarmed
           * Lives - Unlimited
           * Game Time - Unlimited
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Teams - Free for all
           * Respawn - 15 seconds
           * Gender - Male
           * Friendly Fire - On - part of free for all
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 2; // set weapon slot 0 as AMR
          Serial.println("Weapon slot 0 set to AMR");
          SetSlotB = 1; // set weapon slot 1 as unarmed
          Serial.println("Weapon slot 1 set to Unarmed");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=2000000000; // set game time to unlimited
          Serial.println("Game time set to Unlimited");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=2; // set respon mode to auto
          RespawnTimer=15000; // set delay timer for respawns
          Serial.println("Respawn Set to auto 15 seconds"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          Serial.println("Game mode set to GunGame");;
          AudioSelection="OP45";
          SetSlotD = 0;
        }
        if (b==10) { // team death match with energy weapons
          GameMode=10; 
          /*
           * Weapon 0 - Charge Rifle
           * Weapon 1 - Energy Launcher
           * Lives - Unlimited
           * Game Time - 15 Minutes
           * Delay Start - 30 seconds
           * Ammunitions - Unlimited magazines
           * Teams - No change
           * Respawn - Manual (station)
           * Gender - Male
           * Friendly Fire - Off
          */
          BACKGROUNDMUSIC = false;
          SetSlotA = 6; // set weapon slot 0 as Charge rifle
          Serial.println("Weapon slot 0 set to Charge rifle");
          SetSlotB = 7; // set weapon slot 1 as Energy Launcher
          Serial.println("Weapon slot 1 set to Energy Launcher");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=900000;
          Serial.println("Game time set to 15 minute");
          DelayStart=22000; // set delay start to 30 seconds
          Serial.println("Delay Start Set to 30 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=9; // set respon mode to manual
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to manual (station)"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to Death Match Energy Weapons");
          AudioSelection="OP29";
          AUDIO=true;
          SetSlotD = 0;
        }
        if (b==11) { // Kids Mode
          GameMode=11; 

          /*
           * Weapon 0 - Assault Rifle
           * Weapon 1 - Unarmed
           * Lives - Unlimited
           * Game Time - 10 Minutes
           * Delay Start - 15 Seconds
           * Ammunitions - Unlimited Rounds
           * Teams - Free for all
           * Respawn - Immediate
           * Gender - Male
           * Friendly Fire - On
           */
          BACKGROUNDMUSIC = false;
          SetSlotA = 3; // set weapon slot 0 as AMR
          Serial.println("Weapon slot 0 set to AMR");
          SetSlotB = 1; // set weapon slot 1 as unarmed
          Serial.println("Weapon slot 1 set to Unarmed");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=600000;
          Serial.println("Game time set to 10 minute");
          DelayStart=7000; // set delay start to 15 seconds
          Serial.println("Delay Start Set to 15 seconds");  
          UNLIMITEDAMMO=3; // set ammunitions to unlimited rounds
          Serial.println("Ammo set to unlimited rounds"); 
          SetTeam=0; // set to red/alpha team
          Serial.println("Set teams to free for all, red");
          SetRSPNMode=1; // set respon mode to auto
          RespawnTimer=10; // set delay timer for respawns
          Serial.println("Respawn Set to Immediate"); 
          SetGNDR=0; // male player audio selection engaged
          Serial.println("Gender set to Male");
          SetFF=1; // free for all set to on
          Serial.println("Friendly Fire On");
          Serial.println("Game mode set to Kids Mode");
          AudioSelection="OP48";
          SetSlotD = 0;
        }
        if (b==12) { // CONTRA mode
          GameMode=12; 
          /*
           * Weapon 0 - contra AR
           * Weapon 1 - Spread Gun
           * Lives - Unlimited
           * Game Time - 5 Minutes
           * Delay Start - 15 seconds
           * Ammunitions - Unlimited Rounds
           * Teams - Need to assign
           * Respawn - Auto, Imediate
           * Gender - N/A
           * Friendly Fire - Off
           * Enable Music
           * Music Restart = 107000 milliseconds
           * 
           * Weapon 0 - Charge Rifle
           * Weapon 1 - Energy Launcher
           * Teams - No change
           * Respawn - Manual (station)
           * Gender - Male
          */
          MusicSelection = "$PLAY,CC08,4,6,,,,,*";
          BACKGROUNDMUSIC = true;
          MusicRestart = 107000;
          MusicPreviousMillis = 0;
          SetSlotA = 21; // set weapon slot 0 as contra rifle
          Serial.println("Weapon slot 0 set to Contra AR");
          SetSlotB = 22; // set weapon slot 1 as contra spray
          Serial.println("Weapon slot 1 set to Conrta Spray");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=300000;
          Serial.println("Game time set to 5 minute");
          DelayStart=11000; // set delay start to 15 seconds
          Serial.println("Delay Start Set to 15 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=1; // set respon mode to auto immediate
          Serial.println("Respawn Set to Auto (Immediate)"); 
          SetGNDR=2; // contra player audio
          Serial.println("Gender set to Contra");
          SetFF=2; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to Contra");
          AudioSelection="CC07";
          AUDIO=true;
          SetSlotD = 0;
        }
        if (b==13) { // sTARWARS mode
          GameMode=13; 
          /*
           * Weapon 0 - contra AR
           * Weapon 1 - Spread Gun
           * Lives - Unlimited
           * Game Time - 5 Minutes
           * Delay Start - 15 seconds
           * Ammunitions - Unlimited Rounds
           * Free For All - Need to assign
           * Respawn - Auto, Imediate
           * Gender - N/A
           * Friendly Fire - on
           * Enable Music
           * Music Restart = 107000 milliseconds
           *
          */
          MusicSelection = "$PLAY,SW04,4,6,,,,,*"; // JEDI for dark side: $PLAY,SW31,4,6,,,,,*
          BACKGROUNDMUSIC = true;
          Melee = 3;
          MusicRestart = 92000; // JEDI - for dark side: 60000
          MusicPreviousMillis = 0;
          SetSlotA = 23; // set weapon slot 0 as contra rifle
          Serial.println("Weapon slot 0 set to Contra AR");
          SetSlotB = 24; // set weapon slot 1 as contra spray
          Serial.println("Weapon slot 1 set to Conrta Spray");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=300000;
          Serial.println("Game time set to 5 minute");
          DelayStart=11000; // set delay start to 15 seconds
          Serial.println("Delay Start Set to 15 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=1; // set respon mode to auto immediate
          Serial.println("Respawn Set to Auto (Immediate)"); 
          SetGNDR=3; // STARWARS player audio
          Serial.println("Gender set to Contra");
          SetFF=1; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to STARWARS");
          AudioSelection="SW00";
          AUDIO=true;
          SetSlotD = 0;
        }
        if (b==14) { // Halo mode
          GameMode=14; 
          /*
           * Weapon 0 - contra AR
           * Weapon 1 - Spread Gun
           * Lives - Unlimited
           * Game Time - 5 Minutes
           * Delay Start - 15 seconds
           * Ammunitions - Unlimited Rounds
           * Free For All - Need to assign
           * Respawn - Auto, Imediate
           * Gender - N/A
           * Friendly Fire - on
           * Enable Music
           * Music Restart = 107000 milliseconds
           *
          */
          MusicSelection = "$PLAY,JAN,4,6,,,,,*"; // JEDI for dark side: $PLAY,SW31,4,6,,,,,*
          BACKGROUNDMUSIC = true;
          Melee = 3;
          MusicRestart = 92000; // JEDI - for dark side: 60000
          MusicPreviousMillis = 0;
          SetSlotA = 6; // set weapon slot 0 as Charge rifle
          Serial.println("Weapon slot 0 set to Charge rifle");
          SetSlotB = 7; // set weapon slot 1 as Energy Launcher
          Serial.println("Weapon slot 1 set to Energy Launcher");
          SetLives = 32000; // set lives to unlimited
          Serial.println("Lives is set to Unlimited");
          SetTime=300000;
          Serial.println("Game time set to 5 minute");
          DelayStart=11000; // set delay start to 15 seconds
          Serial.println("Delay Start Set to 15 seconds"); 
          UNLIMITEDAMMO=2; // set ammunitions to unlimited mags
          Serial.println("Ammo set to unlimited magazies"); 
          SetRSPNMode=1; // set respon mode to auto immediate
          Serial.println("Respawn Set to Auto (Immediate)"); 
          SetGNDR=0; //
          Serial.println("Gender set to Contra");
          SetFF=1; // free for all set to off
          Serial.println("Friendly Fire off");
          Serial.println("Game mode set to STARWARS");
          AudioSelection="JA5";
          AUDIO=true;
          SetSlotD = 0;
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
          AudioSelection="OP46";
        }
        if (b==2) {
          SetRSPNMode=2; 
          RespawnTimer=15000;
          Serial.println("Respawn Set to 15 seconds"); 
          AudioSelection="OP18";
        }
        if (b==3) {
          SetRSPNMode=3;
          RespawnTimer=30000; 
          Serial.println("Respawn Set to 30 seconds"); 
          AudioSelection="OP19";
        }
        if (b==4) {
          SetRSPNMode=4; 
          RespawnTimer=45000; 
          Serial.println("Respawn Set to 45 seconds"); 
          AudioSelection="OP20";
        }
        if (b==5) {
         SetRSPNMode=5; 
          RespawnTimer=60000;
          Serial.println("Respawn Set to 60 seconds");
          AudioSelection="OP23";
        }
        if (b==6) {
          SetRSPNMode=6;
          RespawnTimer=90000;
          Serial.println("Respawn Set to 90 seconds"); 
          AudioSelection="OP24";
        }
        if (b==7) {
          SetRSPNMode=7; 
          RespawnTimer=120000;
          Serial.println("Respawn Set to 120 seconds");
          AudioSelection="OP17";
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
          AudioSelection="OP53";
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
          DelayStart=7000;
          Serial.println("Delay Start Set to 15 seconds"); 
          AudioSelection="VA2Q";
        }
        if (b==3) {
          DelayStart=22000; 
          Serial.println("Delay Start Set to 30 seconds");
          AudioSelection="VA0R";
        }
        if (b==4) {
          DelayStart=37000; 
          Serial.println("Delay Start Set to 45 seconds");
          AudioSelection="VA0T";
        }
        if (b==5) {
          DelayStart=52000;
          Serial.println("Delay Start Set to 60 seconds"); 
          AudioSelection="VA0V";
        }
        if (b==6) {        
          DelayStart=82000;
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
        AudioSelection="OP15";
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
          AudioSelection="OP08";
        }
        if (b == 1) {
          SetGNDR=1;
          Serial.println("Gender set to Female");
          AudioSelection="OP03";
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
          AudioSelection="OP14";
        }
        if (b==2) {
          UNLIMITEDAMMO=2;
          Serial.println("Ammo set to unlimited magazies"); 
          AudioSelection="OP13";
        }
        if (b==1) {
          UNLIMITEDAMMO=1; 
          Serial.println("Ammo set to limited"); 
          AudioSelection="OP07";
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
         datapacket2 = 1400;
         datapacket1 = 99;
         BROADCASTESPNOW = true;
       }
     }
     if (b==1) {
       if (!INGAME){
         AudioSelection = "OP04";
         AUDIO = true;
         GAMESTART=true; 
         Serial.println("starting game");
         if (SetTeam == 100) {
           SetTeam=Team;
         }
       }
     }
     if (b > 9 && b < 20) { // this is a game winning end game... you might be a winner
       if (INGAME){
         GAMEOVER=true; 
         Serial.println("ending game");
         datapacket2 = 1410 + b;
         datapacket1 = 99;
         BROADCASTESPNOW = true;
         int winnercheck = b - 10;
         if (winnercheck == SetTeam) { // your team won
          delay(3000);
          AudioSelection = "VSF";
          AUDIO=true;
          StringSender = "$HLED,4,2,,,10,100,*";
          STRINGSENDER = true;
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
           AUDIO=true;
           datapacket2 = 1420 + b;
           datapacket1 = 99;
           BROADCASTESPNOW = true;
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
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           BROADCASTESPNOW = true;
         }
         if (leadercheck == 1 && ClosingVictory[1] == 0) {
           AudioSelection = "VB03";
           ClosingVictory[1] = 1;
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           BROADCASTESPNOW = true;
         }
         if (leadercheck == 2 && ClosingVictory[2] == 0) {
           AudioSelection = "VB1F";
           ClosingVictory[2] = 1;
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           BROADCASTESPNOW = true;
         }
         if (leadercheck == 3 && ClosingVictory[3] == 0) {
           AudioSelection = "VB0H";
           ClosingVictory[3] = 1;
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           BROADCASTESPNOW = true;
         }
       }
     }
   }
   if (incomingData2 < 1600 && incomingData2 > 1499) { // Misc. Debug
     int b = incomingData2 - 1500;
     if (b==0) {
      Serial.println("unlocking all hidden menu items");
      UNLOCKHIDDENMENU = true;
     }
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
       StringSender = "$HLOOP,2,1200,*";
       STRINGSENDER = true;
       AudioSelection="OP62";
       AUDIO=true;  
       if (ChangeMyColor > 8) {
         ChangeMyColor = 4; // triggers a gun/tagger color change
       } else { 
        ChangeMyColor++;
       }
       Serial.println("OTA Update Mode");
       EEPROM.write(4, 0); // reset OTA attempts counter
       EEPROM.write(1, 1); // set trigger to run OTA updates
       EEPROM.commit(); // Storar in Flash
       //INITIALIZEOTA = true;
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
        if (teamselector == SetTeam) {
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
    if (incomingData2 < 2000 && incomingData2 > 1899) { // Player ID Assigner
      int b = incomingData2 - 1900;
      if (b < 70) {
      GunID = b;
      EEPROM.write(2, b);
      EEPROM.commit();
      Serial.print("Gun ID changed to: ");
      Serial.println(GunID);
      ChangeMyColor = 9; // triggers a gun/tagger color change
      AudioSelection1="VAO"; // set an announcement "good job team"
      AUDIO1=true; // enabling BLE Audio Announcement Send
      }
      if (b > 70 && b < 74) {
      b = b - 70;
      GunGeneration = b;
      EEPROM.write(3, b);
      EEPROM.commit();
      Serial.print("Gun Gen changed to: ");
      Serial.println(GunGeneration);
      ChangeMyColor = 9; // triggers a gun/tagger color change
      AudioSelection1="VAO"; // set an announcement "good job team"
      AUDIO1=true; // enabling BLE Audio Announcement Send
      }
    }
    if (incomingData2 < 1800 && incomingData2 > 1699) { // Melee Use
      int b = incomingData2 - 1700;
      if (!INGAME) {
        Melee = b;
        if (b == 0) {
          AudioSelection1 = "OP54";
        }
        if (b == 1) {
          AudioSelection1 = "OP55";
        }
        if (b == 2) {
          AudioSelection1 = "OP31";
        }
        AUDIO1 = true;
      }
      
    }
    if (incomingData2 < 1900 && incomingData2 > 1799) { // Melee Use
      int b = incomingData2 - 1800;
      if (!INGAME) {
        SetSlotD = b;
        Serial.println("Perk Changed");
        if (b == 0) {
          AudioSelection1 = "VA4T";
        }
        if (b == 1) {
          AudioSelection1 = "OP26";
        }
        if (b == 2) {
          AudioSelection1 = "OP27";
        }
        if (b == 3) {
          AudioSelection1 = "OP25";
        }
        if (b == 4) {
          AudioSelection1 = "VA6G";
        }
        if (b == 5) {
          AudioSelection1 = "OP51";
        }
        if (b == 6) {
          AudioSelection1 = "OP50";
        }
        if (b == 7) {
          AudioSelection1 = "OP52";
        }
        if (b == 8) {
          AudioSelection1 = "OP57";
        }
        AUDIO1 = true;
      } 
    }
    
    // 10000s used for JBOX
    
    if (incomingData2 == 31100) { // Zone Base Captured
      if (incomingData3 == SetTeam) { // check to see if friendly zone
        CompletedObjectives++; // added one point to player objectives
        AudioSelection1="VAR"; // set an announcement "good job team"
        AUDIO1=true; // enabling BLE Audio Announcement Send
        Serial.println("Zone Base Captured");
      } else {
        // add in action that indicates no acce?
      }
      lastincomingData2 = 0;
    }
    if (incomingData2 == 31000) { 
      LOOT = true;    
      Serial.println("Loot box found");
      lastincomingData2 = 0;
    }
    if (31100 > incomingData2 > 31000) {
      PlayerStatBoost = incomingData2 - 31000;
      STATBOOST = true; 
      Serial.println("Stat boost found");
      // 31101 - Health boost
      // 31102 - armor boost
      // 31103 - shield boost
      // 31104 - ammo boost
      lastincomingData2 = 0;
    }
    if (31300 > incomingData2 > 31199) {
      // notification of a flag captured
      lastincomingData2 = 0;
      Serial.println("a flag was captured");
      int capture = incomingData2 - 31200;
      if (capture == SetTeam) {
        // We captured the flag!
        AudioSelection1="VA2K"; // setting audio play to notify we captured the flag
        AUDIO1 = true; // enabling play back of audio
      }
      else {
        // enemey has our flag
        AudioSelection1="VA2J"; // setting audio play to notify we captured the flag
        AUDIO1 = true; // enabling play back of audio
      }
    }
    if (31600 > incomingData2 > 31500) {
      lastincomingData2 = 0;
      AudioSelection = "VA20";
      AUDIO = true;
      SpecialWeapon = incomingData2 - 31500;
      Serial.println("weapon Picked up");
      if (SpecialWeapon == 99) {
        AudioSelection1="VA2K"; // setting audio play to notify we captured the flag
        AUDIO1 = true; // enabling play back of audio
        HASFLAG = true; // set condition that we have flag to true
        SpecialWeapon = 0; // loads a new weapon that will deposit flag into base
        Serial.println("Flag Capture, Gun becomes the flag"); 
        sendString("$WEAP,1,*");
        sendString("$WEAP,0,1,90,4,0,90,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*");
        sendString("$WEAP,4,*");
        sendString("$WEAP,3,*");
        sendString("$WEAP,5,*");
        datapacket2 = 31200 + SetTeam;
        datapacket1 = 99;
        BROADCASTESPNOW = true;
      }
      else {
        SPECIALWEAPONPICKUP = true;
      }
    }
    if (incomingData2 == 32000) { // if true, this is a kill confirmation
      // need to make sure it is not a duplicate being forwarded
      if (lastkilledplayer != incomingData3) {
        // this is not a duplicate kill confirmation
        lastincomingData2 = 0;
        lastkilledplayer = incomingData3;
        LASTKILLEDPLAYERRESETNEEDED = true;
        lastkilledplayertimestamp = millis();
        MyKills++; // accumulate one kill count
        AnounceScore = 0;
        if (!BACKGROUNDMUSIC) {        
          AudioSelection="VNA"; // set audio play for "kill confirmed
          AUDIO=true;
          AudioDelay = 2000;
          AudioPreviousMillis = millis();
          if (GameMode == 4) {
          // apply weapon pick up from kill  
          // SpecialWeapon = incomingData3;
          // Enable special weapon load by select button
          // SELECTCONFIRM = true; // enables trigger for select button
          // SPECIALWEAPONLOADOUT = true;
          // SelectButtonTimer = millis();
          // AudioSelection1="VA9B"; // set audio playback to "press select button"   
          }
          if (GameMode == 9) { // we are playing gun game
            SetSlotA++;
            Serial.println("Weapon Slot 0 set"); 
            if(SetSlotA == 2) {
              Serial.println("Weapon 0 set to AMR"); 
              StringSender = "$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,32768,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,9999999,75,,*";
            }
            if(SetSlotA == 3) {
              Serial.println("Weapon 0 set to Assault Rifle");
              StringSender = "$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,9999999,75,,*";
            }
            if(SetSlotA == 4) {
              Serial.println("Weapon 0 set to Bolt Rifle");
              StringSender = "$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,32768,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,9999999,75,,*";
            }
            if(SetSlotA == 5) {
              Serial.println("Weapon 0 set to BurstRifle"); 
              StringSender = "$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,32768,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,9999999,75,,*";
            }
            if(SetSlotA == 6) {
              Serial.println("Weapon 0 set to ChargeRifle"); 
              StringSender = "$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,9999999,75,,*";
            }
            if(SetSlotA == 7) {
              Serial.println("Weapon 0 set to Energy Launcher");
              StringSender = "$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,32768,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,9999999,75,,*";
            }
            if(SetSlotA == 8) {
              Serial.println("Weapon 0 set to Energy Rifle"); 
              StringSender = "$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,32768,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,9999999,75,,*";
            }
            if(SetSlotA == 9) {
              Serial.println("Weapon 0 set to Force Rifle"); 
              StringSender = "$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,32768,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,9999999,75,,*";
            }
            if(SetSlotA == 10) {
              Serial.println("Weapon 0 set to Ion Sniper"); 
              StringSender = "$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,32768,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,9999999,75,,*";
            }
            if(SetSlotA == 11) {
              Serial.println("Weapon 0 set to Laser Cannon"); 
              StringSender = "$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,32768,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,9999999,75,,*";
            }
            if(SetSlotA == 12) {
              Serial.println("Weapon 0 set to Plasma Sniper"); 
              StringSender = "$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,32768,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,9999999,75,40,*";
            }  
            if(SetSlotA == 13) {
              Serial.println("Weapon 0 set to Rail Gun"); 
              StringSender = "$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,32768,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,9999999,75,,*";
            }
            if(SetSlotA == 14) {
              Serial.println("Weapon 0 set to Rocket Launcher"); 
              StringSender = "$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,32768,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,9999999,75,30,*";
            }
            if(SetSlotA == 15) {
              Serial.println("Weapon 0 set to Shotgun"); 
              StringSender = "$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,32768,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,9999999,75,30,*";
            }
            if(SetSlotA == 16) {
              Serial.println("Weapon 0 set to SMG"); 
              StringSender = "$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,32768,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,9999999,75,,*";
            }
            if(SetSlotA == 17) {
              Serial.println("Weapon 0 set to Sniper Rifle");
              StringSender = "$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,32768,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,9999999,75,,*";
            }    
            if(SetSlotA == 18) {
              Serial.println("Weapon 0 set to Stinger");
              StringSender = "$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,32768,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,9999999,75,,*";
            }
            if(SetSlotA == 19) {
              Serial.println("Weapon 0 set to Suppressor"); 
              StringSender = "$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,32768,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,9999999,75,,*";
            }
            if(SetSlotA == 20) {
              Serial.println("Weapon 0 set to Pistol"); 
              StringSender = "$WEAP,0,,35,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,20,,*";
            }        
            if (SetSlotA < 10) {
              AudioSelection = ("GN0" + String(SetSlotA));
              AUDIO=true;
              STRINGSENDER = true;
            }
            if (SetSlotA > 9 && SetSlotA < 20) {
              AudioSelection = ("GN" + String(SetSlotA));
              AUDIO=true;
              STRINGSENDER = true;
            }
            if (SetSlotA == 20) {
              AudioSelection = ("VA50");
              AUDIO=true;
              STRINGSENDER = true;
            }
            if (SetSlotA > 20) { // player just won
              datapacket2 = 1400;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              incomingData1 = datapacket1;
              incomingData2 = datapacket2;
              ProcessIncomingCommands();      
              delay(3000);
              AudioSelection = "VSF";
              AUDIO=true;
              StringSender = "$HLED,4,2,,,10,100,*";
              STRINGSENDER = true;
            }
          }
        }
      }
    }
    if (incomingData1 == 99 && incomingData3 == 99) {
      datapacket2 = incomingData2;
      datapacket1 = incomingData1;
      BROADCASTESPNOW = true;
    }  
  }
  }else {
    if (incomingData2 == 32000) { // received a kill confirmation not intended for this tagger
      if (incomingData3 != lastkillconfirmationorigin) { // This is not the last NA killconfirmation received from the last player that sent one
        if (incomingData1 != lastkillconfirmationdestination) { // this is not the last NA killconfirmation destination
          // likely this is a new kill confirmation, ok to forward on to others
          datapacket1 = incomingData1;
          datapacket2 = incomingData2;
          datapacket3 = incomingData3;
          BROADCASTESPNOW = true;
          lastkillconfirmationorigin = incomingData3;
          lastkillconfirmationdestination = incomingData1;
          killconfirmationresettimestamp = millis();
          KILLCONFIRMATIONRESETNEEDED = true;
        }
      }
    }
  }
  digitalWrite(2, LOW);
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
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  Serial.println();
  Serial.println("JEDGE is taking over the BRX");
  sendString("$CLEAR,*"); // clears any brx settings
  sendString("$STOP,*"); // starts the callsign mode up
  sendString("$PLAY,OP47,4,6,,,,,*"); // says "connection established"
  sendString("Tagger now controlled by JEDGE, Awaiting JEDGE Controller Commands, JEDGE Rocks");
}
//**********************************************************************************************

//******************************************************************************************

// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
void sendString(String value) {
  const char * c_string = value.c_str();
  //Serial.print("sending: ");
  if (GunGeneration == 1) {
    for (int i = 0; i < value.length() / 1; i++) { // creates a for loop
      Serial1.println(c_string[i]); // sends one value at a time to gen1 brx
      delay(5); // this is an added delay needed for the way the brx gen1 receives data (brute force method because i could not match the comm settings exactly)
      Serial.print(c_string[i]);
    }
    Serial.println();
    delay(5);
  }
  if (GunGeneration > 1) {
    int matchingdelay = value.length() * 5; // this just creates a temp value for how long the string is multiplied by 5 milliseconds
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    Serial1.println(c_string); // sending the string to brx gen 2
    Serial.println(c_string);
  }
  delay(5);
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
  delay(200);
  WiFi.mode(WIFI_STA);
  delay(200);
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Waiting for WiFi");
  WiFi.begin(OTAssid.c_str(), OTApassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ENABLEOTAUPDATE = true;
  previousMillis = 0;
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
      sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,27,400,10,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,27,40,,*");
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
      sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,40,,*");
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
        if (GameMode == 4) { // battle royale
          sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,9999999,40,,*");
        } else {
          sendString("$WEAP,0,,60,0,0,8,0,,,,,,,,225,850,9,27,400,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,9,27,40,,*");
        }
      } 
  }
  if(SetSlotA == 21) {
    Serial.println("Weapon 0 set to Contra AR"); 
    sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
  }
  if(SetSlotA == 23) {
    Serial.println("Weapon 0 set to STANDARD BLASTER SW"); 
    sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,225,850,9,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,9,9999999,20,,*");
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

//******************************************************************************************

// sets and sends gun for slot 0 based upon stored settings
void weaponsettingsB() {
  if (SLOTB != 100) {
    SetSlotB=SLOTB; 
    SLOTB=100;
  }
  if (UNLIMITEDAMMO==3){// unlimited rounds
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
    if (SetSlotB == 21) {
      Serial.println("Weapon 1 set to Medkit"); 
      sendString("$WEAP,2,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,10,20,,*");
    }
  }  
  if (UNLIMITEDAMMO==2) {// unlimited Mags
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
      if (SetSlotB == 21) {
        Serial.println("Weapon 1 set to Medkit"); 
        sendString("$WEAP,2,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
      }
    }
    if (UNLIMITEDAMMO==1) {// limited rounds
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
      if (SetSlotB == 21) {
        Serial.println("Weapon 1 set to Medkit"); 
        sendString("$WEAP,2,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,10,20,,*");
      }
    }
    if(SetSlotB == 22) {
    Serial.println("Weapon 0 set to Contra AR"); 
    sendString("$WEAP,1,2,100,0,0,45,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
  }
  if(SetSlotB == 24) {
    Serial.println("Weapon 0 set to sw rIFLE"); 
    sendString("$WEAP,1,,100,8,0,100,0,,,,,,,,1250,850,100,32768,2500,0,14,100,100,,14,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
  }
}

void weaponsettingsC() {
  if(SetSlotC == 0){
    Serial.println("Weapon 5 set to Unarmed"); 
    sendString("$WEAP,5,*"); // Set weapon 3 as unarmed
  }
  if(SetSlotD == 0){
    Serial.println("Weapon 3 set to Unarmed"); 
    sendString("$WEAP,3,*"); // Set weapon 5 as unarmed
  }
  if(SetSlotD == 1) {
    Serial.println("Weapon 3 set to Medic");
    sendString("$WEAP,3,1,90,1,0,40,0,,,,,,,,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 2) {
    Serial.println("Weapon 3 set to Sheilds");
    sendString("$WEAP,3,1,90,2,1,70,0,,,,,,,,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 3) {
    Serial.println("Weapon 3 set to Armor");
    sendString("$WEAP,3,1,90,3,0,70,0,,,,,,,,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,10,9999999,20,,*");
  }
  if(SetSlotD == 4) {
    Serial.println("Weapon 3 set to Tear Gas");
    sendString("$WEAP,3,0,100,11,1,1,0,,,,,,1,80,1400,50,10,0,0,10,11,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,10,9999999,75,30,*");
  }
  if(SetSlotD == 5) {
    Serial.println("Weapon 3 set to Medic at Range");
    sendString("$WEAP,3,2,100,1,0,20,0,,,,,,20,80,1400,50,10,0,0,10,1,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 6) {
    Serial.println("Weapon 3 set to Armor at Range");
    sendString("$WEAP,3,2,100,2,1,30,0,,,,,,40,80,1400,50,10,0,0,10,2,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 7) {
    Serial.println("Weapon 3 set to Sheilds at range");
    sendString("$WEAP,3,2,100,3,0,30,0,,,,,,40,80,1400,50,10,0,0,10,3,100,100,,0,,,H29,,,,,,,,,,,,18,9999999,75,30,*");
  }
  if(SetSlotD == 8){
    Serial.println("Weapon 3 set to Respawn"); 
    sendString("$WEAP,3,1,90,15,0,6,100,,,,,,,,1000,100,1,0,0,10,15,100,100,,0,0,,,,,,,,,,,,,,1,0,20,,*"); // Set weapon 3 as respawn
  }
}
//****************************************************************************************
//************** This sends Settings to Tagger *******************************************
//****************************************************************************************
// loads all the game configuration settings into the gun
void gameconfigurator() {
  Serial.println("Running Game Configurator based upon recieved inputs");
  sendString("$CLEAR,*");
  sendString("$CLEAR,*");
  sendString("$START,*");
  sendString("$START,*");
  SetFFOutdoor();
  SetFFOutdoor();
  playersettings();
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
  sendString("$SIR,4,0,,1,0,0,1,,*"); // standard damgat but is actually the captured flag pole
  sendString("$SIR,4,0,,1,0,0,1,,*"); // standard damgat but is actually the captured flag pole
  sendString("$SIR,0,0,,1,0,0,1,,*"); // standard weapon, damages shields then armor then HP
  sendString("$SIR,0,0,,1,0,0,1,,*"); // standard weapon, damages shields then armor then HP
  sendString("$SIR,0,1,,36,0,0,1,,*"); // force rifle, sniper rifle ?pass through damage?
  sendString("$SIR,0,1,,36,0,0,1,,*"); // force rifle, sniper rifle ?pass through damage?
  sendString("$SIR,0,3,,37,0,0,1,,*"); // bolt rifle, burst rifle, AMR
  sendString("$SIR,0,3,,37,0,0,1,,*"); // bolt rifle, burst rifle, AMR
  sendString("$SIR,1,0,H29,10,0,0,1,,*"); // adds HP with this function********
  sendString("$SIR,1,0,H29,10,0,0,1,,*"); // adds HP with this function********
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*"); // adds shields*******
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*"); // adds shields*******
  sendString("$SIR,3,0,VA16,13,0,0,1,,*"); // adds armor*********
  sendString("$SIR,3,0,VA16,13,0,0,1,,*"); // adds armor*********
  sendString("$SIR,6,0,H02,1,0,90,1,40,*"); // rail gun
  sendString("$SIR,6,0,H02,1,0,90,1,40,*"); // rail gun
  sendString("$SIR,8,0,,38,0,0,1,,*"); // charge rifle
  sendString("$SIR,8,0,,38,0,0,1,,*"); // charge rifle
  sendString("$SIR,9,3,,24,10,0,,,*"); // energy launcher
  sendString("$SIR,9,3,,24,10,0,,,*"); // energy launcher
  sendString("$SIR,10,0,X13,1,0,100,2,60,*"); // rocket launcher
  sendString("$SIR,10,0,X13,1,0,100,2,60,*"); // rocket launcher
  sendString("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
  sendString("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
  sendString("$SIR,13,1,H57,1,0,0,1,,*"); // energy blade
  sendString("$SIR,13,1,H57,1,0,0,1,,*"); // energy blade
  sendString("$SIR,13,0,H50,1,0,0,1,,*"); // rifle bash
  sendString("$SIR,13,0,H50,1,0,0,1,,*"); // rifle bash
  sendString("$SIR,13,3,H49,1,0,100,0,60,*"); // war hammer
  sendString("$SIR,13,3,H49,1,0,100,0,60,*"); // war hammer
  sendString("$SIR,1,1,,8,0,100,0,60,*"); // no damage just vibe on impact - use for SWAPBRX, and other game features based upon player ID 0-63
  // protocol for SWAPBRX: bullet type: 1, power: 1, damage: 1, player: 0, team: 2, critical: 0; just look for bullet, power, player for trigger weapon swap
  //sendString("$SIR,15,1,,50,0,0,1,,*");
  //sendString("$SIR,14,0,,14,0,0,1,,*");
  // The $SIR functions above can be changed to incorporate more in game IR based functions (health boosts, armor, shields) or customized over BLE to support game functions/modes
  //delay(500);
  sendString("$SIR,1,1,,8,0,100,0,60,*"); // no damage just vibe on impact - use for SWAPBRX, and other game features based upon player ID 0-63
  sendString("$SIR,1,1,,8,0,100,0,60,*"); // no damage just vibe on impact - use for SWAPBRX, and other game features based upon player ID 0-63
  sendString("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
  sendString("$BMAP,0,0,,,,,*"); // sets the trigger on tagger to weapon 0
  sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
  sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
  sendString("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
  sendString("$BMAP,2,97,,,,,*"); // sets the reload handle as the reload button
  sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5****
  sendString("$BMAP,3,5,,,,,*"); // sets the select button as Weapon 5****
  sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4****
  sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4****
  sendString("$BMAP,5,3,,,,,*"); // Sets the right button as weapon 3, using for perks/respawns etc. 
  sendString("$BMAP,5,3,,,,,*"); // Sets the right button as weapon 3, using for perks/respawns etc. 
  sendString("$BMAP,8,4,,,,,*"); // sets the gyro as weapon 4
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
  sendString("$HLED,,6,,,,,*"); // changes headset to end of game
  // this portion creates a hang up in the program to delay until the time is up
  bool RunInitialDelay = true;
  long actualdelay = 0; // used to count the actual delay versus desired delay
  long delaybeginning = millis(); // sets variable as the current time to track when the actual delay started
  long initialdelay = DelayStart; // this will be used to track current time in milliseconds and compared to the start of the delay
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
  sendString("$SPAWN,,*");
  weaponsettingsA();
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsB();
  weaponsettingsC();
  weaponsettingsC();
  if (Melee == 1) {
    sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
    sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  }
  if (Melee == 2) {
    sendString("$WEAP,4,1,90,13,2,1,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*"); // this is default melee weapon for rifle bash
    sendString("$WEAP,4,1,90,13,2,1,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*"); // this is default melee weapon for rifle bash
  }
  if (Melee == 3) {
    sendString("$WEAP,4,1,90,13,0,115,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
    sendString("$WEAP,4,1,90,13,0,115,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
  }
  
  Serial.println("Delayed Start Complete, should be in game play mode now");
  GameStartTime=millis();
  GAMEOVER=false;
  INGAME=true;
  if (!BACKGROUNDMUSIC) {
    sendString("$PLAY,VA1A,4,6,,,,,*"); // plays the .. let the battle begin
    MusicPreviousMillis = 0;
  }
  //reset sent scores:
  Serial.println("resetting all scores");
  CompletedObjectives = 0;
  int teamcounter = 0;
  while (teamcounter < 4) {
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
  // Standard health = 45; armor = 70; shield =70;
  // Weak Health = 8; armor = 0; shield = 0;
  // jugernaught health = 300; armor = 300; shield = 300;
  String Health = "null";
  if (SetHealth == 0) {
    Health = "45,70,70";
  }
  if (SetHealth == 1) {
    Health = "8,0,0";
  }
  if (SetHealth == 2) {
    Health = "300,300,300";
  }
  if (SetGNDR == 0) {
      sendString("$PSET,"+String(GunID)+","+String(SetTeam)+","+String(Health)+",50,,H44,JAD,V33,V3I,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
  } 
  if (SetGNDR == 1){
      sendString("$PSET,"+String(GunID)+","+String(SetTeam)+","+String(Health)+",50,,H44,JAD,VB3,VBI,VBC,VBG,VBE,VB7,H06,H55,H13,H21,H02,U15,W71,A10,*");
  }
  if (SetGNDR == 2) { // CONTRA
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+","+String(Health)+",50,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
  
  }
  if (SetGNDR == 3) { // STARWARS
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+","+String(Health)+",50,,H44,JAD,SW05,SW01,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
  }
  if (GameMode == 7 && Team == 3) { // infected 
    if (Team == 3) {
      sendString("$PSET,"+String(GunID)+",3,67,105,105,50,,A100,A20,A100,V7J,V16,V96,V52,V66,H06,V76,V16,V56,H06,U15,W71,A10,*");
      SetSlotB = 1;
      SetSlotA = 8;
    } else {
      SetSlotB = 21;
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
  if (!BACKGROUNDMUSIC) {
    sendString("$PLAY,VS6,4,6,,,,,*"); // says game over
    AudioDelay = 3500;
    AudioSelection="VAP"; // "terminated"
    AudioPreviousMillis = millis();
    AUDIO = true;
  }
  if (GameMode == 12) {
    sendString("$PLAY,CC09,4,6,,,,,*"); // says game over
  }
  if (GameMode == 13) {
    sendString("$PLAY,SW03,4,6,,,,,*");
  }
  Serial.println("Game Over Object Complete");
  CurrentDominationLeader = 99; // resetting the domination game status
  ClosingVictory[0] = 0;
  ClosingVictory[1] = 0;
  ClosingVictory[2] = 0;
  ClosingVictory[3] = 0;
}
//******************************************************************************************
// as the name says... respawn a player!
void respawnplayer() {
  //sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  //sendString("$HLOOP,2,1200,*"); // flashes the headset lights in a loop
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
  weaponsettingsA();
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsB();
  weaponsettingsC();
  weaponsettingsC();
  if (Melee == 1) {
    sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
    sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash  
  }
  if (Melee == 2) {
    sendString("$WEAP,4,1,90,13,2,0,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*"); // this is default melee weapon for rifle bash
    sendString("$WEAP,4,1,90,13,2,0,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*"); // this is default melee weapon for rifle bash
  }
  sendString("$GLED,,,,5,,,*"); // changes headset to tagged out color
  sendString("$GLED,,,,5,,,*"); // changes headset to tagged out color
  sendString("$SPAWN,,*"); // respawns player back in game
  sendString("$SPAWN,,*"); // respawns player back in game
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  Serial.println("Player Respawned");
  RESPAWN = false;
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  if (STEALTH){
      Serial.println("delaying stealth start");
      delay(2000);
      sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
      delay(5);
      sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
      Serial.println("sent command to kill leds");
  }
  if (BACKGROUNDMUSIC) {
    sendString(MusicSelection);
    MusicPreviousMillis = 0;
  }
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  sendString("$HLOOP,0,0,*"); // stops headset flashing
  LastRespawnTime = millis();
  RespawnStatus = 1;
}
//****************************************************************************************
// this function will be used when a player is eliminated and needs to respawn off of a base
// or player signal to respawn them... a lot to think about still on this and im using auto respawn 
// for now untill this is further thought out and developed
void ManualRespawnMode() {
  delay(4000); // delay added to allow sound to finish and lights process as well
  sendString("$VOL,0,0,*"); // adjust volume to default
  sendString("$VOL,0,0,*"); // adjust volume to default
  sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,,,,,,,,,,,,,,,,,*");
  sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,,,,,,,,,,,,,,,,,*");
  sendString("$STOP,*"); // this is essentially ending the game for player... need to rerun configurator or use a different command
  sendString("$STOP,*"); // this is essentially ending the game for player... need to rerun configurator or use a different command
  sendString("$SPAWN,,*"); // this is playing "get some as part of a respawn
  sendString("$SPAWN,,*"); // this is playing "get some as part of a respawn
  //sendString("$HLOOP,2,1200,*"); // this puts the headset in a loop for flashing
  //sendString("$HLOOP,2,1200,*"); // this puts the headset in a loop for flashing
  sendString("$GLED,,,,,,,*"); // changes headset to tagged out color
  sendString("$GLED,,,,,,,*"); // changes headset to tagged out color
  //AudioSelection1 = "VA54"; // audio that says respawn time
  //AUDIO1 = true;
  PENDINGRESPAWNIR = true;
  MANUALRESPAWN = false;
  RespawnStartTimer = millis();
  if (BACKGROUNDMUSIC) {
    sendString(MusicSelection);
  }
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
        if (AnounceScore == 2) {
          AnounceScore = 3;
          if (Deaths < 10) {
            AudioSelection = "VA0" + String(Deaths);
          }
          if (Deaths == 10) {AudioSelection = "VA0A";}
          if (Deaths == 11) {AudioSelection = "VA0B";}
          if (Deaths == 12) {AudioSelection = "VA0C";}
          if (Deaths == 13) {AudioSelection = "VA0D";}
          if (Deaths == 14) {AudioSelection = "VA0E";}
          if (Deaths == 15) {AudioSelection = "VA0F";}
          if (Deaths == 16) {AudioSelection = "VA0G";}
          if (Deaths == 17) {AudioSelection = "VA0H";}
          if (Deaths == 18) {AudioSelection = "VA0I";}
          if (Deaths == 19) {AudioSelection = "VA0J";}
          if (Deaths == 20) {AudioSelection = "VA0K";}
          if (Deaths == 21) {AudioSelection = "VA0L";}
          if (Deaths == 22) {AudioSelection = "VA0M";}
          if (Deaths == 23) {AudioSelection = "VA0N";}
          if (Deaths == 24) {AudioSelection = "VA0O";}
          if (Deaths == 25) {AudioSelection = "VA0P";}
          AudioDelay = 1500;
          AudioPreviousMillis = millis();
          AUDIO = true;
        }
        if (AnounceScore == 3) {
          AnounceScore = 0;
          AudioDelay = 1500;
          AudioPreviousMillis = millis();
          AudioSelection="VA7F"; // "Fatality"
          AUDIO = true;
        }
      }
    } else {
      sendString("$PLAY,"+AudioSelection+",4,6,,,,,*");
      AUDIO = false;
      AudioSelection  = "NULL";
    }
    if (AnounceScore == 1) {
      AnounceScore = 2;
      AudioPreviousMillis = millis();
      AudioDelay = 1500;
      AudioSelection="VNA"; // "Kill Confirmed"
      AUDIO = true;
    }
    int resetcheck = EEPROM.read(1);
    if (resetcheck == 1) {
      delay(1500);
      ESP.restart();
    } 
  }
  if (AUDIO1) {
    sendString("$PLAY,"+AudioSelection1+",4,6,,,,,*");
    AUDIO1=false; 
    AudioSelection1 = "NULL";
  }
}
void ChangeColors() {
  String colorstring = ("$GLED," + String(ChangeMyColor) + "," + String(ChangeMyColor) + "," + String(ChangeMyColor) + ",0,10,,*");
  sendString(colorstring);
  colorstring = ("$HLED," + String(ChangeMyColor) + ",0,,,10,,*");
  sendString(colorstring);
  //ChangeMyColor = 99;
  CurrentColor = ChangeMyColor;
}
//********************************************************************************************
// This main game object
void MainGame() {
  if (INITJEDGE) {
    INITJEDGE = false;
    InitializeJEDGE();
  }
  if (UNLOCKHIDDENMENU) {
    UNLOCKHIDDENMENU = false;
    int dlccounter = 0;
    while (dlccounter < 12) {
      sendString("$DLC,"+String(dlccounter)+",1,*");
      dlccounter++;
      delay(150);
    }
    sendString("$PLAY,OP61,4,6,,,,,*");
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
  if (STRINGSENDER) {
    STRINGSENDER = false;
    sendString(StringSender); //
    StringSender = "Null";
  }
  if (GAMEOVER) { // checks if something triggered game over (out of lives, objective met)
    gameover(); // runs object to kick player out of game
    sendString("$HLOOP,0,0,*"); // stops headset flashing
  }
  if (GAMESTART) {
    // need to turn off the trigger audible selections if a player didnt press alt fire to confirm
    GETSLOT0=false; 
    GETSLOT1=false; 
    GETTEAM=false;
    if (GameMode == 13) {
      if (SetTeam == 0 || SetTeam == 2) {
        MusicSelection = "$PLAY,SW31,4,6,,,,,*";
        MusicRestart = 60000;
      } else {
        MusicSelection = "$PLAY,SW04,4,6,,,,,*";
        MusicRestart = 92000;
      }
    }
    if (GameMode == 14) {
      if (SetTeam == 0 || SetTeam == 2) {
        MusicSelection = "$PLAY,JAN,4,6,,,,,*";
        MusicRestart = 59000;
      }
      if (SetTeam == 1) {
        MusicSelection = "$PLAY,JAO,4,6,,,,,*";
        MusicRestart = 59000;
      }
      if (SetTeam == 3) {
        MusicSelection = "$PLAY,JAP,4,6,,,,,*";
        MusicRestart = 59000;
      }
    }
    gameconfigurator(); // send all game settings, weapons etc. 
    delaystart(); // enable the start based upon delay selected
    GAMESTART = false; // disables the trigger so this doesnt loop/
    PlayerLives=SetLives; // sets configured lives from settings received
    GameTimer=SetTime; // sets timer from input of esp8266
    MyKills = 0;
    ClearScores();
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
      Serial.println("delaying stealth start");
      delay(2000);
      sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
      Serial.println("sent command to kill leds");
    }
  }
  if (!INGAME) {
    if (PENDINGRESPAWNIR) {
      if (GameMode == 4) {
        int RespawnTimeCheck = millis() - RespawnStartTimer;
        if (RespawnTimeCheck > DeadButNotOutTimer) {
          GAMEOVER = true;
          PENDINGRESPAWNIR = false;
        }
      }
    }
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
    if (RespawnStatus > 0) {
      long currentesptime = millis();
      if (currentesptime - LastRespawnTime > 1500) {
        sendString("$HLOOP,0,0,*"); // stops headset flashing
        sendString("$HLOOP,0,0,*"); // stops headset flashing
        if (RespawnStatus == 1) {
          RespawnStatus = 2;
        } else {
          RespawnStatus = 0;
        }
      }
    }
    long ActualGameTime = millis() - GameStartTime;
    long GameTimeRemaining = GameTimer - ActualGameTime;
    if (KILLCONFIRMATIONRESETNEEDED) { // recently received and forwarded on a kill confirmation to another player
      if (ActualGameTime - killconfirmationresettimestamp > 4000) { // checking if 4 seconds have past
        // resetting kill confirmation forwarding variables
        KILLCONFIRMATIONRESETNEEDED = false;
        lastkillconfirmationorigin = 9999;
        lastkillconfirmationdestination = 9999;
      }
    }
    if (LASTKILLEDPLAYERRESETNEEDED) { // recently received a kill confirmation
      if (ActualGameTime - lastkilledplayertimestamp > 1000) {
        LASTKILLEDPLAYERRESETNEEDED = false;
        lastkilledplayer = 9999;
      }
    }
    if (BACKGROUNDMUSIC) {
      unsigned long CurrentMusicMillis = millis();
      if (CurrentMusicMillis - MusicPreviousMillis > MusicRestart) {
        MusicPreviousMillis = CurrentMusicMillis;
        if (GameMode == 13) {
          int randomcallout = random(11);
          randomcallout = randomcallout + 40;
          sendString("$PLAY,SW" + String(randomcallout) + ",4,6,,,,,*");
          AudioDelay = 10000;
          AudioPreviousMillis = millis();
        }
        if (GameMode == 14) {
          int randomcallout = random(7);
          sendString("$PLAY,K0" + String(randomcallout) + ",4,6,,,,,*");
          AudioDelay = 10000;
          AudioPreviousMillis = millis();
        }
        AudioSelection = MusicSelection;
        AUDIO = true;
      }    
    }
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
        delay(2000);
        if (BACKGROUNDMUSIC) {
          MusicPreviousMillis = 0;
        }
      } // two minute warning
    }
    if (COUNTDOWN2) {
      if (GameTimeRemaining < 60001) {
        AudioSelection1="VSA"; 
        AUDIO1=true;
        COUNTDOWN2=false; 
        COUNTDOWN3=true; 
        Serial.println("1 minute remaining");
        delay(2000);
        if (BACKGROUNDMUSIC) {
          MusicPreviousMillis = 0;
        }
      } // one minute warning
    }
    if (COUNTDOWN3) {
      if (GameTimeRemaining < 10001) {
        AudioSelection1="VA83"; 
        AUDIO1=true; 
        COUNTDOWN3=false;
        Serial.println("ten seconds remaining");
        //if (BACKGROUNDMUSIC) {
          //MusicPreviousMillis = 0;
        //}
      } // ten second count down
    }
    if (ActualGameTime > GameTimer) {
      GAMEOVER=true; 
      Serial.println("game time expired");
    }
    if (RESPAWN) { // checks if auto respawn was triggered to respawn a player
      if (GameMode == 8) {
        gameconfigurator(); // send all game settings, weapons etc. 
        delaystart(); // enable the start based upon delay selected
        if (STEALTH){
          Serial.println("delaying stealth start");
          delay(2000);
          sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
          Serial.println("sent command to kill leds");
        }
      } else {
        respawnplayer(); // respawns player
      }
    }
    if (MANUALRESPAWN) { // checks if manual respawn was triggered to respawn a player
      ManualRespawnMode();
    }
    if (LOOT) {
      LOOT = false;
      swapbrx();
    }
    if (SPECIALWEAPONPICKUP) {
      SPECIALWEAPONPICKUP = false;
      LoadSpecialWeapon();
    }
    if (STATBOOST) {
      STATBOOST = false;
      // 31101 - Health boost
      // 31102 - armor boost
      // 31103 - shield boost
      // 31104 - ammo boost
      if (PlayerStatBoost = 1) {
        
      }
      if (PlayerStatBoost = 2) {
        
      }
      if (PlayerStatBoost = 3) {
        
      }
      if (PlayerStatBoost = 4) {
        
      }
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
  int randomnumber = random(100); // randomizing between all weapons with total weapons count
  // randomizing: 100% AR:10 BR:25 RL:10 SG:20 SMG:10 SNIPE:10 BURST:15
  if (randomnumber < 25) { // bolt rifle 25% chance 0-24
    SpecialWeapon = 4;
  }
  if (randomnumber > 24 && randomnumber < 45) { // Shotgun 20% chance 25-45
    SpecialWeapon = 15;
  }
  if (randomnumber > 44 && randomnumber < 60) { // Burst rifle 15% chance
    SpecialWeapon = 5;
  }
  if (randomnumber > 59 && randomnumber < 70) { // Assault Rifle 10%
    SpecialWeapon = 3;
  }
  if (randomnumber > 69 && randomnumber < 80) { // Rocket Launcher 10%
    SpecialWeapon = 14;
  }
  if (randomnumber > 79 && randomnumber < 90) { // Sniper Rifle
    SpecialWeapon = 17;
  }
  if (randomnumber >89 && randomnumber < 100) { // SMG
    SpecialWeapon = 16;
  }
  
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
    //TerminalInput = "Modified Gen3 Data to Match Gen 1/2";
    
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

          if (PlayerLives == 0 && INGAME == true) {
            sendString("$PLAY,VA46,4,6,,,,,*"); // says lives depleted
          }


          
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
          } else {
            // lets do a score announcement
            if (!BACKGROUNDMUSIC) {
              AudioSelection1 = "KL" + String(MyKills);
              AUDIO1 = true;
              AudioDelay = 2000;
              AudioPreviousMillis = millis();
              AudioSelection = "DT" + String(Deaths);
              AUDIO = true;            
            }
          }
        }
      }
      if (tokenStrings[1] == "4") {
        if (tokenStrings[2] == "1") {
          Serial.println("left dpad pressed"); // as indicated this is the left button
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
      if (tokenStrings[2] == "13") { // melee attack received
        if (tokenStrings[7] == "2") { // arm/disarm tag recieved
          String TeamChecker = String(SetTeam);
          if (tokenStrings[4] == TeamChecker) {
            if (DISARMED) {
              DISARMED = false;
              weaponsettingsA();
              weaponsettingsB();
              weaponsettingsC();
              sendString("$WEAP,4,1,90,13,2,0,0,,,,,,,,1000,100,1,32768,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,9999999,20,,*"); // this is default melee weapon for rifle bash
              AudioSelection1="VA9T";
              AUDIO1=true;
            }
          } 
          if (tokenStrings[4] != TeamChecker) {
              DISARMED = true;
              sendString("$WEAP,0,*"); // cleared out weapon 0
              sendString("$WEAP,1,*"); // cleared out weapon 1
              sendString("$WEAP,4,*"); // cleared out melee weapon
              AudioSelection1="GN01";
              AUDIO1=true;
            }
          }
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
        if (GameMode == 8) {
          sendString("$STOP,*");
          sendString("$CLEAR,*");
          RESPAWN = true;
          SetTeam = lastTaggedTeam;
          // playersettings();
          if (SetTeam == 0) {sendString("$PLAY,VA13,4,6,,,,,*");}
          if (SetTeam == 1) {sendString("$PLAY,VA1L,4,6,,,,,*");}
          if (SetTeam == 2) {sendString("$PLAY,VA1R,4,6,,,,,*");}
          if (SetTeam == 3) {sendString("$PLAY,VA27,4,6,,,,,*");}
        }
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
        // call audio to announce player's killer
        if (!BACKGROUNDMUSIC) {
          AnounceScore = 0;
          AudioDelay = 5000;
          AudioSelection="KB"+String(lastTaggedPlayer);
          AudioPreviousMillis = millis();
          AUDIO=true;
        }
        Serial.println("Lives Remaining = " + String(PlayerLives));
        Serial.println("Killed by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
        Serial.println("Team: " + String(lastTaggedTeam) + "Score: " + String(TeamKillCount[lastTaggedTeam]));
        Serial.println("Player: " + String(lastTaggedPlayer) + " Score: " + String(PlayerKillCount[lastTaggedPlayer]));
        Serial.println("Death Count: " + String(Deaths));
        if (GameMode == 7) { // infected game mode
          RESPAWN = true;
          SetTeam = 3; // make sure you are now team green
          SetSlotB = 1; // make sure you dont have a med kit
          // SetSlotA = ?;
          playersettings(); // setting your new team
        }
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
          delay(3000);
          //sendString("$HLOOP,2,1200,*"); // this puts the headset in a loop for flashing
          sendString("$PLAY,VA46,4,6,,,,,*"); // says lives depleted
          Serial.println("lives depleted");
          // KILL GUN LIGHTS
          sendString("$GLED,9,9,9,0,10,,*");
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
void AccumulateIncomingScores1() {
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

void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(OTAssid.c_str(), OTApassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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
      EEPROM.write(0, 1); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      Serial.println(payload);
      Serial.println("New firmware detected");
      
      return 1;
    }
  } 
  return 0;  
}
//******************************************************************************************
// ***********************************  DIRTY LOOP  ****************************************
// *****************************************************************************************
void loop1(void *pvParameters) {
  Serial.print("Dirty loop running on core ");
  Serial.println(xPortGetCoreID());   
  //auto control brx start up:
  //InitializeJEDGE();
  if (updatenotification == 1) { // recently updated
    EEPROM.write(0, 0);
    EEPROM.commit();
    AudioSelection="VA6W";
    AUDIO=true;
  }
  if (updatenotification == 2) { // firmware is up to date already
    EEPROM.write(0, 0);
    EEPROM.commit();
    AudioSelection="OPO32";
    AUDIO=true;
  }
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
  Menu[17] = 1700;
  if (!ACTASHOST) {
    RUNWEBSERVER = false;
    WiFi.softAPdisconnect (true);
    Serial.println("Device is not intended to act as WebServer / Host. Shutting down AP and WebServer");
  }
  delay(250);
  ChangeMyColor = 0;
  delay(250);
  ChangeMyColor = 1;
  delay(250);
  ChangeMyColor = 2;
  delay(250);
  ChangeMyColor = 3;
  delay(250);
  ChangeMyColor = 4;
  delay(250);
  ChangeMyColor = 5;
  delay(250);
  ChangeMyColor = 6;
  delay(250);
  ChangeMyColor = 7;
  delay(250);
  ChangeMyColor = 8;
  while (1) { // starts the forever loop
    digitalWrite(ledPin, ledState);
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
          AccumulateIncomingScores1();
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
  //***********************************************************************
  Serial.begin(115200); // set serial monitor to match this baud rate
  Serial.println("Initializing serial output settings, Tagger Generation set to Gen: " + String(GunGeneration));

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  delay(250);
  digitalWrite(ledPin, HIGH);
  delay(250);
  digitalWrite(ledPin, LOW);
  //***********************************************************************
  // initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  int bootstatus = EEPROM.read(1);
  updatenotification = EEPROM.read(0);
  GunID = EEPROM.read(2);
  GunGeneration = EEPROM.read(3);
  Serial.print("GunID = ");
  Serial.println(GunID);
  Serial.print("GunGeneration = ");
  Serial.println(GunGeneration);
  Serial.print("Boot Status = ");
  Serial.println(bootstatus);if (GunGeneration > 1) {
    BaudRate = 115200;
  }
  Serial.println("Serial Buad Rate set for: " + String(BaudRate));
  Serial1.begin(BaudRate, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the serial pins for sending data to BRX
  delay(10);
  if (bootstatus == 1){ // indicates we are in ota mode
    OTAMODE = true;
  }
  //*********** OTA MODE ************************
  if (OTAMODE) {  
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  pinMode(LED_BUILTIN, OUTPUT);

  // Loop check to make sure tagger doesnt try to update endlessly
  int OTAattempts = EEPROM.read(4);
  if (OTAattempts > 3) { // too many attempts, lets time it out
    EEPROM.write(1, 0); // in case not already the case, clearing out eeprom state so it doesnt try to upgrade anymore
    EEPROM.write(0, 2); // set up device to provide notification that there is no update available
    EEPROM.commit(); // saves the eeprom state
    ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
  } else { // add one counter
    OTAattempts++;
    EEPROM.write(4, OTAattempts);
    EEPROM.commit();
  }
  
  bootstatus = EEPROM.read(0);
  Serial.print("Boot Status = ");
  Serial.println(bootstatus);

  InitAP();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  delay(1000);

  datapacket1 = 9999;
  getReadings();
  BroadcastData(); // sending data via ESPNOW
  Serial.println("Sent Data Via ESPNOW");
  ResetReadings();

  while (OTApassword == "dontchangeme") {
    vTaskDelay(1);
  }
  delay(2000);
  Serial.println("wifi credentials");
  Serial.println(OTAssid);
  Serial.println(OTApassword);
  
  connect_wifi();

  delay(1500);
  sendString("$PLAY,OP28,4,6,,,,,*"); // says connection accepted
  delay(1500);
  
  EEPROM.write(1, 0);
  EEPROM.commit();
  } else {
  //************** Play Mode*******************
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.print("setup() running on core ");
  
  if (RUNWEBSERVER) {
    InitAP();
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
  }
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
  }
} // End of setup.
// **********************
// ****  EMPTY LOOP  ****
// **********************
void loop() {
  if (OTAMODE) {
    static int num=0;
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= otainterval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      if (FirmwareVersionCheck()) {
        sendString("$PLAY,OP43,4,6,,,,,*"); // starwars trumpets
        firmwareUpdate();
      }
    }
    if (UPTODATE) {
      EEPROM.write(1, 0); // in case not already the case, clearing out eeprom state so it doesnt try to upgrade anymore
      EEPROM.write(0, 2); // set up device to provide notification that there is no update available
      EEPROM.commit(); // saves the eeprom state
      ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
    }
  }
}

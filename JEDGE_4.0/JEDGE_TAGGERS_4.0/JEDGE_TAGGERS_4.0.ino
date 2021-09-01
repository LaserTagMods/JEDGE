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
 * updated 07/31/2021 fixed manual respawn from IR. Jecked it on JBOX to confirm functionality. All appears to now be in
 *                    working order. 
 *                    changed the tagger weapons for starwars mode to have 200 or so rounds and rapid fire rifle, low damage and higher damage regular blaster
 *                    changed SWAPBRX weapon pick ups and perks etc from a loot. This way it has better flow/function, also updated weapons for upgrades
 *                    and worked on a better pistol upgrade process, changing the pistol sound each time as well as its strength. (needs testing)
 *                    Hopefully assimilation is now working as well. 
 * updated 08/04/2021 Even better improvements for assimilation mode. making it work even smoother.
 *                    Added in off line game mode options/settings
 * updated 08/1202021 Incorporated all pbgame settings for primarily utilizing offline game modes, trouble shooting issues as well                   
 * UPDATED 08/16/2021 FIXED LOTS OF BUGS
 * updated 08/21/2021 removed all not necessary jedge 3.0 stuff
 * updated 08/22/2021 added in eeprom for game settings saves, score saves etc, restart tagger upon reboot of esp, if the gun doesnt respond to query command - used saved game settings
 *                    removed all non-essential broadcasts of espnow data packets - only one remaining is for score sync
 *                    changed game start to only work if all taggers are selected. this prevents operator error for leaving only one player selected
 * updated 08/24/2021 removed all vtask delays to ensure that watchdog is not avoided. we want to be able to allow for the device to reset when having issues
 *                    removed undesired web server code segments and start processes to ensure no unneeded clutter
 *                    added in verification if game time for pbgames indicates time up. if time is up, reset the esp32, also if deaths are equal to initial planned lives game is over and reset as well
 * updated 08/25/2021 fixed audio timing issue for music playing game modes, modified starwars and contra pset health/armor as well as the weapons to be similar to offline game modes.               
 *                    Added in the receiveing and processing of all game settings when start game comes in
 * updated 08/30/2021 fixed two way communication, had to bring back in AP settings post deletion. 
 * 
 */

//*************************************************************************
//********************* LIBRARY INCLUSIONS - ALL **************************
//*************************************************************************
#include <HardwareSerial.h> // for assigining misc pins for tx/rx
#include <WiFi.h> // used for all kinds of wifi stuff
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
//****************************************************************

int sample[5];

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 100 // use 0 for OTA and 1 for Tagger ID
// if eeprom 0 is 1, it is OTA mode, if 0, regular mode.
// eeprom 1-4 is used for update process and resetting as well as attempts counters
// eeprom 5 - player deaths
// eeprom 6 - objective points
// eeprom 7 - game setting - tagger volume - 1-5
// eeprom 8 - game setting - indoor/outdoor - 1-outdoor, 2 indoor, 3 outdoor stealth, 4, indoor stealth
// eeprom 9 - game setting - pbgame mode - 1 free for all, 2 death match, 3 generals, 4, supremacy, 5 commanders, 6 infected, 7 the swarm
// eeprom 10 - game setting - game customizations - 1 none, 2 battle royale, 3 team assimilation, 4 contra, 5 starwars, 6 halo
// eeprom 11 - game setting - pb lives - 1 1, 2 3, 3 5, 4 10, 5 15, 5 infinite
// eeprom 12 - game setting - pb game time - 1 5 minutes, 2 10min, 3 15 min, 4 20 min, 5 30 min, 6 unlimited
// eeprom 13 - game settings - pb respawn time - 1 off, 2 15 sec, 3 30sec, 4 60sec, 5 ramp 45, 6 ramp 90
// eeprom 14 - game settings - team - 0, 1, 2, 3 - run team alignment object
// eeprom 15 - game settings - weapon/class - 0, 1, 2, 3, 4, 5, 6
// eeprom 16 - game settings - pb perks - 0, 1, 2, 3, 4, 5
// eeprom 17 - game status - is in game or not - 0 is not in game, 1 is in game
// eeprom 18 - Software Reset initiated - 0: not intended, 1: intended
// eeprom 19 - Player Lives
// eeprom 20 through 83 - player kill counts for player 0-63 // this is to try and recover scores post an in game failure
// eeprom 84 through 87 - team kill counts for teams 0-3, 84 being team 0


#define SERIAL1_RXPIN 16 // TO BRX TX and BLUETOOTH RX
#define SERIAL1_TXPIN 17 // TO BRX RX and BLUETOOTH TX
bool ESPTimeout = true; // if true, this enables automatic deep sleep of esp32 device if wifi or blynk not available on boot
long TimeOutCurrentMillis = 0;
long TimeOutInterval = 480000;
int BaudRate = 57600; // 115200 is for GEN2/3, 57600 is for GEN1, this is set automatically based upon user input
bool FAKESCORE = false;
bool REQUESTQUERY = false;
long QueryStart = 0;

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
String FirmwareVer = {"4.13"};
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
const int ledPin = 2;
void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();
bool UPTODATE = false;
int updatenotification;

//unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long otainterval = 1000;
const long mini_interval = 1000;


// offline game mode settings
int pbgame = 0;
int pbteam = 0;
int pbweap = 0;
int pbperk = 0;
int pblives = 0;
int pbtime = 0;
int pbspawn = 0;
int pbindoor = 0;
bool PBGAMESTART = false;
bool PBLOCKOUT = false;
bool PBENDGAME = false;
String GameSettings = "0";
String GameSettingsTokens[10];

int PlayerLives = 32000;

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
String tokenStrings[100]; // once processed the data is broken up into this string array for use
char *ptr = NULL; // used for initializing and ending string break up.
// these are variables im using to create settings for the game mode and
// gun settings
int SetTeam=0; // used to configure team player settings, default is 0
long SetTime=2000000000; // used for in game timer functions on esp32 (future
int SetObj=32000; // used to program objectives
int SetVol=80; // set tagger volume adjustment, default is 65
int SwapBRXCounter = 0; // used for weapon swaps in game for all weapons
bool SWAPBRX = false; // used as trigger to enable/disable swapbrx mode
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
int SwapPistolLevel = 0;
long SelectButtonTimer = 0; // used to disable option to swap weapons

long LastDeathTime = 0;
int Deaths = 0; // death counter
int Team=0; // team selection used when allowed for custom configuration
int Objectives = 32000; // objective goals
int CompletedObjectives=0; // earned objectives by player
int PlayerKillCount[64] = {0}; // so its players 0-63 as the player id.
int TeamKillCount[6] = {0}; // teams 0-6, Red-0, blue-1, yellow-2, green-3, purple-4, cyan-5
int GameMode=1; // for setting up general settings
int Special=0; // special settings
int AudioPlayCounter=0; // used to make sure audio is played only once (redundant check)
String AudioSelection; // used to play stored audio files with tagger FOR SERIAL CORE
String AudioSelection1; // used to play stored audio files with tagger FOR BLE CORE
int AudioDelay = 0; // used to delay an audio playback
int AnounceScore = 99; // used for button activated score announcement
long AudioPreviousMillis = 0;
String AudioPerk; // used to play stored audio files with tagger FOR BLE CORE
int AudioPerkTimer = 4000; // used to time the perk announcement
bool PERK = false; // used to trigger audio perk
int CurrentDominationLeader = 99;
int ClosingVictory[4];

int lastTaggedPlayer = -1;  // used to capture player id who last shot gun, for kill count attribution
int lastTaggedTeam = -1;  // used to captures last player team who shot gun, for kill count attribution
int lastTaggedBase = -1; // used to capture last base used for swapbrx etc.

int SpecialWeapon = 0;
int PreviousSpecialWeapon = 0;
int ChangeMyColor = 99; // default, change value from 99 to execute tagger and headset change
int CurrentColor = 99; // default, change value from 99 to execute tagger and headset change
bool VOLUMEADJUST=false; // trigger for audio adjustment
bool AUDIO = false; // used to trigger an audio on tagger FOR SERIAL CORE
bool AUDIO1 = false; // used to trigger an audio on tagger FOR BLE CORE
int AudioChannel = 3;
bool INGAME=false; // status check for game timer and other later for running certain checks if gun is in game.
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool INITIALIZEOTA = false; // enables the object to disable BLE and enable WiFi
bool SPECIALWEAPON = false;
bool HASFLAG = false; // used for capture the flag
bool SELECTCONFIRM = false; // used for using select button to confirm an action
bool SPECIALWEAPONLOADOUT = false; // used for enabling special weapon loading
bool LOOT = false; // used to indicate a loot occured
bool STATBOOST = false; // used to enable a stat boost
int PlayerStatBoost = 0; // used for implementing a stat boost
bool SPECIALWEAPONPICKUP = false; // used to trigger a weapon loadout
bool STEALTH = false; // used to turn off gun led side lights
long laststealthtime = 0; // used for clearing out leds on tagger.
bool STRINGSENDER = false;
String StringSender = "Null";
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

long WebAppUpdaterTimer = 0;
int WebAppUpdaterProcessCounter = 0;
int WebSocketData;

// Replace with your network credentials
const char* ssid = GunName;

int ScoreRequestCounter = 0;
String ScoreTokenStrings[73];
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

// object to generate  numbers to send
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
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(GunName, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
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
  
<script>

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

void ClearScores() {
  Serial.println("resetting all scores");
  CompletedObjectives = 0;
  int teamcounter = 0;
  int eepromcounter = 84;
  int playercounter = 0;
  while (teamcounter < 4) {
    TeamKillCount[teamcounter] = 0;
    EEPROM.write(eepromcounter, TeamKillCount[teamcounter]);
    EEPROM.commit();
    teamcounter++;
    eepromcounter++;
    delay(0);
  }
  eepromcounter = 20;
  while (playercounter < 64) {
    PlayerKillCount[playercounter] = 0;
    EEPROM.write(eepromcounter, PlayerKillCount[playercounter]);
    EEPROM.commit();
    playercounter++;
    eepromcounter++;
    delay(0);
  }
  EEPROM.write(6, CompletedObjectives);
  Deaths = 0;
  EEPROM.write(5, Deaths);
  EEPROM.commit();
  Serial.println("Scores Reset");
}
//**************************************************************
void SyncScores() {
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
// adjust settings for offline weapon use and callouts
void weaponalignment() {
  if (pbgame < 3) {// F4A, Death, Generals
    if (pbweap == 0) {
     AudioSelection1 = "VA4B";
    }
    if (pbweap == 1) {
     AudioSelection1 = "VA5Y";
    }
    if (pbweap == 2) {
     AudioSelection1 = "VA5R";
    }
    if (pbweap == 3) {
     AudioSelection1 = "VA6A";
    }
    if (pbweap == 4) {
     AudioSelection1 = "VA4F";
    }
    if (pbweap == 5) {
     AudioSelection1 = "VA6B";
    }
    if (pbweap == 6) {
     AudioSelection1 = "VA9O";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 0) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection1 = "V3M";
    }
    if (pbweap == 1) {
     AudioSelection1 = "VEM";
    }
    if (pbweap == 2) {
     AudioSelection1 = "V8M";
    }
    if (pbweap == 3) {
     AudioSelection1 = "VHM";
    }
    if (pbweap == 4) {
     AudioSelection1 = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection1 = "VS1";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 1) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection1 = "VCM";
    }
    if (pbweap == 1) {
     AudioSelection1 = "V7M";
    }
    if (pbweap == 2) {
     AudioSelection1 = "V2M";
    }
    if (pbweap == 3) {
     AudioSelection1 = "V1M";
    }
    if (pbweap == 4) {
     AudioSelection1 = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection1 = "VS1";
    }
  }
  if (pbgame > 2 && pbgame < 5 && pbteam == 2) {// supremacy, commanders
    if (pbweap == 0) {
     AudioSelection1 = "VKM";
    }
    if (pbweap == 1) {
     AudioSelection1 = "VDM";
    }
    if (pbweap == 2) {
     AudioSelection1 = "VGM";
    }
    if (pbweap == 3) {
     AudioSelection1 = "VJM";
    }
    if (pbweap == 4) {
     AudioSelection1 = "VNM";
    }
    if (pbweap == 5) {
     AudioSelection1 = "VS1";
    }
  }
  if (pbgame > 4 && pbteam == 0) {// survival, swarm
    if (pbweap == 0) {
     AudioSelection1 = "VA4B";
    }
    if (pbweap == 1) {
     AudioSelection1 = "VA5Y";
    }
    if (pbweap == 2) {
     AudioSelection1 = "VA5R";
    }
    if (pbweap == 3) {
     AudioSelection1 = "VA6A";
    }
    if (pbweap == 4) {
     AudioSelection1 = "VA4F";
    }
    if (pbweap == 5) {
     AudioSelection1 = "VA6B";
    }
    if (pbweap == 6) {
     AudioSelection1 = "VA9O";
    }
  }
  AUDIO1 = true;
}
// adjust settings for offline game mode team settings
void teamalignment() {
  if (SetTeam == 0 || Team == 0) {
    if (pbgame == 3 || pbgame == 4) {
      AudioSelection = "RS9"; pbteam = 0;
    } 
    if (pbgame > 4) {
      AudioSelection = "VA3T"; pbteam = 0;
    } 
    if (pbgame < 3) {
      AudioSelection = "VA13"; pbteam = 0;
    }
  }
  if (SetTeam == 1 || Team == 1) {
    if (pbgame == 3 || pbgame == 4) {
      AudioSelection = "VA4N"; pbteam = 1;
    } 
    if (pbgame > 4) {
      AudioSelection = "VA1L"; pbteam = 0;
    } 
    if (pbgame < 3) {
      AudioSelection = "VA1L"; pbteam = 1;
    }
  }
  if (SetTeam == 3 || Team == 3) {
    if (pbgame == 3 || pbgame == 4) {
      AudioSelection = "VA71"; pbteam = 2;
    } 
    if (pbgame > 4) {
      AudioSelection = "VA3Y"; pbteam = 1;
    } 
    if (pbgame < 3) {
      AudioSelection = "VA27"; pbteam = 2;
    }
  }
  if (SetTeam == 2 || Team == 2) {
    if (pbgame == 3 || pbgame == 4) {
      AudioSelection = "VA1R"; pbteam = 0;
    } 
    if (pbgame > 4) {
      AudioSelection = "VA1R"; pbteam = 0;
    } 
    if (pbgame < 3) {
      AudioSelection = "VA1R"; pbteam = 1;
    }
  }
  AUDIO1 = true;
}
void ProcessIncomingCommands() {
  ledState = !ledState;
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  
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
    if (incomingData2 < 600 && incomingData2 > 500) { // setting team modes
      int b = incomingData2 - 500;
      
      if (INGAME==false){
        int A = 0;
        int B = 1;
        int C = 2;
        int D = 3;
        if (b==1) {
          Serial.println("Free For All"); 
          SetTeam=0;
          ProcessTeamsAssignmnet();
          //AUDIO1 = true;
          //AudioSelection="VA30";
        }
        if (b==2) {
          Serial.println("Teams Mode Two Teams (odds/evens)");
          if (GunID % 2) {
            SetTeam=0; 
            //AudioSelection="VA13";
          } else {
            SetTeam=1; 
            //AudioSelection="VA1L";
          }
          ProcessTeamsAssignmnet();
          //AUDIO1=true;
        }
        if (b==3) {
          Serial.println("Teams Mode Three Teams (every third player)");
          while (A < 64) {
            if (GunID == A) {
              SetTeam=0;
              //AudioSelection="VA13";
            }
            if (GunID == B) {
              SetTeam=1;
              //AudioSelection="VA1L";
            }
            if (GunID == C) {
              SetTeam=3;
              //AudioSelection1="VA27";
            }
            A = A + 3;
            B = B + 3;
            C = C + 3;
            delay(0);
          }
          A = 0;
          B = 1;
          C = 2;
          ProcessTeamsAssignmnet();
          //AUDIO1 = true;
        }
        if (b==4) {
          Serial.println("Teams Mode Four Teams (every fourth player)");
          while (A < 64) {
            if (GunID == A) {
              SetTeam=0;
              //AudioSelection="VA13";
            }
            if (GunID == B) {
              SetTeam=1;
              //AudioSelection="VA1L";
            }
            if (GunID == C) {
              SetTeam=2;
              //AudioSelection="VA1R";
            }
            if (GunID == D) {
              SetTeam=3;
              //AudioSelection="VA27";
            }
            A = A + 4;
            B = B + 4;
            C = C + 4;
            D = D + 4;
            delay(0);
          }
          A = 0;
          B = 1;
          C = 2;
          D = 3;
          ProcessTeamsAssignmnet();
          //AUDIO1 = true;
        }
        if (b==5) {
          Serial.println("Teams Mode Player's Choice"); 
          SetTeam=100; 
          AudioSelection="VA5E";
          AUDIO=true;
        } // this one allows for manual input of settings... each gun will need to select a team now
        //teamalignment();
        ChangeMyColor = SetTeam; // triggers a gun/tagger color change
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
       EEPROM.write(7, SetVol);
       EEPROM.commit();
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
         PBENDGAME=true; 
         Serial.println("ending game");
         datapacket2 = 1400;
         datapacket1 = 99;
         // BROADCASTESPNOW = true;
       }
     }
     if (b > 9 && b < 20) { // this is a game winning end game... you might be a winner
       if (INGAME){
         PBENDGAME=true; 
         Serial.println("ending game");
         datapacket2 = 1410 + b;
         datapacket1 = 99;
         //BROADCASTESPNOW = true;
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
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 1 && ClosingVictory[1] == 0) {
           AudioSelection = "VB03";
           ClosingVictory[1] = 1;
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 2 && ClosingVictory[2] == 0) {
           AudioSelection = "VB1F";
           ClosingVictory[2] = 1;
           AUDIO=true;
           datapacket2 = 1430 + b;
           datapacket1 = 99;
           //BROADCASTESPNOW = true;
         }
         if (leadercheck == 3 && ClosingVictory[3] == 0) {
           AudioSelection = "VB0H";
           ClosingVictory[3] = 1;
           AUDIO=true;
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
      UNLOCKHIDDENMENU = true;
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
         delay(0);
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
    // 2100s used for offline games
    if (incomingData2 > 2099 && incomingData2 < 3000) {
      Serial.println("received a 2100 series command");
      if (!INGAME) {
        if (ChangeMyColor > 8) {
          ChangeMyColor = 4; // triggers a gun/tagger color change
        } else { 
          ChangeMyColor++;
        }
        if (incomingData2 == 2100) { pbgame = 0; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA30"; Serial.println("Free For All");} // f4A
        if (incomingData2 == 2101) { pbgame = 1; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA26"; Serial.println("Death Match");} // DMATCH
        if (incomingData2 == 2102) { pbgame = 2; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA36"; Serial.println("Generals");} // GENERALS
        if (incomingData2 == 2103) { pbgame = 3; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA62"; Serial.println("supremacy");} // SUPREM
        if (incomingData2 == 2104) { pbgame = 4; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA1Y"; Serial.println("comander");} // COMMNDR
        if (incomingData2 == 2105) { pbgame = 5; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA64"; Serial.println("survival");} // SURVIVAL
        if (incomingData2 == 2106) { pbgame = 6; EEPROM.write(9, pbgame); EEPROM.commit(); AudioSelection1="VA6J"; Serial.println("swarm");} // SWARM
        if (incomingData2 == 2107) { SetTeam = 0; EEPROM.commit(); Serial.println("team0"); if (pbgame == 3 || pbgame == 4) {AudioSelection1 = "VS9"; pbteam = 0;} if (pbgame > 4) {AudioSelection1 = "VA3T"; pbteam = 0;} if (pbgame < 3) {AudioSelection1 = "VA13"; pbteam = 0;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = SetTeam; EEPROM.write(14, pbteam);}
        if (incomingData2 == 2108) { SetTeam = 1; EEPROM.commit(); Serial.println("team1"); if (pbgame == 3 || pbgame == 4) {AudioSelection1 = "VA4N"; pbteam = 1;} if (pbgame > 4) {AudioSelection1 = "VA1L"; pbteam = 0;} if (pbgame < 3) {AudioSelection1 = "VA1L"; pbteam = 1;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = SetTeam; EEPROM.write(14, pbteam);}
        if (incomingData2 == 2109) { SetTeam = 3; EEPROM.commit(); Serial.println("team3"); if (pbgame == 3 || pbgame == 4) {AudioSelection1 = "VA71"; pbteam = 2;} if (pbgame > 4) {AudioSelection1 = "VA3Y"; pbteam = 1;} if (pbgame < 3) {AudioSelection1 = "VA27"; pbteam = 0;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = SetTeam; EEPROM.write(14, pbteam);}
        if (incomingData2 == 2143) { SetTeam = 2; EEPROM.commit(); Serial.println("team2"); if (pbgame == 3 || pbgame == 4) {AudioSelection1 = "VA1R"; pbteam = 1;} if (pbgame > 4) {AudioSelection1 = "VA1R"; pbteam = 0;} if (pbgame < 3) {AudioSelection1 = "VA1R"; pbteam = 1;} Serial.println("pbteam = " + String(pbteam)); ChangeMyColor = SetTeam; EEPROM.write(14, pbteam);}
        if (incomingData2 == 2110) { pbweap = 0; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap0"); weaponalignment();}
        if (incomingData2 == 2111) { pbweap = 1; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap1"); weaponalignment();}
        if (incomingData2 == 2112) { pbweap = 2; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap2"); weaponalignment();}
        if (incomingData2 == 2113) { pbweap = 3; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap3"); weaponalignment();}
        if (incomingData2 == 2114) { pbweap = 4; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap4"); weaponalignment();}
        if (incomingData2 == 2115) { pbweap = 5; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap5"); weaponalignment();}
        if (incomingData2 == 2116) { pbweap = 6; EEPROM.write(15, pbweap); EEPROM.commit(); Serial.println("weap6"); weaponalignment();}
        if (incomingData2 == 2117) { pbperk = 0; EEPROM.write(16, pbperk); EEPROM.commit(); Serial.println("perk0"); AudioSelection1 = "VA38";}
        if (incomingData2 == 2118) { pbperk = 1; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA4D"; Serial.println("perk1");}
        if (incomingData2 == 2119) { pbperk = 2; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA1G"; Serial.println("perk2");}
        if (incomingData2 == 2120) { pbperk = 3; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA2N"; Serial.println("perk3");}
        if (incomingData2 == 2121) { pbperk = 4; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA1Z"; Serial.println("perk4");}
        if (incomingData2 == 2122) { pbperk = 5; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA24"; Serial.println("perk5");}
        if (incomingData2 == 2157) { pbperk = 6; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA2W"; Serial.println("perk6");}
        if (incomingData2 == 2158) { pbperk = 7; EEPROM.write(16, pbperk); EEPROM.commit(); AudioSelection1="VA4T"; Serial.println("perkoff");}
        if (incomingData2 == 2123) { pblives = 0; PlayerLives = 1; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA01"; Serial.println("lives0");}
        if (incomingData2 == 2124) { pblives = 1; PlayerLives = 3; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA03"; Serial.println("lives1");}
        if (incomingData2 == 2125) { pblives = 2; PlayerLives = 5; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA05"; Serial.println("lives2");}
        if (incomingData2 == 2126) { pblives = 3; PlayerLives = 10; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA0A"; Serial.println("lives3");}
        if (incomingData2 == 2127) { pblives = 4; PlayerLives = 15; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA0F"; Serial.println("lives4");}
        if (incomingData2 == 2128) { pblives = 5; PlayerLives = 32000; EEPROM.write(19, PlayerLives); EEPROM.write(11, pblives); EEPROM.commit(); AudioSelection1="VA6V"; Serial.println("lives5");}
        if (incomingData2 == 2129) { pbtime = 0; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA2S"; Serial.println("time0");}
        if (incomingData2 == 2130) { pbtime = 1; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA6H"; Serial.println("time1");}
        if (incomingData2 == 2131) { pbtime = 2; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA2P"; Serial.println("time2");}
        if (incomingData2 == 2132) { pbtime = 3; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA6Q"; Serial.println("time3");}
        if (incomingData2 == 2133) { pbtime = 4; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA0Q"; Serial.println("time4");}
        if (incomingData2 == 2134) { pbtime = 5; EEPROM.write(12, pbtime); EEPROM.commit(); AudioSelection1="VA6V"; Serial.println("time5");}
        if (incomingData2 == 2135) { pbspawn = 0; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA4T"; Serial.println("spawn0");}
        if (incomingData2 == 2136) { pbspawn = 1; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA2Q"; Serial.println("spawn1");}
        if (incomingData2 == 2137) { pbspawn = 2; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA0R"; Serial.println("spawn2");}
        if (incomingData2 == 2138) { pbspawn = 3; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA0V"; Serial.println("spawn3");}
        if (incomingData2 == 2139) { pbspawn = 4; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA0S"; Serial.println("spawn4");}
        if (incomingData2 == 2140) { pbspawn = 5; EEPROM.write(13, pbspawn); EEPROM.commit(); AudioSelection1="VA0W"; Serial.println("spawn5");}
        if (incomingData2 == 2141) { pbindoor = 0; EEPROM.write(8, 0); EEPROM.commit(); AudioSelection1="VA4W"; Serial.println("outdoor");}
        if (incomingData2 == 2142) { pbindoor = 1; EEPROM.write(8, 1); EEPROM.commit(); AudioSelection1="VA3W"; Serial.println("indoor");}
        if (incomingData2 == 2151) { pbindoor = 0; EEPROM.write(8, 2); EEPROM.commit(); STEALTH = true; AudioSelection1="VA60"; Serial.println("outdoorS");}
        if (incomingData2 == 2152) { pbindoor = 1; EEPROM.write(8, 3); EEPROM.commit(); STEALTH = true; AudioSelection1="VA60"; Serial.println("indoorS");}
        if (incomingData2 == 2144) { GameMode = 0; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="VA5Z"; BACKGROUNDMUSIC = false; Serial.println("nogamemod"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // standard - defaul
        if (incomingData2 == 2145) { GameMode = 4; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="VA8J"; BACKGROUNDMUSIC = false; Serial.println("royalegamemod"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // battle royale
        if (incomingData2 == 2146) { GameMode = 8; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="OP01"; BACKGROUNDMUSIC = false; Serial.println("assimilationgamemod"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // assimilation
        if (incomingData2 == 2147) { // JEDGE SUPREMACY 5V5
          GameMode = 9;  
          EEPROM.write(10, GameMode);
          SetVol = 100;
          EEPROM.write(7, SetVol);
          pbindoor = 0; 
          EEPROM.write(8, pbindoor);
          pbgame = 4; 
          EEPROM.write(9, pbgame);
          pblives = 5; 
          PlayerLives = 32000; 
          EEPROM.write(19, PlayerLives); 
          EEPROM.write(11, pblives);
          pbtime = 4; 
          EEPROM.write(12, pbtime);
          pbperk = 7; 
          EEPROM.write(16, pbperk);
          EEPROM.commit();
          if (GunID == 0 || GunID == 5 || GunID == 10 || GunID == 15) { // Soldier
            pbweap = 1;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 1 || GunID == 6 || GunID == 11 || GunID == 16) { // heavy
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 2 || GunID == 7 || GunID == 12 || GunID == 17) { // Mercenary
            pbweap = 4;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 3 || GunID == 8 || GunID == 13 || GunID == 18) { // Sniper
            pbweap = 1;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 4 || GunID == 9 || GunID == 14 || GunID == 19) { // Wraith
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID < 5) {
            SetTeam = 0;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 10 && GunID > 4) {
            SetTeam = 1;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 15 && GunID > 9) {
            SetTeam = 2;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 20 && GunID > 14) {
            SetTeam = 3;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          Serial.println("Volume set to" + String(SetVol));
          AudioSelection1="VA62"; 
          BACKGROUNDMUSIC = false; 
          Serial.println("SUPREMACY 5V5"); 
          StringSender = "$PLAYX,4,*"; 
          STRINGSENDER = true;
          VOLUMEADJUST=true;
        }
        if (incomingData2 == 2147) { // JEDGE SUPREMACY 5V5
          GameMode = 10;  
          EEPROM.write(10, GameMode);
          SetVol = 100;
          EEPROM.write(7, SetVol);
          pbindoor = 0; 
          EEPROM.write(8, pbindoor);
          pbgame = 4; 
          EEPROM.write(9, pbgame);
          pblives = 5; 
          PlayerLives = 32000; 
          EEPROM.write(19, PlayerLives); 
          EEPROM.write(11, pblives);
          pbtime = 4; 
          EEPROM.write(12, pbtime);
          pbperk = 7; 
          EEPROM.write(16, pbperk);
          EEPROM.commit();
          if (GunID == 0 || GunID == 6 || GunID == 12 || GunID == 18) { // Soldier
            pbweap = 1;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 1 || GunID == 7 || GunID == 13 || GunID == 19) { // heavy
            pbweap = 0;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 2 || GunID == 8 || GunID == 14 || GunID == 20) { // Mercenary
            pbweap = 4;
            pbteam = 0;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 3 || GunID == 9 || GunID == 15 || GunID == 21) { // Sniper
            pbweap = 1;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 4 || GunID == 10 || GunID == 16 || GunID == 22) { // Wraith
            pbweap = 0;
            pbteam = 2;
            pbspawn = 0; 
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID == 5 || GunID == 11 || GunID == 17 || GunID == 23) { // Medic/Commander
            pbweap = 5;
            pbteam = 0;
            pbspawn = 2;
            pblives = 1; 
            PlayerLives = 3; 
            EEPROM.write(19, PlayerLives); 
            EEPROM.write(11, pblives);
            EEPROM.write(14, pbteam);
            EEPROM.write(15, pbweap);
            EEPROM.write(13, pbspawn);
            EEPROM.commit();
          }
          if (GunID < 6) {
            SetTeam = 0;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 12 && GunID > 5) {
            SetTeam = 1;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 18 && GunID > 11) {
            SetTeam = 2;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          if (GunID < 24 && GunID > 17) {
            SetTeam = 3;
            EEPROM.write(88, SetTeam);
            EEPROM.commit();
          }
          Serial.println("Volume set to" + String(SetVol));
          AudioSelection1="VA62"; 
          BACKGROUNDMUSIC = false; 
          Serial.println("SUPREMACY 6V6"); 
          StringSender = "$PLAYX,4,*"; 
          STRINGSENDER = true;
          VOLUMEADJUST=true;
        }
        if (incomingData2 == 2148) { GameMode = 12; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="CC07"; Serial.println("contra"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // contra
        if (incomingData2 == 2149) { GameMode = 13; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="SW00"; Serial.println("starwars"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // starwars
        if (incomingData2 == 2150) { GameMode = 14; EEPROM.write(10, GameMode); EEPROM.commit(); AudioSelection1="JA5"; Serial.println("halo"); StringSender = "$PLAYX,4,*"; STRINGSENDER = true;} // halo
        if (incomingData2 == 2153) {
          SetTeam = random(2);
          ProcessTeamsAssignmnet();
        }  
        if (incomingData2 == 2154) {
          SetTeam = random(3);
          ProcessTeamsAssignmnet();
        }
        if (incomingData2 == 2155) {
          SetTeam = random(4);
          ProcessTeamsAssignmnet();
        }   
        if (incomingData2 == 2156) {
          GameMode = 15;
          AudioSelection1="VA9H";
        }  
        if (incomingData2 == 2199 && incomingData1 == 99) {
          ApplyGameSettings();
          Serial.println("start game");
          PBGAMESTART = true;
          ChangeMyColor = 9; // turns off led
        }
        if (incomingData2 == 2198 && incomingData1 == 99) {
          ApplyGameSettings();
          Serial.println("delay start");
          AudioSelection = "OP04"; // says game count down initiated
          AUDIO = true; 
          delay(13000);
          PBGAMESTART = true;
        }
      }
      if (incomingData2 == 2197) { 
        PBLOCKOUT = true;
      }
      if (incomingData2 == 2196) { 
        PBENDGAME = true;
      }
      AUDIO1 = true;
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
      if (INGAME) {
        LOOT = true;    
        Serial.println("Loot box found");
        lastincomingData2 = 0;
      }
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
        //BROADCASTESPNOW = true;
      }
      else {
        SPECIALWEAPONPICKUP = true;
      }
    } 
  }
  digitalWrite(2, LOW);
}
void ApplyGameSettings() {
    Serial.println(String(millis()));
    GameSettings = incomingData4; // saves incoming data as a temporary string within this object
    Serial.println("printing data received: ");
    Serial.println(GameSettings);
    char *ptr = strtok((char*)GameSettings.c_str(), ","); // looks for commas as breaks to split up the string
    int index = 0;
    while (ptr != NULL)
    {
      GameSettingsTokens[index] = ptr; // saves the individual characters divided by commas as individual strings
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
    }
    // (VolumeSetting, token 0), (OutdoorSetting, token 1), (GameMode, token 2) (CustomGameMode, tokens 3), (PlayerLives, tokens 4), (GameTime, token 5), (RespawnTime, token 6), (TeamSetting, Token 7), (WeaponSetting, Token 8), (PerkSetting, token 9)
    int Data[10];
    int count=0;
    while (count<10) {
      sendString("$VOL,0,0,*"); // adjust volume to default
      Data[count]=GameSettingsTokens[count].toInt();
      // Serial.println("Converting String character "+String(count)+" to integer: "+String(Data[count]));
      incomingData1 = 99;
      incomingData2 = Data[count];
      ProcessIncomingCommands();
      count++;
      //delay(2000);
    }
    Serial.println("completed applying Settings");
}
void PBGameStart() {
          sendString("$PBLOCK,*");
          sendString("$PLAYX,0,*");
          sendString("$PLAYX,0,*");
          sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
          sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
          sendString("$PLAY,VAQ,3,10,,,,,*"); // says let battle begin  
          PBGAMESTART = false;
          if (!INGAME) {
            ClearScores();
            INGAME = true;
          }
          sendString("$PBINDOOR," + String(pbindoor) + ",*");
          sendString("$PBINDOOR," + String(pbindoor) + ",*");
          sendString("$PBGAME," + String(pbgame) + ",*");
          sendString("$PBGAME," + String(pbgame) + ",*");
          sendString("$PBLIVES," + String(pblives) + ",*");
          sendString("$PBLIVES," + String(pblives) + ",*");
          sendString("$PBTIME," + String(pbtime) + ",*");
          sendString("$PBTIME," + String(pbtime) + ",*");
          sendString("$PBSPAWN," + String(pbspawn) + ",*");
          sendString("$PBSPAWN," + String(pbspawn) + ",*");
          sendString("$PBWEAP," + String(pbweap) + ",*");
          sendString("$PBWEAP," + String(pbweap) + ",*");
          if (pbperk < 7 && pbgame < 3) {
            sendString("$PBPERK," + String(pbperk) + ",*");
            sendString("$PBPERK," + String(pbperk) + ",*");
          }
          delay(1000);
          sendString("$PBSTART,*");
          sendString("$PBSTART,*");
          sendString("$PID," + String(GunID) + ",*");
          sendString("$PID," + String(GunID) + ",*");
          sendString("$TID," + String(SetTeam) + ",*");
          sendString("$TID," + String(SetTeam) + ",*");
          EEPROM.write(17, 1);
          EEPROM.commit();
          sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4****
          sendString("$BMAP,4,4,,,,,*"); // sets the left button as weapon 4****
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
            sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,0,,100,0,0,18,0,,,,,,,,175,850,32,32768,1400,10,0,100,100,,0,,,CC03,,,,CC03,,,D18,,,,,99,9999999,75,,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
            sendString("$WEAP,1,2,100,0,0,50,10,,,,,,70,80,400,850,6,32768,400,10,7,100,100,,0,,,CC04,,,,CC04,,,,,,,,99,9999999,75,30,*");
            MusicSelection = "$PLAY,CC08,4,10,,,,,*"; 
            BACKGROUNDMUSIC = true; 
            MusicRestart = 107000; 
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            AUDIO = true;
          }
          if (GameMode == 13) { // starwars
            sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,50,,H44,JAD,SW05,SW01,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
            sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,50,,H44,JAD,SW05,SW01,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$BMAP,1,100,0,1,99,99,*"); // sets the alt fire weapon to alternate between weapon 0 and 1 (99,99 can be changed for additional weapons selections)
            sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
            sendString("$WEAP,0,,100,0,0,50,0,,,,,,,,225,850,200,32768,400,0,7,100,100,,0,,,SW36,,,,D04,D03,D02,D18,,,,,200,9999999,20,,*");
            sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
            sendString("$WEAP,1,,100,0,0,18,0,,,,,,,,100,850,100,32768,1400,0,0,100,100,,5,,,SW23,C15,C17,,D30,D29,D37,A73,SW40,C04,20,150,100,9999999,75,,*");
            sendString("$WEAP,4,1,100,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
            sendString("$WEAP,4,1,100,13,0,125,0,,,,,,,,800,50,100,32768,0,10,13,100,100,,0,0,,SW17,,,,,,SW15,U64,,,0,,100,9999999,20,,*"); // this is default melee weapon for rifle bash
            if (SetTeam == 0 || SetTeam == 2) {
              MusicSelection = "$PLAY,SW31,4,10,,,,,*";
              MusicRestart = 60000;
            } else {
              MusicSelection = "$PLAY,SW04,4,10,,,,,*";
              MusicRestart = 92000;
            }
            BACKGROUNDMUSIC = true;
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            AUDIO = true;
          }
          if (GameMode == 14) { // halo
            if (SetTeam == 0 || SetTeam == 2) {
              MusicSelection = "$PLAY,JAN,4,10,,,,,*";
              MusicRestart = 59000;
            }
            if (SetTeam == 1) {
              MusicSelection = "$PLAY,JAO,4,10,,,,,*";
              MusicRestart = 59000;
            }
            if (SetTeam == 3) {
              MusicSelection = "$PLAY,JAP,4,10,,,,,*";
              MusicRestart = 59000;
            }
            BACKGROUNDMUSIC = true;
            MusicPreviousMillis = 0;
            AudioSelection = MusicSelection;
            AUDIO = true;
          }

          // used to test a reset eeprom stored score:
          // EepromScoreTest();
}
void EepromScoreTest() {
  lastTaggedPlayer = 0;
  lastTaggedTeam = 0;
  readStr = "$HP,0,0,0,*";
  ProcessBRXData();
  
          Serial.println(lastTaggedPlayer);
          //Serial.println(peepromid);
          Serial.println(lastTaggedTeam);
          //Serial.println(teepromid);
          Serial.println(EEPROM.read(20));
          Serial.println(EEPROM.read(84));
          Serial.println(TeamKillCount[0]);
          
  delay(3000);
  lastTaggedPlayer = 1;
  lastTaggedTeam = 1;
  readStr = "$HP,0,0,0,*";
  ProcessBRXData();
  
          Serial.println(lastTaggedPlayer);
          //Serial.println(peepromid);
          Serial.println(lastTaggedTeam);
          //Serial.println(teepromid);
          Serial.println(EEPROM.read(21));
          Serial.println(EEPROM.read(85));
          Serial.println(TeamKillCount[1]);
  delay(3000);
  lastTaggedPlayer = 2;
  lastTaggedTeam = 2;
  readStr = "$HP,0,0,0,*";
  ProcessBRXData();
  
          Serial.println(lastTaggedPlayer);
          //Serial.println(peepromid);
          Serial.println(lastTaggedTeam);
          //Serial.println(teepromid);
          Serial.println(EEPROM.read(22));
          Serial.println(EEPROM.read(86));
          Serial.println(TeamKillCount[2]);
  delay(3000);
  lastTaggedPlayer = 3;
  lastTaggedTeam = 3;
  readStr = "$HP,0,0,0,*";
  ProcessBRXData();
  
          Serial.println(lastTaggedPlayer);
         // Serial.println(peepromid);
          Serial.println(lastTaggedTeam);
         // Serial.println(teepromid);
          Serial.println(EEPROM.read(23));
          Serial.println(EEPROM.read(87));
          Serial.println(TeamKillCount[3]);
          Serial.println(EEPROM.read(5));
}
void ProcessTeamsAssignmnet() {
          if (SetTeam == 0) {
            Serial.println("team0"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection1 = "VS9"; 
              pbteam = 0;
            } 
            if (pbgame > 4) {
              AudioSelection1 = "VA3T"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection1 = "VA13"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          if (SetTeam == 1) {
            Serial.println("team1"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection1 = "VA4N"; 
              pbteam = 1;
            }
            if (pbgame > 4) {
              AudioSelection1 = "VA1L"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection1 = "VA1L"; 
              pbteam = 1;
            }
            Serial.println("pbteam = " + String(pbteam));
          }
          if (SetTeam == 2) {
            Serial.println("team2"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection1 = "VA71"; 
              pbteam = 2;
            } 
            if (pbgame > 4) {
              AudioSelection1 = "VA3Y"; 
              pbteam = 0;
            } 
            if (pbgame < 3) {
              AudioSelection1 = "VA27"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          if (SetTeam == 3) {
            Serial.println("team3"); 
            if (pbgame == 3 || pbgame == 4) {
              AudioSelection1 = "VA1R"; 
              pbteam = 2;
            } 
            if (pbgame > 4) {
              AudioSelection1 = "VA1R"; 
              pbteam = 1;
            } 
            if (pbgame < 3) {
              AudioSelection1 = "VA1R"; 
              pbteam = 0;
            } 
            Serial.println("pbteam = " + String(pbteam));
          }
          EEPROM.write(14, pbteam);
          EEPROM.write(88, SetTeam);
          EEPROM.commit();
          AUDIO1 = true;
          ChangeMyColor = SetTeam;
}
void PBLockout() {
  BACKGROUNDMUSIC = false;
  PBLOCKOUT = false;
  sendString("$VOL,0,0,*"); // adjust volume to default
  Serial.println();
  Serial.println("JEDGE is taking over the BRX");
  sendString("$PHONE,*");
  sendString("$PHONE,*");
  sendString("$PLAYX,0,*");
  sendString("$PLAYX,0,*");
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  INGAME = false;
  sendString("$PLAY,OP47,3,10,,,,,*"); // says "jedge init."
}
void PBEndGame() {
  sendString("$VOL,0,0,*"); // adjust volume to default
  PBENDGAME = false;
  sendString("$PBLOCK,*");
  sendString("$PLAYX,0,*");
  sendString("$PLAYX,0,*");
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  sendString("$VOL,"+String(SetVol)+",0,*"); // adjust volume to default
  INGAME = false;
  EEPROM.write(17, 0);
  EEPROM.commit();
  BACKGROUNDMUSIC = false;
  SwapBRXCounter = 0;
  SWAPBRX = false;
  SELECTCONFIRM = false;
  SPECIALWEAPONLOADOUT = false;
  PreviousSpecialWeapon = 0;
  Serial.println("sending game exit commands");
  delay(500);
  
  AudioSelection1 = "VA33"; // "game over"
  AUDIO1 = true;
  AudioDelay = 1500;
  AudioSelection="VAP"; // "head back to base"
  AudioPreviousMillis = millis();
  AUDIO = true;
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
}
  

//******************************************************************************************

// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
void sendString(String value) {
  int genonedelay = 6;
  const char * c_string = value.c_str();
  Serial.println("Sending the following:");
  //Serial.print("sending: ");
  if (GunGeneration == 1) {
    for (int i = 0; i < value.length() / 1; i++) { // creates a for loop
      Serial1.println(c_string[i]); // sends one value at a time to gen1 brx
      Serial.print(c_string[i]);
      delay(genonedelay); // this is an added delay needed for the way the brx gen1 receives data (brute force method because i could not match the comm settings exactly)
    }
  }
  if (GunGeneration > 1) {
    int matchingdelay = value.length() * genonedelay ; // this just creates a temp value for how long the string is multiplied by 5 milliseconds
    delay(matchingdelay); // now we delay the same amount of time.. this is so if we have gen1 and gen2 brx in the mix, they receive the commands about the same time
    Serial1.println(c_string); // sending the string to brx gen 2
    Serial.println(c_string);
  }
  Serial.println();
  delay(20);
}

void InitAP() {
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.mode(WIFI_AP_STA);
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

//****************************************************************************
void Audio() {
  if (AudioSelection1=="VA20") {
    sendString("$CLEAR,*");
    sendString("$START,*");
  }
  if (AUDIO) {
    if (AudioDelay > 1) {
      if (millis() - AudioPreviousMillis > AudioDelay) {
        if (AudioChannel == 3) {
          sendString("$PLAY,"+AudioSelection+",3,10,,,,,*");
        } else {
          sendString("$PLAY,"+AudioSelection+"," + AudioChannel + ",10,,,,,*");
          AudioChannel = 3;
        }
        AUDIO = false;
        AudioSelection  = "NULL";
        AudioDelay = 0; // reset the delay for another application
      }
    } else {
      sendString("$PLAY,"+AudioSelection+",3,10,,,,,*");
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
    sendString("$PLAY,"+AudioSelection1+",3,10,,,,,*");
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
  if (REQUESTQUERY) {
    long currentquerytime = millis();
    if (currentquerytime - QueryStart > 1200) {
      delay(7000);
      REQUESTQUERY = false;
      //PBLockout();
      PBGameStart();
    }
  }
  if (PBGAMESTART) {
    PBGameStart();
  }
  if (PBLOCKOUT) {
    PBLockout();
  }
  if (PBENDGAME) {
    PBEndGame();
  }
  if (UNLOCKHIDDENMENU) {
    UNLOCKHIDDENMENU = false;
    int dlccounter = 0;
    while (dlccounter < 12) {
      sendString("$DLC,"+String(dlccounter)+",1,*");
      dlccounter++;
      delay(150);
    }
    sendString("$PLAY,OP61,3,10,,,,,*");
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
  if (VOLUMEADJUST) {
    VOLUMEADJUST=false;
    sendString("$VOL,"+String(SetVol)+",0,*"); // sets max volume on gun 0-100 feet distance
  }    
  // In game checks and balances to execute actions for in game functions
  if (INGAME) {
    if (STEALTH){ // periodically turn off gun lights
      long currentstealthtime = millis();
      if (currentstealthtime - laststealthtime > 2000) {
        Serial.println("delaying stealth start");
        sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
        sendString("$GLED,,,,5,,,*"); // TURNS OFF SIDE LIGHTS
        Serial.println("sent command to kill leds");
        laststealthtime = millis();
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
        sendString("$PLAY,"+AudioPerk+",3,10,,,,,*");
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
  AUDIO = true;
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
  //TerminalInput = readStr;
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
  } 
  // TIME REMAINING IN OFFLINE GAME MODE OR PB GAMES 
  if (tokenStrings[0] == "$TIME") {
    if (tokenStrings[1] == "0") {
      // THIS INDICATES THAT TIME IS UP IN GAME
      INGAME = false;
      EEPROM.write(18, 1);
      EEPROM.commit();
      ESP.restart(); // REBOOT ESP
    }
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
    if (tokenStrings[2] == "15" && tokenStrings[5] == "16") { // this is a KOTH tag
      // add objective point for being in the radius of a hill
      CompletedObjectives++;
    }
    lastTaggedPlayer = tokenStrings[3].toInt();
    lastTaggedTeam = tokenStrings[4].toInt();
    //Serial.println("Just tagged by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
  }
  // this is a cool pop up... lots of info provided and broken out below.
  // this pops up everytime you pull the trigger and deplete your ammo.hmmm you
  // can guess what im using that for... see more below
  // ****** note i still need to assign the ammo to the lcd outputs********
  //if (tokenStrings[0] == "$ALCD") {
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
  //}
  if (tokenStrings[0] == "$LCD") {
    /* SPAWN STATS UPDATE status update occured 
     *  space 0 is LCD indicator
     *  space 1 is HEALTH
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
    int currenthealth = tokenStrings[1].toInt(); // 
    if (currenthealth == 100 || currenthealth == 45) { // player just spawned
      sendString("$PID," + String(GunID) + ",*");
      sendString("$PID," + String(GunID) + ",*");
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
            //sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
            //sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",100,25,,CC01,JAD,CC01,CC02,CC05,CC05,CC05,V37,H06,CC05,CC05,CC05,CC10,U15,W71,A10,*");
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
    //health = tokenStrings[1].toInt(); // setting variable to be sent to esp8266
    //armor = tokenStrings[2].toInt(); // setting variable to be sent to esp8266
    //shield = tokenStrings[3].toInt(); // setting variable to be sent to esp8266
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
          int peepromid = lastTaggedPlayer + 20;
          int teepromid = lastTaggedTeam + 84;
          EEPROM.write(peepromid, PlayerKillCount[lastTaggedPlayer]);
          EEPROM.write(teepromid, TeamKillCount[lastTaggedTeam]);
          EEPROM.write(5, Deaths);
          EEPROM.commit();
          //   call audio to announce player's killer
          AnounceScore = 0;
          AudioDelay = 5000;
          AudioChannel = 1;
          AudioSelection="KB"+String(lastTaggedPlayer);
          AudioPreviousMillis = millis();
          AUDIO=true;
          if (Deaths == PlayerLives) {
            // player is out of lives - game over for player
            // before player goes back to base, reset esp32
            INGAME = false;
            EEPROM.write(18, 1);
            EEPROM.commit();
            ESP.restart(); // REBOOT ESP
          }
      }
    }
  }
}

//******************************************************************************************



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
    AudioSelection="OP32";
    AUDIO=true;
  }
  int softreset = EEPROM.read(18);
  int gamestatus = EEPROM.read(17);
  if (gamestatus == 1 || softreset == 1) {
    Serial.println("Tagger was reset");
    // esp32 was reset while player is in game
    REQUESTQUERY = true;
    QueryStart = millis();
    sendString("$PHONE,*");
    sendString("$QUERY,*"); // REQUESTING STATUS FROM TAGGER
    if (gamestatus == 1) { // only the case if not soft reset
      INGAME = true; // setting parameter for module to know that it is in a game currently
      Serial.println("setting in game status to in game");
    }
    ESPTimeout = false; // disabling timeout deep sleep of esp
    // lets reload our current scores:
    Serial.println("loading player's previous scores");
    int scorecounter = 20;
    int playercounter = 0;
    while (scorecounter < 84) {
      PlayerKillCount[playercounter] = EEPROM.read(scorecounter);
      Serial.println("Player " + String(playercounter) + "has a score of: " + String(PlayerKillCount[playercounter]));
      playercounter++;
      scorecounter++;
    }
    Serial.println("Loading Previous Team Scores");
    TeamKillCount[0] = EEPROM.read(84);
    TeamKillCount[1] = EEPROM.read(85);
    TeamKillCount[2] = EEPROM.read(86);
    TeamKillCount[3] = EEPROM.read(87);
    Serial.println("Loading Objectives and Deaths");
    Deaths = EEPROM.read(5);
    CompletedObjectives = EEPROM.read(6);
    // now lets set the game settings from eeprom memory
    Serial.println("loading all previous game settings");
    SetVol = EEPROM.read(7);
    pbindoor = EEPROM.read(8);
    if (pbindoor > 1) {
      pbindoor = pbindoor - 2;
      STEALTH = true;
    }
    pbgame = EEPROM.read(9);
    GameMode = EEPROM.read(10);
    pblives = EEPROM.read(11);
    pbtime = EEPROM.read(12);
    pbspawn = EEPROM.read(13);
    pbteam = EEPROM.read(14);
    SetTeam = EEPROM.read(88);
    pbweap = EEPROM.read(15);
    pbperk = EEPROM.read(16);
    Serial.println("all data retreived from EEPROM");
  }
  if (softreset == 1) {
    // reset the soft reset indicator
    EEPROM.write(18, 0);
    EEPROM.commit();
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
// *************************************  ESPNOW LOOP  *************************************
// *****************************************************************************************
void loop2(void *pvParameters) {
  Serial.print("cOMMS loop running on core ");
  Serial.println(xPortGetCoreID());
  WiFi.softAPdisconnect (true);
  Serial.println("Device is not intended to act as WebServer / Host. Shutting down AP and WebServer");
  while (1) { // starts the forever loop
    digitalWrite(ledPin, ledState);
    static unsigned long lastEventTime = millis();
    static const unsigned long EVENT_INTERVAL_MS = 1000;
    if (BROADCASTESPNOW) {
      BROADCASTESPNOW = false;
      getReadings();
      BroadcastData(); // sending data via ESPNOW
      Serial.println("Sent Data Via ESPNOW");
      ResetReadings();
      // ESP.restart(); // REBOOT ESP
    }
    if (ESPTimeout) {
      TimeOutCurrentMillis = millis();
      if (TimeOutCurrentMillis > TimeOutInterval) {
        Serial.println("Enabling Deep Sleep");
        delay(500);
        esp_deep_sleep_start();
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
  pinMode(LED_BUILTIN, OUTPUT);
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
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.print("setup() running on core ");

  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  
  initWebSocket();
  
  // Start server
  server.begin();

  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  
  if (bootstatus == 1){ // indicates we are in ota mode
    OTAMODE = true;
  }
  //*********** OTA MODE ************************
  if (OTAMODE) {  
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
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
    int passwordrequestattempt = 0;
    while (OTApassword == "dontchangeme") {
      datapacket1 = 9999;
      getReadings();
      BroadcastData(); // sending data via ESPNOW
      Serial.println("Sent Data Via ESPNOW");
      ResetReadings();
      delay(2000);
      passwordrequestattempt++;
      if (passwordrequestattempt > 5) {
        ESP.restart();
      }
    }
    delay(2000);
    Serial.println("wifi credentials");
    Serial.println(OTAssid);
    Serial.println(OTApassword);
  
    connect_wifi();

    delay(1500);
    sendString("$PLAY,OP28,3,10,,,,,*"); // says connection accepted
    delay(1500);
  
    EEPROM.write(1, 0);
    EEPROM.commit();
  } else {
    //************** Play Mode*******************
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
        sendString("$PLAY,OP43,3,10,,,,,*"); // starwars trumpets
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

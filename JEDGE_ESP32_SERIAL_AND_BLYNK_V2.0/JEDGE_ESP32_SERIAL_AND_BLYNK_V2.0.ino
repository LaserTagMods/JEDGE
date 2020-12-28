/*
 * by Jay Burden
 * 
 * use pins 16 & 17 to connect to BLUETOOTH pins on tagger. 17 goes to RX of brx or TX of the bluetooth module, 16 goes to the TX of tagger or RX of bluetooth
 * optionally power from laser tag battery with a Dorhea MP1584EN mini buck converter or use available power supply port from tagger board
 * this devices uses the serial communication to set tagger and game configuration settings
 * Note the sections labeled *****IMPORTANT***** as it requires customization on your part
 *
 * Wifi data from a blynk server triggers configurable tagger settings
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
 * 
 */
//****************************************************************
// libraries to include:
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//****************************************************************
#define SERIAL1_RXPIN 16 // TO BRX TX and BLUETOOTH RX
#define SERIAL1_TXPIN 17 // TO BRX RX and BLUETOOTH TX
bool AllowTimeout = true; // if true, this enables automatic deep sleep of esp32 device if wifi or blynk not available on boot
int BaudRate = 57600; // 115200 is for GEN2/3, 57600 is for GEN1, this is set automatically based upon user input
char auth[] = "BRX-Taggers"; // you should get auth token in the blynk app. use the one for "configurator"
//char auth[] = "wDEwTa0gq3UG4uBVamw1-2Ta5XDZFTjr"; // You should get Auth Token in the Blynk App. Go to the Project Settings (nut icon).


//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//*********** YOU NEED TO CHANGE INFO IN HERE FOR EACH GUN!!!!!!***********
int GunID = 0; // this is the gun or player ID, each esp32 needs a different one, set "0-63"
char GunName[] = "GUN#1"; // used for OTA id recognition on network
char ssid[] = "JEDGE"; // this is needed for your wifi credentials
char pass[] = "9165047812"; // this is needed for your wifi credentials
char server[] = "10.10.0.67"; // this is the ip address of your local server (PI or PC)
int GunGeneration = 2; // change to gen 1, 2, 3
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
//******************* IMPORTANT *********************
BlynkTimer CheckBlynkConnection; // created a timer object used for checking blynk server connection status

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
int SetVol=65; // set tagger volume adjustment, default is 65
int CurrentWeapSlot; // used for indicating what weapon slot is being used, primarily for unlimited ammo
int ReloadType; // used for unlimited ammo... maybe 10 is for unlimited

int Deaths = 0; // death counter
int Team=0; // team selection used when allowed for custom configuration
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
int UNLIMITEDAMMO = 1; // used to trigger ammo auto replenish if enabled
bool OutofAmmoA = false; // trigger for auto respawn of ammo weapon slot 0
bool OutofAmmoB = false; // trigger for auto respawn of ammo weapon slot 1
String AudioSelection; // used to play stored audio files with tagger FOR SERIAL CORE
String AudioSelection1; // used to play stored audio files with tagger FOR BLE CORE

int lastTaggedPlayer = -1;  // used to capture player id who last shot gun, for kill count attribution
int lastTaggedTeam = -1;  // used to captures last player team who shot gun, for kill count attribution

// used to send to ESP8266 for LCD display
int ammo1 = 0;
int ammo2 = 0;
int weap = 0;
int health = 45;
int armor = 70;
int shield =70;
int SpecialWeapon = 0;

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
bool FAKESCORE = false; // used for score testing do not use except for testing
bool IRDEBUG = false; // used to enabling IR tag data send to esp8266
bool IRTOTRANSMIT = false; // used to enable transmitting of IR data to ESP8266
bool ENABLEOTAUPDATE = false; // enables the loop for updating OTA
bool INITIALIZEOTA = false; // enables the object to disable BLE and enable WiFi
bool SPECIALWEAPON = false;
bool HASFLAG = false; // used for capture the flag
bool Connected2Blynk = false;

long startScan = 0; // part of BLE enabling

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED

bool WEAP = false; // not used anymore but was used to auto load gun settings on esp boot

//*****************************************************************************************
//***************************************************************
// Blynk Functions and Objects to configure settings on brx
// these assign a values and or enable emidiate functions to send to the brx
BLYNK_WRITE(V0) { // Sets Weapon Slot 0
int b=param.asInt();
  if (INGAME==false){
    if (b == 1) {
      settingsallowed=1; 
      AudioSelection="VA5F";
      SetSlotA=100;
      Serial.println("Weapon Slot 0 set to Manual");
    }
    if (b > 1 && b < 50) {
      SetSlotA=b-1;
      Serial.println("Weapon Slot 0 set"); 
      if(SetSlotA < 10) {
        AudioSelection = ("GN0" + String(SetSlotA));
      }
      if (SetSlotA > 9) {
        AudioSelection = ("GN" + String(SetSlotA));
      }
    }
  AUDIO=true;
  }
}
BLYNK_WRITE(V1) {  // Sets Weapon Slot 1
  int b=param.asInt();
  if (INGAME==false){
    if (b==1) {
      settingsallowed=2; 
      AudioSelection="VA5F"; 
      SetSlotB=100; 
      Serial.println("Weapon Slot 1 set to Manual");
    }
    if (b>1 && b < 50) {
      SetSlotB=b-1; 
      Serial.println("Weapon Slot 1 set"); 
      if(SetSlotB < 10) {
        AudioSelection = ("GN0" + String(SetSlotB));
      }
      if (SetSlotB > 9) {
      AudioSelection = ("GN" + String(SetSlotB));
      }
    }
    AUDIO=true;
}
}
BLYNK_WRITE(V4) {// Sets Lives);
int b=param.asInt();
if (INGAME==false){
if (b==7) {SetLives = 32000; Serial.println("Lives is set to Unlimited"); AudioSelection="VA6V";}
if (b==6) {SetLives = 25; Serial.println("Lives is set to 25"); AudioSelection="VA0P";}
if (b==5) {SetLives = 15; Serial.println("Lives is set to 15"); AudioSelection="VA0F";}
if (b==4) {SetLives = 10; Serial.println("Lives is set to 10"); AudioSelection="VA0A";}
if (b==3) {SetLives = 5; Serial.println("Lives is set to 5"); AudioSelection="VA05";}
if (b==2) {SetLives = 3; Serial.println("Lives is set to 3"); AudioSelection="VA03";}
if (b==1) {SetLives = 1; Serial.println("Lives is set to 1"); AudioSelection="VA01";}
        AUDIO=true;
}
}
BLYNK_WRITE(V5) {// Sets Game Time
int b=param.asInt();
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
}
}
BLYNK_WRITE(V6) {// Sets Lighting-Ambience
int b=param.asInt();
  if (INGAME==false){
    if (b==1) {
      SetODMode=0;
      Serial.println("Outdoor Mode On"); 
      AudioSelection="VA4W";
    }
    if (b==2) {
      SetODMode=1; 
      Serial.println("Indoor Mode On"); 
      AudioSelection="VA3W";
    }
    if (b==3) {}
  AUDIO=true;
  }
}
BLYNK_WRITE(V7) {// Sets Team Modes
  int b=param.asInt();
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
       }
      if (GunID == B) {
        SetTeam=1;
      }
      if (GunID == C) {
        SetTeam=2;
      }
      A = A + 3;
      B = B + 3;
      C = C + 3;
    }
    A = 0;
    B = 1;
    C = 2;
    AudioSelection="VA1L";
  }    
  if (b==4) {
    Serial.println("Teams Mode Four Teams (every fourth player)");
    while (A < 64) {
      if (GunID == A) {
        SetTeam=0;
      }
      if (GunID == B) {
        SetTeam=1;
      }
      if (GunID == C) {
        SetTeam=2;
      }
      if (GunID == D) {
        SetTeam=3;
      }
      A = A + 4;
      B = B + 4;
      C = C + 4;
      D = D + 4;
    }
    A = 0;
    B = 1;
    C = 2;
    D = 3;
    AudioSelection="VA1L";
    }
  if (b==5) {
    Serial.println("Teams Mode Player's Choice"); 
    settingsallowed=3; 
    SetTeam=100; 
    AudioSelection="VA5E";
  } // this one allows for manual input of settings... each gun will need to select a team now
  AUDIO=true;
}
}
BLYNK_WRITE(V8) {// Sets Game Mode
int b=param.asInt();
if (INGAME==false){
if (b==1) {
        GameMode=1;
        Serial.println("Game mode set to Deathmatch");
        AudioSelection="VA26";
        }
if (b==2) {
        GameMode=2;
        Serial.println("Game mode set to Capture the Flag"); 
        AudioSelection="VA8P";
        }
if (b==3) {
        GameMode=3; 
        Serial.println("Game mode set to Assault");
        AudioSelection="VA8F";
        }
if (b==4) {
        GameMode=4;
        Serial.println("Game mode set to King of the Hill"); 
        AudioSelection="VA93";
        }
if (b==5) {
        GameMode=5; 
        Serial.println("Game mode set to Survival"); 
        AudioSelection="VA64";
        }
if (b==6) {
        GameMode=6; 
        Serial.println("Game mode set to Trouble in Terrorist Town");
        AudioSelection="VA5U";
        }
if (b==7) {
        GameMode=7; 
        Serial.println("Game mode set to You only live twice");
        AudioSelection="VA8I";
        }
if (b==8) {
        GameMode=8; 
        Serial.println("Game mode set to One Shot Kills");
        AudioSelection="VA8K";
        }
if (b==9) {
        GameMode=9;
        Serial.println("Game mode set to Gun Game");
        AudioSelection="VA9T";
        }
if (b==10) {
        GameMode=10; 
        Serial.println("Game mode set to One Domination"); 
        AudioSelection="VA21";
        }
if (b==11) {
        GameMode=11; 
        Serial.println("Game mode set to Battle Royale"); 
        AudioSelection="VA8J";
        }
        AUDIO=true;
}
}
BLYNK_WRITE(V9) {// Sets Respawn Mode
int b=param.asInt();
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
}
}
BLYNK_WRITE(V10) {// Sets Delayed Start Time
int b=param.asInt();
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
}
}
BLYNK_WRITE(V11) { // Sends Request for locking out tagger
int b=param.asInt(); // set up a temporary object based variable to assign as the incoming data
if (b == GunID) {
        //SyncScores();
        Serial.println("Request Recieved to Sync Scoring");
        AudioSelection="VA91";
        }
                AUDIO=true;

}
BLYNK_WRITE(V12) {// Sets Player Gender
int b=param.asInt();
if (INGAME==false){
if (b==0) {
        SetGNDR=0; 
        Serial.println("Gender set to Male");
        AudioSelection="V3I";
        }
if (b==1) {
        SetGNDR=1;
        Serial.println("Gender set to Female");
        AudioSelection="VBI";
        }
        AUDIO=true;
}
}
BLYNK_WRITE(V13) {// Sets Ammo Settings
int b=param.asInt();
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
}
}
BLYNK_WRITE(V14) {// Sets Friendly Fire
int b=param.asInt();
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
}
}
BLYNK_WRITE(V15) {// Sets Volume of taggers
int b=param.asInt();
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

   }
}
BLYNK_WRITE(V16) {// Start/Ends a game
int b=param.asInt();
if (b==0) {
        GAMEOVER=true; 
        Serial.println("ending game");
        }
      // enable audio notification for changes
if (b==1) {
        GAMESTART=true; 
        Serial.println("starting game");
        if (SetTeam == 100) {
          SetTeam=Team;
          }
        }
                AUDIO=true;

}
BLYNK_WRITE(V17) {// Locks out Tagger power on/off
int b=param.asInt();
//if (b==1) {Serial.println("Manual Power Switch Override Engaged"); digitalWrite(PowerPin, HIGH);}
//if (b==0) {Serial.println("Manual Power Switch Override Dis-Engaged"); digitalWrite(PowerPin, LOW);}
}
BLYNK_WRITE(V18) {// tagger control engaged
int b=param.asInt();
if (b==1) {
  Serial.println("locking out player buttons");
  AudioSelection1="VA20";
  AUDIO1 = true;
}
}
BLYNK_WRITE(V19) {// Sleep ESP32 BLE device
int b=param.asInt();
if (b==1) {
        esp_deep_sleep_start();
        }
}
//**************************************************************

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
  if (tokenStrings[6] == "1" && tokenStrings[2] == "0") { // we determined that it is a custom tag for game interaction
    Serial.println("received a game interaction tag");
    // now that we know weve got a special tag, what to do with it...
    // respawning tag
    if (tokenStrings[7] == "3") { // we just determined that it is a respawn tag
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
    // objective tag
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
void InitializeOTAUpdater() {
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
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
  INITIALIZEOTA = false;
}
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
  if (UNLIMITEDAMMO==3){
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
  }
  if (UNLIMITEDAMMO==2) {
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
  }
  if (UNLIMITEDAMMO==1) {
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
  }
}
//*****************************************************************************************
// In Game Weapon Upgrade
void LoadSpecialWeapon() {
  if(SpecialWeapon == 1) {
    Serial.println("Weapon 0 set to Unarmed"); 
    sendString("$WEAP,0,*");
    } // cleared out weapon 0
  if(SpecialWeapon == 2) {
    Serial.println("Weapon 0 set to AMR"); 
    sendString("$WEAP,0,,100,0,3,18,0,,,,,,,,360,850,14,56,1400,0,7,100,100,,0,,,S07,D20,D19,,D04,D03,D21,D18,,,,,14,28,75,,*");
    }
  if(SpecialWeapon == 3) {
    Serial.println("Weapon 0 set to Assault Rifle");
    sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,100,850,32,384,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,192,75,,*");
    }
  if(SpecialWeapon == 4) {
    Serial.println("Weapon 0 set to Bolt Rifle"); 
    sendString("$WEAP,0,,100,0,3,13,0,,,,,,,,225,850,18,180,2000,0,7,100,100,,0,,,R12,,,,D04,D03,D02,D18,,,,,18,90,75,,*");
    }
  if(SpecialWeapon == 5) {
    Serial.println("Weapon 0 set to BurstRifle");
    sendString("$WEAP,0,,100,0,3,9,0,,,,,,,,75,850,36,216,1700,0,9,100,100,275,0,,,R18,,,,D04,D03,D02,D18,,,,,36,108,75,,*");
    }
  if(SpecialWeapon == 6) {
    Serial.println("Weapon 0 set to ChargeRifle");
    sendString("$WEAP,0,,100,8,0,100,0,,,,,,,,1250,850,100,200,2500,0,14,100,100,,14,,,E03,C15,C17,,D30,D29,D37,A73,C19,C04,20,150,100,100,75,,*");
    }
  if(SpecialWeapon == 7) {
    Serial.println("Weapon 0 set to Energy Launcher"); 
    sendString("$WEAP,0,,100,9,3,115,0,,,,,,,,360,850,1,6,1400,0,0,100,100,,0,,,J15,,,,D14,D13,D12,D18,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 8) {
    Serial.println("Weapon 0 set to Energy Rifle");
    sendString("$WEAP,0,,100,0,0,9,0,,,,,,,,90,850,300,600,2400,0,0,100,100,,6,,,E12,,,,D17,D16,D15,A73,D122,,,,300,300,75,,*");
    }
  if(SpecialWeapon == 9) {
    Serial.println("Weapon 0 set to Force Rifle");
    sendString("$WEAP,0,,100,0,1,9,0,,,,,,,,100,850,36,144,1700,0,9,100,100,250,0,,,R23,D20,D19,,D23,D22,D21,D18,,,,,36,72,75,,*");
    }
  if(SpecialWeapon == 10) {
    Serial.println("Weapon 0 set to Ion Sniper"); 
    sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1000,850,2,12,2000,0,7,100,100,,0,,,E07,D32,D31,,D17,D16,D15,A73,,,,,2,6,75,,*");
    }
  if(SpecialWeapon == 11) {
    Serial.println("Weapon 0 set to Laser Cannon"); 
    sendString("$WEAP,0,,100,0,0,115,0,,,,,,,,1500,850,4,8,2000,0,3,100,100,,0,,,C06,C11,,,D17,D16,D15,A73,,,,,4,4,75,,*");
    }
  if(SpecialWeapon == 12) {
    Serial.println("Weapon 0 set to Plasma Sniper"); 
    sendString("$WEAP,0,2,100,0,0,80,0,,,,,,80,80,225,850,10,80,2000,0,7,100,100,,30,,,E17,,,,D35,D34,D36,A73,D122,,,,10,40,75,40,*");
    }
  if(SpecialWeapon == 13) {
    Serial.println("Weapon 0 set to Rail Gun"); 
    sendString("$WEAP,0,0,100,6,0,115,0,,,,,,,,1200,850,1,6,2400,0,2,100,100,,0,,,C03,C08,,,D36,D35,D34,A73,,,,,1,3,75,,*");
    }
  if(SpecialWeapon == 14) {
    Serial.println("Weapon 0 set to Rocket Launcher"); 
    sendString("$WEAP,0,2,100,10,0,115,0,,,,,,115,80,1000,850,2,8,1200,0,7,100,100,,0,,,C03,,,,D14,D13,D12,D18,,,,,2,4,75,30,*");
    }
  if(SpecialWeapon == 15) {
    Serial.println("Weapon 0 set to Shotgun"); 
    sendString("$WEAP,0,2,100,0,0,45,0,,,,,,70,80,900,850,6,24,400,2,7,100,100,,0,,,T01,,,,D01,D28,D27,D18,,,,,6,12,75,30,*");
    }
  if(SpecialWeapon == 16) {
    Serial.println("Weapon 0 set to SMG"); 
    sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,90,850,72,288,2500,0,0,100,100,,5,,,G03,,,,D26,D25,D24,D18,D11,,,,72,144,75,,*");
    }
  if(SpecialWeapon == 17) {
    Serial.println("Weapon 0 set to Sniper Rifle"); 
    sendString("$WEAP,0,,100,0,1,80,0,,,,,,,,300,850,4,24,1700,0,7,100,100,,0,,,S16,D20,D19,,D04,D03,D21,D18,,,,,4,12,75,,*");
    }
  if(SpecialWeapon == 18) {
    Serial.println("Weapon 0 set to Stinger"); 
    sendString("$WEAP,0,,100,0,0,15,0,,,,,,,,120,850,18,72,1700,0,0,100,100,,0,,,E11,,,,D17,D16,D15,A73,,,,,18,36,75,,*");
    }
  if(SpecialWeapon == 19) {
    Serial.println("Weapon 0 set to Suppressor"); 
    sendString("$WEAP,0,,100,0,0,8,0,,,,,,,,75,850,48,288,2000,0,0,100,100,,0,2,50,Q06,,,,D26,D25,D24,D18,,,,,48,144,75,,*");
    }
  if(SpecialWeapon == 99) {
    Serial.println("Flag Capture, Gun becomes the flag"); 
    sendString("$WEAP,0,*"); 
    sendString("$WEAP,1,*");
    sendString("$WEAP,1,*"); 
    sendString("$BMAP,3,2,,,,,*");
    sendString("$WEAP,3,,100,2,0,0,0,,,,,,,,100,850,32,32768,1400,0,0,100,100,,0,,,R01,,,,D04,D03,D02,D18,,,,,32,32768,75,,*");
    } 
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
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
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
  sendString("$SIR,0,0,,1,0,0,1,,*");
  sendString("$SIR,0,1,,36,0,0,1,,*");
  sendString("$SIR,0,3,,37,0,0,1,,*");
  sendString("$SIR,1,0,,10,0,0,1,,*");
  sendString("$SIR,2,1,VA8C,11,0,0,1,,*");
  sendString("$SIR,3,0,VA16,13,0,0,1,,*");
  sendString("$SIR,6,0,H02,1,0,90,1,40,*");
  sendString("$SIR,8,0,,38,0,0,1,,*");
  sendString("$SIR,9,3,,24,10,0,,,*");
  sendString("$SIR,10,0,X13,1,0,100,2,60,*");
  sendString("$SIR,11,0,VA2,28,0,0,1,,*"); // tear gas, out of possible ir recognitions, max = 14
  sendString("$SIR,13,1,H57,1,0,0,1,,*");
  sendString("$SIR,13,0,H50,1,0,0,1,,*");
  sendString("$SIR,13,3,H49,1,0,100,0,60,*");
  //sendString("$SIR,15,0,,14,0,0,1,,");
  //sendString("$SIR,14,0,,14,0,0,1,,");
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
  //sendString("$PLAY,VA84,4,5,,,,,*"); // plays a ten second countdown
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
  Serial.println("Delayed Start Complete, should be in game play mode now");
  GameStartTime=millis();
  GAMEOVER=false;
  INGAME=true;
}

//******************************************************************************************

// process used to send string properly to gun... splits up longer strings in bytes of 20
// to make sure gun understands them all... not sure about all of what does what below...
// had some major help from Sri Lanka Guy!
void sendString(String value) {
  const char * c_string = value.c_str();
  Serial.print("sending ");
  if (GunGeneration == 1) {
    Serial.print("sending: ");
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
    Serial.print("sending: ");
    Serial.println(c_string);
  }
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
  if(SetGNDR == 0) {
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,V33,V3I,V3C,V3G,V3E,V37,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
  else {
    sendString("$PSET,"+String(GunID)+","+String(SetTeam)+",45,70,70,50,,H44,JAD,VB3,VBI,VBC,VBG,VBE,VB7,H06,H55,H13,H21,H02,U15,W71,A10,*");
    }
}
//******************************************************************************************
//******************************************************************************************
// ends game... thats all
void gameover() {
  sendString("$STOP,*"); // stops everything going on
  sendString("$CLEAR,*"); // clears out anything stored for game settings
  sendString("$PLAY,VS6,4,6,,,,,*"); // says game over
  GAMEOVER = false;
  INGAME=false;
  COUNTDOWN1=false;
  COUNTDOWN2=false;
  COUNTDOWN3=false;
}
//******************************************************************************************
// as the name says... respawn a player!
void respawnplayer() {
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
  }
  }
  Serial.println("Respawning Player");
  //sendString("$WEAP,0,*"); // cleared out weapon 0
  //sendString("$WEAP,1,*"); // cleared out weapon 1
  //sendString("$WEAP,4,*"); // cleared out melee weapon
  //sendString("$WEAP,3,*"); // cleared out melee weapon
  weaponsettingsA();
  weaponsettingsB();
  weaponsettingsC();
  sendString("$WEAP,4,1,90,13,1,90,0,,,,,,,,1000,100,1,0,0,10,13,100,100,,0,0,,M92,,,,,,,,,,,,1,0,20,*"); // this is default melee weapon for rifle bash
  sendString("$GLED,,,,5,,,*"); // changes headset to tagged out color
  sendString("$SPAWN,,*"); // respawns player back in game
  Serial.println("Player Respawned");
  RESPAWN = false;
}
//****************************************************************************************
// disclaimer... incomplete... 
// this function will be used when a player is eliminated and needs to respawn off of a base
// or player signal to respawn them... a lot to think about still on this and im using auto respawn 
// for now untill this is further thought out and developed
void ManualRespawnMode() {
  sendString("$STOP,*"); // this is essentially ending the game for player... need to rerun configurator or use a different command
  sendString("$SPAWN,,*");
  sendString("$HLOOP,2,1200,*");
  AudioSelection1 = "VA54";
  AUDIO1 = true;
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
    sendString("$PLAY,"+AudioSelection+",4,6,,,,,*");
    AUDIO = false;
    AudioSelection = "NULL";
      }
  if (AUDIO1) {
    sendString("$PLAY,"+AudioSelection1+",4,6,,,,,*");
    AUDIO1=false; 
    AudioSelection1 = "NULL";
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
  Serial.println();
  Serial.println("JEDGE is taking over the BRX");
  sendString("$CLEAR,*");
  sendString("$START,*");
  delay(100);
  sendString("$PLAY,VA20,4,6,,,,,*");
}
//**********************************************************************************************
void SyncScores() {
  // create a string that looks like this: 
  // (Player ID, token 0), (Player Team, token 1), (Player Objective Score, token 2) (Team scores, tokens 3-8), (player kill counts, tokens 9-72 
  String LCDText = String(GunID)+","+String(SetTeam)+","+String(CompletedObjectives)+","+String(TeamKillCount[0])+","+String(TeamKillCount[1])+","+String(TeamKillCount[2])+","+String(TeamKillCount[3])+","+String(TeamKillCount[4])+","+String(TeamKillCount[5])+","+String(PlayerKillCount[0])+","+String(PlayerKillCount[1])+","+String(PlayerKillCount[2])+","+String(PlayerKillCount[3])+","+String(PlayerKillCount[4])+","+String(PlayerKillCount[5])+","+String(PlayerKillCount[6])+","+String(PlayerKillCount[7])+","+String(PlayerKillCount[8])+","+String(PlayerKillCount[9])+","+String(PlayerKillCount[10])+","+String(PlayerKillCount[11])+","+String(PlayerKillCount[12])+","+String(PlayerKillCount[13])+","+String(PlayerKillCount[14])+","+String(PlayerKillCount[15])+","+String(PlayerKillCount[16])+","+String(PlayerKillCount[17])+","+String(PlayerKillCount[18])+","+String(PlayerKillCount[19])+","+String(PlayerKillCount[20])+","+String(PlayerKillCount[21])+","+String(PlayerKillCount[22])+","+String(PlayerKillCount[23])+","+String(PlayerKillCount[24])+","+String(PlayerKillCount[25])+","+String(PlayerKillCount[26])+","+String(PlayerKillCount[27])+","+String(PlayerKillCount[28])+","+String(PlayerKillCount[29])+","+String(PlayerKillCount[30])+","+String(PlayerKillCount[31])+","+String(PlayerKillCount[32])+","+String(PlayerKillCount[33])+","+String(PlayerKillCount[34])+","+String(PlayerKillCount[35])+","+String(PlayerKillCount[36])+","+String(PlayerKillCount[37])+","+String(PlayerKillCount[38])+","+String(PlayerKillCount[39])+","+String(PlayerKillCount[40])+","+String(PlayerKillCount[41])+","+String(PlayerKillCount[42])+","+String(PlayerKillCount[43])+","+String(PlayerKillCount[44])+","+String(PlayerKillCount[45])+","+String(PlayerKillCount[46])+","+String(PlayerKillCount[47])+","+String(PlayerKillCount[48])+","+String(PlayerKillCount[49])+","+String(PlayerKillCount[50])+","+String(PlayerKillCount[51])+","+String(PlayerKillCount[52])+","+String(PlayerKillCount[53])+","+String(PlayerKillCount[54])+","+String(PlayerKillCount[55])+","+String(PlayerKillCount[56])+","+String(PlayerKillCount[57])+","+String(PlayerKillCount[58])+","+String(PlayerKillCount[59])+","+String(PlayerKillCount[60])+","+String(PlayerKillCount[61])+","+String(PlayerKillCount[62])+","+String(PlayerKillCount[63]);
  Serial.println(LCDText);
  //SerialLCD.println(LCDText);
  Serial.println("Sent LCD data to ESP8266");
}
//********************************************************************************************
// This main game object
void MainGame() {
  if (AUDIO) {
        Audio();
      }
  if (AUDIO1) {
        Audio();
      }
      if (INGAME == false) {
      if (TAGGERUPDATE1) {
        TAGGERUPDATE=false; 
        Serial.println("Disabled LCD Data Send");
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
      if (GAMESTART) {
        // need to turn off the trigger audible selections if a player didnt press alt fire to confirm
        GETSLOT0=false; 
        GETSLOT1=false; 
        GETTEAM=false;
        gameconfigurator(); // send all game settings, weapons etc. 
        delaystart(); // enable the start based upon delay selected
        GAMESTART = false; // disables the trigger so this doesnt loop/
        PlayerLives=SetLives; // sets configured lives from settings received
        GameTimer=SetTime; // sets timer from input of esp8266
        Deaths = 0;
      }
      }
      // In game checks and balances to execute actions for in game functions
      if (INGAME) {
        long ActualGameTime = millis() - GameStartTime;
        long GameTimeRemaining = GameTimer - ActualGameTime;
        if (ActualGameTime > GameTimer) {
          GAMEOVER=true; 
          Serial.println("game time expired");
          }
        if (OutofAmmoA) {
          weaponsettingsA();      
          Serial.println("Weapon Slot 0 has been reloaded, disabling reload");
          OutofAmmoA=false;
        }
        if (OutofAmmoB) {
          weaponsettingsB();
          Serial.println("Weapon Slot 1 has been reloaded, disabling reload");
          OutofAmmoB=false;
        }
        if (RESPAWN) { // checks if auto respawn was triggered to respawn a player
          respawnplayer(); // respawns player
        }
        if (MANUALRESPAWN) { // checks if manual respawn was triggered to respawn a player
          ManualRespawnMode();
        }
        if (GAMEOVER) { // checks if something triggered game over (out of lives, objective met)
          gameover(); // runs object to kick player out of game
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
        if (SPECIALWEAPON) {
          SPECIALWEAPON = false;
          LoadSpecialWeapon();
        }
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
  int stringnumber = 0;
  while (stringnumber < index) {
    Serial.print("Token " + String(stringnumber) + ": ");
    Serial.println(tokenStrings[stringnumber]);
    stringnumber++;
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
            if (Team==5) {
              Team=0; 
              STATUSCHANGE=true; 
              AudioSelection1="VA13"; 
              AUDIO1=true; Serial.println("team changed from 5 to 0");
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
              AUDIO1=true; Serial.println("team changed from 3 to 0");
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
      TagPerks();
      if (IRDEBUG) {
        IRTOTRANSMIT = true; 
        Serial.println("IR to Transmit Enabled");
        }
      TAGGERUPDATE=true;
      Serial.println("Enabled LCD Data Send");
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
       TAGGERUPDATE=true;
       //Serial.println("Enabled LCD Data Send");    
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
      TAGGERUPDATE=true;
      Serial.println("Enabled LCD Data Send");
      health = tokenStrings[1].toInt(); // setting variable to be sent to esp8266
      armor = tokenStrings[2].toInt(); // setting variable to be sent to esp8266
      shield = tokenStrings[3].toInt(); // setting variable to be sent to esp8266
      if ((tokenStrings[1] == "0") && (tokenStrings[2] == "0") && (tokenStrings[3] == "0")) { // player is dead
        if (HASFLAG) { HASFLAG = false; AudioSelection1 = "VA2I"; AUDIO1 = true;}
        PlayerKillCount[lastTaggedPlayer]++; // adding a point to the last player who killed us
        TeamKillCount[lastTaggedTeam]++; // adding a point to the team who caused the last kill
        PlayerLives--; // taking our preset lives and subtracting one life then talking about it on the monitor
        Deaths++;
        AudioSelection1="KBP"+String(lastTaggedPlayer);
        AUDIO1=true;
        Serial.println("Lives Remaining = " + String(PlayerLives));
        Serial.println("Killed by: " + String(lastTaggedPlayer) + " on team: " + String(lastTaggedTeam));
        Serial.println("Team: " + String(lastTaggedTeam) + "Score: " + String(TeamKillCount[lastTaggedTeam]));
        Serial.println("Player: " + String(lastTaggedPlayer) + " Score: " + String(PlayerKillCount[lastTaggedPlayer]));
        Serial.println("Death Count: " + String(Deaths));
        if (PlayerLives > 0 && SetRSPNMode < 9) { // doing a check if we still have lives left after dying
          RESPAWN = true;
          Serial.println("Auto respawn enabled");
        }
        if (PlayerLives > 0 && SetRSPNMode == 9) { // doing a check if we still have lives left after dying
          MANUALRESPAWN = true;
          Serial.println("Manual respawn enabled");
        }
        if (PlayerLives == 0) {
          GAMEOVER=true; 
          AudioSelection1="VA46"; 
          AUDIO1=true; 
          Serial.println("lives depleted");
          }
      }
    }
  }
//******************************************************************************************
void CheckConnection() {
  Connected2Blynk = Blynk.connected();
  if (!Connected2Blynk) {
    Serial.println("Not connected to Blynk server");
    Blynk.connect(3333);
  }
  else {
    Serial.println("Connected to Blynk Server");
  }
}
//******************************************************************************************
void runOTAupdater() {
  ArduinoOTA.handle();
  //loop to blink without delay
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    ledState = not(ledState);
    // set the LED with the ledState of the variable:
    digitalWrite(led,  ledState);
  }
}
//******************************************************************************************
// ***********************************  DIRTY LOOP  ****************************************
// *****************************************************************************************
void loop1(void *pvParameters) {
  Serial.print("Dirty loop running on core ");
  Serial.println(xPortGetCoreID());   
  //auto control brx start up:
  InitializeJEDGE();
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
// **************************************  BLYNK LOOP  *************************************
// *****************************************************************************************
void loop2(void *pvParameters) {
  Serial.print("Blynk loop running on core ");
  Serial.println(xPortGetCoreID());
  while (1) { // starts the forever loop
    // place main blynk functions in here, everything wifi related:
    if (Connected2Blynk) {
      Blynk.run();
    } else {
      // possibly do something else while not connected
    }
    CheckBlynkConnection.run();
    //ArduinoOTA.handle(); // for OTA
    delay(1); // this has to be here or the esp32 will just keep rebooting
  }
}
//******************************************************************************************
//*************************************** SET UP *******************************************
//******************************************************************************************
void setup() {
  Serial.begin(115200); // set serial monitor to match this baud rate
  Serial.println("Starting Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  int wifiattempts = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > 120000 && AllowTimeout == true) {
      Serial.println("Too many attempts to reach wifi network");
      Serial.println("Enabling Deep Sleep");
      delay(500);
      esp_deep_sleep_start();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  InitializeOTAUpdater();
  Serial.println("Connecting to Blynk Server");
  Blynk.config(auth, server, 8080); // used to connect to blynk cloud server
  //Blynk.config(auth, IPAddress(192,168,1,169), 8080); // used to connect to local blynk server
  int attemptcounter = 0;
  Blynk.connect(3333); // timeout set to ten seconds then continue without blynk
  while (!Connected2Blynk) {
     //wait until connected
     Connected2Blynk = Blynk.connect();
     attemptcounter++;
     Serial.println("Blynk Connection Attempts = " + String(attemptcounter));
     if (attemptcounter > 10 && AllowTimeout == true) {
      Serial.println("Too many attempts to reach server on start up, putting device to sleep");
      Serial.println("Enabling Deep Sleep");
      delay(500);
      esp_deep_sleep_start();
     }
  }
  Serial.println("");
  CheckBlynkConnection.setInterval(11000L, CheckConnection); // checking blynk connection
  Serial.println("Connected to Blynk Server");
  Serial.println("...");
  Serial.println();
  Serial.println("Initializing serial output settings, Tagger Generation set to Gen: " + String(GunGeneration));
  if (GunGeneration > 1) {
    BaudRate = 115200;
  }
  Serial.println("Serial Buad Rate set for: " + String(BaudRate));
  Serial1.begin(BaudRate, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the serial pins for sending data to BRX
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  Serial.print("setup() running on core ");
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

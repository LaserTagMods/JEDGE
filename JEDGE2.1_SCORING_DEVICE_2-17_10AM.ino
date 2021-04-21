
#define BLYNK_PRINT Serial


#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V2);

// set up bridge widget on V1 of Device 1
// this creates the widget needed to send data to the controller
WidgetBridge bridge1(V1);
// set the bridge token


// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
//char auth[] = "BFtL4hUCqAGjQpe6CBabW_JslSV13CLg";
//char auth[] = "p8YBnsPWJ8MokfwUcHzRg4TaVcwuXQTr";
//char auth[] = "Master-Controller";
char auth[] = "LaJHPkah4L-AveBwJ4fywg_Hu8rbSbwx"; // updated master for jedge@jedge.com user
char taggers[] = "a5AVNp-BOw8SItRUo3JRsV67HoZrvkmH";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "JEDGE";
char pass[] = "9165047812";

// local blynk server ip address
//char server[] = "192.168.1.227"; // this is the ip address of your local server (PI or PC)
char server[] = "192.168.50.2"; // this is the ip address of your local server (PI or PC)

BLYNK_CONNECTED() {
  bridge1.setAuthToken(taggers); // Token of the taggers
  //bridge1.setAuthToken("XKzYNms8i6U21pZAVR3bZcBUw-byhscM"); // Token of the taggers
  //bridge1.setAuthToken("p8YBnsPWJ8MokfwUcHzRg4TaVcwuXQTr"); // Token of the taggers
  
}

int PlayerCount = 12;  // How many Taggers are you playing with? 
                       // they should be Identified as 0-63, 0-9, etc. depending on your count
                       // This is important to speed up syncing scores and not waste time waiting
                       // for non existing guns-players

// Variables needed:
int ToTaggers=0;
String tokenStrings[8];
String ScoreTokenStrings[73];
int PlayerDeaths[64];
int PlayerKills[64];
int PlayerObjectives[64];
int TeamKills[4];
int TeamObjectives[4];

int GameCounter = 0;

String readStr="0";

String TerminalInput; // used for sending incoming terminal widget text to Tagger


int ScoreRequestCounter = 0;

bool INGAME = false;
bool SCORESYNC = false;

long gametimer = 0;

// timers for running certain applications periodically with the blynk program
BlynkTimer UpdateClock; // created a timer object called "UpdateClock"
BlynkTimer RequestScores; // created a timer object called "RequestScores"

//******************************************************************
// object used to receive data from each configurator
BLYNK_WRITE(V0) {
    readStr = param.asStr(); // saves incoming data as a temporary string within this object
    Serial.println("printing data received: ");
    Serial.println(readStr);
    char *ptr = strtok((char*)readStr.c_str(), ","); // looks for commas as breaks to split up the string
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
      Serial.println("Converting String character "+String(count)+" to integer: "+String(Data[count]));
      count++;
    }
    Player=Data[0];
    Serial.println("Player ID: " + String(Player));
    Deaths = Data[3] + Data[4] + Data[5] + Data[6] + Data[7] + Data[8]; // added the total team kills to accumulate the number of deaths of this player
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
      Serial.println("Player " + String(j) + " Killed this player " + String(Data[p]) + " times, Player's new score is " + String(PlayerKills[j]));
      p++;
      j++;
    }
    Team = Data[1]; // setting the player's team for accumulation of player objectives
    PlayerObjectives[Player] = Data[2]; // converted temporary data storage to main memory for score reporting of player objectives
    Serial.println("Player Objectives completed: "+String(Data[2]));
    TeamObjectives[Team] = TeamObjectives[Team] + Data[2]; // added this player's objective score to his team's objective score
    }

BLYNK_WRITE(V5) {// Sets Game Time
int b=param.asInt();
if (INGAME==false){
if (b==1) {
        gametimer=60000; 
        Serial.println("Game time set to 1 minute");
        }
if (b==2) {
        gametimer=300000;
        Serial.println("Game time set to 5 minute");
        }
if (b==3) {
        gametimer=600000; 
        Serial.println("Game time set to 10 minute");
        }
if (b==4) {
        gametimer=900000;
        Serial.println("Game time set to 15 minute"); 
        
        }
if (b==5) {
        gametimer=1200000; 
        Serial.println("Game time set to 20 minute");
        }
if (b==6) {
        gametimer=1500000;
        Serial.println("Game time set to 25 minute");
        }
if (b==7) {
        gametimer=1800000; 
        Serial.println("Game time set to 30 minute");
        }
if (b==8) {
        gametimer=2000000000; 
        Serial.println("Game time set to Unlimited");
        }
}
}
// object used to que score reporting requests from each gun
BLYNK_WRITE(V127) {
  int b=param.asInt();
  if (b==1) {
    ClearScores();
    SCORESYNC = true; // enables object to request scores
    INGAME = false; // disables the counter clock
    Serial.println("Commencing Score Sync Process");
    ScoreRequestCounter = 0;
    }
}

BLYNK_WRITE(V16) {
  int b=param.asInt();
  if (b==1) {
    INGAME = true; // enables game counter
    gametimer = 0; // reset game timer
    ClearScores(); // need to reset scores from previous game
  }
  if (b == 0){
    INGAME = false; // disables game counter
  }
}

BLYNK_WRITE(V2) { // Terminal Widget
  TerminalInput = param.asStr(); // storing the input from widget as a string to use elswhere if needed
  // if you type "ping" into Terminal Widget, it will respond "pong"
  Serial.println("Received something from widget terminal");
  Serial.println(TerminalInput);
  if(String("ping") == param.asStr()) {
    //int terminaldelay = 1000 * GunID;
    //delay(terminaldelay);
    terminal.println("You said: 'ping'");
    terminal.print("I said: 'pong' - from: Scoring Device");
  } else {
    // process command as a request to command brx taggers
    terminal.print("You said: ");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }
  //ensure everything is sent
  terminal.flush();
}
//*****************************************************************************************************************************

void ClearScores() {
  int playercounter = 0;
  while (playercounter < 64) {
    PlayerKills[playercounter] = 0;
    playercounter++;
  }
  int teamcounter = 0;
  while (teamcounter < 4) {
    TeamKills[teamcounter] = 0;
    TeamObjectives[teamcounter] = 0;
    teamcounter++;
  }
  Serial.println("Reset all stored scores from previous game");
}

void updateblynkapp() {    
    // send scores to blynk accumulator device over bridge to be summed and then posted to blynk server/app
    // designating certain pins for score accumulation. 
    // Pins 0-63 are for player kill counts, this sends all player scores
    Serial.println("Sending All Scores to Blynk App");

    // send player scores to app
    terminal.println();
    terminal.println();
    terminal.println("****************************************");
    terminal.println("*********   Game Number " + String(GameCounter) + "   ************");
    terminal.println("****************************************");
    GameCounter++;
    
    // player individual score reportings: 
    terminal.println();
    terminal.println("Individual Player Scores:");
    int terminalplayercounter = 0;
    while (terminalplayercounter < PlayerCount) {
      terminal.println("Player " + String(terminalplayercounter) + ": Kills: "+ String(PlayerKills[terminalplayercounter]) + " Deaths: " + String(PlayerDeaths[terminalplayercounter]) + " ObjPts: " + String(PlayerObjectives[terminalplayercounter]));
      terminalplayercounter++;
    }
    terminal.println();
    
    // team kill scores 
    terminal.println("Team Scores:");
    terminal.print("Red Team Kills: ");
    terminal.print(TeamKills[0]);
    terminal.print(" Objectives: ");
    terminal.println(TeamObjectives[0]);
    terminal.print("Blue Team Kills: ");
    terminal.print(TeamKills[1]);
    terminal.print(" Objectives: ");
    terminal.println(TeamObjectives[1]);
    terminal.print("Yellow Team Kills: ");
    terminal.print(TeamKills[2]);
    terminal.print(" Objectives: ");
    terminal.println(TeamObjectives[2]);
    terminal.print("Green Team Kills: ");
    terminal.print(TeamKills[3]);
    terminal.print(" Objectives: ");
    terminal.println(TeamObjectives[3]);
    terminal.println();

    // now we calculate and send leader board information
    terminal.println("Game Highlights:");
    terminal.println();
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
      terminal.println("Most Deadliest, Players with the most kills:");
      terminal.println("1st place: Player " + String(KillsLeader[0]) + " with " + String(LeaderScore[0]));
      terminal.println("2nd place: Player " + String(KillsLeader[1]) + " with " + String(LeaderScore[1]));
      terminal.println("3rd place: Player " + String(KillsLeader[2]) + " with " + String(LeaderScore[2]));
      terminal.println();
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
      terminal.println("The Dominator, Players with the most objective points:");
      terminal.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      terminal.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      terminal.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      terminal.println();
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
      terminal.println("Easy Target, Players with the most deaths:");
      terminal.println("1st place: Player " + String(DeathsLeader[0]) + " with " + String(LeaderScore[0]));
      terminal.println("2nd place: Player " + String(DeathsLeader[1]) + " with " + String(LeaderScore[1]));
      terminal.println("3rd place: Player " + String(DeathsLeader[2]) + " with " + String(LeaderScore[2]));
      terminal.println();
      // now get the player's scores back where they should be:
      PlayerDeaths[DeathsLeader[0]] = LeaderScore[0];
      PlayerDeaths[DeathsLeader[1]] = LeaderScore[1];
      PlayerDeaths[DeathsLeader[2]] = LeaderScore[2];
      terminal.flush();
    }

void GameTimer() { // runs when a game is started
  gametimer--;
  Blynk.virtualWrite(V120, gametimer);
  Serial.println("running game timer and added one second to the clock");
}

void RequestingScores() {
  bridge1.virtualWrite(V11, ScoreRequestCounter); // sending the Player Score Request number to all players/taggers
  if (ScoreRequestCounter < PlayerCount) {
    Serial.println("Sent Request for Score to Player "+String(ScoreRequestCounter)+" out of "+String(PlayerCount));
    ScoreRequestCounter++;
  } else {
    SCORESYNC = false; // disables the score requesting object until a score is reported back from a player
    ScoreRequestCounter = 0;
    updateblynkapp();
    Serial.println("All Scores Requested, Closing Score Request Process, Resetting Counter");
  }
}
//****************************************************************
//*****************************************************************************************
//*****************************************************************************************

void setup()
{
  // put your setup code here, to run once:
  // Debug console
  Serial.begin(115200);

  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,117), 8080);
  //Blynk.begin(auth, ssid, pass);
  Blynk.begin(auth, ssid, pass, server, 8080);

  // Clear the terminal content
  terminal.clear();

  // timer settings (currently not used)
  UpdateClock.setInterval(1000L, GameTimer); // Sending game time updates to blynk app
  RequestScores.setInterval(200L, RequestingScores); // requesting player scores via bridge
}

void loop()
{
  Blynk.run();
  if (INGAME) {UpdateClock.run();}
  if (SCORESYNC) {RequestScores.run();}
}

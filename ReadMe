# Blynk---ESP32
Blynk Esp32 Configurator
//***************************************************
//************   General      Stuff   ***************
//***************************************************

install esp32 boards to your arduino IDE:
https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

install esp8266 boards to your arduino IDE:
https://arduino-esp8266.readthedocs.io/en/latest/installing.html

Tip: when installing the JSON resources, you can return in the JSON library references to keep both the 8266, esp32 boards always available from the dropdown board selections

My boards Purchased:
https://www.aliexpress.com/item/32827149250.html?spm=a2g0s.9042311.0.0.6f5d4c4drgvrDP
https://www.aliexpress.com/item/32651747570.html?spm=a2g0s.9042311.0.0.6f5d4c4drgvrDP

Pi purchased/used:
https://www.amazon.ae/CanaKit-Raspberry-Starter-Premium-Black/dp/B07BCC8PK7

//***************************************************
//******** Step 1. Setting up Blynk App *************
//***************************************************
1. I recommend you using a blynk server on your pc or a raspberry pi
   this will make it so you dont have to ever buy energy credits from blynk
   it also is not dependent upon an internet connection
2. go to the github and open up the only jpeg image in the repository main folder: https://github.com/LaserTagMods/Blynk---ESP32/tree/master/Laser%20Tag%20Configurator%20with%20LCD
3. download and install blynk to your tablet/phone of choice
4. create account / login
5. determine if you will be using a local server and get that set up... see next section., otherwise proceed to next step
6. Once you are in blynk and ready to start, you can add a new project by scanning a qr in the upper corner by pressing the qr scann symbol. scan the jpeg and it will automatically upload the app into your server


//***************************************************
//*********   2.  Installing Blynk Server   *********
//***************************************************

https://github.com/blynkkk/blynk-server

0. remove old java stuff! (uninstall any existing java versions)
1. Install java environment
2. verify our ip address
3. run the blynk server!!!
4. setting up designated or static ip addresses with router
5. initial login on blynk server
6. building the app



1. install java environment:
 go to https://github.com/blynkkk/blynk-server#blynk-server
 verify if i have java already! java -version, this is done in command prompt
 download version 8 or 11 of java and install
 check version again, if not right, go to settings and fix the path!
 if you have java already, uninstall the old version and make sure you fix the path... the bin file folder in the java program folders

2. Verifiy Ip Address
	open command prompt
	then type ipconfig hit enter
	identify the network, wired or wireless
		look for ipv4 address... should be something like: 192.168.1.80

3. run blynk server!
	go to web and download it or open my file im giving you! here is the link https://github.com/blynkkk/blynk-server/releases/download/v0.41.12/server-0.41.12-java8.jar
	run the jar file in jav application... through command prompt!
		first get command prompt to the right file folder
		i copied the blynk server file to simplify name for execution
		enter in command prompt:
			"java -jar blynk.jar -dataFolder /server_data"
		login email for blynk is admin@blynk.cc and password is admin
	**** I made it easier for you though, just double click the run blynk server application!

4. setting static ip addresses:
	this process may vary but im using an asus router and the process is all the same for all asus routers.
	connect the device to the wifi network
	use a pc to go to the admin settings of the router, follow the ip address for the "gateway" to router
	log in with admin priveledges, if you dont know them, reset router and use admin/admin as user and password or see the manual
	identify connected clients and enable manual ip address assignment in the LAN settings for each device.

5. Initial login to blynk server
	pull it together now... we need to do two things and in this order!!!!
		first connect to the router or access point.. then run the blynk server.. in that order!
	then you can open the blynk app and login to the server...

**** Alternatively, install on a raspberry Pi *******
https://github.com/blynkkk/blynk-server

reference raspberry pi install instructions, then set it so it launches automatically by following instructions labeled: 
"Enabling server auto restart on unix-like systems"

//***************************************************
//********   Step 3. Prepping the BRX   *************
//***************************************************
The BRX has some anoying audio files and limited gun name conventions in the audio driver. 
Within the repository are new audio files that should be added to each BRX Audio folder. The files
Are GN01 through GN19. These are used by the Configurator to identify what gun players are selecting when manual
Weapon selection is enabled. This is highly recommended. Also it's Paul's voice which makes it kinda cool

Also, delete or rename the following audio file names to make them stop being annoying or interupting start up time

I just renamed them so i can recall them later if I want:
VA9U changed to XYZ1
JA9 changed to XYZ2

Others just have changed the extension from LTP to OLD, whatever works.

Keep in mind that if you remove the music, it does limit your ability to put the tagger into target mode, target mode requires a button held for an extended time before getting into game options.

You need to rename your BRX Gen2s in order for the esp32 to pair correctly. Make sure you know the name of the bluetooth module for each tagger and have it written down, on the gun, whatever. So when you get to section 4 below you have what you need. 

If you need to rename a tagger because of duplicate tagger bluetooth ID/Name, DO IT NOW!!! they all need unique names or your going to have pairing issues and configurators fighting over taggers. 

If you do not know how to rename the bluetooth ID, simply pair the tagger with your phone in callsign, but log in to callsign with a different account/name each time... yes this requires you to create an email for each tagger/callsign ID. sorry, but this is the long way to do it... the other way to do it is by sending the bluetooth tag to the tagger, which is a rough way to go.. I havent developed an easy way to do that... i guess I should have.. but it is what it is, I created emails for each tagger of mine a long time ago so its not something i really plan on doing for y'all. sorry, You may be able to pair a tagger with an android and then change the device name that way, or pair it with a PC and serial device to send a new name to it... if your not experienced with sending custom UART commands though... never mind... just create a bunch of gmail accounts and callsign accounts...

//***************************************************
//****  4. Preping Controller - scoring device ******
//***************************************************
Use ESP8266 mini or other is used for score tracking and game clock tracking as well
This is a non-essential device but if you want scoring reporting on the blynk app, it is essential

Check to see the authorization code for both the scoring device as well as the configurator. Both of these codes need to be copied and pasted and entered into the code...

Also make sure to adjust the quantity of taggers you have/using with configurators.

Watch these videos for tips and understand what im talking about above:
https://youtu.be/oSk3vjSn00U
https://youtu.be/WS5DaqnMdFM


//***************************************************
//************ Preping a Configurator ***************
//***************************************************

Gen 2 and later:
Hardware: ESP32 D1 Mini & ESP8266 D1 Mini

Solder, connect, wire, etc. pins: 
16 to D4, 17 to D3, Vcc to Vcc, Gnd to Gnd (this is if your using the recommended hardware selection

Board Manager Specifics:
ESP32: Use Wemos D1 Mini ESP32 board and change partition scheme to "Minimum Spiffs (large apps with OTA)"
ESP8266: Use Lolin(Wemos) D1 R2 and Mini

Before thinking about uploading the software, watch the following tips videos for the programs to make sure you set up the code properly:
ESP32 code: https://youtu.be/S4vJf7yOa5A
ESP 8266 code: https://youtu.be/wFU6d7hrfCg
Also make sure you do the blynk stuff first!!! you need your blynk authentication codes!!!

Key points:
1. Update the Tagger ID # for each tagger in both codes/modules (32/8266)
2. Update the wifi network you are using (32/8266)
3. update the Bluetooth ID for the tagger your working on (32)
4. update the server settings in the setup section of the code near the bottom of the code under the setup portion (8266)
5. repeat #1 and #3 for each additional configurator, #'s 2/4 stay the same for each tagger

When uploading the code to the 8266, you may need to hold the esp32 reset button if arduino IDE doesnt detect the 8266 while attempting to upload.

I recommend testing one tagger out before proceeding the the next one to make sure you got it right and before you repeat the process for multiple taggers


//***************************************************
//************     5. OTA Updating    ***************
//***************************************************

If you are updating to a new configurator firmware version/release and are already running an OTA version:

1. Pressing the OTA button causes both the esp8266 and the esp32 for the configurator to enter OTA UPDATE MODE. You will see that the LEDs on the modules begin to flash.
2. They must be on the wifi network entered in the original settings changed in section 4 above.
3. Your computer running Arduino IDE must be on the same network. 
4. Close and Re-open your Arduino IDE
5. Under tools in the Arduino IDE, you will now see the serial numbers listed for all the taggers that are sitting in OTA UPDATE Mode. IF they are not listed, restart your computer (ive seen this required some times for me, not sure why. Also, If not fixed, then you havent installed python on your PC... Follow these online tutorials

Skip to step #2 here for how to upload a new sketch, obviously change the sketch to be an updated configurator 8266 or 32 sketch: https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/

For Python installation to enable OTA update from your Arduino IDE, follow Step #1: https://lastminuteengineers.com/esp8266-ota-updates-arduino-ide/


//***************************************************
//********     6. Configurator install    ***********
//***************************************************

Gen 2 install:

https://youtu.be/rDPo6_XNH6k



//***************************************************
//******     7.  Using the Configurator    ********
//***************************************************

Turn in the taggers, have the Configurator running, press enable BLE controll

Here is the settings walkthrough:
https://youtu.be/9SlVQpf7_Y8



//***************************************************
//******     8.  App Updates    ********
//***************************************************

Any time you need/want to update the blynk app for changes 
made, this brings in a brand new app with a new authentication 
token for your devices. This will make it impossible for your 
devices to connect to the new app because the old auth token 
is written into the code of the esp devices... therefore, your 
devices need to be updated with a new sketch from arduino before 
you can use the new app. This is easy to manage by just following 
an important order of operations.

1. In the blink app back out of the current app/project by tapping on 
The top left hand icon that looks like several pages.
2. A week scanner icon will appear in the top right corner. Tap it.
3. Scan the updated app QR code to automatically bring in the new app.
4. The old app will still be available on your phone and the server
5. Check the new app's updated auth tokens, and copy them down to type or paste.
6. Update the old token in arduino ide or on the new sketch if applicable
7. Using the old app, place your devices in OTA mode and follow the instructions for OTA UPDATES
8. After updating them the new app will allow for the devices to connect since it uses the new auth token
9. Rename the old app if you want to keep it on your server or delete it if you decide to no longer keep use it

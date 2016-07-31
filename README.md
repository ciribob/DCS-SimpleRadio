# This project is no longer under active development and has been replace by SimpleRadio Standalone: https://github.com/ciribob/DCS-SimpleRadioStandalone

# DCS-SimpleRadio

DCS-SimpleRadio allows a better integration between TeamSpeak 3 and DCS. DCS-SimpleRadio will use the currently selected radio and frequency to decide if others can hear you or not.

I decided to build this after playing with TARS, and wanted to be able to use more aircraft than TARS currently supports as well as use the latest version of TeamSpeak.

If you want to compile or modify it, feel free and the project was built using Visual Studio 2015 - Community Edition.

Thanks to Towsim, Creator of ARIES, [FSF] Ian for DCS-Bios.

Special thanks to Teo (ED Forums) and his GitHub Project (https://github.com/olanykvist/mars) without which this project would've taken a lot longer :)

# Features
  - Full radio integration with all clickable cockpit aircraft - Partial integration with Hawk (Only the UHF Radio)
    - Working Volume Control - A10C radio starts with all radio volumes at 0 - Except for the Hawk
    - Working Frequency Control
    - Maximum of 3 radios supported per aircraft
  - FC3 Aircraft have 3 radios controllable by hotkeys or the optional DCS-SimpleRadio.exe program
  - Stand alone optional radio status panel
  - Ability to hear others and talk to them when not currently in game or not in an aircraft (hotkey to disable this if you want)
  - Ability to force the Radio system on when spectating, Ground Commanding or JTAC
  - L-39C Intercom Support - Private channel between from and back seat in multiplayer
  - Basic optional radio effects
  - Guard Mode for UH-1/A10C/F-86/Mi-8/KA-50 
 
# Known Limitations
 - On some US radios the currently selected preset radio channel selector doesn't seem to work, as in all the frequencies are the same...
 - No Signal degredation or line of sight limitations to broadcasts unlike TARS
 
# Installation

 - First download the latest release from https://github.com/ciribob/DCS-SimpleRadio/releases/latest and Download DCS-SimpleRadio.zip, (you don't need to download the sources) and extract the file to somewhere.

- Extract the zip file and run the Installer.exe following the instructions

### Manual Installation

- Open the DCS-SimpleRadio Folder
- Copy "DCS-SimpleRadio-x64.dll" and "DCS-SimpleRadio-x86.dll"  to your Teamspeak "Plugins" folder, located in your root TS3 folder.
- Copy what's inside the "Scripts" folder to "C:\Users\<Your User>\Saved Games\DCS\Scripts".  or "C:\Users\<Your User>\Saved Games\DCS.openbeta\Scripts" if you're using the open beta.
   When finished, your DCS "Scripts" folder should have a **folder** inside it called "DCS-SimpleRadio" and a **file** called Export.lua

   ****IMPORTANT**** Do NOT place it in the DCS root directory! It has to be in "C:\Users\<Your User>\Saved Games\DCS\Scripts" otherwise it will not work!

- If you already have an "Export.lua" file in the "Scripts" folder: 
  
  ---Use a text editing program such as "Notepad ++" (NOT regular Notepad!)
  
  ---Copy the last line of my "Export.lua" to your existing "Export.lua"

- Open up TeamSpeak and go to Settings -> Plugins and enable the "DCS-SimpleRadio" plugin (by ticking the box).

- HotKeys for the Plugin can be configured in Teamspeak under -> Settings -> Options -> Hotkeys. 
   After clicking Add, you need to click "Show Advanced Actions" and navigate to: Plugins -> Plugin Hotkey -> DCS-SimpleRadio  (Located under "--Server--") 

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/Ts3%20Hotkeys_zpsgxbh1aid.png~original "Hotkeys")

- An optional Radio Info display program is included. If you set DCS to windowed mode and run this program, you'll see the status of the radio. 
   If the DCS-SimpleRadio.exe doesn't run or work correctly, copy the Newtonsoft.Json.dll to the same place as  DCS-SimpleRadio.exe. If it still doesn't work, try disabling or adding rules to your firewall.

### Teamspeak is freaking out - Anti Flood Protection
The plugin uses Teamspeak to synchronise everyones radio frequencies with one another and can trigger TS3's anti-flood protection. There is no need to worry and it's a simple fix for your TS3 Admin.

Connect to the server you administer
Go to Permissions -> Server Groups

Switch to advanced permissions as shown in the screenshot below
![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/permissions%20edit_zpskssuokrz.png "Permissions")

Search for "Flood" and Tick "Ignore Anti-Flood" measures
![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/anti%20flood_zpsoc2vsnuy.png "Permissions")

Apply this to all the server groups that will be using DCS-SimpleRadio

### It's Not Working!
First thing to check is that you carried out the installation correctly.

Secondly, make sure you say 'yes' to any firewall prompts, as the Plugin communicates with the DCS-SimpleRadio status panel and with the DCS sim using UDP, a networking technology.

If it's still not working, we've had one user have an issue where Windows was blocking the data sent from DCS, even with the firewall rules added or with the firewall disabled. If this happens to you, use a good text editor like Notepad++ or Sublime and edit the second line of "SimpleRadioInit.lua" from ```SR.unicast = false``` to ```SR.unicast = true```.
Once this is done, open Teamspeak and type ```/sr switch``` in the chat window. You should then see a success message in Teamspeak saying *Switched to Unicast*. You will only need to do this once. You can also undo the switch by typing ```/sr switch``` again.

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/ts3%20switch_zpszjdlko7i.png~original "Switch")

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/ts3%20switched_zps1gjbtqdu.png~original "Switch")

***STILL NOT WORKING*** - Please contact me on the DCS Forums, giving the version of Teamspeak and its build type, Windows version and as much information as possible. It's also possible that the DCS-SimpleRadio plugin could conflict with other radio plugins such as TARS or ACRE (ARMA), so make sure they're disabled if you have issues.

# Using in Game
Once the installation is complete, nothing more needs to be done to get DCS-Simple Radio working. You can also restart Teamspeak without issues while in game and it will automatically reconnect to the radio system. You can also disable the system at any time using either the HotKey or unticking the DCS-SimpleRadio plugin under Settings->Plugins in Teamspeak.

### Radio Status Panel

The radio status panel can be launched by double clicking DCS-SimpleRadio.exe and should appear on top of the DCS window as long as DCS is in **windowed mode**. It can be launched or restarted at any time without issues. The DCS-SimpleRadio status panel can be run from any location as long as where ever it's copied to also as the ```Newtonsoft.Json.dll``` there as well.

The buttons and volume slider on the radio status panel can be used as long as you are in an FC3 Aircraft such as the F15 that doesn't have an interactive cockpit. You can also select the current active radio by either clicking on the radio frequency or the radio status dot.

Below are a few examples of what the radio status can look like:

**FC3 Aircraft:**

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/connected%20fc3_zpsni5feef6.jpg "FC3 Aircraft")

**Huey with two radios off or damaged:**

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/connected%20but%20some%20radios%20off_zpsefmiwtay.jpg "Huey")

**Connection to DCS lost or the player is not in an Aircraft and has not forced the radio on with a HotKey:**

![alt text](http://i1056.photobucket.com/albums/t379/cfisher881/Not%20Connected_zpsk8pwnuwt.jpg
 "Radio Off")

### Aircraft Radios

I will list below the location of the Radio switch in all clickable aircraft to help you along.

WIP. For now this video runs through all the aircraft and their radios: https://www.youtube.com/watch?v=FFHQY0L1lGU

[![YouTube Radio Run-through ](http://img.youtube.com/vi/FFHQY0L1lGU/0.jpg)](https://www.youtube.com/watch?v=FFHQY0L1lGU)


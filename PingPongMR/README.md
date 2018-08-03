# Multiplayer AR PingPong

Here you can find the full UE4 project of Stereolabs pingpong demo shown at the AWE2018 expo.
A video is available [here](https://www.youtube.com/watch?v=rfskhlS-XT0).


## Installation

First you need to have our custom engine installed and ready to use. If you don't, see our [installation guide](https://docs.stereolabs.com/mixed-reality/unreal/getting-started/)).

Then follow these steps:
- Download this repository.
- Go to your new root project folder. Create a new folder called **Plugins**.
- Drop the Stereolabs folder within the plugin download into this folder. If you haven’t downloaded the plugin yet, you can get it [here](https://github.com/stereolabs/zed-unreal-plugin).
- Right click on the *PingPongMR.uproject*, select *switch Unreal Engine version* and choose our custom engine you previously installed in the list.
- Open the new *PingPongMR.sln* file with Visual Studio 2015.
- Build the solution by pressing **Ctrl + F5**. Once the build is done, it will start the project in the UE4 Editor. 

In the future, you can start the project by double clicking on *PingPongMR.uproject*.


## Inside the Editor

The game is almost completely done in Blueprint, so you don't need to be skilled in C++ in order to get deep into the game code. Only the Stereolabs plugin and some small classes are in C++.
You'll have access to our all assets and a lot of comments in each Blueprint to help you understand the game logic.

In order to test the game, **you need a Oculus CV1 or HTC Vive + controllers** and use the start **VR preview** button in editor.

Enjoy!

## Oculus CV1 controls

- Aim in menu with the controller
- B or X: open/move the menu
- Trigger: spawn table | spawn a ball | select in menu

## HTC Vive controls

- Aim in menu with the controller
- Menu button: open/move the menu
- Trigger: spawn table | spawn a ball | select in menu


## Multiplayer

*Multiplayer in the same Vive room setup is not officially supported by Valve or HTC. This game multiplayer should only be used by advanced users and you might experience some issues with HTC Vive tracking or setup. We do not provide support for this.*

In order to try the multiplayer experience, you first need 2 computers, each with an HTC Vive + controllers. You will also need a ZED Mini on each one. However, you only need one pair of Vive Lighthouses. 

Both players have to play in the same Vive room setup/calibration. That means you must put the same setup file on each computers. To do this:
- Set up 2 base stations in your room.
- Make a room setup on both computers indivudually with these base stations (one will be overwritten but you need to have run room setup previously).
- On the host computer, find your Steam root directory (usually *C:/Program Files (x86)/Steam/*)
- You have to copy the two following files from the host computer to the other one, overwriting the old ones . **Only do this with SteamVR closed**:
    - *SteamRoot/config/chaperone_info.vrchap*
    - *SteamRoot/config/lighthouse/lighthousedb.json*

Now you can start the game on both computers and select multiplayer in menu. Both players should see the table at the same place once the match is started and the paddle in both controllers. If this is not the case, you should retry the previous steps for Vive multiplayer setup.
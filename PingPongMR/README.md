# Multiplayer AR Pingpong

Here you can find the full UE4 project of Stereolabs pingpong demo used in AWE2018 expo.
A video is available [here](https://www.youtube.com/watch?v=rfskhlS-XT0).

*This project was originally designed as an internal tech demo.*


## How to use it ?

First you need to have our custom engine installed and ready to use (otherwise you can follow our [installation guide](https://docs.stereolabs.com/mixed-reality/unreal/getting-started/)).

Then you can follow these steps:
- Download this repository.
- Go to your new root project folder. Create a new folder called **Plugins**.
- Drop the Stereolabs folder within the plugin download into this folder. If you haven’t downloaded the plugin yet, you can get it [here](https://github.com/stereolabs/zed-unreal-plugin).
- Right click on the *PingPongMR.uproject*, select *switch Unreal Engine version...* and choose our custom engine you previously installed in the list.
- Open the *PingPongMR.sln* new file with Visual Studio 2015.
- Build the solution by pressing **Ctrl + F5**. Once the build is done, it will start the project in the UE4 Editor.

You can now start the project with a double click on *PingPongMR.uproject*.


## Inside the Editor

The game is close to be exclusively done in blueprint so you don't need to be accustomed to C++ in order to get deep into the game code (only the Stereolabs plugin and some small classes are in C++).
You have access to our all assets and a lot of comments in each blueprint to help you understand the game logic.

In order to test the game, **you need a Oculus CV1 or HTC Vive + controllers** and use the start **VR preview** button in editor.

Enjoy it !

## Oculus CV1 controls

- Aim in menu with the controller
- B or X : open/move the menu
- Trigger : spawn table | spawn a ball | select in menu

## HTC Vive controls

- Aim in menu with the controller
- Menu button : open/move the menu
- Trigger : spawn table | spawn a ball | select in menu


## Multiplayer

*Multiplayer in the same Vive room setup is not officially supported. This game multiplayer should only be used by advanced users and you might experience some issues with HTC Vive tracking or setup. We do not provide any support on this.*

In order to try the multiplayer experience, you first need 2 computers with both an HTC Vive + controllers.

Both players have to play in the same Vive room setup. Here are the steps to acheive it:
- Setup 2 base stations in your room.
- Make a room setup on both computers with these base stations.
- Find your steam root directory (usually *C:/Program Files (x86)/Steam/*)
- You have to copy the two following files from one computer to the other one (by erasing them one the second computer). **Only do this with steamVR closed**:
    - *SteamRoot/config/chaperone_info.vrchap*
    - *SteamRoot/config/lighthouse/lighthousedb.json*

Now you can start the game on both computer and select multiplayer in menu. Both players should see the table at the same place once the match is started and the paddle in both controllers. If this is not the case, you should retry the previous steps for Vive multiplayer setup.

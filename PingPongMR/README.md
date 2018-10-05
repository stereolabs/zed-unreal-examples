# Multiplayer AR PingPong

Here you can find the full UE4 project of Stereolabs pingpong demo shown at the AWE2018 expo.
A full video is available [here](https://www.youtube.com/watch?v=rfskhlS-XT0).

[![](https://thumbs.gfycat.com/CreativeAmusingGentoopenguin-size_restricted.gif)](https://gfycat.com/CreativeAmusingGentoopenguin)


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

In order to test the game, **you need an Oculus Rift or HTC Vive + controllers**. Make sure these are plugged in before you launch the project in Unreal Editor. 

The game is almost completely done in Blueprint, so you don't need to be skilled in C++ in order to get deep into the game code. Only the Stereolabs plugin and some small classes are in C++.
You'll have access to all of our assets and a lot of comments in each Blueprint to help you understand the game logic.

[![](https://thumbs.gfycat.com/TallImpressiveEyelashpitviper-size_restricted.gif)](https://gfycat.com/TallImpressiveEyelashpitviper)

To run the game in the editor, click on **Play** -> **VR preview** at the top.

[![](https://thumbs.gfycat.com/PointedBlindAfricanfisheagle-size_restricted.gif)](https://gfycat.com/PointedBlindAfricanfisheagle)


##Playing the Game

The controls differ slightly depending on your headset:

### Oculus Rift controls

- B or X: Open/move the menu
- Trigger: Spawn table | Spawn a ball | Select in menu

### HTC Vive controls

- Menu button: Open/move the menu
- Trigger: Spawn table | Spawn a ball | Select in menu



Put on your headset and you'll see your real-world environment inside. Open the menu using the controls explained above. For now, click **Solo Mode.**

[![](https://thumbs.gfycat.com/GenuineRichJapanesebeetle-size_restricted.gif)](https://gfycat.com/GenuineRichJapanesebeetle)

The next options are **Target Practice** and **Versus AI**

**Target Practice** is like the batting cages for ping pong. A machine will serve balls that you should return while aiming for a target on the other side. 

[![](https://thumbs.gfycat.com/BossyValuableDiplodocus-size_restricted.gif)](https://gfycat.com/BossyValuableDiplodocus)

**Versus AI** gives you a practice opponent to play with. Spawn a ball with your off-hand and hit it with your paddle hand, and the floating AI paddle will attempt to return it. 

[![](https://thumbs.gfycat.com/HandmadeUnrealisticHamadryad-size_restricted.gif)](https://gfycat.com/HandmadeUnrealisticHamadryad)

The first time you pick either option, you'll have to place the ping pong table in your environment. Aim where you want to place it with your headset. The table outline will turn from red to blue when it's in a valid location - pull the trigger to confirm its position. 

[![](https://thumbs.gfycat.com/SeparateAdmirableIguanodon-size_restricted.gif)](https://gfycat.com/SeparateAdmirableIguanodon)


## Multiplayer

*Multiplayer in the same Vive room setup is not officially supported by Valve or HTC. This game multiplayer should only be used by advanced users and you might experience some issues with HTC Vive tracking or setup. We do not provide support for this.*

In order to try the multiplayer experience, you first need 2 computers, each with an HTC Vive + controllers (it does not work with Oculus Rift). You will also need a ZED Mini on each one. However, you only need one pair of Vive Lighthouses. 

Both players have to play in the same Vive room setup/calibration. That means you must put the same setup file on each computers. To do this:
- Set up 2 base stations in your room
- Make a room setup on both computers indivudually with these base stations (one will be overwritten but you need to have run room setup previously)
- On the host computer, find your Steam root directory (usually *C:/Program Files (x86)/Steam/*)
- You have to copy the two following files from the host computer to the other one, overwriting the old ones. **Only do this with SteamVR closed**:
    - *SteamRoot/config/chaperone_info.vrchap*
    - *SteamRoot/config/lighthouse/lighthousedb.json*

Both computers should also be on the same local network. 

Now you can start the game on both computers and select **Multiplayer** in the menu. Once the match has started, both players should see the table at the same place, and the virtual paddle in the other player's hand. If this is not the case, you should retry the previous steps for Vive multiplayer setup.

[![](https://thumbs.gfycat.com/DefensiveGlossyHind-size_restricted.gif)](https://gfycat.com/DefensiveGlossyHind)
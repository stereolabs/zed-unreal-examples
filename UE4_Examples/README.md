# Stereolabs ZED - UE4 Samples

This repository contains Unreal sample projects that demonstrate how to create advanced mixed reality applications using the ZED SDK and Unreal Engine 4.

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

You can now start the project with a double click on *ZEDSamples.uproject*.

## Inside the Editor

You can find several maps in the Content folder. Each map show you how to use the plugin for a specific use case. Don't hesitate to try them all!

Note that to use an Oculus Rift or HTC Vive in any of the samples, you should have the headset plugged in and the relevant application running (Oculus or SteamVR) before you open the project. 

Find additional documentation  on [lights and shadows](https://docs.stereolabs.com/mixed-reality/unreal/lighting/) and on [spatial mapping](https://docs.stereolabs.com/mixed-reality/unreal/spatial-mapping/).
.
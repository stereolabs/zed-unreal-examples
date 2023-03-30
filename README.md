# **Please update to [our Unreal Engine 5 plugin](https://github.com/stereolabs/zed-UE5) to use the last features of the ZED SDK**

As of the release of the [ZED SDK 4.0](https://www.stereolabs.com/developers/release/), this sample is deprecated and will not be maintained anymore.
To use the most recent features and capabilities of the ZED SDK, we invite you to use our [Unreal Engine 5 plugin](https://github.com/stereolabs/zed-UE5).

Thanks for your comprehension.

--------------------------------------

# Stereolabs ZED - UE4 Examples

This repository contains Unreal examples projects that demonstrate how to create advanced Mixed Reality application using the ZED SDK and Unreal.

**UE4_Examples** is a single project with a variety of small samples, each meant to highlight a single feature of the ZED UE4 plugin. **PingPongMR** is a game combining multiple ZED features, and demonstrating how to have a shared multiplayer experience in AR with multiple headsets or observer cameras. 

## Before You Start

Our plugin requires our SDK and a custom build of the Unreal engine. Follow the steps in our [Getting Started](https://docs.stereolabs.com/mixed-reality/unreal/getting-started/) and [Building the Engine](https://docs.stereolabs.com/mixed-reality/unreal/building-the-engine/) guides before using these samples. 

Then, see the readme file in the UE4_Examples and PingPongMR folders for how to build each project. 

## UE4_Examples

All samples are in the **UE4_Examples** project as a level, found in **Content** -> **Levels**. 


### Passthrough

[![](https://thumbs.gfycat.com/SecondOrangeGlassfrog-size_restricted.gif)](https://gfycat.com/SecondOrangeGlassfrog)

See the real world through an Oculus Rift or HTC Vive, with virtual objects mixed in. Must have a compatible headset plugged in and running when the project is loaded. 

### Lighting

[![](https://thumbs.gfycat.com/DevotedDenseGermanspitz-size_restricted.gif)](https://gfycat.com/DevotedDenseGermanspitz)

Casts a spotlight that properly illuminates the real world just as well as the virtual. Also has a directional light that causes virtual spheres to cast shadows on the real floor. See our [lighting documentation](https://docs.stereolabs.com/mixed-reality/unreal/lighting/) to learn how it works. 

### Dynamic Crosshair

[![](https://thumbs.gfycat.com/PreciousElegantKiskadee-size_restricted.gif)](https://gfycat.com/PreciousElegantKiskadee)

Puts a 3D crosshair in the center of the screen that moves itself closer or further to be just in front of the real-world object it overlaps. Explore to see how to use the GetDepthAtScreenPosition function, and experiment with the other GetDepth and GetNormal functions included in the plugin. 

### HitTest

[![](https://thumbs.gfycat.com/VerifiableAcrobaticDungbeetle-size_restricted.gif)](https://gfycat.com/VerifiableAcrobaticDungbeetle)

Left click to shoot virtual balls that will stop when they collide with a real-world object. Demonstrates that you don't need to pre-scan an environment for such interactions thanks to the ZED's real-time depth map. See the BP_HitTestBall blueprint and the HitTestReal function to learn how it works. 

### Object Placement

[![](https://thumbs.gfycat.com/ColorfulBareFlatfish-size_restricted.gif)](https://gfycat.com/ColorfulBareFlatfish)

Click on a flat, upwards-facing surface to spawn a cyborg standing on top of it. Demonstrates how to use the ZED's Plane Detection feature to place objects without pre-scanning. See the level Blueprint and the GetFloorPlaneAtScreenPosition function to see how it works.

### Spatial Mapping

[![](https://thumbs.gfycat.com/PersonalThankfulGrizzlybear-size_restricted.gif)](https://gfycat.com/PersonalThankfulGrizzlybear)

Map your environment into a 3D mesh, then shoot balls at real objects that realistically bounce off using Unreal's collision detection and physics. Optionally, save the mesh, mesh texture and .area spatial memory file for later. Use to pre-scan an environment for when you need persistent geometry, such as when collisions with the real-world need to occur even when you can't see them. See our [spatial mapping](https://docs.stereolabs.com/mixed-reality/unreal/spatial-mapping/) documentation to learn how it works. 

### Motion Controllers

[![](https://thumbs.gfycat.com/FaroffThankfulLark-size_restricted.gif)](https://gfycat.com/FaroffThankfulLark)

Makes virtual motion controllers follow real motion controllers tracked by an Oculus Rift or HTC Vive. Must have a compatible headset plugged in and running when the project is loaded. 

### Open New Level

[![](https://thumbs.gfycat.com/CaringJoyousHerring-size_restricted.gif)](https://gfycat.com/CaringJoyousHerring)

Initiates the ZED, then properly closes it before opening a new level (in this case, itself). Follow this procedure (visible in the level Blueprint) any time you wish to change levels while the ZED is running. 

### Advanced

[![](https://thumbs.gfycat.com/KnobbyEvilGuineapig-size_restricted.gif)](https://gfycat.com/KnobbyEvilGuineapig)

Shows advanced ways to grab the ZED's image from the camera using two example Blueprints: one that retrieves images directly, and one that retrieves via a texture batch. 



## PingPongMR

[![](https://thumbs.gfycat.com/CreativeAmusingGentoopenguin-size_restricted.gif)](https://gfycat.com/CreativeAmusingGentoopenguin)

**PingPongMR** puts a virtual ping pong table in your environment. Bring a friend with their own Vive and ZED Mini to compete against you - they'll see the same ball, table and paddles as you. You can even use another ZED and Vive Tracker to view the match from third person in case you want to stream it live. 

**PingPongMR** has additional equipment requirements for multiplayer, and has a more involved setup. See the readme file in the PingPongMR folder for instructions. 


## Support
If you need assistance go to our Community site at https://community.stereolabs.com/

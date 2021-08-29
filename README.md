# Botcraft

![Linux](https://github.com/Lemiort/Botcraft/workflows/Linux/badge.svg) ![Windows](https://github.com/Lemiort/Botcraft/workflows/Windows/badge.svg)

Botcraft is a cross-platform C++17 library to connect and interact with Minecraft servers with (optional) integrated OpenGL renderer.

This was my first project using OpenGL, multi-threading, networking and "sort-of-complete cmake" so it's a "learn-by-doing" code and many things are probably (really) badly done or should be refactored. Do not hesitate to open an issue or to send a PR if you know how to improve some part of it!

## Features

- Connection to minecraft server (both offline mode and online connection with Mojang account, Microsoft accounts are supported using a json file created by the official launcher)
- Automatic refresh of the token when required
- DNS name resolution with and without SRV record
- All official releases from 1.12.2 to 1.17.1 supported
- Compression
- Physics and collisions
- (Optional) Rendering of all the blocks (including entity-blocks like chests, banners...)
- Bot control with mouse and keyboard
- Path finding
- Block breaking
- Inventory managing
- Block placing
- Block interaction (button, lever etc...)

Example of pathfinding. Right of the screen is the integrated renderer
![Path finding gif](gifs/video.gif)

More complex example with 10 survival bots collaborating on a pixel art build. They are all in survival, so they have to pick the right blocks in the chests, eat food and obviously can't fly. There is no global supervision, and they can't communicate with each other. Better quality video in [this reddit post](https://www.reddit.com/r/Minecraft/comments/mwzm26/my_survival_bot_project_applied_to_map_pixelart/).
![Building an map art gif](gifs/mapart.gif)

## Dependencies

All dependencies are managed by conanfile so you need conan.

## Building and Installation

To build the library for the latest version of the game with both encryption and compression support, but without OpenGL rendering support:

```console
git clone https://github.com/Lemiort/Botcraft.git
cd Botcraft
mkdir build
cd build
conan remote add lemiort https://lemiort.jfrog.io/artifactory/api/conan/public-conan
conan install .. --build=missing
conan config set general.revisions_enabled=1
cmake -DGAME_VERSION=latest -DBOTCRAFT_BUILD_EXAMPLES=ON -DBOTCRAFT_COMPRESSION=ON -DBOTCRAFT_ENCRYPTION=ON -DBOTCRAFT_USE_OPENGL_GUI=OFF ..
make all
```

At this point, you should have all the examples compiled and ready to run. Please note that you don't have to clone recursively or download and install the dependencies manually, cmake will automatically take care of these steps based on your build configuration and what is already installed on your machine. On Windows with Visual, you can also use cmake-gui and then compile the .sln directly from Visual.


There are several cmake options you can modify:

- GAME_VERSION [1.XX.X or latest]
- BOTCRAFT_BUILD_EXAMPLES [ON/OFF]
- BOTCRAFT_INSTALL_ASSETS [ON/OFF] Copy all the needed assets to the installation folder along with the library and executable
- BOTCRAFT_COMPRESSION [ON/OFF] Add compression ability, must be ON to connect to a server with compression enabled
- BOTCRAFT_ENCRYPTION [ON/OFF] Add encryption ability, must be ON to connect to a server in online mode
- BOTCRAFT_USE_OPENGL_GUI [ON/OFF] If ON, botcraft will be compiled with the OpenGL GUI enabled
- BOTCRAFT_USE_IMGUI [ON/OFF] If ON, additional information will be displayed on the GUI (need BOTCRAFT_USE_OPENGL_GUI to be ON)

## Examples

Examples can be found in the [Examples](Examples/) folder:

- [0_HelloWorld](Examples/0_HelloWorld): Connect to a server, sends Hello World! in the chat then disconnect
- [1_UserControlledExample](Examples/1_UserControlledExample): Best with GUI, mouse and keyboard controlled player. Can be used in a dummy world (without any server) to test things like physics or rendering
- [2_ChatCommandExample](Examples/2_ChatCommandExample): Simple bot that obey commands sent through vanilla chat. Known commands at this point:
  - pathfinding
  - disconnecting
  - checking its surroundings for spawnable blocks (useful if you want to check whether or not a perimeter is spawn proof)
  - placing a block
  - interacting with a block (lever, button ...)
- [3_SimpleAFKExample](Examples/3_SimpleAFKExample): Simple example to stay at the same position. Physics is not processed, chunks are not saved in memory to save RAM.
- [4_MapCreatorExample](Examples/4_MapCreatorExample): Much more complex example, with autonomous behaviour implemented to build a map based pixel art. Can be launched with multiple bot simultaneously. They can share their internal representation of the world to save some RAM, at the cost of slowing down if too many share the same. Only extensively tested on 1.16.5, but should work with minor to none adaptation on previous/older versions.

## Connection

If the server is in online-mode: false mode, you can connect with any username.

If the server is in online-mode: true mode, you can connect with a Mojang account (login+password) or with a json file created by the official minecraft launcher. For details about the second approach, please see [this page](launcher_json_connection.md). For users with a Microsoft account, only the second option is available, as implementing the full [Microsoft oauth procedure](https://wiki.vg/Microsoft_Authentication_Scheme) would be rather complicated in C++.

If the access token present in the launcher file is too old, it will be automatically refreshed, and the file will be updated with the new one.

## To-do list

It's only a free time project, but there are still a lot of things to do! Right now the only usecase is an AFK bot to activate a farm with almost 0% usage of CPU/GPU. The next step is to add some functionalities like entities handling/rendering, attacking, crafting... Everything needed to automate more tasks in survival vanilla Minecraft!

There are also some minor improvements to existing features that have to be done:

- Improve scaffolding/ladder physics (the player can't climb the ladders)
- Improve pathfinding with real colliders (the pathfinding considers every solid block as box of size 1 but the collisions are computed with the real shapes which lead to bad situations, for example with fences)

## License

GPL v3

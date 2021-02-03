OSX/Linux: [![Build Status](https://api.travis-ci.org/hku-ect/gazebosc.png?branch=master)](https://travis-ci.org/hku-ect/gazebosc)

# GazebOSC

A node-based implementation to author high-level sphactor node actors. A multi-in-single-out (for now) information hub that outputs OSC. May support different protocols in future that OSC does not handle well (eg. streaming video)

The UI based on ImGui, but we will eventually also support headless running of pre-created sketches.

## Building

There is one main dependencies:

 * libzmq

Dependencies for the build process / dependencies are:

   * git, libtool, autoconf, automake, cmake, make, pkg-config, pcre

### OSX

#### Building Dependencies

 * Get build dependencies via brew:
```
brew install libtool autoconf automake pkg-config cmake make
```
Clone and build libzmq
```
git clone https://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh && ./configure --without-documentation
make
sudo make install
```
#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc. 

* Clone the repo
```
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
```

##### Build with XCode

To create an XCode project, perform the following commands from the root gazebosc git folder:

```
mkdir xcodeproj
cd xcodeproj
cmake -G Xcode ..
```
This should generate a valid Xcode project that can run and pass tests.

##### Build using make

In the root gazebosc git folder:
```
mkdir build
cd build
cmake ..
make
```
The gazebosc executable will be in the bin folder!

### (Debian/Ubuntu) Linux

*(tested on Ubuntu 16.04)*

* First install required dependencies

```
sudo apt-get update
sudo apt-get install -y \
    build-essential libtool cmake \
    pkg-config autotools-dev autoconf automake \
    uuid-dev libpcre3-dev libsodium-dev 
```

#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc:

* Clone the repo and build Gazebosc
```
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
cd gazebosc
mkdir build
cd build
cmake ..
make
```
You'll find the Gazebosc binary in the bin directory, to run:
```
cd bin
./gazebosc
```

If you want to work on Gazebosc it's easiest to use QtCreator. Just load the CMakeLists.txt as a project in QtCreator and run from there.

### Windows

* Install Visual Studio 2019: https://visualstudio.microsoft.com/downloads/ , make sure to include:
	- CMake
	- Git
* Clone gazebosc repository

```
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
```
* Run "x64 Native Tools Command Prompt for VS 2019" as Administrator
	- Navigate to gazebosc project root
    	- Run "build_windows.bat"
* Run Visual Studio, and select Open -> CMake
	- Navigate to gazebosc/CMakeListst.txt
* Select "gazebosc.vcxproj" from debug targets

You are now ready to code/debug as normal!

## Adding new nodes

*This is deprecated, see libsphactor documentation*

### GNode inheritance, important bits

*(Also see the QtCreator tutorial below)*
The first step in creating custom nodes is to inherit from GNode. This means including GNode.h, and calling the GNode explicit constructor when your own node class is being constructed. This should look roughly like this when done from within the header file:

```
struct MyCustomNode : GNode
{
    explicit MyCustomNode(const char* uuid) : GNode(   "MyCustomNodeName",              // title
                                                      { {"OSC", NodeSlotOSC} },         // Input slots
                                                      { {"OSC", NodeSlotOSC} },         // Output slots
                                                        uuid )                          // uuid pass-through
    {
       
    }
};
```
### Registering the new node
In order for the system to find and be able to create your node, you will need to add a line to the **nodes.cpp** *RegisterCPPNodes* function:
```
void RegisterCPPNodes() {
    ...
    
    RegisterNode( "MyCustomNodeName", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new MyCustomNode(uuid); });
    
    ...
}
```

### Node Lifetime

Once a node has been created, it goes through the following steps:
 * **Construction**
   * if performed from loading a file, also passes and Deserializes data
 * **CreateActor** (this is called after instantiation to preserve polymorphic response)
 * **Threaded Actor events**
   * Init: actor has been created, and can be used to do threaded initializations (see OSCListener example)
   * Message: actor has received a message
   * Callback: actor has received a timeout (timed event, probably scheduled by calling the SetRate function)
   * Stop: actor has been stopped and threaded resources can be cleaned up (see OSCListener example)
 * **Destruction**

#### Construction & Destructions
During these phases, you can prepare and clean up resources used by the class. Examples include UI char buffers for text or values (see PulseNode).

#### CreateActor
This GNode function can be overriden to perform main-thread operations once the actor has been created. Primary use-case at this time is calling the *SetRate* function (an API-call, which must be called from the main thread) to tell the node to send timeout events at a set rate (x times per second).

#### Threaded Actor events
Throughout the lifetime of the actor, the GNode class will receive events, and pass these along to virtual functions. Override these functions to perform custom behaviours (see above description for which events there are). Important to note is that this code runs on the thread, and you should not access or chance main-thread data (such as UI variables). For such cases, we are still designing *report functionality* (copied thread data that you can then use to update UI, for instance).

#### Destruction
When deleting nodes or clearing sketches, the node instance will be destroyed and its actor stopped.


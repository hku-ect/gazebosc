OSX/Linux: [![Build Status](https://api.travis-ci.org/hku-ect/gazebosc.png?branch=master)](https://travis-ci.org/hku-ect/gazebosc)

# GazebOSC

Gazebosc is a high level actor model programming environment. You can visually create actors and author them using a nodal UI. Actors are running concurrently and can be programmed sequentially using C, C++ or Python. Actors communicate using the [OSC](https://en.wikipedia.org/wiki/Open_Sound_Control) serialisation format. 

![screenshot](dist/screenshot.png?raw=true)

## Prebuilt binaries

CI tested binaries with bundled Python are available from:

https://pong.hku.nl/~buildbot/gazebosc/

Sort on date to find the latest build. 

## Build from source

Most dependencies are bundled in the repository. There is one main external ZeroMQ dependency you need to have available:

 * libzmq

Dependencies for the build process / dependencies are:

 * git, libtool, autoconf, automake, cmake, make, pkg-config, pcre

If you want Python support you'll need to have a recent Python >3.7 installed!
 
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
    build-essential libtool-bin cmake libasound2-dev \
    pkg-config autotools-dev autoconf automake \
    uuid-dev libpcre3-dev libsodium-dev python3-dev
```

#### Clone and build libzmq

```
git clone https://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh && ./configure --without-documentation
make
sudo make install
```

#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc:

* Clone the repo and build Gazebosc
```
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
mkdir gazebosc/build
cd gazebosc/build
cmake ..
make
```
You'll find the Gazebosc binary in the bin directory, to run:
```
cd bin
./gazebosc
```

If you want to work on Gazebosc it's easiest to use the QtCreator IDE. Just load the CMakeLists.txt as a project in QtCreator and run from there.

#### Raspberry Pi (Raspberry Pi OS)

Use the following script:

```bash
sudo apt install git libtool-bin libdrm-dev libgbm-dev build-essential libtool-bin cmake \
    pkg-config autotools-dev autoconf automake libevdev2 libgles2-mesa-dev \
    uuid-dev libpcre3-dev libsodium-dev python3-dev libasound2-dev libxext-dev
git clone https://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh && ./configure --without-documentation
make
sudo make install
cd ..
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
mkdir gazebosc/build
cd gazebosc/build
cmake .. -DWITH_OPENVR=OFF 
CFLAGS=-mfpu=neon make
```

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

Easiest method of adding a new node is using Python. You'll need to have a Gazebosc build with Python.

* In Gazebosc create a new Python actor: (right mouse click - Python)
* Click on the edit icon, a text editor will appear
* Paste the following text in the texteditor and click save

```
class MyActor(object):
    def handleSocket(self, addr, msg, type, name, uuid, *args, **kwargs):
    	print("received OSC message {} {}".format(addr, msg))
        return ("/MyActorMsg", [ "hello", "world", 42] )
```

This is just the most basic actor which responds to incoming messages. A template you can use for a full feature actor is as follows:

```python

class actor(object):
    def __init__(self, *args, **kwargs):
        self.timeout = 1000         # Use this timeout value for when you need recurring handleTimer events
                                    # Set to -1 to wait infinite (default)

    def handleApi(self, command, *args, **kwargs):
        print("The API command is {} and its arguments is {}".format(command, args))
        return None

    def handleSocket(self, address, data, *args, **kwargs):
        print("The osc address is {} and its data is {}".format(address, data))
        return ("/myreturnaddress", ["hello", 3, 2, 1])

    def handleTimer(self, *args, **kwargs):
        # This is a timed event, use it as you need
        print("My timed event with type: {}, name: {}, uuid: {}".format(args[0], args[1], args[2]))
        return ("/mytimedreturn", ["hello", 1, 2, 3])

    def handleCustomSocket(self, *args, **kwargs):
        # We'll explain this in the future
        return ("/myreturnaddress", ["hello", "world"])

    def handleStop(self, *args, **kwargs):
        # We are shutting down
        print("Bye bye from {}".format(args[1]))

```
Save this file as actor.py as the filename needs to equal the class name!


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

# Related research

* Sphactor: actor model concurrency for creatives: https://archive.fosdem.org/2020/schedule/event/iotsphactor/
* Code Orchestration: https://archive.fosdem.org/2016/schedule/event/deviot02/
* Orchestrating computer systems, a new protocol: https://archive.fosdem.org/2015/schedule/event/deviot02/

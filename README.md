OSX/Linux: [![Build Status](https://api.travis-ci.org/hku-ect/gazebosc.png?branch=master)](https://travis-ci.org/hku-ect/gazebosc)

# GazebOSC

Gazebosc is a high level actor model programming environment. You can visually create actors and author them using a nodal UI. Actors are running concurrently and can be programmed sequentially using C, C++ or Python. Actors communicate using the [OSC](https://en.wikipedia.org/wiki/Open_Sound_Control) serialisation format. 

![screenshot](dist/screenshot.png?raw=true)

## Prebuilt binaries

CI tested binaries with bundled Python are available from:

https://pong.hku.nl/~buildbot/gazebosc/

Sort on date to find the latest build. 

## Creating Actors

Easiest method of adding a new actor is using Python. You'll need to have a Gazebosc build with Python. Otherwise you can create Actors in C or C++.

### Python Actor

* In Gazebosc create a new Python actor: (right mouse click - Python)
* Click on the edit icon, a text editor will appear
* Paste the following text in the texteditor and click save and save the file as MyActor.py

```python
class MyActor(object):
    def handleSocket(self, addr, msg, type, name, uuid, *args, **kwargs):
    	print("received OSC message {} {}".format(addr, msg))
        return ("/MyActorMsg", [ "hello", "world", 42] )
```

This is just the most basic actor which responds to incoming messages. A template you can use for a full feature actor is as follows:

```python

class MyActor(object):
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
Save this file as MyActor.py as the filename needs to equal the class name!

### C++ Actor

To create an actor in C++ you'll need to build GazebOSC from source as you will add the actor to the source. You can use the following example code as an example:

```cpp
class MyActor : public Sphactor
{
public:
    MyActor() : Sphactor() {}

    static const char *capabilities; // this string is used to describe the capacilitites of the actor

    zmsg_t *handleInit(sphactor_event_t *ev) // this method is called when the actor is started (added to the stage)
    {   // you will receive an event (ev) with an message (ev->msg) which you will need to cleanup
        if ( ev->msg )
            zmsg_destroy(&ev->msg);
        return nullptr; // you can return a message (zmsg) or nothing (nullptr)
    }

    zmsg_t *handleAPI(sphactor_event_t *ev) // this method is called when the actor's is edited from the UI
    { if ( ev->msg ) zmsg_destroy(&ev->msg); return nullptr; }

    zmsg_t *handleSocket(sphactor_event_t *ev) // this method is called when the actor receives a message (usually OSC)
    {
        if ( ev->msg )
            zmsg_destroy(&ev->msg);
        return nullptr;
    }

    zmsg_t *handleStop(sphactor_event_t *ev) // this method is called when the actor is stopped (actor removed)
    { if ( ev->msg ) zmsg_destroy(&ev->msg); return nullptr; }
}
```
Once created an actor needs to be registered by calling:

```cpp
sphactor_register<MyActor>( "My Actor", MyActor::capabilities );
```
This is usually done in GazebOSC's main.cpp

### C Actor

An actor in C only constists of a function receiving an event and a string describing its capabilities:

```c
const char * countCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n"
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t *
my_count_actor( sphactor_event_t *ev, void* args )
{
    static int my_count_actor_count = 0;
    if ( streq(ev->type, "INIT")) {   // at INIT we load the capability string
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(countCapabilities));
    }
    else
    if ( streq(ev->type, "SOCK")) {   // SOCK is when we receive a message (usually OSC)
        my_count_actor_count++; // increment counter

        // set a custom report (used in the UI)
        zosc_t * msg = zosc_create("/report", "si",
                                   "counter", (int32_t)my_count_actor_count);

        sphactor_actor_set_custom_report_data( (sphactor_actor_t*)ev->actor, msg );
    }
    return ev->msg;
}
```

Register the actor as follows:

```c
sphactor_register( "My Count Actor", &my_count_actor, zconfig_str_load(countCapabilities), NULL, NULL );
```

## Actor Life cycle

Once an actor has been created, it has the following states:
 * INIT: actor has been created and runs in its own thread
 * IDLE: actor is waiting for an event
 * SOCK: actor has received a message
 * FDSOCK: actor has received a message on a custom socket (filedescriptor)
 * TIME: actor has received a timeout (timed event, probably scheduled by setting a timeout value
 * STOP: actor has been stopped and threaded resources can be cleaned up (see OSCListener example)
 * DESTROY: actor is destroyed

See [libsphactor](https://github.com/hku-ect/libsphactor) for details on the actor API.

# Build from source

Most dependencies are bundled in the repository. There is one main external ZeroMQ dependency you need to have available:

 * libzmq

Dependencies for the build process / dependencies are:

 * git, libtool, autoconf, automake, cmake, make, pkg-config, pcre

If you want Python support you'll need to have a recent Python >3.7 installed!
 
### OSX

#### Building Dependencies

 * Get build dependencies via brew:
```sh
brew install libtool autoconf automake pkg-config cmake make
```
Clone and build libzmq
```sh
git clone https://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh && ./configure --without-documentation
make
sudo make install
```
#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc. 

* Clone the repo
```sh
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
```

##### Build with XCode

To create an XCode project, perform the following commands from the root gazebosc git folder:

```sh
mkdir xcodeproj
cd xcodeproj
cmake -G Xcode ..
```
This should generate a valid Xcode project that can run and pass tests.

##### Build using make

In the root gazebosc git folder:
```sh
mkdir build
cd build
cmake ..
make
```
The gazebosc executable will be in the bin folder!

### (Debian/Ubuntu) Linux

* First install required dependencies

```sh
sudo apt-get update
sudo apt-get install -y \
    build-essential libtool-bin cmake libasound2-dev \
    pkg-config autotools-dev autoconf automake \
    uuid-dev libpcre3-dev libsodium-dev python3-dev
```

#### Clone and build libzmq

```sh
git clone https://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh && ./configure --without-documentation
make
sudo make install
```

#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc:

* Clone the repo and build Gazebosc
```sh
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
mkdir gazebosc/build
cd gazebosc/build
cmake ..
make
```

You'll find the Gazebosc binary in the bin directory, to run:
```sh
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

```sh
git clone --recurse-submodules http://github.com/hku-ect/gazebosc.git
```
* Run "x64 Native Tools Command Prompt for VS 2019" as Administrator
	- Navigate to gazebosc project root
    	- Run "build_windows.bat"
* Run Visual Studio, and select Open -> CMake
	- Navigate to gazebosc/CMakeListst.txt
* Select "gazebosc.vcxproj" from debug targets

You are now ready to code/debug as normal!

# Related research

* Sphactor: actor model concurrency for creatives: https://archive.fosdem.org/2020/schedule/event/iotsphactor/
* Code Orchestration: https://archive.fosdem.org/2016/schedule/event/deviot02/
* Orchestrating computer systems, a new protocol: https://archive.fosdem.org/2015/schedule/event/deviot02/

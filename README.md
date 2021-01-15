OSX/Linux: [![Build Status](https://api.travis-ci.org/hku-ect/gazebosc.png?branch=master)](https://travis-ci.org/hku-ect/gazebosc)

# GazebOSC

A node-based implementation to author high-level sphactor node actors. A multi-in-single-out (for now) information hub that outputs OSC. May support different protocols in future that OSC does not handle well (eg. streaming video)

The UI based on ImGui, but we will eventually also support headless running of pre-created sketches.

## Building

There are four main dependencies:

 * libzmq
 * czmq
 * sdl2
 * libsphactor
 * liblo

Dependencies for the build process / dependencies are:

   * git, libtool, autoconf, automake, cmake, make, pkg-config, pcre

### OSX

#### Building Dependencies

 * Get build dependencies via brew:
```
brew install libtool autoconf automake pkg-config cmake make zeromq czmq sdl2 liblo
```

#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc. 

* Clone the repo
```
git clone http://github.com/hku-ect/gazebosc.git
```
You can now build using cmake/make or generate an Xcode project file.

* Creating an XCode project

To create an xcode project, perform the following commands from the root git folder:

```
mkdir xcodeproj
cd xcodeproj
cmake -G Xcode ..
```
This should generate a valid Xcode project that can run and pass tests.

* Build using make

In the repository root:
```
mkdir build
cd build
cmake ..
make
```

#### Building on Windows

* Install Visual Studio 2019: https://visualstudio.microsoft.com/downloads/
When installing, make sure to include:
- CMake
- Git

* Clone gazebosc repository
```
git clone http://github.com/hku-ect/gazebosc.git
```

* Run "x86 Native Tools Command Prompt for VS 2019" as Administrator
* Navigate to gazebosc project root
    Run "build_windows.bat"

* Run Visual Studio, and select Open -> CMake
    Navigate to gazebosc/CMakeListst.txt

* Select "gazebosc.vcxproj" from debug targets

You are now ready to debug as normal!

#### Alternatively install dependencies from source

 * Clone & build libzmq, czmq, libsphactor & liblo
```
git clone git://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh
./configure
make check
sudo make install
cd ..

git clone git://github.com/zeromq/czmq.git
cd czmq
./autogen.sh 
./configure 
make check
sudo make install
cd ..

git clone git://github.com/hku-ect/libsphactor.git
cd libsphactor
./autogen.sh
./configure 
make check
sudo make install
cd ..

git clone git://liblo.git.sourceforge.net/gitroot/liblo/liblo
cd liblo
./autogen.sh
./configure 
make check
sudo make install
cd ..
```

### (Debian/Ubuntu) Linux

*(tested on Ubuntu 16.04)*

* First install required dependencies *(don't install liblo on Ubuntu 16.04, see below)*

```
sudo apt-get update
sudo apt-get install -y \
    build-essential libtool cmake \
    pkg-config autotools-dev autoconf automake \
    uuid-dev libpcre3-dev libsodium-dev libzmq5-dev \
    libczmq-dev liblo-dev libsdl2-dev
```

* Clone & build libsphactor

```
git clone git://github.com/hku-ect/libsphactor.git
cd libsphactor
./autogen.sh
./configure
make check
sudo make install
cd ..
```

On Ubuntu 16.04 liblo is broken, install liblo from source:

```
git clone git://liblo.git.sourceforge.net/gitroot/liblo/liblo
cd liblo
./autogen.sh
./configure 
make check
sudo make install
cd ..
```

#### Building Gazebosc

Once the above dependencies are installed, you are ready to build Gazebosc:

* Clone the repo and build Gazebosc
```
git clone http://github.com/hku-ect/gazebosc.git
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

---

## Adding new nodes

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

## QtCreator example

Create a new class in Nodes:

* right click the nodes folder, Add new
* Select C++ class
* Enter HelloWorldNomnbvcxzzxcvb8de as the name
* Enter GNode as the base class
* Finish the wizard

It will probably ask you to add the source file to the CMakeLists.txt. Do so as follows:
```
    ...
	GNode.h
	GNode.cpp
	Nodes/DefaultNodes.h
	Nodes/UtilityNodes.cpp
	Nodes/MidiNode.cpp
	Nodes/UDPSendNode.cpp
	Nodes/OSCListenerNode.cpp
    Nodes/HelloWorldNode.h
    Nodes/HelloWorldNode.cpp
)
```
You'll now have an empty skeleton class. In the .h file make the header file look like this:
```
#ifndef HELLOWORLDNODE_H
#define HELLOWORLDNODE_H

#include "GNode.h"

class HelloWorldNode : public GNode
{
public:
    explicit HelloWorldNode(const char* uuid) : GNode(   "MyCustomNodeName",              // title
                                                          { {"OSC", NodeSlotOSC} },       // Input slots
                                                          { {"OSC", NodeSlotOSC} },       // Output slots
                                                            uuid )                        // uuid pass-through
    {

    }
    
    //  this the method which will be called on an event
    zmsg_t *ActorMessage( sphactor_event_t *ev );
};

#endif // HELLOWORLDNODE_H
```

Open "nodes.cpp" and add this node to the registration block and include it:
```
#include "Nodes/HelloWorldNode.h"

...

void RegisterCPPNodes() {
    ...
    
    RegisterNode( "HelloWorldNodeName", GNode::_actor_handler, [](const char * uuid) -> GNode* { return new HelloWorldNode(uuid); });
    
    ...
}
```

Now we can finalize the implementation in HelloWorldNode.cpp. See the comments in the source:
```
#include "HelloWorldNode.h"
#include <iostream>     //  needed for cout
#include <lo/lo_cpp.h>  //  needed to construct an OSC message

zmsg_t *
HelloWorldNode::ActorMessage(sphactor_event_t *ev)
{
    //  just print the event fields
    std::cout << "Hello World Node: name=" << ev->name << " type=" << ev->type << " uuid=" << ev->uuid << "\n";

    //  create a new OSC message
    lo_message oscmsg = lo_message_new();
    //  add a string to the message
    lo_message_add_string(oscmsg, "Hello World");

    //  create a buffer for the message
    byte* buf = new byte[2048];
    size_t len = sizeof(buf);

    //  place the osc message in the buffer
    lo_message_serialise(oscmsg, "hello", buf, &len);
    lo_message_free(oscmsg);

    //  create a zmsg to return
    zmsg_t *returnMsg = zmsg_new();
    //  place the buffer in the zmsg
    zmsg_pushmem(returnMsg, buf, len);
    return returnMsg;
}
```

---

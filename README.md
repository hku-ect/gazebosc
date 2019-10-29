# GazebOSC

A node-based implementation to author high-level sphactor node actors. A multi-in-single-out (for now) information hub that outputs OSC. May support different protocols in future that OSC does not handle well (eg. streaming video)

Also includes a UI based on ImGui, but will eventually also support headless running of pre-created sketches.


## Building

### OSX

### Building Dependencies

There are four main dependencies:

 * libzmq
 * czmq
 * sdl2
 * libsphactor

Dependencies for the build process / dependencies are:

   * git, libtool, autoconf, automake, cmake, make, pkg-config, pcre

Building them should be pretty straight forward:

 * Get dependencies via brew:
```
brew install libtool autoconf automake pkg-config cmake make zeromq sdl2
```
 * Clone & build libzmq, czmq & libsphactor
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

git clone git://github.com/sphaero/libsphactor.git
cd libsphactor
./autogen.sh
./configure 
make check
sudo make install
cd ..
```

---

### Building Gazebosc

Once the above dependencies are installed, you are ready to build libsphactor. The process for this is much the same:

 * Clone the repo
```
git clone http://github.com/sphaero/gazebosc.git
```
 * Creating an XCode project

To create an xcode project, perform the following commands from the root git folder:

```
mkdir xcodeproj
cd xcodeproj
cmake -G Xcode ..
```
This should generate a valid Xcode project that can run and pass tests.

---

## Adding new nodes

### GNode inheritance, important bits

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

---

//
//  GActor.h
//  gazebosc
//
//  Created by aaronvark on 26/09/2019.
//

#ifndef GActor_h
#define GActor_h

#include <string>
#include <vector>
#include <sphactor_library.h>
#include "ImNodes.h"
#include "ImNodesEz.h"

/// A structure defining a connection between two slots of two actors.
struct Connection
{
    /// `id` that was passed to BeginNode() of input node.
    void* input_node = nullptr;
    /// Descriptor of input slot.
    const char* input_slot = nullptr;
    /// `id` that was passed to BeginNode() of output node.
    void* output_node = nullptr;
    /// Descriptor of output slot.
    const char* output_slot = nullptr;

    bool operator==(const Connection& other) const
    {
        return input_node == other.input_node &&
               input_slot == other.input_slot &&
               output_node == other.output_node &&
               output_slot == other.output_slot;
    }

    bool operator!=(const Connection& other) const
    {
        return !operator ==(other);
    }
};

enum GActorSlotTypes
{
    ActorSlotAny = 1,    // ID can not be 0
    ActorSlotPosition,
    ActorSlotRotation,
    ActorSlotMatrix,
    ActorSlotInt,
    ActorSlotOSC
};

/// A structure holding actor state.
struct GActor
{
    /// Title which will be displayed at the center-top of the actor.
    const char* title = nullptr;
    /// uuid string loaded or generated that stores which actor we are
    const char* uuidStr = nullptr;
    /// Flag indicating that actor is selected by the user.
    bool selected = false;
    /// Actor position on the canvas.
    ImVec2 pos{};
    /// List of actor connections.
    std::vector<Connection> connections{};
    /// A list of input slots current actor has.
    std::vector<ImNodes::Ez::SlotInfo> input_slots{};
    /// A list of output slots current actor has.
    std::vector<ImNodes::Ez::SlotInfo> output_slots{};
    /// sphactor instance
    sphactor_t *actor = NULL;
    
    // Construction / Destruction
    //  Inheriting classes should call this constructor with correct in/out data
    explicit GActor(const char* title,
    const std::vector<ImNodes::Ez::SlotInfo>&& input_slots,
                   const std::vector<ImNodes::Ez::SlotInfo>&& output_slots, const char* uuidStr);
    
    virtual ~GActor();
    
    // UI Functions
    void DeleteConnection(const Connection& connection);
    virtual void Render(float deltaTime);
    
    // Sphactor thread functions
    void SetRate( int rate );
    virtual void CreateActor();
    virtual void DestroyActor();
    
    virtual void ActorInit(const sphactor_actor_t *actor);
    virtual void ActorStop(const sphactor_actor_t *actor);
    virtual zmsg_t *ActorCallback();
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
    
    static zmsg_t *_actor_handler(sphactor_event_t *ev, void *args)
    {
        GActor *self = (GActor *)args;
                
        if ( streq(ev->type, "INIT")) {
            self->ActorInit(ev->node);
        }
        else
        if ( streq(ev->type, "TIME")) {
            return self->ActorCallback();
        }
        else
        if ( streq(ev->type, "STOP")) {
            self->ActorStop(ev->node);
        }
        else
        if ( streq(ev->type, "DESTROY")) {
            //TODO: Implement destroy callback
        }
        else
        if ( streq(ev->type, "SOCK")) {
            assert(ev->msg);
            return self->ActorMessage(ev);
        }
        else {
            zsys_warning("EVENT TYPE: %s \n", ev->type);
        }
        
        
        return nullptr;
    }
    
    // Serialization functions
    virtual void SerializeActorData(zconfig_t *section);
    virtual void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it);
};

#endif /* GActor_h */

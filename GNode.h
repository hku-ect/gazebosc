//
//  GNode.h
//  gazebosc
//
//  Created by aaronvark on 26/09/2019.
//

#ifndef GNode_h
#define GNode_h

#include <string>
#include <vector>
#include <sphactor_library.h>
#include "ImNodes.h"
#include "ImNodesEz.h"

/// A structure defining a connection between two slots of two nodes.
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

enum GNodeSlotTypes
{
    NodeSlotAny = 1,    // ID can not be 0
    NodeSlotPosition,
    NodeSlotRotation,
    NodeSlotMatrix,
    NodeSlotInt,
    NodeSlotOSC
};

/// A structure holding node state.
struct GNode
{
    /// Title which will be displayed at the center-top of the node.
    const char* title = nullptr;
    /// Flag indicating that node is selected by the user.
    bool selected = false;
    /// Node position on the canvas.
    ImVec2 pos{};
    /// List of node connections.
    std::vector<Connection> connections{};
    /// A list of input slots current node has.
    std::vector<ImNodes::Ez::SlotInfo> input_slots{};
    /// A list of output slots current node has.
    std::vector<ImNodes::Ez::SlotInfo> output_slots{};
    /// sphactor instance
    sphactor_t *actor;
    
    // Construction / Destruction
    explicit GNode(const char* title,
    const std::vector<ImNodes::Ez::SlotInfo>&& input_slots,
                   const std::vector<ImNodes::Ez::SlotInfo>&& output_slots, const char* uuidStr);
    
    virtual ~GNode();
    
    // UI Functions
    void DeleteConnection(const Connection& connection);
    virtual void Render(float deltaTime);
    
    // Sphactor thread functions
    void SetRate( int rate );
    virtual zmsg_t *ActorCallback();
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
    
    static zmsg_t *_actor_handler(sphactor_event_t *ev, void *args)
    {
        GNode *self = (GNode *)args;
        
        if ( ev->msg == NULL ) {
            return self->ActorCallback();
        }
        
        return self->ActorMessage(ev);
    }
    
    // Serialization functions
    virtual void SerializeNodeData(zconfig_t *section);
    virtual void DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it);
};

#endif /* GNode_h */

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

    explicit GNode(const char* title,
        const std::vector<ImNodes::Ez::SlotInfo>&& input_slots,
        const std::vector<ImNodes::Ez::SlotInfo>&& output_slots, const char* uuidStr)
    {
        zuuid_t *uuid = NULL;
        if ( uuidStr != nullptr ) {
            uuid = zuuid_new();
            zuuid_set_str(uuid, uuidStr);
        }
        
        actor = sphactor_new_by_type(title, this, NULL, uuid);
        sphactor_set_actor_type(actor, title);
        sphactor_set_verbose(actor, true);
        this->title = title;
        this->input_slots = input_slots;
        this->output_slots = output_slots;
    }
    
    virtual void HandleArgs( ImVector<char*> *args, ImVector<char*>::iterator it) {
        char* xpos = *it;
        it++;
        char* ypos = *it;
        it++;
        
        pos.x = atof(xpos);
        pos.y = atof(ypos);
        
        free(xpos);
        free(ypos);
    }

    virtual ~GNode()
    {
        sphactor_destroy(&(this->actor));
    }
    
    void SetRate( int rate ) {
        rate = 1000/rate;
        zstr_sendm(sphactor_socket(this->actor), "SET TIMEOUT");
        char* strRate = new char[64];
        sprintf( strRate, "%i", rate );
        zstr_send(sphactor_socket(this->actor), strRate );
        delete[] strRate;
    }
    
    /// Deletes connection from this node.
    void DeleteConnection(const Connection& connection)
    {
        for (auto it = connections.begin(); it != connections.end(); ++it)
        {
            if (connection == *it)
            {
                connections.erase(it);
                break;
            }
        }
    }
    
    static zmsg_t *_actor_handler(sphactor_event_t *ev, void *args)
    {
        GNode *self = (GNode *)args;
        
        if ( ev->msg == NULL ) {
            return self->Update();
        }
        
        return self->HandleMessage(ev);
    }
    
    virtual zmsg_t *Update()
    {
        return nullptr;
    }

    virtual zmsg_t *HandleMessage(sphactor_event_t *ev)
    {
        assert( ev->msg );
        return ev->msg;
    }
    
    virtual void RenderUI() {
        
    }
    
    virtual void Serialize(zconfig_t *section) {
        zconfig_t *xpos = zconfig_new("xpos", section);
        zconfig_set_value(xpos, "%f", pos.x);
        
        zconfig_t *ypos = zconfig_new("ypos", section);
        zconfig_set_value(ypos, "%f", pos.y);
    }
};

#endif /* GNode_h */

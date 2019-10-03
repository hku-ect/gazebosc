//
//  GNode.h
//  gazebosc
//
//  Created by Call +31646001602 on 26/09/2019.
//

#ifndef GNode_h
#define GNode_h

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
        const std::vector<ImNodes::Ez::SlotInfo>&& output_slots)
    {
        actor = sphactor_new(GNode::_actor_handler, this, title, nullptr);
        sphactor_set_verbose(actor, true);
        this->title = title;
        this->input_slots = input_slots;
        this->output_slots = output_slots;
    }

    virtual ~GNode()
    {
        sphactor_destroy(&(this->actor));
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
        return self->HandleMessage(ev);
    }

    virtual zmsg_t *HandleMessage(sphactor_event_t *ev)
    {
        assert( ev->msg );
        ImGui::LogText("Node %s says: %s", ev->name, zmsg_popstr(ev->msg));
        return nullptr;
    }
    
    virtual zmsg_t *Timer()
    {
        return nullptr;
    }
    
    virtual void RenderUI() {
        
    }
};

#endif /* GNode_h */

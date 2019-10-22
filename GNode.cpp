//
//  GNode.cpp
//  gazebosc
//
//  Created by Call +31646001602 on 22/10/2019.
//

#include "GNode.h"

GNode::GNode(const char* title,
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

GNode::~GNode()
{
    sphactor_destroy(&(this->actor));
}

// Front-end functions

/// Deletes connection from this node.
void GNode::DeleteConnection(const Connection& connection)
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

void GNode::Render(float deltaTime) {
    
}

// Back-end thread functions

void GNode::SetRate( int rate ) {
    rate = 1000/rate;
    zstr_sendm(sphactor_socket(this->actor), "SET TIMEOUT");
    char* strRate = new char[64];
    sprintf( strRate, "%i", rate );
    zstr_send(sphactor_socket(this->actor), strRate );
    delete[] strRate;
}

zmsg_t *GNode::ActorCallback()
{
    return nullptr;
}

zmsg_t *GNode::ActorMessage(sphactor_event_t *ev)
{
    assert( ev->msg );
    return ev->msg;
}

// Serialization functions
//  Make sure when overriding these to always call the GNode version

void GNode::SerializeNodeData(zconfig_t *section) {
    zconfig_t *xpos = zconfig_new("xpos", section);
    zconfig_set_value(xpos, "%f", pos.x);
    
    zconfig_t *ypos = zconfig_new("ypos", section);
    zconfig_set_value(ypos, "%f", pos.y);
}

void GNode::DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it) {
    char* xpos = *it;
    it++;
    char* ypos = *it;
    it++;
    
    pos.x = atof(xpos);
    pos.y = atof(ypos);
    
    free(xpos);
    free(ypos);
}

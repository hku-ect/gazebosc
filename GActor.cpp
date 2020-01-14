//
//  GActor.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "GActor.h"

GActor::GActor(const char* title,
    const std::vector<ImNodes::Ez::SlotInfo>&& input_slots,
    const std::vector<ImNodes::Ez::SlotInfo>&& output_slots, const char* uuidStr)
{
    this->title = title;
    this->uuidStr = uuidStr;
    this->input_slots = input_slots;
    this->output_slots = output_slots;
}

GActor::~GActor()
{
    
}

void GActor::CreateActor() {
    if ( actor != NULL ) {
        DestroyActor();
    }
    
    zuuid_t *uuid = NULL;
    if ( uuidStr != nullptr ) {
        uuid = zuuid_new();
        zuuid_set_str(uuid, uuidStr);
    }
    
    actor = sphactor_new_by_type(title, this, NULL, uuid);
    sphactor_ask_set_actor_type(actor, title);
    //sphactor_set_verbose(actor, false);
}

void GActor::DestroyActor() {
    sphactor_destroy(&this->actor);
    this->actor = NULL;
}

// Front-end functions

/// Deletes connection from this actor.
void GActor::DeleteConnection(const Connection& connection)
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

void GActor::Render(float deltaTime) {
    
}

// Back-end thread functions

void GActor::SetRate( int rate ) {
    rate = 1000/rate;
    zstr_sendm(sphactor_socket(this->actor), "SET TIMEOUT");
    char* strRate = new char[64];
    sprintf( strRate, "%i", rate );
    zstr_send(sphactor_socket(this->actor), strRate );
    delete[] strRate;
}

void GActor::ActorInit( const sphactor_actor_t *actor )
{
    
}

void GActor::ActorStop( const sphactor_actor_t *actor )
{
    
}

zmsg_t *GActor::ActorCallback()
{
    return nullptr;
}

zmsg_t *GActor::ActorMessage(sphactor_event_t *ev)
{
    assert( ev->msg );
    return ev->msg;
}

// Serialization functions
//  Make sure when overriding these to always call the GActor version

void GActor::SerializeActorData(zconfig_t *section) {
    zconfig_t *xpos = zconfig_new("xpos", section);
    zconfig_set_value(xpos, "%f", pos.x);
    
    zconfig_t *ypos = zconfig_new("ypos", section);
    zconfig_set_value(ypos, "%f", pos.y);
}

void GActor::DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it) {
    char* xpos = *it;
    it++;
    char* ypos = *it;
    it++;
    
    pos.x = atof(xpos);
    pos.y = atof(ypos);
    
    free(xpos);
    free(ypos);
}

#ifndef ACTORS_H
#define ACTORS_H

#include "Actors/OSCListenerActor.h"
#include "Actors/PulseActor.h"
#include "Actors/UDPSendActor.h"

zmsg_t * LogActor( sphactor_event_t *ev, void* args  );
zmsg_t * OSCListenerActor( sphactor_event_t *ev, void* args );
zmsg_t * PulseActor( sphactor_event_t *ev, void* args );
zmsg_t * UDPSendActor( sphactor_event_t* ev, void* args );

#endif

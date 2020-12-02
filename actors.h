#ifndef ACTORS_H
#define ACTORS_H

#include "Actors/CountActor.h"
#include "Actors/PulseActor.h"

zmsg_t * CountActor( sphactor_event_t *ev, void* args );
zmsg_t * LogActor( sphactor_event_t *ev, void* args  );
zmsg_t * PulseActor( sphactor_event_t *ev, void* args );

#endif

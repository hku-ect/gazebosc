#ifndef ACTORS_H
#define ACTORS_H

#include "Actors/PulseActor.h"

zmsg_t * LogActor( sphactor_event_t *ev, void* args  );
zmsg_t * PulseActor( sphactor_event_t *ev, void* args );

#endif

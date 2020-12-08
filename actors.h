#ifndef ACTORS_H
#define ACTORS_H

#include "Actors/ClientActor.h"
#include "Actors/CountActor.h"
#include "Actors/NatNetActor.h"
#include "Actors/OSCInputActor.h"
#include "Actors/PulseActor.h"

zmsg_t * LogActor( sphactor_event_t *ev, void* args  );

#endif

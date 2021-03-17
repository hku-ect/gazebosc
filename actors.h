#ifndef ACTORS_H
#define ACTORS_H

#include "Actors/ClientActor.h"
#include "Actors/CountActor.h"
#include "Actors/LiveLinkActor.h"
#include "Actors/NatNetActor.h"
#include "Actors/NatNet2OSCActor.h"
#include "Actors/OSCInputActor.h"
#include "Actors/PulseActor.h"
#include "Actors/ModPlayerActor.h"
#ifdef PYTHON3_FOUND
#include "Actors/pythonactor.h"
#endif

zmsg_t * LogActor( sphactor_event_t *ev, void* args  );

#endif

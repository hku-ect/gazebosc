#ifndef PULSEACTOR_H
#define PULSEACTOR_H

#include "libsphactor.hpp"

class Pulse : public Sphactor {
private:
    int rate;

public:
    Pulse() : Sphactor() {

    }

    zmsg_t * handleInit( sphactor_event_t *ev );
    zmsg_t * handleTimer( sphactor_event_t *ev );

};

#endif // PULSEACTOR_H

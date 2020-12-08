#ifndef PULSEACTOR_H
#define PULSEACTOR_H

struct Pulse {
    int rate;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    Pulse() {

    }
};

#endif // PULSEACTOR_H

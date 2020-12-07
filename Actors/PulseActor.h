#ifndef PULSEACTOR_H
#define PULSEACTOR_H

struct Pulse {
    int rate;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    Pulse() {
        this->rate = 16; //60 fps default
    }
};

#endif // PULSEACTOR_H

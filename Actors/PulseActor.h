#ifndef PULSEACTOR_H
#define PULSEACTOR_H

struct Pulse {
    int rate;

    static void *ConstructPulse( void* args ) {
        return new Pulse(60);
    }

    Pulse( int rate ) {
        this->rate = rate;
    }
};

#endif // PULSEACTOR_H

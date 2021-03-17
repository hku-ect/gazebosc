#ifndef LIVELINKACTOR_H
#define LIVELINKACTOR_H
#include "libsphactor.hpp"

class LiveLink : public Sphactor {
public:
    zmsg_t* handleInit(sphactor_event_t* ev);
    zmsg_t* handleSocket(sphactor_event_t* ev);

    float* values;
    char* name;

    LiveLink() : Sphactor() {}
};

#endif // LIVELINKACTOR_H
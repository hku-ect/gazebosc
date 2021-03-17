#ifndef COUNTACTOR_H
#define COUNTACTOR_H
#include "libsphactor.hpp"

class Count : public Sphactor {
public:
    int msgCount;

    zmsg_t* handleInit(sphactor_event_t* ev);
    zmsg_t* handleSocket(sphactor_event_t* ev);

    Count() : Sphactor() {
        msgCount = 0;
    }
};

#endif // COUNTACTOR_H

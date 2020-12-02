#ifndef COUNTACTOR_H
#define COUNTACTOR_H

struct Count {
    int msgCount;

    static void *ConstructCount( void* args ) {
        return new Count();
    }

    Count() {
        msgCount = 0;
    }
};

#endif // COUNTACTOR_H

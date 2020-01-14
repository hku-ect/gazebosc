#ifndef PYTHONNODE_H
#define PYTHONNODE_H

#include <string>
#include "GActor.h"
#include "Python.h"

void python_init();

class PythonActor : public GActor
{
public:
    explicit PythonActor(const char* uuid) : GActor(   "Python",              // title
                                                          { {"OSC", ActorSlotOSC} },       // Input slots
                                                          { {"OSC", ActorSlotOSC} },       // Output slots
                                                            uuid )                        // uuid pass-through
    {

    }

    int UpdatePythonFile();

    void ActorInit( const sphactor_actor_t *actor );
    zmsg_t *ActorMessage( sphactor_event_t *ev );
    //zmsg_t *ActorCallback( );
    //void ActorStop(const sphactor_node_t *node);

    std::string filename = "tester";
    PyObject *pClassInstance;
};

#endif // PYTHONNODE_H

#ifndef PYTHONNODE_H
#define PYTHONNODE_H

#include <string>
#include "GNode.h"
#include "Python.h"

void python_init();

class PythonNode : public GNode
{
public:
    explicit PythonNode(const char* uuid) : GNode(   "PythonNode",              // title
                                                          { {"OSC", NodeSlotOSC} },       // Input slots
                                                          { {"OSC", NodeSlotOSC} },       // Output slots
                                                            uuid )                        // uuid pass-through
    {

    }

    int UpdatePythonFile();

    void ActorInit( const sphactor_node_t *node );
    zmsg_t *ActorMessage( sphactor_event_t *ev );
    //zmsg_t *ActorCallback( );
    //void ActorStop(const sphactor_node_t *node);

    std::string filename = "tester";
    PyObject *pClassInstance;
};

#endif // PYTHONNODE_H

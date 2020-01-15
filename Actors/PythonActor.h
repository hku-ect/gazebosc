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
        if ( filename == nullptr ) {
            filename = new char[32];
            sprintf(filename, "tester");
            this->UpdatePythonFile();
        }
    }
    
    virtual ~PythonActor() {
        delete[] filename;
    }

    int UpdatePythonFile();

    void ActorInit( const sphactor_actor_t *actor );
    zmsg_t *ActorMessage( sphactor_event_t *ev );
    
    // Serialization overrides
    void SerializeActorData( zconfig_t *section );
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it );
    
    // UI overrides
    void Render(float deltaTime);

    char * filename = nullptr;
    PyObject *pClassInstance;
};

#endif // PYTHONNODE_H

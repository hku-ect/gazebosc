//
//  TestNodes.h
//  gazebosc
//
//  Created by Call +31646001602 on 26/09/2019.
//

#ifndef TestNodes_h
#define TestNodes_h

struct CountNode : GNode
{
    using GNode::GNode;

    int count = 0;

    virtual zmsg_t *Update(sphactor_event_t *ev, void*args)
    {
        count += 1;
        zframe_t *f = zframe_new(&count, sizeof(int));
        zmsg_t* ret = zmsg_new();
        zmsg_add(ret, f);
        return ret;
    }
};

//Most basic form of node that performs its own (threaded) behaviour
struct PulseNode : GNode
{
    using GNode::GNode;
    
    int millis = 16;    //60 fps
    
    virtual zmsg_t *Timer() {
        
        return nullptr;
    }
};

#endif /* TestNodes_h */

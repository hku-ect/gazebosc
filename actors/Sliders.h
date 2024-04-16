#ifndef SLIDERS_H
#define SLIDERS_H
#include <libsphactor.hpp>

class IntSlider : public Sphactor
{
public:
    IntSlider() : Sphactor() {}
    zmsg_t * handleAPI(sphactor_event_t *ev);
    static const char *capabilities;
};

class FloatSlider : public Sphactor
{
public:
    FloatSlider() : Sphactor() {}
    zmsg_t * handleAPI(sphactor_event_t *ev);
    static const char *capabilities;
};

#endif // SLIDERS_H

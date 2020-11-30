#include "libsphactor.h"
#include "PulseActor.h"

zmsg_t * PulseActor( sphactor_event_t *ev, void* args ) {
  if ( streq(ev->type, "INIT")) {
    zsys_info("PULSE ACTOR INIT");

    Pulse * pulse = (Pulse*) args;
    assert(pulse);

    zsys_info("Rate: %i", pulse->rate);
    //TODO: can/should we set rate from here?

    return ev->msg;
  }
  else
  if ( streq(ev->type, "TIME")) {
    zosc_t * osc = zosc_create("/pulse", "s", "PULSE");

    zmsg_t *msg = zmsg_new();
    zframe_t *frame = zframe_new(zosc_data(osc), zosc_size(osc));
    zmsg_append(msg, &frame);

    //TODO: figure out if this needs to be destroyed here...
    zframe_destroy(&frame);

    return msg;
  }
  else return ev->msg;
}

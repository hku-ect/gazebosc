#include "libsphactor.h"

zmsg_t * PulseActor( sphactor_event_t *ev, void* args ) {
  zsys_info("RUNNING");

  zosc_t * osc = zosc_create("/pulse", "s", "PULSE");

  zmsg_t *msg = zmsg_new();
  zframe_t *frame = zframe_new(zosc_data(osc), zosc_size(osc));
  zmsg_append(msg, &frame);

  //TODO: figure out if this needs to be destroyed here...
  zframe_destroy(&frame);

  return msg;
}

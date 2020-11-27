#include "libsphactor.h"

zmsg_t * LogActor( sphactor_event_t *ev, void* args ) {
  if ( streq(ev->type, "SOCK")) {
    if ( ev->msg == NULL ) return NULL;

    byte *msgBuffer;
    zframe_t* frame;

    do {
        frame = zmsg_pop(ev->msg);
        if ( frame ) {
          //parse zosc_t msg
          msgBuffer = zframe_data(frame);
          size_t len = zframe_size(frame);

          const char* msg;
          zosc_t * oscMsg = zosc_frommem( (char*)msgBuffer, len);
          zosc_retr( oscMsg, "s", &msg );

          zsys_info("%s: %s", msgBuffer, msg);
        }
    } while (frame != NULL );

    return NULL;
  }
  else
  return ev->msg;
}

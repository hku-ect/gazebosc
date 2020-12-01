#include "libsphactor.h"

const char * logCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t * LogActor( sphactor_event_t *ev, void* args ) {
  if ( streq(ev->type, "INIT")) {
      sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(logCapabilities));
  }
  else
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

  return ev->msg;
}

#include "libsphactor.h"

zmsg_t * LogActor( sphactor_event_t *ev, void* args ) {
  if ( ev->msg == NULL ) return NULL;

  zsys_info("GOT MSG");

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

        zsys_info(msg);
      }
  } while (frame != NULL );

  return NULL;
}

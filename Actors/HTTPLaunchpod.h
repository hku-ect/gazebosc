#ifndef HTTPLAUNCHPOD_H
#define HTTPLAUNCHPOD_H

#include "libsphactor.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _httplaunchpodactor_t
{
    int port;
    zhttp_server_t *server;
    zsock_t *worker;
};

typedef struct _httplaunchpodactor_t httplaunchpodactor_t;

static const char *httplaunchpodactorcapabilities =
    "capabilities\n"
    "    data\n"
    "        name = \"port\"\n"
    "        type = \"integer\"\n"
    "        help = \"Port number for the webserver\"\n"
    "        value = \"8080\"\n"
    "        api_call = \"SET PORT\"\n"
    "        api_value = \"i\"\n"           // optional picture format used in zsock_send
    "outputs\n"
    "    output\n"
    "        type = \"OSC\"\n";

void * httplaunchpodactor_new_helper(void *args);
httplaunchpodactor_t * httplaunchpodactor_new();
void httplaunchpodactor_destroy(httplaunchpodactor_t **self_p);

zmsg_t *httplaunchpodactor_init(httplaunchpodactor_t *self, sphactor_event_t *ev);
zmsg_t *httplaunchpodactor_timer(httplaunchpodactor_t *self, sphactor_event_t *ev);
zmsg_t *httplaunchpodactor_api(httplaunchpodactor_t *self, sphactor_event_t *ev);
zmsg_t *httplaunchpodactor_socket(httplaunchpodactor_t *self, sphactor_event_t *ev);
zmsg_t *httplaunchpodactor_custom_socket(httplaunchpodactor_t *self, sphactor_event_t *ev);
zmsg_t *httplaunchpodactor_stop(httplaunchpodactor_t *self, sphactor_event_t *ev);

zmsg_t * httplaunchpodactor_handler(sphactor_event_t *ev, void *args);
zmsg_t * httplaunchpodactor_handle_msg(httplaunchpodactor_t *self, sphactor_event_t *ev);

#ifdef __cplusplus
} // end extern
#endif
#endif // HTTPLAUNCHPOD_H

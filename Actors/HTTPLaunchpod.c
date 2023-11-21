#include "HTTPLaunchpod.h"

const char *launchpodhtml =
    "<!doctypehtml><html lang=\"en\"><meta charset=\"UTF-8\"><meta content=\"width=device-width,initial-scale=1\"name=\"viewport\"><title>GazebOSC LaunchPod</title><style>body{margin:0;padding:0;display:grid;grid-template-rows:repeat(3,1fr);grid-template-columns:repeat(3,1fr);min-height:100vh}div{color:#ad5d4e;background-color:#255f85;border:5px solid #204e6e;padding:.1em;text-align:center;cursor:pointer;transition:background-color .3s ease;display:flex;align-items:center;justify-content:center;font-size:15vw}div:hover{background-color:#539987}div:active{background-color:#edae49}</style><div onclick=\"sendPostRequest(1)\">1</div><div onclick=\"sendPostRequest(2)\">2</div><div onclick=\"sendPostRequest(3)\">3</div><div onclick=\"sendPostRequest(4)\">4</div><div onclick=\"sendPostRequest(5)\">5</div><div onclick=\"sendPostRequest(6)\">6</div><div onclick=\"sendPostRequest(7)\">7</div><div onclick=\"sendPostRequest(8)\">8</div><div onclick=\"sendPostRequest(9)\">9</div><script>function sendPostRequest(divNumber){fetch('/launchpod',{method:'POST',headers:{'Content-Type':'application/json',},body:JSON.stringify({divNumber:divNumber}),}).then(response=>{if(!response.ok){throw new Error('Network response was not ok');}\n"
    "return response.json();}).then(data=>{console.log('POST request successful',data);}).catch(error=>{console.error('Error during POST request:',error);});}</script>";

void
s_reset_server(httplaunchpodactor_t *self, sphactor_actor_t *actor)
{
    assert(self);
    if (self->worker)
        zsock_destroy(&self->worker);

    if (self->server)
        zhttp_server_destroy(&self->server);

    assert(self->server == NULL);
    assert(self->worker == NULL);

    zhttp_server_options_t *options = zhttp_server_options_new();
    zhttp_server_options_set_port (options, self->port);
    self->server = zhttp_server_new (options);
    assert (self->server);

    zsock_t *worker = zsock_new_dealer (zhttp_server_options_backend_address (options));
    sphactor_actor_poller_add(actor, worker);
    zsys_info("Init Webserver port: %i", self->port);
}

zosc_t *
s_handle_post(zhttp_request_t *req, zhttp_response_t *resp)
{
    assert(req);
    assert (streq (zhttp_request_method (req), "POST"));
    zosc_t *ret = zosc_new(zhttp_request_url(req));
    const char *content = zhttp_request_content(req);
    const char *type = zhttp_request_content(req);
    zosc_append(ret, "ss", content, type);
    return ret;
}

zosc_t *
s_handle_get(zhttp_request_t *req, zhttp_response_t *resp)
{
    char *fnreq = (char *)zhttp_request_url (req);
    if (streq (fnreq, "/") )
    {
        zhttp_response_set_content_type( resp, "text/html"); //response owns the text buffer
        zhttp_response_set_content_const(resp, launchpodhtml);
    }
    else if (zfile_exists (fnreq) ) {
        zfile_t *frequested = zfile_new( NULL, fnreq);
        assert(frequested);
        assert(zfile_input (frequested) == 0);
        zchunk_t *chunk = zfile_read (frequested, zfile_cursize(frequested), 0);
        assert(chunk);
        char *html = zchunk_strdup(chunk);
        assert(html);
        zhttp_response_set_content_type( resp, "text/html"); //response owns the text buffer
        zhttp_response_set_content( resp, &html);
        zchunk_destroy(&chunk);
        zfile_destroy(&frequested);
    }
    else
    {
        zdir_t *curdir = zdir_new(".", "-");
        zfile_t **files = zdir_flatten (curdir);
        uint index;
        char *content = zmalloc(2000);
        size_t content_size = 2000;
        size_t content_needle = 0;

        for (index = 0;; index++) {
            zfile_t *file = files[index];
            if (!file)
                break;

            const char *fn = zfile_filename(file, NULL);

            // Calculate the remaining space in the content buffer
            size_t remaining_space = content_size - content_needle;

            // Check if there's enough space to append the filename
            if (strlen(fn) + 1 < remaining_space) {  // +1 for the newline character
                // Append the filename to the content buffer
                size_t written = snprintf(content + content_needle, remaining_space, "%s\n", fn);
                assert(written >= 0 && written < remaining_space);
                content_needle += written;
            } else {
                // If not enough space, reallocate the buffer
                char *new_content = realloc(content, content_size + 1000);
                assert(new_content); // Memory allocation error?
                content = new_content;
                content_size = content_size + 1000;

                // Append the filename to the content buffer
                size_t written = snprintf(content + content_needle, content_size - content_needle, "%s\n", fn);
                assert(written >= 0 && written < content_size - content_needle);
                content_needle += written;
            }
        }
        zhttp_response_set_content_type( resp, "text/plain"); //response owns the text buffer
        zhttp_response_set_content( resp, &content);
        zdir_flatten_free (&files);
    }
    return NULL;
}

// this is needed because we have to conform to returning a void * thus we need to wrap
// the httplaunchpodactor_new method, unless some know a better method?
void *
httplaunchpodactor_new_helper(void *args)
{
    return (void *)httplaunchpodactor_new();
}

httplaunchpodactor_t *
httplaunchpodactor_new()
{
    httplaunchpodactor_t *self = (httplaunchpodactor_t *) zmalloc (sizeof (httplaunchpodactor_t));
    assert (self);

    self->port = 8080;
    self->server = NULL;
    self->worker = NULL;
    return self;
}

void
httplaunchpodactor_destroy(httplaunchpodactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        httplaunchpodactor_t *self = *self_p;
        assert(self);
        if( self->server )
            zhttp_server_destroy(&self->server);
        if( self->worker )
            zsock_destroy(&self->worker);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

zmsg_t *
httplaunchpodactor_init(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    //s_reset_server(self, (sphactor_actor_t*)ev->actor);
    return NULL;
}

zmsg_t *httplaunchpodactor_timer(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    return NULL;
}

zmsg_t *httplaunchpodactor_api(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd)
    {
        if ( streq(cmd, "SET PORT") )
        {
            char *port = zmsg_popstr(ev->msg);
            int newport = atoi(port);
            if (newport > 1024 && newport < 65535)
            {
                self->port = newport;
                s_reset_server(self, (sphactor_actor_t*)ev->actor);
            }
            else
                zsys_error("HTTPLauchpod: port should >1024 & <65535");
            zstr_free(&port);
        }
        zstr_free(&cmd);
    }
    return NULL;
}

zmsg_t *httplaunchpodactor_socket(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    return NULL;
}


zmsg_t *httplaunchpodactor_custom_socket(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    assert(ev->msg);
    zframe_t *frame = zmsg_pop(ev->msg);
    zmsg_t *ret = NULL;
    if (zframe_size(frame) == sizeof( void *) )
    {
        void *p = *(void **)zframe_data(frame);
        if ( zsock_is( p ) )
        {
            zsock_t *sock = (zsock_t*)p;
            zhttp_request_t *request = zhttp_request_new ();
            void *connection = zhttp_request_recv (request, sock);
            assert (connection);
            //zsys_info("Request %s", zhttp_request_url(request));
            zhttp_response_t *response = zhttp_response_new (); // 200 by default
            if (streq (zhttp_request_method (request), "GET"))
            {
                zosc_t *ret = s_handle_get(request, response);
                assert(ret == NULL);
            }
            else if (streq (zhttp_request_method (request), "POST"))
            {
                zosc_t *oscm = s_handle_post(request, response);
                ret = zmsg_new();
                zframe_t *f = zosc_packx(&oscm);
                zmsg_append(ret, &f);
            }
            int rc = zhttp_response_send (response, sock, &connection);
            assert (rc == 0);
        }
    }
    return ret;
}

zmsg_t *httplaunchpodactor_stop(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    return NULL;
}

zmsg_t * httplaunchpodactor_handler(sphactor_event_t *ev, void *args)
{
    assert(args);
    httplaunchpodactor_t *self = (httplaunchpodactor_t *)args;
    return httplaunchpodactor_handle_msg(self, ev);
}

zmsg_t * httplaunchpodactor_handle_msg(httplaunchpodactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    assert(ev);
    zmsg_t *ret = NULL;
    // these calls don't require a python instance
    if (streq(ev->type, "API"))
    {
        ret = httplaunchpodactor_api(self, ev);
    }
    else if (streq(ev->type, "DESTROY"))
    {
        httplaunchpodactor_destroy(&self);
        return NULL;
    }
    else if (streq(ev->type, "INIT"))
    {
        return httplaunchpodactor_init(self, ev);
    }

    // these calls require a working server?
    else if ( streq(ev->type, "TIME") )
    {
        ret = httplaunchpodactor_timer(self, ev);
    }
    else if ( streq(ev->type, "SOCK") )
    {
        ret = httplaunchpodactor_socket(self, ev);
    }
    else if ( streq(ev->type, "FDSOCK") )
    {
        ret = httplaunchpodactor_custom_socket(self, ev);
    }
    else if ( streq(ev->type, "STOP") )
    {
        return httplaunchpodactor_stop(self, ev);
    }

    return ret;
}

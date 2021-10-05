#include "ProcessActor.h"

const char * processactorcapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"cmd\"\n"
        "        type = \"string\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET PROCESS\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "inputs\n"
        "    input\n"
        "        type = \"OSC\"\n"
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

void
s_init_zproc(zproc_t **proc_p)
{
    assert(proc_p);
    zproc_t *proc = *proc_p;
    if (proc != nullptr)
    {
        zproc_destroy(&proc);
        assert(proc == nullptr);
    }
    *proc_p = zproc_new();
    assert(*proc_p);
    // capture stdout
    zproc_set_stdout(*proc_p, NULL);
}

zmsg_t * ProcessActor::handleInit(sphactor_event_t *ev)
{
    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(processactorcapabilities));
    s_init_zproc(&this->proc);

    return nullptr;
}


// handle stopping your actor (destroying it is handled for you in the base class)
zmsg_t * ProcessActor::handleStop(sphactor_event_t *ev)
{
    // initialize zproc
    if ( this->proc != nullptr)
    {
        if ( zproc_running(this->proc) )
            zproc_shutdown(this->proc, 100);
        zproc_destroy(&this->proc);
    }

    if ( ev->msg ) zmsg_destroy(&ev->msg); return nullptr;
}

// handle your own API commands
zmsg_t * ProcessActor::handleAPI(sphactor_event_t *ev)
{
    char *cmd = zmsg_popstr(ev->msg);
    assert(cmd);
    if ( streq(cmd, "SET PROCESS") )
    {
        char *arg = zmsg_popstr(ev->msg);
        assert(arg);
        if (strlen( arg ) )
        {
            s_init_zproc(&this->proc);
            zlist_t *arglist = zlist_new();
            zlist_autofree (arglist);
            zlist_append(arglist, (void *)arg);
            // get additional args
            arg = zmsg_popstr(ev->msg);
            while (arg)
            {
                zlist_append(arglist, (void *)arg);
                arg = zmsg_popstr(ev->msg);
            }
            zproc_set_args( this->proc, &arglist);
            int rc = zproc_run(this->proc);
            assert(rc == 0);
            // poll on the stdout socket
            sphactor_actor_poller_add((sphactor_actor_t *)ev->actor, zproc_stdout(this->proc));

        }
        else
            zstr_free(&arg);
    }
    zstr_free(&cmd);
    if ( ev->msg ) zmsg_destroy(&ev->msg); return nullptr;
}

zmsg_t * ProcessActor::handleCustomSocket(sphactor_event_t *ev)
{
    // Get the socket...
    assert(ev->msg);
    zframe_t *frame = zmsg_pop(ev->msg);
    zmsg_t *retmsg = nullptr;
    if (zframe_size(frame) == sizeof( void *) )
    {
        void *p = *(void **)zframe_data(frame);
        if ( zsock_is( p ) && p == zproc_stdout(this->proc) )
        {
            //zframe_t *frame;
            //zsock_brecv (zproc_stdout (this->proc), "f", &frame);
            //zframe_print(frame);
            //zframe_destroy (&frame);

            retmsg = zmsg_recv(zproc_stdout (this->proc));
            //zmsg_dump(retmsg);
            zosc_t *retosc = zosc_new("process/%s");
            while (zmsg_size(retmsg))
            {
                char *s  = zmsg_popstr(retmsg);
                if (strlen(s))
                    zosc_append(retosc, "s", s);
                    //zsys_info("stdout: %s", s);
            }
            if ( strlen(zosc_address(retosc)) )
            {
                zframe_t *data = zosc_packx(&retosc);
                zmsg_append(retmsg, &data);
            }
            else
                zosc_destroy(&retosc);
        }
    }
    if ( ev->msg ) zmsg_destroy(&ev->msg);
    return retmsg;
}

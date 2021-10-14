#include "ProcessActor.h"

const char *ProcessActor::capabilities =
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

zlist_t *
s_string_split(char *stringtosplit, const char delim)
{
    assert(stringtosplit);
    assert(delim);
    const char d[2] = { delim, (char)0x0 };
    zlist_t *retlst = zlist_new();
    zlist_autofree(retlst);

    //  get the first token */
    char *token = strtok(stringtosplit, d);
    if (token == NULL)
    {
        zlist_append(retlst, stringtosplit);
    }
    while (token != NULL)
    {
        zsys_info("%s", token);
        zlist_append(retlst, token);
        token = strtok(NULL, d);
    }
    // free(stringtosplit); the string is modified into the splitted string so will be freed when the zlist is freed
    return retlst;
}

zmsg_t * ProcessActor::handleInit(sphactor_event_t *ev)
{
    s_init_zproc(&this->proc);
    zproc_set_verbose(this->proc, true);

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
    zmsg_t *retmsg = NULL;
    if ( streq(cmd, "SET PROCESS") )
    {
        char *arg = zmsg_popstr(ev->msg);
        assert(arg);
        if (strlen( arg ) )
        {
            // are we polling on stdout?
            sphactor_actor_poller_remove( (sphactor_actor_t *)ev->actor, zproc_stdout(this->proc) );
            s_init_zproc(&this->proc);
            zlist_t *arglist = s_string_split(arg, ' ');
            zproc_set_args( this->proc, &arglist);
            arglist = zproc_args(this->proc); // we need it for reports but set_args destroyed it :S

            int rc = zproc_run(this->proc);
            if (rc != 0)
            {
                //TODO: set error state
                zsys_error("Failed to run command %s", (char *)zlist_first(zproc_args(this->proc)));
                zosc_t *oscm = zosc_create("/failed", "ss", (char *)zlist_first(arglist), "cannot run!");
                sphactor_actor_set_custom_report_data((sphactor_actor_t *)ev->actor, oscm);
            }
            else
            {
                // poll on the stdout socket
                if ( zproc_running )
                {
                    sphactor_actor_poller_add((sphactor_actor_t *)ev->actor, zproc_stdout(this->proc));
                    zosc_t *oscm = zosc_create("/running", "ss", (char *)zlist_first(arglist), "running");
                    sphactor_actor_set_custom_report_data((sphactor_actor_t *)ev->actor, oscm);
                }
                else
                {
                    zsys_info("the process has already finished");
                    int retcode = zproc_returncode(this->proc);
                    zosc_t *oscm = zosc_create("/finished", "si", (char *)zlist_first(arglist), retcode);
                    sphactor_actor_set_custom_report_data((sphactor_actor_t *)ev->actor, oscm);
                }
            }
            zlist_destroy(&arglist);
        }
        zstr_free(&arg);
    }
    zstr_free(&cmd);
    if ( ev->msg ) zmsg_destroy(&ev->msg); return retmsg;
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
                zstr_free(&s);
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

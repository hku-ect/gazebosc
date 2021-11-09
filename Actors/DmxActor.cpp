#include "DmxActor.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

int
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr\n", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr\n", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes\n", errno);
}

const char *DmxActor::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"serial_port\"\n"
        "        type = \"string\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET PORT\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "inputs\n"
        "    input\n"
        "        type = \"OSC\"\n";

zmsg_t *
DmxActor::handleInit(sphactor_event_t *ev)
{
#ifdef __UTYPE_LINUX
    zdir_t *serdir =zdir_new("/dev/serial/by-id", "-");
    zlist_t *dirlist = zdir_list(serdir);
    zfile_t *item = (zfile_t *)zlist_first(dirlist);
    while(item)
    {
        const char *name = zfile_filename(item, NULL);
        zsys_info("serial port: %s", name);
        item = (zfile_t *)zlist_next(dirlist);
    }
    zlist_destroy(&dirlist);
    zdir_destroy(&serdir);
#elif defined __UTYPE_OSX
    zdir_t *serdir =zdir_new("/dev/", "-");
    zlist_t *dirlist = zdir_list(serdir);
    zfile_t *item = (zfile_t *)zlist_first(dirlist);
    while(item)
    {
        const char *name = zfile_filename(item, NULL);
        if ( strncmp("/dev/tty.", name, 9) == 0 )
        {
            zsys_info("serial port: %s", name);
        }
        item = (zfile_t *)zlist_next(dirlist);
    }
    zlist_destroy(&dirlist);
    zdir_destroy(&serdir);
#endif
    return NULL;
}

zmsg_t *
DmxActor::handleAPI(sphactor_event_t *ev)
{
    char *cmd = zmsg_popstr(ev->msg);
    if ( streq(cmd, "SET PORT") )
    {
        char *portname = zmsg_popstr(ev->msg);
        assert(portname);
        if ( streq(portname, "" ) )
        {
            zstr_free(&cmd);
            zstr_free(&portname);
            return nullptr;
        }
        int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
        if (fd < 0)
        {
            zosc_t *msg = zosc_create("/error", "sssiss",
                                      "open error", portname,
                                      "errno", errno,
                                      "error", strerror(errno));
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
            zsys_error ("error %d opening %s: %s\n", errno, portname, strerror (errno));
            zstr_free(&cmd);
            zstr_free(&portname);
            return nullptr;
        }
        else
        {
            zosc_t *msg = zosc_create("/success", "ss",
                                      "opened", portname);
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
        }

        set_interface_attribs (fd, B57600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
        set_blocking (fd, 0);                // set no blocking

        if (this->portname)
        {
            zstr_free(&this->portname);
        }
        this->portname = portname;
        this->fd = fd;

    }
    return NULL;
}

zmsg_t *
DmxActor::handleSocket(sphactor_event_t *ev)
{
    zframe_t *oscf = zmsg_pop(ev->msg);
    assert(oscf);
    zosc_t *oscm = zosc_fromframe(oscf);
    assert(oscm);
    //const char *oscaddress = zosc_address(oscm);
    //assert(oscaddress);
    // we just forward the two ints we receive
    if ( zosc_format(oscm)[0] == 'i' &&  zosc_format(oscm)[1] == 'i' )
    {
        uint32_t channel = 0;
        uint32_t val = 0;
        zosc_retr( oscm, "ii", &channel, &val);
        if (channel > 512) channel = 512;
        if (channel < 0) channel = 0;
        if (val > 255) val = 255;
        if (val < 0) val = 0;
        dmxdata[channel+4] = (unsigned char)val;
        write (this->fd, this->dmxdata, 513);
    }
    return NULL;
}

zmsg_t *
DmxActor::handleTimer(sphactor_event_t *ev)
{
    return NULL;
}

zmsg_t *
DmxActor::handleStop(sphactor_event_t *ev)
{
    close(this->fd);
    return NULL;
}

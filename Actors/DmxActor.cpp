#ifdef HAVE_DMX
#include "DmxActor.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#ifdef __UNIX__
#include <termios.h>
#include <unistd.h>
#endif

#ifdef __WINDOWS__
#include <winbase.h>
#include <tchar.h>
#include <iostream>
#include <string.h>
#include <devpropdef.h>
#include <setupapi.h>
#include <regstr.h>
/// \cond INTERNAL
#define MAX_SERIAL_PORTS 256
/// \endcond
#include <winioctl.h>

// needed for serial bus enumeration:
// 4d36e978-e325-11ce-bfc1-08002be10318}
DEFINE_GUID (GUID_SERENUM_BUS_ENUMERATOR, 0x4D36E978, 0xE325,
0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);

zlist_t *
list_winserialports()
{
    zlist_t *ports = zlist_new();
    zlist_autofree(ports);
    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA DeviceInterfaceData;
    DWORD dataType, actualSize = 0;

    // Search device set
    hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR, 0, 0, DIGCF_PRESENT);
    if (hDevInfo) {
        int i = 0;
        unsigned char dataBuf[MAX_PATH + 1];
        while (TRUE) {
            ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
            DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
            if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)) 
            {
                // SetupDiEnumDeviceInfo failed
                break;
            }

            if (SetupDiGetDeviceRegistryPropertyA(hDevInfo,
                &DeviceInterfaceData,
                SPDRP_FRIENDLYNAME,
                &dataType,
                dataBuf,
                sizeof(dataBuf),
                &actualSize)) 
            {
                // turn blahblahblah(COM4) into COM4

                /*char* begin = nullptr;
                char * end = nullptr;
                begin = strstr((char *)dataBuf, "COM");
                char *portname;
                if(begin)
                {
                    end = strstr(begin, ")");
                    if(end)
                    {
                        *end = 0;   // get rid of the )...
                        strcpy(portname, begin);
                    }
                    if(nPorts++ > MAX_SERIAL_PORTS)
                        break;
                }*/
                zlist_append(ports, (void*)strdup((const char*)dataBuf));
            }               

            i++;
        }
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);

    return ports;
 };

static bool
open_winserialport(HANDLE hComm, const char *portname, int baud)
{
    // I just borrowed this from openframeworks, credits go there!
    char pn[sizeof(portname)];
    int num;
    if(sscanf(portname, "COM%d", &num) == 1){
        // Microsoft KB115831 a.k.a if COM > COM9 you have to use a different
        // syntax
        sprintf(pn, "\\\\.\\COM%d", num);
    } else {
        strncpy(pn, portname, sizeof(portname)-1);
    }

    // open the serial port:
    // "COM4", etc...

    hComm = CreateFileA(pn, GENERIC_READ|GENERIC_WRITE, 0, 0,
                        OPEN_EXISTING, 0, 0);

    if(hComm == INVALID_HANDLE_VALUE){
        zsys_error("open_winserialport: can't open %s", portname);
        return false;
    }


    // now try the settings:
    COMMCONFIG cfg;
    DWORD cfgSize;
    WCHAR buf[80];

    cfgSize = sizeof(cfg);
    GetCommConfig(hComm, &cfg, &cfgSize);
    int bps = baud;
    swprintf(buf, L"baud=%d parity=N data=8 stop=1", baud);

    if(!BuildCommDCBW(buf, &cfg.dcb)){
        zsys_error("open_winserialport: unable to build comm dcb %s", buf);
    }

    // Set baudrate and bits etc.
    // Note that BuildCommDCB() clears XON/XOFF and hardware control by default

    if(!SetCommState(hComm, &cfg.dcb))
    {
        zsys_error("open_winserialport: couldn't set comm state: %i bps, xio %i / %i set baud %i", cfg.dcb.BaudRate, cfg.dcb.fInX, cfg.dcb.fOutX, baud);
    }

    // Set communication timeouts (NT)
    COMMTIMEOUTS tOut;
    //GetCommTimeouts(hComm, &oldTimeout);
    //tOut = oldTimeout;
    // Make timeout so that:
    // - return immediately with buffered characters
    tOut.ReadIntervalTimeout = MAXDWORD;
    tOut.ReadTotalTimeoutMultiplier = 0;
    tOut.ReadTotalTimeoutConstant = 0;
    SetCommTimeouts(hComm, &tOut);

    return true;
}

#endif

static zlist_t *
s_get_serialports()
{
#ifdef __UTYPE_LINUX
    zdir_t *serdir =zdir_new("/dev/serial/by-id", "-");
    zlist_t *dirlist = zdir_list(serdir);
    zfile_t *item = (zfile_t *)zlist_first(dirlist);
    zlist_t *names = zlist_new();
    zlist_autofree(names);
    while(item)
    {
        const char *name = zfile_filename(item, NULL);
        zsys_info("serial port: %s", name);
        zlist_append(names, (void *)name);
        item = (zfile_t *)zlist_next(dirlist);
    }
    zlist_destroy(&dirlist);
    zdir_destroy(&serdir);

    return names;
#elif defined __UTYPE_OSX
    zdir_t *serdir =zdir_new("/dev/", "-");
    zlist_t *dirlist = zdir_list(serdir);
    zfile_t *item = (zfile_t *)zlist_first(dirlist);
    zlist_t *names = zlist_new();
    zlist_autofree(names);
    while(item)
    {
        const char *name = zfile_filename(item, NULL);
        if ( strncmp("/dev/tty.", name, 9) == 0 )
        {
            zsys_info("serial port: %s", name);
            zlist_append(names, (void *)name);
        }
        item = (zfile_t *)zlist_next(dirlist);
    }

    zlist_destroy(&dirlist);
    zdir_destroy(&serdir);
#elif defined __WINDOWS__
    zlist_t *dirlist = list_winserialports();
    char *item = (char *)zlist_first(dirlist);
    while(item)
    {
        zsys_info("serial port: %s", item);
        item = (char *)zlist_next(dirlist);
    }
    return dirlist;
#endif
}

#ifdef __UNIX__
static int
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

static void
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
#endif

const char *DmxActor::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"serial_port\"\n"
        "        type = \"string\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET PORT\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"max channels\"\n"
        "        type = \"int\"\n"
        "        value = \"512\"\n"
        "        min = \"1\"\n"
        "        max = \"512\"\n"
        "        api_call = \"SET CHANNELS\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"refresh ports\"\n"
        "        type = \"trigger\"\n"
        "        api_call = \"REFRESH PORTS\"\n"
        "inputs\n"
        "    input\n"
        "        type = \"OSC\"\n";

zmsg_t *
DmxActor::handleInit(sphactor_event_t *ev)
{
    if (available_ports)
        zlist_destroy(&available_ports);

    available_ports = s_get_serialports();


    if (zlist_size(this->available_ports) > 0)
    {
        zosc_t *msg = zosc_create("/serialports", "ss",
                                  "available", "ports");
        char *item = (char *)zlist_first(this->available_ports);
        int i = 0;
        while(item)
        {
            char num[2] = { '0' + i,0x0 };
            zosc_append(msg, "ss", num, item);
            item = (char*)zlist_next(this->available_ports);

            //zosc_append(msg, "is", i, item);
            //item = (char *)zlist_next(this->available_ports);
            i++;
        }
        sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);

    }
    else
    {
        zosc_t *msg = zosc_create("/serialports", "ss",
                                  "no ports", "available");
        sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
    }

    return nullptr;
}


void
DmxActor::open_serialport()
{
#ifdef __UNIX__
    int fd = open (portname, O_RDWR | O_NOCTTY | O_NONBLOCK);
#endif
}

void
DmxActor::close_serialport()
{
#ifdef __WINDOWS__
    //SetCommTimeouts(hComm, &oldTimeout);
    CloseHandle(hComm);
    hComm = INVALID_HANDLE_VALUE;
#else
    if (this->fd != -1)
        // reset attributes?
        close(this->fd);
    this->fd = -1;
#endif
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
        close_serialport();
#ifdef __WINDOWS__
        if ( ! open_winserialport(this->hComm, portname, 57600) )
        {
            zosc_t *msg = zosc_create("/error", "ss",
                                      "open error", portname);
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
            zstr_free(&cmd);
            zstr_free(&portname);
            return nullptr;
        }
        fd = 666;
#else
        int fd = open (portname, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
#endif

        if (this->portname)
        {
            zstr_free(&this->portname);
        }
        this->portname = portname;
        this->fd = fd;

    }
    else if (streq(cmd, "SET CHANNELS") )
    {
        char *c = zmsg_popstr(ev->msg);
        this->channels = atoi(c);
        zsys_info("SET CHANNELS to: %i", channels);
        zstr_free(&c);
    }
    else if (streq(cmd, "REFRESH PORTS") )
    {
        if (available_ports)
            zlist_destroy(&available_ports);

        available_ports = s_get_serialports();


        if (zlist_size(this->available_ports) > 0)
        {
            zosc_t *msg = zosc_create("/serialports", "ss",
                                      "available", "ports");
            char *item = (char *)zlist_first(this->available_ports);
            int i = 0;
            while(item)
            {
                char num[2] = { '0' + i,0x0 };
                zosc_append(msg, "ss", num[0], item);
                item = (char *)zlist_next(this->available_ports);
                i++;
            }
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);

        }
        else
        {
            zosc_t *msg = zosc_create("/serialports", "ss",
                                      "no ports", "available");
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
        }
    }
    zstr_free(&cmd);
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
        dmxdata[2] = channels & 0xFF;
        dmxdata[3] = channels >> 8;
        dmxdata[channels + 4 ] = 0xe7; // end value
#ifdef __WINDOWS__
        DWORD written;
        if( !WriteFile(this->hComm, this->dmxdata, channels+5, &written, 0) )
        {
               zsys_error("DMX actor: couldn't write to port");
        }
#else
        write (this->fd, this->dmxdata, channels + 5);
#endif
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
    close_serialport();
    if (this->available_ports)
        zlist_destroy(&this->available_ports);
    return NULL;
}
#endif

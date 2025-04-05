// ------------------------------------------------------------------------------------------------
// net/rlog.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/networking/rlog.h"
#include "src/tools/networking/buf.h"
#include "src/tools/networking/intf.h"
#include "src/tools/networking/port.h"
#include "src/tools/networking/udp.h"
#include "src/tools/stdlib/link.h"
#include "src/tools/stdlib/format.h"
#include "src/tools/stdlib/stdarg.h"
#include "src/tools/stdlib/string.h"

#include "src/tools/console/console.h"

// ------------------------------------------------------------------------------------------------
void RlogPrint(const char *fmt, ...)
{
    char msg[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    ConsolePrint(msg);

    uint len = strlen(msg) + 1;

    // For each interface, broadcast a packet
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        if (!Ipv4AddrEq(&intf->broadcastAddr, &g_nullIpv4Addr))
        {
            NetBuf *pkt = NetAllocBuf();

            strcpy((char *)pkt->start, msg);
            pkt->end += len;

            UdpSend(&intf->broadcastAddr, PORT_OSHELPER, PORT_OSHELPER, pkt);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// net/net.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/networking/net.h"
#include "src/tools/networking/arp.h"
#include "src/tools/networking/dhcp.h"
#include "src/tools/networking/loopback.h"
#include "src/tools/networking/tcp.h"

// ------------------------------------------------------------------------------------------------
// Globals

u8 g_netTrace = 0;

// ------------------------------------------------------------------------------------------------
void NetInit()
{
    LoopbackInit();
    ArpInit();
    TcpInit();

    // Initialize interfaces
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        // Check if interface needs IP address dynamically assigned
        if (!intf->ipAddr.u.bits)
        {
            DhcpDiscover(intf);
        }
    }
}

// ------------------------------------------------------------------------------------------------
void NetPoll()
{
    // Poll interfaces
    NetIntf *intf;
    ListForEach(intf, g_netIntfList, link)
    {
        intf->poll(intf);
    }

    TcpPoll();
}

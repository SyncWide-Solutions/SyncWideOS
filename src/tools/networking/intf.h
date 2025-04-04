// ------------------------------------------------------------------------------------------------
// net/intf.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/networking/addr.h"
#include "src/tools/networking/buf.h"
#include "src/tools/stdlib/link.h"

// ------------------------------------------------------------------------------------------------
// Net Interface

typedef struct NetIntf
{
    Link link;
    EthAddr ethAddr;
    Ipv4Addr ipAddr;
    Ipv4Addr broadcastAddr;
    const char *name;

    void (*poll)(struct NetIntf *intf);
    void (*send)(struct NetIntf *intf, const void *dstAddr, u16 etherType, NetBuf *buf);
    void (*devSend)(NetBuf *buf);
} NetIntf;

// ------------------------------------------------------------------------------------------------
// Globals

extern Link g_netIntfList;

// ------------------------------------------------------------------------------------------------
// Functions

NetIntf *NetIntfCreate();
void NetIntfAdd(NetIntf *intf);

// ------------------------------------------------------------------------------------------------
// net/intf.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/networking/intf.h"
#include "src/tools/mem/vm.h"
#include "src/tools/stdlib/string.h"

// ------------------------------------------------------------------------------------------------
// Globals

Link g_netIntfList = { &g_netIntfList, &g_netIntfList };

// ------------------------------------------------------------------------------------------------
NetIntf *NetIntfCreate()
{
    NetIntf *intf = VMAlloc(sizeof(NetIntf));
    memset(intf, 0, sizeof(NetIntf));
    LinkInit(&intf->link);

    return intf;
}

// ------------------------------------------------------------------------------------------------
void NetIntfAdd(NetIntf *intf)
{
    LinkBefore(&g_netIntfList, &intf->link);
}

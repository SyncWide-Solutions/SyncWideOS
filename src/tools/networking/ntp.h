// ------------------------------------------------------------------------------------------------
// net/ntp.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/networking/intf.h"

// ------------------------------------------------------------------------------------------------
void NtpRecv(NetIntf *intf, const NetBuf *pkt);
void NtpSend(const Ipv4Addr *dstAddr);

void NtpPrint(const NetBuf *pkt);

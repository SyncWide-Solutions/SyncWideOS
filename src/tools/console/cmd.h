// ------------------------------------------------------------------------------------------------
// console/cmd.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/types.h"

typedef struct ConsoleCmd
{
    const char *name;
    void (*exec)(uint argc, const char **argv);
} ConsoleCmd;

extern const ConsoleCmd g_consoleCmdTable[];

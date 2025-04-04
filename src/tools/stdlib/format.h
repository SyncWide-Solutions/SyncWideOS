// ------------------------------------------------------------------------------------------------
// format.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/stdarg.h"
#include "src/tools/stdlib/types.h"

int vsscanf(const char *str, const char *fmt, va_list args);
int sscanf(const char *str, const char *fmt, ...);

int vsnprintf(char *str, size_t size, const char *fmt, va_list args);
int snprintf(char *str, size_t size, const char *fmt, ...);

unsigned long int strtoul(const char *nptr, char **endptr, int base);


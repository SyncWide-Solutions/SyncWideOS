// ------------------------------------------------------------------------------------------------
// mem/vm.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/types.h"

// ------------------------------------------------------------------------------------------------
// Page Flags

#define PAGE_WRITE_THROUGH              0x08    // Page-level write-through
#define PAGE_CACHE_DISABLE              0x10    // Page-level cache-disable

// ------------------------------------------------------------------------------------------------
void VMInit();

void *VMAlloc(uint size);
void *VMAllocAlign(uint size, uint align);

void VMMapPages(void *addr, uint size, uint flags);

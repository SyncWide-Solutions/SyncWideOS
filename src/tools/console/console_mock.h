// ------------------------------------------------------------------------------------------------
// console/console_mock.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/types.h"

void Expect_ConsoleOnKeyDown(uint code);
void Expect_ConsoleOnKeyUp(uint code);
void Expect_ConsoleOnChar(char ch);

void Mock_ConsoleInit();

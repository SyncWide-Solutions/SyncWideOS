// ------------------------------------------------------------------------------------------------
// console/console_mock.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/test/test.h"
#include "src/tools/console/console_mock.h"

// ------------------------------------------------------------------------------------------------
static struct MockData
{
    DECLARE_QUEUE(ConsoleOnKeyDown, code);
    DECLARE_QUEUE(ConsoleOnKeyUp, code);
    DECLARE_QUEUE(ConsoleOnChar, ch);
} Mock;

// ------------------------------------------------------------------------------------------------
void ConsoleOnKeyDown(uint code)
{
    MATCH_INT(ConsoleOnKeyDown, code);
}

// ------------------------------------------------------------------------------------------------
void Expect_ConsoleOnKeyDown(uint code)
{
    EXPECT_INT(ConsoleOnKeyDown, code);
}

// ------------------------------------------------------------------------------------------------
void ConsoleOnKeyUp(uint code)
{
    MATCH_INT(ConsoleOnKeyUp, code);
}

// ------------------------------------------------------------------------------------------------
void Expect_ConsoleOnKeyUp(uint code)
{
    EXPECT_INT(ConsoleOnKeyUp, code);
}

// ------------------------------------------------------------------------------------------------
void ConsoleOnChar(char ch)
{
    MATCH_CHAR(ConsoleOnChar, ch);
}

// ------------------------------------------------------------------------------------------------
void Expect_ConsoleOnChar(char ch)
{
    EXPECT_CHAR(ConsoleOnChar, ch);
}

// ------------------------------------------------------------------------------------------------
void Mock_ConsoleInit()
{
    INIT_QUEUE(ConsoleOnKeyDown, code);
    INIT_QUEUE(ConsoleOnKeyUp, code);
    INIT_QUEUE(ConsoleOnChar, ch);
}

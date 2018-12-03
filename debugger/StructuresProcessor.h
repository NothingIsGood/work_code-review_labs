
#ifndef DEBUGGER_STRUCTURESPROCESSOR_H
#define DEBUGGER_STRUCTURESPROCESSOR_H

#include <cinttypes>

#pragma pack(push, 1)
struct command_8_bit // 8 бит -- КОП + бит b
{
    uint8_t code: 7;
    uint8_t b: 1;
};

struct command_24_bit // 24 бита -- КОП + b + adr
{
    uint8_t code: 7;
    uint8_t b: 1;
    uint16_t adr;
};

struct DR_Property
{
    uint8_t mode_ : 3; // 3 бита -- Read Write eXec
    uint8_t active_ : 1;
};

enum BP_FLAGS: uint8_t
{
    READ = 1, WRITE = 2, EXEC = 4 // 001 010 100 соответственно
};

#pragma pack(pop)


enum CommandSizes : size_t {OneByte = 1, ThreeBytes = 3};

union proc_union // Общее объединение для данных
{
    int int_code;
    float float_code;
    uint16_t addr;
    command_8_bit code_8;
    command_24_bit code_24;
};

#endif //DEBUGGER_STRUCTURESPROCESSOR_H

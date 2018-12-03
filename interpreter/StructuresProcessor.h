//
// Created on 05.10.2018.
//

#ifndef INTERPRETER_STRUCTURESPROCESSOR_H
#define INTERPRETER_STRUCTURESPROCESSOR_H

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
#pragma pack(pop)

union proc_union // Общее объединение для данных
{
    int int_code;
    float float_code;
    uint16_t addr;
    command_8_bit code_8;
    command_24_bit code_24;
};

#endif //INTERPRETER_STRUCTURESPROCESSOR_H

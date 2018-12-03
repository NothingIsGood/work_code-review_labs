//
// Created on 01.10.2018.
//

#ifndef INTERPRETER_STRUCTURES_H
#define INTERPRETER_STRUCTURES_H

#include <cinttypes>
#include <string>
#include <functional>
#include <vector>
#include <typeindex>

class Context;

using address = uint16_t;

enum class Error : uint8_t // Все коды ошибок
{
    NoError,
    InvalidChar,
    InvalidLabel,
    RedefenitionLabel,
    InvalidCode,
    EmptyParameter,
    InvalidNumParameters,
    InvalidExpression,
    InvalidOperator,
    InvalidNumber,
    UnexceptedLabel,
    InvalidByteString,
};

union Bytes // Для записи в битовое представление
{
    uint8_t byteArr[4];
    int intVal;
    float fltVal;
    address ptrVal;
};

struct Operation // Структура для каждого оператора программы
{
    std::string Label; // Метка (заканчивается символом :)
    std::string Code; // КОП
    std::vector<std::string> Arguments; // список аргументов
    std::string Comment;

    Error Err = Error::NoError;
    bool work = true; // принадлежит ли обработке на текущем шаге
    size_t LC = 0; // Счетчик размещения
    std::vector<uint8_t> binary; // Двоичное представление
};

using oper_function = std::function<void(Operation&, Context&)>;

struct Operator
{
    oper_function function;     // транслирующая функция
    uint8_t Code;               // у директив = 0
    uint8_t Length;             // у директив = 0
};

/*
    s <адрес> — установка IP
    a <адрес> — установка адреса загрузки
    c <КОП> <b> <адрес> — формат-24 (3-байтовые команды, КОП от 64 до 127)
    c <КОП> <b> — формат-8 (1-байтовые команды, КОП от 0 до 63)
    i <целое число> — загрузка 4-байтового целого
    f <вещественное число> — загрузка 4-байтового вещественного
    p <целое число> — загрузка 2-байтового целого
 */

enum Prefixes : char
{
    intVal  = 'i',
    ptrVal  = 'p',
    comVal  = 'c',
    fltVal  = 'f',
    IPVal   = 's',
    LoadVal = 'a',
    byteStr = 'b',
};


enum OpCodes
{
    aload = 140, aloadoff = 141,
    astore = 142, astoreoff = 143,

    push = 150,
    peek = 152,
    pop = 154,

    iadd = 20,
    isub = 22, isubr = 23,
    imul = 24,
    idiv = 26, idivr = 27,
    imod = 28, imodr = 29,
    ixor = 30,
    ior = 32,
    iand = 34,
    inot = 36,
    ineg = 38,

    fadd = 40,
    fsub = 42, fsubr = 43,
    fmul = 44,
    fdiv = 46, fdivr = 47,
    fsin = 48,
    fcos = 50,
    flog2 = 52,
    fatg = 54,
    flog = 56,
    fexp = 58,
    fsqrt = 60,

    icmp = 80, icmpr = 81,
    fcmp = 82, fcmpr = 83,

    ini = 160,
    inf = 162,
    inp = 164,
    in = 166,

    outi = 170,
    outf = 172,
    outp = 174,
    out = 176,

    jmp = 180, jmpoff = 181,
    jz = 182, jzoff = 183,
    jnz = 184, jnzoff = 185,
    jgz = 186, jgzoff = 187,
    jlz = 188, jlzoff = 189,
    jg = 190, jgoff = 191,
    jl = 192, jloff = 193,

    call = 200,
    ret = 70,
};


#endif //INTERPRETER_STRUCTURES_H

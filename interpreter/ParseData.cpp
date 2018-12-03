//
// Created on 03.10.2018.
//

#include "ParseData.h"

using namespace std;

int digit(char c, int base)
{
    // Возвращает цифру в заданной системе счисления для заданного символа, если нет такого символа, то -1
    // Максимальное основание = 16
    int res = -1; // значение по умолчанию, если выбран неверный символ

    if ((c >= '0' && c <= '9') && ((c - '0') < base))
        res = c - '0';
    else
        if (c >= 'A' && c <= 'Z' && (c - 'A' + 10) < base)
            res = c - 'A' + 10;

    return res;
}

int parseInt(const std::string& value, Error& err)
{
    // перевод последовательности символов в число
    // err = код ошибки (по умолчанию -- NoError)
    // десятичное число = [+-][0-9](n)
    // двоичное = 0b[0-1](n)
    // 16-ричное = 0x[0-9A-F](n)
    // так как могут быть и A-F и a-f, сразу устанавливаем регистр
    std::string parse_str = value;
    transform(begin(parse_str), end(parse_str), begin(parse_str), [](auto ch){return toupper(ch);});

    bool sign = false;
    int base = 10; // основание системы счисления
    int res = 0;

    enum class States
    {
        Start,
        WaitBase,
        Work,
        Error,
    };

    size_t i = 0, n = parse_str.size();
    auto curState = States::Start;

    while (i < n)
    {
        auto ch = parse_str[i];
        switch (curState)
        {
            case States::Start:
                if (ch == '-') // отрицательное десятичное
                {
                    sign = true; curState = States::Work; base = 10;
                }
                else if (ch == '+') // положительное десятичное
                {
                    sign = false; curState = States::Work; base = 10;
                }
                else if (ch == '0') // 0x или 0b
                {
                    curState = States::WaitBase;
                }
                else if (isdigit(ch)) // обычное полож. число
                {
                    sign = false; curState = States::Work; base = 10; res = digit(ch, base);
                }
                else curState = States::Error;
                break;
            case States::WaitBase:
                if (ch == 'X') // 0xabcdef
                {
                    base = 16; curState = States::Work;
                }
                else if (ch == 'B') // 0b1000010010
                {
                    base = 2; curState = States::Work;
                }
                else curState = States::Error;
                break;
            case States::Work:
                if (digit(ch, base) != -1) // цифра подходит к заданной системе счисления
                    res = res * base + digit(ch, base);
                else
                    curState = States::Error;
                break;
            case States::Error:
                err = Error::InvalidNumber;
                break;
        }
        ++i;
    }

    if (curState == States::Error) // если в конце строки ошибка
        err = Error::InvalidNumber;
    if (sign) res = -res;

    return res;
}

float parseFloat(const std::string& value, Error& err)
{
    // Перевод строки в вещественное число
    // Формат -- [знак][целая часть][.][дробная часть][e][знак][порядок]
    // Убрал конечный автомат из-за избыточности, оставил стандартный вызов stof()
    float res{};
    try
    {
        res = stof(value);
    }
    catch (...)
    {
        err = Error::InvalidNumber;
    }
    return res;
}

std::string no_space_symbols(const std::string &value)
{
    // Возврат строки без пробелов
    auto no_space_str = value;
    no_space_str.erase(remove_if(begin(no_space_str), end(no_space_str), [](char c){return isspace(c);}), end(no_space_str));
    return no_space_str;
}

std::string parseString(const std::string &value, Error &err)
{
    // Парсинг строки
    // Задана символами ''
    // Без экранирования символов
    // Читает аргумент до ''
    string res;
    char delim = '\'';
    auto cmp_f = [](auto ch) {return isspace(ch);};

    if (count(begin(value), end(value), delim) == 2) // Без экранирования!
    {
        // Находим границы
        auto iter_start = find(begin(value), end(value), delim);
        auto iter_last = find(iter_start + 1, end(value), delim);

        res = string(iter_start + 1, iter_last); // Строим строку
        if (!(all_of(begin(value), iter_start, cmp_f) && all_of(iter_last + 1, end(value), cmp_f))) // Если до первой и после последней кавычки только пробельные символы
            err = Error::InvalidByteString;
    }
    else err = Error::InvalidByteString;

    return res;
}


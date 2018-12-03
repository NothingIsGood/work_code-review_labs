#include <utility>

//
// Created on 11.09.2018.
//
#pragma once
#ifndef LAB_SPO_1_V2_PROCESSOR_H
#define LAB_SPO_1_V2_PROCESSOR_H

#include <cinttypes>
#include <array>
#include <string>
#include <fstream>
#include <functional>
#include <typeindex>
#include <iostream>
#include <cmath>
#include <memory>

#include "StructuresProcessor.h"

using address = uint16_t;

class Command;

size_t getCommandSize(uint8_t code_op); // Размер команды в байтах

class Processor {
public:
    const float eps = 1.0e-12;
    union stack_value // Объединение для стека -- целые и дробные числа
    {
        int int_val;
        float float_val;
    };
    
    Processor();

    // Не копируемый
    Processor(const Processor& proc) = delete;
    Processor& operator=(const Processor& proc) = delete;

    // Установка флагов
    void setFlags(int res_op);
    void setFlags(float res_float);

    // Аксессоры для полей PSW и адресного регистра
    // Используются тольуо внутри процессора в некоторых функциях
    uint8_t getZF() const noexcept {return PSW_.ZF;}
    void setZF(uint8_t ZF) noexcept {PSW_.ZF = ZF;}

    uint8_t getSF() const noexcept {return PSW_.SF;}
    void setSF(uint8_t SF) noexcept {PSW_.SF = SF;}

    uint8_t getTF() const noexcept {return PSW_.TF;}
    void setTF(uint8_t TF) noexcept {PSW_.TF = TF;}

    uint8_t getSP() const noexcept {return PSW_.SP;}
    void setSP(uint8_t SP) noexcept {PSW_.SP = SP;}

    address getIP() const noexcept {return PSW_.IP;}
    void setIP(address IP) noexcept {PSW_.IP = IP;}

    address getAdrReg() const noexcept {return adr_reg_;}
    void setAdrReg(address adr_reg) noexcept {adr_reg_ = adr_reg;}

    void setByte(size_t idx, char val) {memory_[idx] = (uint8_t) val;}
    uint8_t getByte(size_t idx) {return memory_[idx];}

    void reset()
    {
        // Возвращение полям процессора значений по умолчанию
        PSW_.SP = 15;
        PSW_.IP = 0;
        PSW_.SF = 0; // Знак
        PSW_.ZF = 0; // Ноль
        PSW_.TF = 0; // Трассировка (не установлена по умолчанию) -- нужна для отладчика (ЛР_3)
                     // Переполнение OF не реализовано, в данный момент нет необходимости
        setAdrReg(0);
        for (size_t i = 0; i < MemorySize; ++i)
            memory_[i] = 0;
    }

    // Стек -- 16 значений в процессоре, реализован как массив
    stack_value pop() {stack_value ret = stack_proc_[PSW_.SP]; --PSW_.SP; return ret;}
    stack_value peek() {return stack_proc_[PSW_.SP];}
    void push(stack_value val) {++PSW_.SP; stack_proc_[PSW_.SP] = val; }

    uint32_t loadData(size_t sizeData, int idx) noexcept; // выгрузка заданного числа байт с заданного места в памяти в 32-битную переменную
    void pushData(size_t sizeData, uint32_t data, address currentID) noexcept; // загрузка заданного числа байт в память из поля data

    void Execute(); // выполнение программы

    std::unique_ptr<uint8_t[]> memory_{new uint8_t[MemorySize]{}}; // Память -- байтовая, динамический массив
    // Напрямую не используется, нужна только в выгрузке-загрузке данных, для чего разумнее отдельные методы завести

private:

    static constexpr size_t MaxStackSize {16}; // Стек из 16 элементов
    static constexpr size_t MemorySize {65536}; // Память -- размер адреса 16 бит
    static constexpr size_t CommandsVectorSize {128}; // Для формирования вектора команд, которых может быть только 128

    std::array<stack_value, MaxStackSize> stack_proc_; // Реализация стека для целых и дробных чисел (на основе массива)

    std::array<Command*, CommandsVectorSize> commands_;

    struct
    {
        uint8_t ZF: 1; // Установлен в 1 при результате = 0
        uint8_t SF: 1; // Установлен в 1 при результате < 0
        uint8_t SP: 4; // Указатель стека -- от 0 до 15
        uint8_t TF: 1; // Флаг трассировки
        address : 9;   // Зарезервировано , в данный момент не используется
        address IP;   // Текущий адрес команды
    } PSW_;

    address adr_reg_ {0}; // Адресный регистр 2 байта
};

void Uploader(const std::string& filename, Processor& proc); // загрузка из бинарного файла
void UploaderTxt(const std::string& filename_txt, const std::string& filename_bin); // перевод из текстового файла в бинарный


enum CommandSizes : size_t {OneByte = 1, ThreeBytes = 3};

class Command {
public:
    explicit Command(address size): mode_{size} {}
    address getMode() const noexcept {return mode_;}
    virtual void operator()(Processor& proc, proc_union un) = 0;
private:
    address mode_; // Размер команды, нужен для разделения форматов команд и работы с IP при выполнении
};

// Абстрактные классы для разных форматов команд
class Command_1_Byte : public Command
{
public:
    Command_1_Byte() : Command(CommandSizes::OneByte) {}
};

class Command_3_Bytes : public Command
{
public:
    Command_3_Bytes() : Command(CommandSizes::ThreeBytes) {}
};

// Загрузка-выгрузка адресного регистра
class AdrRegLoader : public Command_3_Bytes
{
public:
    void operator()(Processor& proc, proc_union un) override;
};

class AdrRegSaver : public Command_3_Bytes
{
public:
    void operator()(Processor& proc, proc_union un) override;
    /*
     * Здесь и далее -- override используется только для виртуальных функций
     * Одна из целей -- избавление от повторения virtual и проверка на переопределение
     */
};

// Стек -- загрузка и выгрузка
class StackLoader : public Command_3_Bytes
{
public:
    void operator()(Processor& proc, proc_union un) override;
};

class StackSaver : public Command_3_Bytes // Сохранение вершины стека
{
public:
    void operator()(Processor& proc, proc_union un) override;
};

class StackPop : public Command_3_Bytes // Выгрузка вершины стека
{
public:
    void operator()(Processor& proc, proc_union un) override;
};

// Арифметика -- 2 аргумента
template <typename T>
class MathTwoArgs;

template <>
class MathTwoArgs<float> : public Command_1_Byte // Бинарные математические функции
{
public:
    using T = float;
    explicit MathTwoArgs(std::function<T(T, T)> f) : Command_1_Byte() {func_ = std::move(f);}
    void operator()(Processor& proc, proc_union un) override
    {
        T arg1{}; T arg2 {};
        // Проверки типов

        arg1 = proc.pop().float_val;
        arg2 = proc.pop().float_val;

        T res{};
        if (un.code_8.b) // бит b отвечает за порядок аргументов в бинарной функции, для некоторых операций разницы в порядке нет
            res = func_(arg1, arg2);
        else
            res = func_(arg2, arg1);

        Processor::stack_value val {};

        val.float_val = res;
        proc.setFlags(val.float_val);

        proc.push(val);

    }
private:
    std::function <T(T, T)> func_;
};

template <>
class MathTwoArgs<int> : public Command_1_Byte // Бинарные математические функции
{
public:
    using T = int;
    explicit MathTwoArgs(std::function<T(T, T)> f) : Command_1_Byte() {func_ = std::move(f);}
    void operator()(Processor& proc, proc_union un) override
    {
        T arg1{}; T arg2 {};
        // Проверки типов
        arg1 = proc.pop().int_val;
        arg2 = proc.pop().int_val;

        T res{};
        if (un.code_8.b) // бит b отвечает за порядок аргументов в бинарной функции, для некоторых операций разницы в порядке нет
            res = func_(arg1, arg2);
        else
            res = func_(arg2, arg1);

        Processor::stack_value val {};

        val.int_val = res;
        proc.setFlags(val.int_val);

        proc.push(val);

    }
private:
    std::function <T(T, T)> func_;
};

// Унарная арифметика
template <typename T>
class MathOneArg;

template <> // Для дробных чисел
class MathOneArg<float> : public Command_1_Byte // Поддержка унарных математических функций
{
public:
    using T = float;
    explicit MathOneArg(std::function<T(T)> f) : Command_1_Byte() {func_ = std::move(f);}
    void operator()(Processor& proc, proc_union un) override
    {

        T arg = proc.pop().float_val;
        T res = func_(arg);

        Processor::stack_value val {};
        val.float_val = res;
        proc.setFlags(val.float_val);

        proc.push(val);

    }
private:
    std::function <T(T)> func_;
};

template <> // Для целых чисел
class MathOneArg<int> : public Command_1_Byte // Поддержка унарных математических функций
{
public:
    using T = int;
    explicit MathOneArg(std::function<T(T)> f) : Command_1_Byte() {func_ = std::move(f);}
    void operator()(Processor& proc, proc_union un) override
    {
        T arg = proc.pop().int_val;
        T res = func_(arg);

        Processor::stack_value val {};
        val.int_val = res;
        proc.setFlags(val.int_val);

        proc.push(val);
    }
private:
    std::function <T(T)> func_;
};

// Безусловный переход
class UnconditionalTransition : public Command_3_Bytes
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        address adr_move = un.code_24.adr;
        if (adr_move != 0) // Адрес не равен нулю
        {
            if (un.code_24.b == 1) // Смещение по необходимости
                adr_move += proc.getAdrReg();

        }
        else // Адрес = 0, тогда косвенный переход по адресному регистру
        {
            adr_move = (address) proc.loadData(sizeof(adr_move), proc.getAdrReg());
        }
        proc.setIP(adr_move - getMode()); // Вычитаем адрес команды, так как он прибавляется в любом случае при выполнении команды
    }
};

// Условные переходы
class ConditionalTransition: public Command_3_Bytes
{
public:
    // predicate == функция типа bool(ZF, SF)
    // Работа аналогично безусловному переходу, только с проверкой флагов
    explicit ConditionalTransition(std::function<bool(uint8_t, uint8_t)>&& f): Command_3_Bytes() {predicate_ = f;} // Семантика перемещений для указателя на функцию
    void operator()(Processor& proc, proc_union un) override
    {
        if (predicate_(proc.getZF(), proc.getSF()))
        {
            address adr_move = un.code_24.adr;
            if (adr_move != 0)
            {
                if (un.code_24.b == 1) // Смещение по необходимости
                    adr_move += proc.getAdrReg();
            }
            else
            {
                adr_move = (address) proc.loadData(sizeof(adr_move), proc.getAdrReg());
            }
            proc.setIP(adr_move - getMode()); // Вычитаем адрес команды, так как он прибавляется в любом случае при выполнении команды

        }
    }
private:
    std::function<bool(uint8_t, uint8_t)> predicate_;
};

// Процедуры -- команды

class CallProc: public Command_3_Bytes // Вызов процедуры
{
    void operator()(Processor& proc, proc_union un) override
    {
        // Сохраняем адрес возврата в адресном регистре
        address adr_return = proc.getIP() + getMode(); // IP + размер команды
        proc.setAdrReg(adr_return);
        proc.setIP(un.code_24.adr - getMode()); // getMode() увеличивается при каждом цикле Execute()
    }
};

class ReturnProc: public Command_1_Byte // Возврат из процедуры
{
    void operator()(Processor& proc, proc_union un) override
    {
        proc.setIP(proc.getAdrReg() - getMode()); // Адрес возврата лежит в адресном регистре
    }
};

// Ввод-вывод
template <typename T>
class InputCommand;

template <>
class InputCommand<int>: public Command_3_Bytes // Ввод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        using T = int;
        T value{}; std::cin >> value;
        proc_union data_load{};

        data_load.int_code = value;
        proc.pushData(sizeof(T), static_cast<uint32_t>(data_load.int_code), un.code_24.adr);
    }
};

template <>
class InputCommand<float>: public Command_3_Bytes // Ввод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        using T = int;
        T value{}; std::cin >> value;
        proc_union data_load{};

        data_load.float_code = value;
        proc.pushData(sizeof(T), static_cast<uint32_t>(data_load.int_code), un.code_24.adr);
    }
};

template <>
class InputCommand<address>: public Command_3_Bytes // Ввод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        using T = address;
        T value{}; std::cin >> value;
        proc_union data_load{};

        data_load.addr = value;
        proc.pushData(sizeof(T), static_cast<uint32_t>(data_load.int_code), un.code_24.adr);
    }
};

template<>
class InputCommand<uint8_t>: public Command_3_Bytes
{
    void operator()(Processor& proc, proc_union un) override
    {
        // Символьный ввод
        // Число элементов для ввода берётся из адресного регистра
        // Адрес, куда вводим -- берём из команды
        size_t counter = proc.getAdrReg(), idx = un.code_24.adr;
        for (size_t i = 0; i < counter; ++i)
        {
            char val;
            std::cin >> val;
            proc.setByte(idx, val); ++idx;
        }
    }
};

template <typename T>
class OutputCommand;

template <>
class OutputCommand<int> : public Command_3_Bytes // Вывод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        proc_union data_load{};
        data_load.int_code = proc.loadData(sizeof(int), un.code_24.adr);

        std::cout << data_load.int_code << std::endl;
    }
};

template <>
class OutputCommand<float> : public Command_3_Bytes // Вывод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        proc_union data_load{};
        data_load.int_code = proc.loadData(sizeof(float), un.code_24.adr);

        std::cout << data_load.float_code << std::endl;
    }
};

template <>
class OutputCommand<address> : public Command_3_Bytes // Вывод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        using T = address;
        proc_union data_load{};
        data_load.int_code = proc.loadData(sizeof(T), un.code_24.adr);

        std::cout << data_load.addr << std::endl;
    }
};

template <>
class OutputCommand<uint8_t> : public Command_3_Bytes // Вывод данных в соответствии с типом
{
public:
    void operator()(Processor& proc, proc_union un) override
    {
        // Символьный вывод
        // Число элементов для вывода берётся из адресного регистра
        // Адрес, откуда выводим -- берём из команды
        size_t counter = proc.getAdrReg(), idx = un.code_24.adr;
        for (size_t i = 0; i < counter; ++i)
        {
            char val = proc.getByte(idx);
            std::cout << val;
            ++idx;
        }
    }
};

template <typename T>
class CommandCMP;

template <>
class CommandCMP<int> : public Command_1_Byte
{
public:
    using T = int;
    void operator()(Processor& proc, proc_union un) override
    {
        // Аналогично командам арифметики, только без добавления в стек
        T arg1{}; T arg2 {};
        // Проверки типов

        arg1 = proc.pop().int_val;
        arg2 = proc.pop().int_val;

        T res{};
        if (un.code_8.b) // бит b отвечает за порядок аргументов в бинарной функции, для некоторых операций разницы в порядке нет
            res = arg1 - arg2;
        else
            res = arg2 - arg1;

        Processor::stack_value val {};

        val.int_val = res;
        proc.setFlags(val.int_val);
    }
};

template <>
class CommandCMP<float> : public Command_1_Byte
{
public:
    using T = float;
    void operator()(Processor& proc, proc_union un) override
    {
        // Аналогично командам арифметики, только без добавления в стек
        T arg1{}; T arg2 {};
        // Проверки типов
        arg1 = proc.pop().float_val;
        arg2 = proc.pop().float_val;

        T res{};
        if (un.code_8.b) // бит b отвечает за порядок аргументов в бинарной функции, для некоторых операций разницы в порядке нет
            res = arg1 - arg2;
        else
            res = arg2 - arg1;

        Processor::stack_value val {};
        val.float_val = res;
        proc.setFlags(val.float_val);
    }
};
#endif //LAB_SPO_1_V2_PROCESSOR_H

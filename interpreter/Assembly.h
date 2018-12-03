#include <utility>

//
// Created on 03.10.2018.
//

#ifndef INTERPRETER_ASSEMBLY_H
#define INTERPRETER_ASSEMBLY_H

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <typeindex>
#include <sstream>
#include <iomanip>
#include <numeric>

#include "MathInterpreter.h"
#include "Structures.h"

static const std::string LC_Lex = "$"; // Обозначение счётчика размещения

// Вспомогательные функции для вывода префикса для исполняемого файла в зависимости от типа

template <typename T, typename U>
bool cmpTypes()
{
    return std::type_index(typeid(T)) == std::type_index(typeid(U));
}

template <typename T>
unsigned char getPrefix() // Возвращает префикс-символ в зависимости от типа (указано в описании ВМ)
{
    // i = int
    // f = float
    // p = address
    if (cmpTypes<T, int>()) return Prefixes::intVal;
    else if (cmpTypes<T, float>()) return Prefixes::fltVal;
    else if (cmpTypes<T, address>()) return Prefixes::ptrVal;
    else if (cmpTypes<T, uint8_t>()) return Prefixes::byteStr;

    return 0;
}

template <typename T>
void defineData(Operation& op, Context& c)
{
    // Директивы объявления данных
    size_t sizeVal = sizeof(T);

    T current{}; Error err_current_step; // Текущее значение и наличие ошибки на текущем обрабатываемом аргументе директивы
    Bytes b{}; // Объединение для данных ВМ

    auto prefix = getPrefix<T>(); // Префикс = начальный символ
    // Аналогично виртуальной машине
    // i == целые числа и т.д

    for (const auto& el: op.Arguments)
    {
        err_current_step = Error::NoError;
        if (cmpTypes<T, float>())
        {
            current = parseFloat(no_space_symbols(el), err_current_step); // Дробные без арифметики
            b.fltVal = current;
        }
        else
        {
            current = (T) parseIntExpr(el, c, err_current_step); // Целая арифметика

            if (cmpTypes<T, int>()) // T == int
                b.intVal = current;
            if (cmpTypes<T, address>()) // T == float
                b.ptrVal = current;
        }

        if (err_current_step == Error::NoError) // Если не было ошибки перевода чисел
        {
            op.binary.push_back(prefix); // Префикс - значение как в ВМ (i 1000)
            for (size_t i = 0; i < sizeVal; ++i)
            {
                op.binary.push_back(b.byteArr[i]); // Разбиение по байтам
            }
        }
        else op.Err = err_current_step;
        c.Assign(LC_Lex, c.Lookup(LC_Lex) + sizeVal); // меняем счетчик размещения каждую итерацию
    }
}

template <>
void defineData<uint8_t>(Operation& op, Context& c);


template <typename T>
void memoryData(Operation& op, Context& c) // Директивы определения данных
{
    if (op.Arguments.size() != 1) // Принимаем 1 аргумент
    {
        op.Err = Error::InvalidNumParameters;
    }
    else
    {
        Error parsed_expression = Error::NoError;
        auto counter = (address) parseIntExpr(op.Arguments[0], c, parsed_expression);
        if (parsed_expression == Error::NoError) // Нет ошибок при разборе выражения
        {
            uint8_t prefix = getPrefix<T>(); // Префикс исполняемого файла
            for (size_t i = 0; i < counter; ++i)
            {
                op.binary.push_back(prefix);
                for (size_t j = 0; j < sizeof(T); ++j)
                    op.binary.push_back(0); // зануляем зарезервированные данные
            }
        }
        else
        {
            op.Err = parsed_expression;
        }
        c.Assign(LC_Lex, c.Lookup(LC_Lex) + sizeof(T) * counter);
    }
}

template<uint8_t prefix_in_exec>
void changeAdr(Operation& op, Context& c)
{
    // org == меняет счётчик размещения
    // end == меняет IP

    if (op.Arguments.size() != 1) // Не подходит число аргументов?
        op.Err = Error::InvalidNumParameters;
    else
    {
        address adr_value {}; // Счётчик размещения до 2^16
        Error err_parse = Error::NoError;
        adr_value = (address) parseIntExpr(op.Arguments[0], c, err_parse);
        if (err_parse == Error::NoError)
        {
            // Добавляем префикс и адрес
            op.binary.push_back(prefix_in_exec); // Префикс смены адреса загрузки
            Bytes b{}; b.ptrVal = adr_value; // Используем объединение
            for (address i = 0; i < sizeof(address); ++i) // Загружаем в память
                op.binary.push_back(b.byteArr[i]);

            if (prefix_in_exec == Prefixes::LoadVal) // Меняем ещё и счётчик размещения
                c.Assign(LC_Lex, adr_value);
            else
                op.binary.push_back(0);
        }
        else op.Err = err_parse;
    }
}

template <uint8_t size_of_operands>
void checkCommands(Operation& op, Context& c)
{
    // Проверка команд сводится к вычислению операндов и указание проверки их числа (если требуется)
    // Коды операций и префикс добавляются перед проходом
    Error commandError;   // Ошибка на каждом операнде
    Bytes currentValue{}; // Текущий операнд

    if (op.Arguments.size() == size_of_operands)
    {
        for (const auto& elem: op.Arguments)
        {
            commandError = Error::NoError;
            auto adr_value = (address) parseIntExpr(elem, c, commandError);
            if (commandError == Error::NoError) // Все правильно
            {
                currentValue.ptrVal = adr_value;
                for (address i = 0; i < sizeof(address); ++i)
                {
                    op.binary.push_back(currentValue.byteArr[i]);
                }
            }
            else
            {
                op.Err = commandError; // Неверное выражение
            }
        }
    }
    else op.Err = Error::InvalidNumParameters; // Неверно число параметров

}

/*
 * Первый проход ассемблера =
 *  Построение таблицы имён
 *  При этом =
 *      Для директив == считаем счетчик размещения и формируем представление в байтах
 *      Для команд == проверяем а является ли командой, считаем её размер
 *      Встретили имя -- проверили на переопределение и добавили со значением LC
 *      Если встретили пустые строки, считаем как необрабатываемые
 *      Сохраняем все ошибки
 *
 * Второй проход ассемблера
 *      Для команд == формируем представление в байтах
 *      Сохраняем все ошибки
 *
 *      В конце -- если ошибок нет == формируем exe + листинг (по запросам командной строки)
 *      Иначе -- Формируем только листинг (аналогично)
 *
 *
 * Напомним, что операторы помечаются как необрабатываемые по следующим причинам:
    	оператор является комментарием;
    	оператор является директивой, обработка которой полностью выполнена в первой фазе;
    	в операторе была обнаружена ошибка на первом проходе;

 */


class Assembly
{
public:
    explicit Assembly(std::string filename_asm, std::string filename_exec, std::string filename_list, bool need_listing);

    class FileNotFound : public std::exception
    {
        const char* what() const noexcept override
        {
            return "Критическая ошибка, не найден файл с кодом на ассемблере";
        }
    };

    class NoEndStatement : public std::exception
    {
        const char* what() const noexcept override
        {
            return "Критическая ошибка, отсутствует директива end";
        }
    };

    friend struct ListingAssembly;
private:
    void init(); // Инициализация доступных операций
    Operation tokenize(const std::string& command_str);
    bool has_errors() const
    {
        return std::any_of(std::begin(operations_), std::end(operations_), [](auto elem){return elem.Err != Error::NoError;});
    }
    void first_step(Operation& op, size_t& LC);
    void second_step(); // Второй шаг
    void make_binary_file(); // Создание исполняемого файла

    Context table_symbolic_names_; // Контекст интерпретатора
    std::vector<Operation> operations_; // Программа в операторах
    std::unordered_map<std::string, Operator> asm_operators_; // Операторы и директивы

    std::string filename_asm_; // Файл с программой на ассемблере
    std::string filename_exec_; // Имя для исполняемого файла
    std::string filename_list_; // Имя файла для листинга
};

struct ListingAssembly
{
    // Листинг в виде структуры-функтора
    void operator()(const Assembly& Program);

    std::string getErrorMessage(const Error& err);                  // Сообщение об ошибке
    std::string formStr(const Operation& operation, size_t ind);    // Формирование строки листинга из строки программы
    std::string formTable(const Context& c);                        // Формирование таблицы имён
    bool getErrorStatus(const Error& err);                          // Возвращает true если текущая инструкция ошибочна
    bool isVariable(const std::string& elem);
};


#endif //INTERPRETER_ASSEMBLY_H

#include <utility>

//
// Created on 01.10.2018.
//

#ifndef INTERPRETER_INTERPRETER_H
#define INTERPRETER_INTERPRETER_H

#include "Structures.h"
#include "ParseData.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <stack>

class VariableExp;

class Context // Контекст интерпретатора -- состоит из переменных
{
public:
    int Lookup(const std::string& str) const;
    void Assign(const VariableExp* var, int value);
    void Assign(const std::string& str, int value) {context_[str] = value;}
    bool IsDefined(const std::string& str) const;
    std::unordered_map<std::string, int> getTable() const noexcept {return context_;}
private:
    std::unordered_map<std::string, int> context_; // переменные в ассоц. контейнере
};

class ArithmeticExp // Абстрактный класс для арифметических выражений
{
public:
    virtual int Evaluate(const Context&) = 0;
};

class VariableExp : public ArithmeticExp // Переменная
{
public:
    VariableExp() {};
    VariableExp(std::string var_name) : name_ {std::move(var_name)} {}
    int Evaluate(const Context& aContext) override {return aContext.Lookup(name_);} // Берет значение из контекста
    std::string getName() const noexcept {return name_;}
private:
    std::string name_;
};

class Constant : public VariableExp // Константа
{
public:
    Constant(int m_val) : value_ {m_val} {}
    int Evaluate(const Context& aContext) override {return value_;} // Возвращает непосредственное значение
private:
    int value_;
};

class AddExp : public ArithmeticExp // Целочисленное сложение
{
public:
    AddExp(ArithmeticExp* v1, ArithmeticExp* v2): operand1{v1}, operand2{v2} {}

    virtual int Evaluate(const Context& aContext) override
    {
        return operand1->Evaluate(aContext) + operand2->Evaluate(aContext);
    }
private:
    ArithmeticExp* operand1;
    ArithmeticExp* operand2;
};

class SubExp : public ArithmeticExp // Целочисленное вычитание
{
public:
    SubExp(ArithmeticExp* v1, ArithmeticExp* v2): operand1{v1}, operand2{v2} {}

    virtual int Evaluate(const Context& aContext) override
    {
        return operand2->Evaluate(aContext) - operand1->Evaluate(aContext);
    }
private:
    ArithmeticExp* operand1;
    ArithmeticExp* operand2;
};

class MultExp : public ArithmeticExp // Целочисленное умножение
{
public:
    MultExp(ArithmeticExp* v1, ArithmeticExp* v2): operand1{v1}, operand2{v2} {}

    virtual int Evaluate(const Context& aContext) override
    {
        return operand1->Evaluate(aContext) * operand2->Evaluate(aContext);
    }
private:
    ArithmeticExp* operand1;
    ArithmeticExp* operand2;
};

class DivExp : public ArithmeticExp // Целочисленное деление
{
public:
    DivExp(ArithmeticExp* v1, ArithmeticExp* v2): operand1{v1}, operand2{v2} {}

    virtual int Evaluate(const Context& aContext) override
    {
        return operand2->Evaluate(aContext) / operand1->Evaluate(aContext);
    }
private:
    ArithmeticExp* operand1;
    ArithmeticExp* operand2;
};

class ModExp : public ArithmeticExp // Остаток от деления
{
public:
    ModExp(ArithmeticExp* v1, ArithmeticExp* v2): operand1{v1}, operand2{v2} {}

    virtual int Evaluate(const Context& aContext) override
    {
        return operand2->Evaluate(aContext) % operand1->Evaluate(aContext);
    }
private:
    ArithmeticExp* operand1;
    ArithmeticExp* operand2;
};

class InvalidLex : public std::exception // Ошибка при лексическом анализе
{
    const char* what() const noexcept override {return "Invalid Lexem in Expression";}
};

class InvalidNotation : public std::exception // Ошибка при переводе в обратную польскую нотацию
{
    const char* what() const noexcept override {return "Invalid Notation in Expression";}
};

class InvalidExpression : public std::exception // Ошибка при построении дерева
{
    const char* what() const noexcept override {return "Invalid Expression";}
};

std::vector<std::string> get_lexems(const std::string& parse_str); // Разбиение на токены
std::vector<std::string> get_notation(const std::vector<std::string> & base_vec); // Перевод в обратную польскую нотацию

ArithmeticExp* get_expr_tree(const std::vector<std::string>& vec_notation, const Context& c);

class ArithmeticFactory
{
public:
    // Фабричный метод для вывода операции по её строковому токену
    static ArithmeticExp* getExpr(const std::string& oper, ArithmeticExp* operand1, ArithmeticExp* operand2);
};

int parseIntExpr(const std::string& expr, const Context& c, Error& err); // Объединяет весь код интерпретатора

#endif //INTERPRETER_INTERPRETER_H

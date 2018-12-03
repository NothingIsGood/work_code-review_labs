//
// Created on 01.10.2018.
//

#include "MathInterpreter.h"

using namespace std;

int Context::Lookup(const string &str) const // Возврат переменной по имени из контекста
{
    return context_.find(str)->second;
}

void Context::Assign(const VariableExp *var, int value) // Добавление и инициализация переменной в констексте
{
    context_.insert({var->getName(), value});
}

bool Context::IsDefined(const string &str) const
{
    return context_.find(str) != end(context_);
}

bool is_op(const string& op) // Является ли заданная строка операцией
{
    return op == "*" || op == "/" || op == "%" || op == "+" || op == "-" || op == "(" || op == ")";
};

vector<string> get_lexems(const string& parse_str)
{
    // Возвращает строку лексем, может выбросить исключение InvalidLex (ошибка во время разбора)
    // Разбиение условное, отлавливает только неверные символы

    // Допущение -- числа без знака спереди (без унарных операций вообще)

    vector<string> res;
    size_t i = 0, n = parse_str.size();
    string current_token;

    while (i < n)
    {
        auto ch = parse_str[i];
        if (is_op(string(1, ch)) || ch == '$')
        {
            res.emplace_back(string(1, ch)); ++i;
        } // Операции
        else if (isalpha(ch) || ch == '_') // Идентификаторы
        {
            do
            {
                current_token += ch; ++i; ch = parse_str[i];
            }
            while ((isalpha(ch) || isdigit(ch) || (ch == '_')) && (i < n));
            res.push_back(current_token); current_token = "";
        }
        else if (isdigit(ch)) // Нашли число -- формат не проверяем
        {
            do
            {
                current_token += ch; ++i; ch = parse_str[i];
            }
            while ((isalpha(ch) || isdigit(ch)) && (i < n));
            res.push_back(current_token); current_token = "";
        }
        else throw InvalidLex(); // Другой символ -- ошибка!
    }

    return res;
}

std::vector<std::string> get_notation(const std::vector<std::string> &base_vec)
{
    // Упрощенный алгоритм на основе вывода обратной польской нотации
    // Выбрасывает исключение в случае неверных скобок

    // Допущение -- только бинарные операции

    vector<string> res;   // результирующая строка
    stack<string> alg_stack; // стек операций

    auto priority = [](const string& op) // Возвращает приоритет операции
    {
        if (op == "*" || op == "%" || op == "/") return 2;
        else if (op == "+" || op == "-") return 1;
        else return 0;
    };

    for (auto& elem: base_vec)
    {
        if (elem == "(") // Открывающая строка -- добавили в стек
            alg_stack.push(elem);
        else if (elem == ")")
        {
            // Пока не дошли до открывающей строки -- добавляем в результирующий вектор
            while (!alg_stack.empty() && alg_stack.top() != "(")
            {
                res.push_back(alg_stack.top());
                alg_stack.pop();
            }
            if (!alg_stack.empty()) // Удаляем открывающую скобку, если дошли, иначе кидаем исключение
                alg_stack.pop();
            else throw InvalidNotation();
        }
        else if (is_op(elem))
        {
            const string &op = elem;
            // Пока приоритет вершины на непустом стеке больше чем искомой операции, кидаем из стека в результат
            while (!alg_stack.empty() && (priority(alg_stack.top()) >= priority(op)))
            {
                res.push_back(alg_stack.top());
                alg_stack.pop();
            }
            alg_stack.push(op);
        }
        else // В эту ветку попадут только числа (их корректность проверяется на следующем шаге, как и корректность операндов)
        {
            res.push_back(elem);
        }
    }

    while (!alg_stack.empty()) // Выталкиваем операции из стека в результат
    {
        res.push_back(alg_stack.top());
        alg_stack.pop();
    }

    if (find_if(begin(res), end(res), [](auto token){return token == "(" || token == ")";}) != end(res)) // нашли скобки -- неправильно поставили
        throw InvalidNotation();

    return res;
}

ArithmeticExp* ArithmeticFactory::getExpr(const std::string& oper, ArithmeticExp* operand1, ArithmeticExp* operand2)
{
    // Фабричный метод для возврата соответствующей операции
    ArithmeticExp* res = nullptr;

    if (oper == "+") res = new AddExp(operand1, operand2);
    else if (oper == "-") res = new SubExp(operand1, operand2);
    else if (oper == "*") res = new MultExp(operand1, operand2);
    else if (oper == "/") res = new DivExp(operand1, operand2);
    else if (oper == "%") res = new ModExp(operand1, operand2);

    return res;
}


// Возвращает синтаксическое дерево для заданного выражения
// В случае неверного порядка аргументов выбрасывает исключение
ArithmeticExp* get_expr_tree(const std::vector<std::string>& vec_notation, const Context& c)
{
    // vec_notation = токены в обратной польской нотации
    // c = Контекст выражения (в нём будет храниться таблица символических имён и счётчик размещения)

    stack<ArithmeticExp*> work_stack;

    for(auto& elem: vec_notation)
    {
        if (is_op(elem))
        {
            if (work_stack.size() >= 2) // Операция строго бинарна!
            {
                // Забираем операнды
                ArithmeticExp* op1 = work_stack.top(); work_stack.pop();
                ArithmeticExp* op2 = work_stack.top(); work_stack.pop();

                ArithmeticExp* expr = ArithmeticFactory::getExpr(elem, op1, op2);
                work_stack.push(expr);
            }
            else throw InvalidExpression();
            // Иначе ошибка!
        }
        else if (isalpha(elem[0]) || elem == "$")
        {
            if (c.IsDefined(elem)) // Переменная определена?
                work_stack.push(new Constant(c.Lookup(elem)));
            else throw InvalidExpression(); // Иначе бросаем исключение
        }
        else if (isdigit(elem[0]))
        {
            Error e = Error::NoError;
            int value = parseInt(elem, e); // Парсим как число
            if (e == Error::NoError)
                work_stack.push(new Constant(value));
            else throw InvalidExpression(); // Иначе -- неверный числовой формат!
        }
    }

    if (work_stack.size() == 1) // Корневой элемент
        return work_stack.top();
    else throw InvalidExpression(); // Иначе -- исключение!
}

int parseIntExpr(const std::string& expr, const Context& c, Error& err) // Обертка над остальным функционалом
{
    // c -- содержит все переменные на текущем шаге
    // Работа с контекстом -- на первом проходе!!!
    // В нем храним счетчик размещения, который меняем каждый шаг!!
    // Строка подаётся на вход без пробелов = это делается при парсинге аргументов!
    int res{};
    try
    {
        auto no_space_str = expr;
        no_space_str.erase(remove_if(begin(no_space_str), end(no_space_str), [](char c){return isspace(c);}), end(no_space_str));

        auto tokens = get_lexems(no_space_str);
        auto notation = get_notation(tokens);
        ArithmeticExp* expression = get_expr_tree(notation, c);
        res = expression->Evaluate(c);
    }
    catch (exception&)
    {
        err = Error::InvalidExpression;
    }


    return res;
}
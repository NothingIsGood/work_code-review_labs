#include <utility>

#ifndef DEBUGGER_DEBUGGER_H
#define DEBUGGER_DEBUGGER_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <exception>
#include <iostream>
#include <map>

#include "Processor.h"

static const size_t CountBP = 4; // Число точек останова

class DebugCommand;

class InvalidCommandException : public std::exception
{
public:
    InvalidCommandException(std::string message) : message_{std::move(message)} {}
    const char* what() const noexcept override
    {
        return message_.data();
    }
private:
    std::string message_;
};

std::vector<std::string> getArguments(const std::string& command_str); // Разбиение строки на команду и аргументы

template <typename T>
void setValue(T value, uint16_t adress, Processor& p)
{
    // Установка значения определенного типа в памяти
    union data
    {
        T typeData;
        uint8_t bytes[sizeof(T)];
    };

    data d{}; d.typeData = value;
    for (size_t i = 0; i < sizeof(T); ++i)
        p.memory_[adress + i] = d.bytes[i];
}

template <typename T>
void watchValues(const Processor& proc, size_t counter, uint16_t adr, std::ostream& os)
{
    // просмотр значений определенного типа из памяти
    union data
    {
        T typeData;
        uint8_t bytes[sizeof(T)];
    };

    data d{};

    for (size_t i = 0; i < counter; ++i)
    {
        for (size_t j = 0; j < sizeof(T); ++j)
        {
            d.bytes[j] = proc.memory_[adr];
            ++adr;
        }
        os << d.typeData << " ";
    }
    os << std::endl;
}

void watchCommands(const Processor& proc, size_t counter, uint16_t adr, std::ostream& os);

template <typename T>
void watchStack(const Processor::stack_type& st, std::ostream& os);

template<>
void watchStack<int>(const Processor::stack_type& st, std::ostream& os);

template<>
void watchStack<float>(const Processor::stack_type& st, std::ostream& os);

class Debugger {
public:

    Debugger(const std::string& filename) : filename_ {filename}
    {
        Uploader(filename, p_);
        showInformationMessage();
        initCommands();
    }

    void Start();

private:
    std::string filename_;

    void showInformationMessage();
    void initCommands();
    Processor p_; // Для загрузки программы и выполнения команд ВМ
    std::stringstream protocol_debug; // Протокол отладки

    // Работа с точками останова
    friend class SetBP;
    friend class RemBP;

    // Работа с регистрами и памятью
    friend class WatchReg;
    friend class SetReg;
    friend class WatchStack;
    friend class SetStack;
    friend class WatchAdr;
    friend class SetAdr;

    // Управление выполнением
    friend class ExecNoBP;
    friend class ExecToBP;
    friend class ExecTraceBP;

    std::map<std::string, DebugCommand*> commands_;
};

class DebugCommand
{
public:
    virtual void operator()(const std::vector<std::string>& args, Debugger& debug) = 0;
};


class SetBP : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class RemBP : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class WatchReg : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class SetReg : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class WatchStack : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class SetStack : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class WatchAdr : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class SetAdr : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class ExecNoBP : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class ExecToBP : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};

class ExecTraceBP : public DebugCommand
{
public:
    void operator()(const std::vector<std::string>& args, Debugger& debug) override;
};



#endif //DEBUGGER_DEBUGGER_H

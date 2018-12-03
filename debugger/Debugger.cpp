
#include "Debugger.h"

using namespace std;

void watchCommands(const Processor &proc, size_t counter, uint16_t adr, std::ostream &os)
{
    for (size_t i = 0; i < counter; ++i)
    {
        // Берем первый КОП
        uint8_t code_op = proc.memory_[adr++];
        os << std::hex << code_op << ' ';

        size_t command_size = getCommandSize(code_op);
        for (size_t j = 1; j < command_size; ++j)
        {
            os << setw(2) << setfill('0') << std::hex << (int)proc.memory_[adr++];
        }
        os << " | ";
    }

    os << std::endl;
}

void Debugger::initCommands() {
    // Инициализация команд
    commands_["set_bp"] = new SetBP();
    commands_["rem_bp"] = new RemBP();
    commands_["watch_reg"] = new WatchReg();
    commands_["set_reg"] = new SetReg();
    commands_["watch_stack"] = new WatchStack();
    commands_["set_stack"] = new SetStack();
    commands_["watch_adr"] = new WatchAdr();
    commands_["set_adr"] = new SetAdr();
    commands_["exec"] = new ExecNoBP();
    commands_["exec_bp"] = new ExecToBP();
    commands_["exec_trace"] = new ExecTraceBP();
}

template<>
void watchStack<int>(const Processor::stack_type& st, std::ostream& os)
{
    // Просмотр всего стека (как целые числа)
    for (auto i : st)
        os << i.int_val << " ";
    os << std::endl;
}

template<>
void watchStack<float>(const Processor::stack_type& st, std::ostream& os)
{
    // Просмотр всего стека (как дробные числа)
    for (auto i : st)
        os << i.float_val << " ";
    os << std::endl;
}

void Debugger::showInformationMessage() {
    // Вывод приветствия
    cout << "Отладочная сессия начата" << endl;
    cout << "IP = " << p_.getIP() << endl;
}

void Debugger::Start() {
    // Основной цикл работы с отладчиком тут
    ofstream ofs(filename_ + "_protocol.txt"); // Для протокола

    const string exec_command = "quit";
    string current_line;
    string current_command;

    do {
        getline(cin, current_line);
        auto vec_list = getArguments(current_line);
        if (!vec_list.empty())
            current_command = vec_list.front();
        try
        {
            if (commands_[current_command])
                commands_[current_command]->operator()(vec_list, *this);
        }
        catch (exception & e) // Ошибка в операндах
        {
            cout << e.what() << endl;
        }

    } while (current_command != exec_command);

    ofs << protocol_debug.str();
    ofs.close();
}

std::vector<std::string> getArguments(const std::string &command_str)
{
    // Возвращает разбитую команду и аргументы
    vector<string> res;
    stringstream str(command_str);

    while (!str.eof())
    {
        string tmpValue; str >> tmpValue;
        res.push_back(tmpValue);
    }

    return res;
}

// Команды отладчика

void SetBP::operator()(const std::vector<std::string>& args, Debugger& debug)
{
    // Установка точки останова
    // set_bp <номер> <тип> <адрес>
    // <тип> -- 1 (Read) - 2 (Write) - 4 (Exec)

    if (args.size() < 4) throw InvalidCommandException("Неверно число параметров");

    size_t number; uint8_t type; uint16_t adr;
    number = static_cast<size_t>(stoi(args[1])); type = static_cast<uint8_t>(stoi(args[2])); adr = static_cast<uint16_t>(stoi(args[3]));

    if (number >= CountBP) throw InvalidCommandException("Неверный номер точки останова!");
    debug.p_.setBP(adr, number, type);

    debug.protocol_debug << "set_bp : " << " adr = " << adr << " type " << (int) type << " number = " << number << endl;
}


void RemBP::operator()(const std::vector<std::string> &args, Debugger& debug)
{
    // Удаление точки останова по номеру, либо, если не указано, удаляем все
    if (args.size() > 1)
    {
        size_t numb = stoul(args[1]);
        if (numb >= CountBP) throw InvalidCommandException("Неверный номер точки останова!");
        debug.p_.remBP(numb);
        debug.protocol_debug << "rem_bp : " << numb << endl;
    }
    else
    {
        for (size_t i = 0; i < CountBP; ++i)
            debug.p_.remBP(i);
        debug.protocol_debug << "rem_bp : all" << endl;
    }
}

void WatchReg::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // watch_reg
    debug.protocol_debug << "watch_reg : " << debug.p_.getAdrReg() << endl;
    std::cout << "watch_reg : " << debug.p_.getAdrReg() << endl;
}

void SetReg::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // set_reg <адрес>
    if (args.size() < 2)
        throw InvalidCommandException("Неверное число аргументов");

    auto adr = static_cast<uint16_t>(stoi(args[1]));
    debug.p_.setAdrReg(adr);
    debug.protocol_debug << "set_reg : " << adr << endl;

}

void WatchStack::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // watch_stack -f
    // watch_stack -i
    // Просмотр всего стека как дробные и целые числа

    debug.protocol_debug << "watch_stack : ";

    if (args.size() < 2 || (args[1] != "-f" && args[1] != "-i")) throw InvalidCommandException("Пропущен или неверно указан параметр для типа");
    if (args[1] == "-f")
    {
        watchStack<float>(debug.p_.ArrayStack(), debug.protocol_debug);
        watchStack<float>(debug.p_.ArrayStack(), cout);
    }
    else
    {
        watchStack<int>(debug.p_.ArrayStack(), debug.protocol_debug);
        watchStack<int>(debug.p_.ArrayStack(), cout);
    }

}

void SetStack::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // set_stack -f/-i <номер> <значение>

    if (args.size() < 4) throw InvalidCommandException("Неверно число параметров!");
    if (args[1] != "-f" && args[1] != "-i") throw InvalidCommandException("Неверно указан параметр для типа");
    size_t number = stoul(args[2]);
    if (args[1] == "-f")
    {
        // float
        float fVal = stof(args[3]);
        debug.p_.ArrayStack()[number].float_val = fVal;
    }
    else
    {
        // int
        int fVal = stoi(args[3]);
        debug.p_.ArrayStack()[number].int_val = fVal;
    }

    debug.protocol_debug << "set_stack : " << args[1] << ' ' << args[2] << ' ' << args[3];
}

void WatchAdr::operator()(const std::vector<std::string> &args, Debugger &debug) {
    // watch_adr -i/-f/-b/-c <адрес> <количество>
    if (args.size() < 4) throw InvalidCommandException("Неверно число параметров!");
    if (args[1] != "-f" && args[1] != "-i" && args[1] != "-b" && args[1] != "-c") throw InvalidCommandException("Неверно указан параметр для типа");

    auto adr = static_cast<uint16_t>(stoul(args[2]));
    auto count = static_cast<unsigned int>(stoul(args[3]));

    debug.protocol_debug << "watch_adr : " << args[1] << ' ' << args[2] << ' ' << args[3] << endl;

    if (args[1] == "-f")
    {
        // float
        watchValues<float>(debug.p_, count, adr, std::cout);
        watchValues<float>(debug.p_, count, adr, debug.protocol_debug);
    }
    else if (args[1] == "-i")
    {
        // int
        watchValues<int>(debug.p_, count, adr, std::cout);
        watchValues<int>(debug.p_, count, adr, debug.protocol_debug);
    }
    else if (args[1] == "-b")
    {
        // char
        watchValues<uint8_t>(debug.p_, count, adr, std::cout);
        watchValues<uint8_t>(debug.p_, count, adr, debug.protocol_debug);
    }
    else
    {
        watchCommands(debug.p_, count, adr, std::cout);
        watchCommands(debug.p_, count, adr, debug.protocol_debug);
    }
}


void SetAdr::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // set_adr -i/-f/-p <адрес> <значение>
    if (args.size() < 4) throw InvalidCommandException("Неверно число параметров!");
    if (args[1] != "-f" && args[1] != "-i" && args[1] != "-p") throw InvalidCommandException("Неверно указан параметр для типа");

    auto adr = static_cast<uint16_t>(stoul(args[2]));

    if (args[1] == "-f")
    {
        // float
        float tmp = stof(args[3]);
        setValue(tmp, adr, debug.p_);
    }
    else if (args[1] == "-i")
    {
        // int
        int tmp = stoi(args[3]);
        setValue(tmp, adr, debug.p_);
    }
    else
    {
        auto tmp = static_cast<uint16_t>(stoul(args[3]));
        setValue(tmp, adr, debug.p_);
        // address
    }

    debug.protocol_debug << "set_adr : " << args[1] << ' ' << args[2] << ' ' << args[3] << endl;
}

void ExecNoBP::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // exec -- выполнение без учета трассировки и точек останова
    debug.p_.setRB(0); debug.p_.setTF(0);
    debug.p_.Execute();

    debug.protocol_debug << "exec : " << endl;
    debug.protocol_debug << debug.p_.getTraceLog();

    cout << debug.p_.getTraceLog();
}

void ExecToBP::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // exec_bp -- выполнение до точки останова
    debug.p_.setRB(1);
    debug.p_.Execute();

    debug.protocol_debug << "exec_bp : " << endl;
    debug.protocol_debug << debug.p_.getTraceLog();

    cout << debug.p_.getTraceLog();
}

void ExecTraceBP::operator()(const std::vector<std::string> &args, Debugger &debug)
{
    // exec_trace -- выполнение покомандное, с учетом точек останова
    debug.p_.setRB(1); debug.p_.setTF(1);
    debug.p_.Execute();

    debug.protocol_debug << "exec_trace : " << endl;
    debug.protocol_debug << debug.p_.getTraceLog();

    cout << debug.p_.getTraceLog();
}
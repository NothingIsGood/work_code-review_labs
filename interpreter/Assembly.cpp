//
// Created on 03.10.2018.
//

#include "Assembly.h"

using namespace std;

template<>
void defineData<uint8_t>(Operation &op, Context &c)
{
    // Директивы объявления данных
    // Важно!!
    // В аргументах одна строка!
    // Без экранирования символов
    // Запятые используются при разделении аргументов до основной обработки директив
    size_t sizeVal = sizeof(uint8_t);

    std::string current_str{}; Error err_current_step; // Текущее значение и наличие ошибки на текущем обрабатываемом аргументе директивы

    auto prefix = getPrefix<uint8_t>(); // Префикс = начальный символ

    auto el = accumulate(begin(op.Arguments), end(op.Arguments), std::string());

    err_current_step = Error::NoError;
    current_str = parseString(el, err_current_step);

    if (err_current_step == Error::NoError) // Если не было ошибки перевода чисел
     {
         for (char i : current_str) {
             op.binary.push_back(prefix); // Префикс - значение как в ВМ (i 1000)
             op.binary.push_back((uint8_t) i); // Разбиение по байтам
             c.Assign(LC_Lex, c.Lookup(LC_Lex) + sizeVal); // меняем счетчик размещения каждую итерацию
         }
     }
     else op.Err = err_current_step;

}

void proc(Operation& op, Context& c) // Обработка директивы proc
{
    if (op.Label.empty())
        op.Err = Error::UnexceptedLabel;
    else if (!op.Arguments.empty())
        op.Err = Error::InvalidNumParameters;

}

void endp(Operation& op, Context& c) // Обработка директивы endp
{
    if (op.Arguments.size() != 1)
        op.Err = Error::InvalidNumParameters;
    else if (!c.IsDefined(no_space_symbols(op.Arguments[0])))
        op.Err = Error::InvalidLabel;
}

void equ(Operation& op, Context& c) // Обработка директивы equ
{
    // метка equ значение
    if (op.Label.empty()) // Нет метки?
        op.Err = Error::UnexceptedLabel;

    else
    {
        Error value_error = Error::NoError;
        auto value = parseIntExpr(op.Arguments[0], c, value_error);
        if (value_error == Error::NoError) // Выражение без ошибок?
            c.Assign(op.Label, value);
        else
            op.Err = value_error;

    }

}

Assembly::Assembly(std::string filename_asm, std::string filename_exec, std::string filename_list, bool need_listing)
{
    filename_asm_ = std::move(filename_asm);
    filename_exec_ = std::move(filename_exec);
    filename_list_ = std::move(filename_list);

    // Добавление операций и контекста
    init();

    std::ifstream ofs(filename_asm_);

    size_t LC = 0;
    bool gotEnd = false; // Дошли до директивы end

    if (ofs.is_open())
    {
        while (!ofs.eof() && !gotEnd)
        {
            string current_line;
            getline(ofs, current_line);

            auto op = tokenize(current_line);

            if ((op.Label.empty() && op.Code.empty()) || (op.Err != Error::NoError)) // Ошибки или нечего обрабатывать
                op.work = false;

            first_step(op, LC);

            if (op.Code == "end" && op.Err == Error::NoError)
                gotEnd = true;

            operations_.push_back(op);
        }

        if (!gotEnd)
        {
            throw NoEndStatement();
        }

        // formListing(); == отдельный класс

        if (!has_errors()) // Нет ошибок на первом проходе ?
        {
            second_step(); // Второй проход ассемблера
            if (!has_errors())
                make_binary_file();
        }
        if (need_listing) // Если нужен листинг
        {
            ListingAssembly lst; // Формируем листинг
            lst(*this);
        }

    }
    else
    {
        throw FileNotFound();
    }

}

vector<string> parse_arguments(const std::string& str)
{
    // Переводит строку формата "Аргумент1, Аргумент2" в последовательность {Аргумент1, Аргумент2}
    // Разбивает строку по символу - разделителю и возвращает итоговую последовательность

    vector<string> res;

    char delimeter = ',';
    size_t prev = 0;
    size_t next;

    while((next = str.find(delimeter, prev)) != string::npos)
    {
        string tmp = str.substr(prev, next - prev);
        res.push_back(str.substr(prev, next - prev));
        prev = next + 1;
    }
    res.push_back(str.substr(prev));

    return res;
}

Operation Assembly::tokenize(const string &command_str)
{
    // разбиение строки на последовательность токенов
    // разделители -- пробелы и запятые (запятые остаются в токенах)
    Operation res {};
    // Переписан на основе конечного автомата -- сложность уменьшилась до линейной
    enum class State
    {
        Start,      // Стартовое состояние
        WaitOp,     // Ожидание кода операции
        Label,      // Ввод метки
        NoColon,    // Отсутствие символа :
        CodeOp,     // Ввод кода операции
        WaitArg,    // Ожидание аргументов
        Arg,        // Ввод аргумента
        Finish,     // Конечное состояние
        Error       // Состояние ошибки
    };

    enum class CharClass
    {
        Blank,      // пробелы и табуляция
        Colon,      // Двоеточие
        Comment,    // Комментарий
        Digit,      // Цифра
        Id,         // Идентификатор
        Underline,  // Подчеркивание
        Arg,        // Аргумент (они же любые символы)
        NotBlank = Arg,   // Непробельный символ
    };

    auto get_char_class = [](char x) -> CharClass
    {
        if (isspace(x))	    return CharClass::Blank;
        if (x == ':')       return CharClass::Colon;
        if (x == ';')	    return CharClass::Comment;    // Начало комментария
        if (isdigit(x))     return CharClass::Digit;    // Цифра
        if (isalpha(x))	    return CharClass::Id;		    // Латиница
        if (x == '_')	    return CharClass::Underline;	// Подчеркивание
        return CharClass::NotBlank;
    };

    char current; size_t i = 0; size_t n = command_str.length();
    State cur_state = State::Start; string parsed_str;

    while (i < n)
    {
        current = command_str[i];
        CharClass ch = get_char_class(current);
        switch (cur_state)
        {
            case State::Start: // Первый символ -- пробел == значит будет код операции дальше, иначе -- будет метка
                if (ch == CharClass::Blank) cur_state = State::WaitOp;
                else if ((ch == CharClass::Id) || (ch == CharClass::Underline))
                {
                    parsed_str = current;
                    cur_state = State::Label;
                }
                else if (ch == CharClass::Comment)
                {
                    res.work = false;
                    res.Comment = parsed_str;  cur_state = State::Finish;
                }
                else cur_state = State::Error;
                break;
            case State::WaitOp: // Ждем первого символа кода операции
                if (ch == CharClass::Blank) cur_state = State::WaitOp;
                else if (ch == CharClass::Id) // Дождались
                {
                    parsed_str = current;
                    cur_state = State::CodeOp;
                }
                else if (ch == CharClass::Comment) // дошли до комментария
                {
                    if (res.Label.empty())
                        res.work = false;

                    res.Comment = parsed_str;
                    cur_state = State::Finish;
                }
                else cur_state = State::Error; break;
            case State::Label: // Метка
                if ((ch == CharClass::Id) || (ch == CharClass::Underline) || (ch == CharClass::Digit)) // Допустимый символ
                {
                    parsed_str += current;
                    cur_state = State::Label;
                }
                else if (ch == CharClass::Colon) // Конец метки
                {
                    res.Label = parsed_str; parsed_str = "";
                    cur_state = State::WaitOp;
                }
                else if (ch == CharClass::Blank) // пробел без конца метки
                {
                    res.Label = parsed_str; parsed_str = "";
                    cur_state = State::NoColon;
                }
                else cur_state = State::Error; break;
            case State::CodeOp: // Вводим код
                if (ch == CharClass::Id) // Допустимый символ
                {
                    parsed_str += current;
                    cur_state = State::CodeOp;
                }
                else if (ch == CharClass::Blank) // Конец кода операции -- ждем операнды
                {
                    res.Code = parsed_str;
                    parsed_str = "";
                    cur_state = State::WaitArg;
                }
                else if(ch == CharClass::Comment) // Дошли до комментария
                {
                    res.Code = parsed_str; parsed_str = "";
                    res.Comment = command_str.substr(i);
                    cur_state = State::Finish;
                }
                else cur_state = State::Error; break;
            case State::WaitArg: // Ждем аргументы
                if (ch == CharClass::Blank) cur_state = State::WaitArg; // Считаем пробелы
                else if (ch == CharClass::Comment) // Дождались комментария
                {
                    res.Comment = command_str.substr(i);
                    cur_state = State::Finish;
                }
                else // Аргумент
                {
                    parsed_str = current;
                    cur_state = State::Arg;
                }
                break;
            case State::Arg:
                if(ch == CharClass::Comment) // Комментарий
                {
                    res.Arguments = parse_arguments(parsed_str);
                    parsed_str = "";
                    res.Comment = command_str.substr(i);
                    cur_state = State::Finish;
                }
                else // Считаем аргумент
                {
                    parsed_str += current;
                    cur_state = State::Arg;
                }
                break;

            case State::Finish: // До конца -- все хорошо
                return res;
            case State::Error: // Ошибка -- неверный символ
                res.Err = Error::InvalidChar;
                res.work = false;
                res.Comment = parsed_str;
                return res;
            case State::NoColon: // Неверная метка
                res.Err = Error::InvalidLabel;
                cur_state = State::WaitOp;
                break;
        }

        ++i;
    }

    // Дозапись при конце строки в заданных состояниях
    if (cur_state == State::CodeOp)  res.Code = parsed_str;
    else if (cur_state == State::Arg) res.Arguments = parse_arguments(parsed_str);
    else if (cur_state == State::Label)
    {
        res.Err = Error::InvalidOperator;
        res.work = false;
        res.Comment = command_str;
    }

    if (!res.Arguments.empty()) // Есть ли пустые строки-аргументы?
    {
        if (any_of(begin(res.Arguments), end(res.Arguments), [](auto elem) {return elem.empty();}))
        {
            res.Err = Error::EmptyParameter;
        }
    }

    return res;
}

void Assembly::first_step(Operation& op, size_t& LC)
{
    // Алгоритм первого прохода для каждого оператора ассемблера
    if (op.work) // Если не комментарий и не ошибочная строка
    {
        auto label = op.Label;  // Берём метку
        auto code = op.Code;

        if (!label.empty()) // Есть метка?
        {
            if (table_symbolic_names_.IsDefined(label)) // Определена?
                op.Err = Error::RedefenitionLabel;
            else
            {
                VariableExp* var = new VariableExp(label);
                table_symbolic_names_.Assign(var, LC);
            }
        }

        if (!code.empty()) // Есть код операции?
        {
            if (asm_operators_.find(code) != end(asm_operators_))
            {
                auto cur_operator = asm_operators_[code];
                op.LC = LC;
                if (cur_operator.Length == 0) // Директива
                {
                    cur_operator.function(op, table_symbolic_names_);
                    LC = (size_t) table_symbolic_names_.Lookup(LC_Lex);
                    op.work = false;
                }
                else // Команда == берём размер команды
                {
                    LC += cur_operator.Length;
                }
                table_symbolic_names_.Assign(LC_Lex, LC);
                //cout << op.LC << " " << LC << endl;
            }
            else
            {
                op.Err = Error::InvalidCode;
            }

        }
        else
        {
            op.LC = LC; op.work = false;
        }

        if (op.Err != Error::NoError)
            op.work = false;
    }
}

void Assembly::init()
{
    asm_operators_["int"] = {defineData<int>, 0, 0};
    asm_operators_["float"] = {defineData<float>, 0, 0};
    asm_operators_["ptr"] = {defineData<address>, 0, 0};
    asm_operators_["str"] = {defineData<uint8_t>, 0, 0};

    asm_operators_["memint"] = {memoryData<int>, 0, 0};
    asm_operators_["memflt"] = {memoryData<float>, 0, 0};
    asm_operators_["memptr"] = {memoryData<address>, 0, 0};
    asm_operators_["memstr"] = {memoryData<uint8_t>, 0, 0};

    asm_operators_["equ"] = {equ, 0, 0};
    asm_operators_["proc"] = {proc, 0, 0};
    asm_operators_["endp"] = {endp, 0, 0};

    asm_operators_["org"] = {changeAdr<Prefixes::LoadVal>, 0, 0};
    asm_operators_["end"] = {changeAdr<Prefixes::IPVal>, 0, 0};

    // Пересылка
    asm_operators_["aload"] = {checkCommands<1>, OpCodes::aload, 3};
    asm_operators_["aloadoff"] = {checkCommands<1>, OpCodes::aloadoff, 3};
    asm_operators_["astore"] = {checkCommands<1>, OpCodes::astore, 3};
    asm_operators_["astoreoff"] = {checkCommands<1>, OpCodes::astoreoff, 3};

    asm_operators_["push"] = {checkCommands<1>, OpCodes::push, 3};
    asm_operators_["peek"] = {checkCommands<1>, OpCodes::peek, 3};
    asm_operators_["pop"] = {checkCommands<1>, OpCodes::pop, 3};

    // Математика
    asm_operators_["iadd"] = {checkCommands<0>, OpCodes::iadd, 1};
    asm_operators_["isub"] = {checkCommands<0>, OpCodes::isub, 1};
    asm_operators_["isubr"] = {checkCommands<0>, OpCodes::isubr, 1};
    asm_operators_["imul"] = {checkCommands<0>, OpCodes::imul, 1};
    asm_operators_["idiv"] = {checkCommands<0>, OpCodes::idiv, 1};
    asm_operators_["idivr"] = {checkCommands<0>, OpCodes::idivr, 1};
    asm_operators_["imod"] = {checkCommands<0>, OpCodes::imod, 1};
    asm_operators_["imodr"] = {checkCommands<0>, OpCodes::imodr, 1};

    asm_operators_["or"] = {checkCommands<0>, OpCodes::ior, 1};
    asm_operators_["xor"] = {checkCommands<0>, OpCodes::ixor, 1};
    asm_operators_["and"] = {checkCommands<0>, OpCodes::iand, 1};
    asm_operators_["not"] = {checkCommands<0>, OpCodes::inot, 1};
    asm_operators_["ineg"] = {checkCommands<0>, OpCodes::ineg, 1};

    asm_operators_["fadd"] = {checkCommands<0>, OpCodes::fadd, 1};
    asm_operators_["fsub"] = {checkCommands<0>, OpCodes::fsub, 1};
    asm_operators_["fsubr"] = {checkCommands<0>, OpCodes::fsubr, 1};
    asm_operators_["fmul"] = {checkCommands<0>, OpCodes::fmul, 1};
    asm_operators_["fdiv"] = {checkCommands<0>, OpCodes::fdiv, 1};
    asm_operators_["fdivr"] = {checkCommands<0>, OpCodes::fdivr, 1};

    asm_operators_["fsin"] = {checkCommands<0>, OpCodes::fsin, 1};
    asm_operators_["fcos"] = {checkCommands<0>, OpCodes::fcos, 1};
    asm_operators_["flog2"] = {checkCommands<0>, OpCodes::flog2, 1};
    asm_operators_["fatg"] = {checkCommands<0>, OpCodes::fatg, 1};
    asm_operators_["fexp"] = {checkCommands<0>, OpCodes::fexp, 1};
    asm_operators_["flog"] = {checkCommands<0>, OpCodes::flog, 1};
    asm_operators_["fsqrt"] = {checkCommands<0>, OpCodes::fsqrt, 1};

    // Ввод-вывод
    asm_operators_["ini"] = {checkCommands<1>, OpCodes::ini, 3};
    asm_operators_["inf"] = {checkCommands<1>, OpCodes::inf, 3};
    asm_operators_["inp"] = {checkCommands<1>, OpCodes::inp, 3};
    asm_operators_["in"] = {checkCommands<1>, OpCodes::in, 3};

    asm_operators_["outi"] = {checkCommands<1>, OpCodes::outi, 3};
    asm_operators_["outf"] = {checkCommands<1>, OpCodes::outf, 3};
    asm_operators_["outp"] = {checkCommands<1>, OpCodes::outp, 3};
    asm_operators_["out"] = {checkCommands<1>, OpCodes::out, 3};

    // Переходы
    asm_operators_["call"] = {checkCommands<1>, OpCodes::call, 3};
    asm_operators_["ret"] = {checkCommands<0>, OpCodes::ret, 1};

    asm_operators_["jmp"] = {checkCommands<1>, OpCodes::jmp, 3};
    asm_operators_["jmpoff"] = {checkCommands<1>, OpCodes::jmpoff, 3};

    asm_operators_["jz"] = {checkCommands<1>, OpCodes::jz, 3};
    asm_operators_["jnz"] = {checkCommands<1>, OpCodes::jnz, 3};
    asm_operators_["jl"] = {checkCommands<1>, OpCodes::jl, 3};
    asm_operators_["jlz"] = {checkCommands<1>, OpCodes::jlz, 3};
    asm_operators_["jg"] = {checkCommands<1>, OpCodes::jg, 3};
    asm_operators_["jgz"] = {checkCommands<1>, OpCodes::jgz, 3};

    asm_operators_["jzoff"] = {checkCommands<1>, OpCodes::jzoff, 3};
    asm_operators_["jnzoff"] = {checkCommands<1>, OpCodes::jnzoff, 3};
    asm_operators_["jloff"] = {checkCommands<1>, OpCodes::jloff, 3};
    asm_operators_["jlzoff"] = {checkCommands<1>, OpCodes::jlzoff, 3};
    asm_operators_["jgoff"] = {checkCommands<1>, OpCodes::jgoff, 3};
    asm_operators_["jgzoff"] = {checkCommands<1>, OpCodes::jgzoff, 3};

    // Сравнение
    asm_operators_["icmp"] = {checkCommands<0>, OpCodes::icmp, 1};
    asm_operators_["icmpr"] = {checkCommands<0>, OpCodes::icmpr, 1};
    asm_operators_["fcmp"] = {checkCommands<0>, OpCodes::fcmp, 1};
    asm_operators_["fcmpr"] = {checkCommands<0>, OpCodes::fcmpr, 1};

    // Добавление базовых констант + счетчика размещения в контекст
    table_symbolic_names_.Assign(new VariableExp("SIZE_INT"), sizeof(int));
    table_symbolic_names_.Assign(new VariableExp("SIZE_FLOAT"), sizeof(float));
    table_symbolic_names_.Assign(new VariableExp("SIZE_PTR"), sizeof(address));

    table_symbolic_names_.Assign(new VariableExp("$"), 0); // Счетчик размещения
}

void Assembly::second_step()
{
    // Второй проход -- генерация кода для команд
    // Валидность проверили на первом шаге
    for (auto& op: operations_)
    {
        if (op.work) // Можно работать (не ошибочно и не директива)
        {
            auto command = asm_operators_[op.Code];
            table_symbolic_names_.Assign(LC_Lex, op.LC); // Обновляем счетчик при проходе (чтобы было актуальное для оператора значение)
            op.binary.push_back(Prefixes::comVal); // байтовый префикс команды
            op.binary.push_back(command.Code);

            command.function(op, table_symbolic_names_); // Функция обработки аргументов
            op.work = false; // Проходим
        }
    }
}

void Assembly::make_binary_file()
{
    // Создает исполняемый файл
    ofstream fbin(filename_exec_, ios::binary);

    for (const auto& elem: operations_)         // Для каждой операции
    {
        if (!elem.binary.empty())               // Если есть что записать
        {
            for (const auto& byte: elem.binary) // Для каждого байта
            {
                fbin.write((char*)&byte, sizeof(byte)); // Записываем в файл
            }
        }
    }

    fbin.close();
}

// Логика обработки листинга !!!

void ListingAssembly::operator()(const Assembly &Program)
{
    // Нужную информацию берём из дружественного класса

    ofstream ofs(Program.filename_list_);
    size_t counter = 0;
    for (const auto& elem: Program.operations_) // Формируем каждую строку
    {
        ofs << formStr(elem, counter); ++counter;
    }

    ofs << formTable(Program.table_symbolic_names_); // Формируем таблицу имён
    ofs.close();
}

string ListingAssembly::getErrorMessage(const Error &err)
{
    // Сообщение об ошибке по его коду
    string res{};
    switch (err)
    {
        case Error::NoError: res = "OK"; break;
        case Error::InvalidChar: res = "Неверный символ при вводе"; break;
        case Error::InvalidLabel: res = "Неверный символ при вводе"; break;
        case Error::RedefenitionLabel: res = "Переопределение метки";  break;
        case Error::InvalidCode: res = "Неверный код операции"; break;
        case Error::EmptyParameter: res = "Не указан один из параметров"; break;
        case Error::InvalidNumParameters: res = "Неверно задано число параметров"; break;
        case Error::InvalidExpression: res = "Синтаксически неверное выражение"; break;
        case Error::InvalidOperator: res = "Неопределённая команда"; break;
        case Error::InvalidNumber: res = "Неверный формат числового значения"; break;
        case Error::UnexceptedLabel: res = "Ожидалась метка"; break;
        case Error::InvalidByteString: res = "Неверный строковый формат"; break;
    }
    return res;
}

bool ListingAssembly::getErrorStatus(const Error &err)
{
    return err != Error::NoError; // true = есть ошибка, false = нет
}

string ListingAssembly::formStr(const Operation &operation, size_t ind)
{
    ostringstream ostr;
    // Номер строки = LC = Двоичное представление = Метка = Код операции = Операнды = Ошибки
    ostr << ind << "  " << setw(6) << setfill ('0') <<  operation.LC;
    for (const auto& byte: operation.binary)
        ostr << setw(3) << setfill(' ') << hex << (int)byte << " ";

    ostr << endl << operation.Label << setw(8) << setfill(' ') << operation.Code;
    for (size_t i = 0; i < operation.Arguments.size(); ++i)
    {
        ostr << setw(10) << operation.Arguments[i];
        if (i != operation.Arguments.size() - 1)
            ostr << ',';
        ostr << ' ';
    }

    if (!getErrorStatus(operation.Err))
        ostr << "Ошибок нет";
    else
        ostr << "Код ошибки " << (int) operation.Err << " = " << getErrorMessage(operation.Err);

    ostr << endl;
    return ostr.str();
}

std::string ListingAssembly::formTable(const Context& c)
{
    // Вывод таблицы имен в листинге
    ostringstream ostr; ostr << endl;
    for (auto& elem: c.getTable())
        if (isVariable(elem.first))
            ostr << setw(10) << right << elem.first << " = " << elem.second << endl;
    return ostr.str();
}

bool ListingAssembly::isVariable(const string &elem)
{
    // Избавляемся от переменных и констант, определённых транслятором
    return elem !="$" && elem != "SIZE_INT" && elem != "SIZE_FLOAT" && elem != "SIZE_PTR";
}

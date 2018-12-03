#include <iostream>
#include "Assembly.h"
#include "Processor.h"

using namespace std;

// Основное исключение
class InvalidArguments: exception
{
    const char* what() const noexcept override {return "Неверный формат аргументов";}
};

static const vector<string> defined_args {string("-list"), string("-exec")}; // Доступные аргументы ком. строки

struct CommandLineArgs
{
    CommandLineArgs() = default;

    CommandLineArgs(int argc, char* argv[]) // Разбор аргументов ком. строки
    {
        if (argc == 0) throw InvalidArguments();   // Ничего нет!
        for (int i = 1; i < argc; ++i)
        {
            auto elem = string(argv[i]);
            if (i == 1 && !is_arg(elem)) // Имя файла?
                filename = elem;
            else if (i > 1 && is_arg(elem) && (find(begin(defined_args), end(defined_args), elem) != end(defined_args))) // Определённый аргумент?
                arguments.push_back(elem);
            else
                throw InvalidArguments();
        }
    }

    bool is_arg(const string& arg) const {return arg.size() > 1 && arg[0] == '-';} // Начинаются с '-'
    bool found(const string& arg) const { return (find(begin(arguments), end(arguments), arg) != end(arguments));} // Есть ли такой аргумент?

    string filename;           // Имя файла
    vector<string> arguments;  // Аргументы

    enum class arg_ids : size_t {Listing, Exec}; // Перечисления для индексов массивов аргумента
};

int main(int argc, char* argv[])
{
    // program filename -list -exec
    // filename = Файл с программой на ассемблере
    // -list - формирование листинга
    // -exec - выполнение
    // Имена исполняемых файлов формируются по аналогии с filename
    try
    {
        auto args = CommandLineArgs(argc, argv);
        //CommandLineArgs args{}; args.filename = "first5.txt"; args.arguments = defined_args;
        string file_asm = args.filename;
        string file_lst = file_asm + ".lst.txt";
        string file_bin = file_asm + ".bin";


        Assembly asmm(file_asm, file_bin, file_lst, args.found(defined_args[(size_t)CommandLineArgs::arg_ids::Listing]));
        if (args.found(defined_args[(size_t)CommandLineArgs::arg_ids::Exec]))
        {
            Processor proc;
            Uploader(file_bin, proc);
            proc.Execute();
        }
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
    system("pause");
    return 0;
}
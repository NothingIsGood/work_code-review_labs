//
// Created on 11.09.2018.
//

#include <iostream>
#include <vector>

#include "Processor.h"

Processor::Processor() {
    setIP(0);
    setSP(static_cast<uint8_t>(-1)); // SP = -1 изначально, увеличивается при добавлении
    setZF(0);
    setSF(0);

    enum code_ops : size_t
    {
        loadaddr = 70,
        storeaddr = 71,

        pushstack = 75,
        peekstack = 76,
        popstack = 77,

        iadd = 10,
        isub = 11,
        imul = 12,
        idiv = 13,
        imod = 14,
        ixor = 15,
        ior = 16, // обычные or-and-xor = ключевые слова C++ !
        iand = 17,
        inot = 18,
        ineg = 19,

        fadd = 20,
        fsub = 21,
        fmul = 22,
        fdiv = 23,
        fsin = 24,
        fcos = 25,
        flog2 = 26,
        fatg = 27,
        flog = 28,
        fexp = 29,
        fsqrt = 30,

        icmp = 40,
        fcmp = 41,

        ini = 80,
        inf = 81,
        inp = 82,
        in = 83,

        outi = 85,
        outf = 86,
        outp = 87,
        out = 88,

        jmp = 90,
        jz = 91,
        jnz = 92,
        jgz = 93,
        jlz = 94,
        jg = 95,
        jl = 96,

        call = 100,
        ret = 35,
    };

    commands_[code_ops::loadaddr] = new AdrRegLoader();
    commands_[code_ops::storeaddr] = new AdrRegSaver();

    commands_[code_ops::pushstack] = new StackLoader();
    commands_[code_ops::peekstack] = new StackSaver();
    commands_[code_ops::popstack] = new StackPop();

    commands_[code_ops::iadd] = new MathTwoArgs<int>(std::plus<>());
    commands_[code_ops::isub] = new MathTwoArgs<int>(std::minus<>());
    commands_[code_ops::imul] = new MathTwoArgs<int>(std::multiplies<>());
    commands_[code_ops::idiv] = new MathTwoArgs<int>(std::divides<>());
    commands_[code_ops::imod] = new MathTwoArgs<int>(std::modulus<>());
    commands_[code_ops::ixor] = new MathTwoArgs<int>(std::bit_xor<>());
    commands_[code_ops::ior] = new MathTwoArgs<int>(std::bit_or<>());
    commands_[code_ops::iand] = new MathTwoArgs<int>(std::bit_and<>());
    commands_[code_ops::inot] = new MathOneArg<int>(std::bit_not<>());
    commands_[code_ops::ineg] = new MathOneArg<int>(std::negate<>());

    commands_[code_ops::fadd] = new MathTwoArgs<float>(std::plus<>());
    commands_[code_ops::fsub] = new MathTwoArgs<float>(std::minus<>());
    commands_[code_ops::fmul] = new MathTwoArgs<float>(std::multiplies<>());
    commands_[code_ops::fdiv] = new MathTwoArgs<float>(std::divides<>());

    commands_[code_ops::fcos] = new MathOneArg<float>(cosf);
    commands_[code_ops::fsin] = new  MathOneArg<float>(sinf);
    commands_[code_ops::flog2] = new MathOneArg<float>(log2f);
    commands_[code_ops::fatg] = new MathOneArg<float>(atanf);
    commands_[code_ops::flog] = new MathOneArg<float>(logf);
    commands_[code_ops::fexp] = new MathOneArg<float>(expf);
    commands_[code_ops::fsqrt] = new MathOneArg<float>(sqrtf);

    commands_[code_ops::ini] = new InputCommand<int>();
    commands_[code_ops::inf] = new InputCommand<float>();
    commands_[code_ops::inp] = new InputCommand<address>();
    commands_[code_ops::in] = new InputCommand<uint8_t>();

    commands_[code_ops::outi] = new OutputCommand<int>();
    commands_[code_ops::outf] = new OutputCommand<float>();
    commands_[code_ops::outp] = new OutputCommand<address>();
    commands_[code_ops::out] = new OutputCommand<uint8_t>();

    commands_[code_ops::jmp] = new UnconditionalTransition();
    commands_[code_ops::jz] = new ConditionalTransition([](auto ZF, auto SF) {return (ZF == 1);}); // res == 0
    commands_[code_ops::jnz] = new ConditionalTransition([](auto ZF, auto SF) {return (ZF == 0);}); // res != 0
    commands_[code_ops::jgz] = new ConditionalTransition([](auto ZF, auto SF) {return (ZF == 1 || SF == 0);}); // res >= 0
    commands_[code_ops::jlz] = new ConditionalTransition([](auto ZF, auto SF) {return (ZF == 1 || SF == 1);}); // res <= 0
    commands_[code_ops::jg] = new ConditionalTransition([](auto ZF, auto SF) {return (SF == 0 && ZF != 1);}); // res > 0
    commands_[code_ops::jl] = new ConditionalTransition([](auto ZF, auto SF) {return (SF == 1 && ZF != 1);}); // res < 0

    commands_[code_ops::call] = new CallProc();
    commands_[code_ops::ret] = new ReturnProc();

    commands_[code_ops::icmp] = new CommandCMP<int>();
    commands_[code_ops::fcmp] = new CommandCMP<float>();

}

uint32_t Processor::loadData(const size_t sizeData, int idx) noexcept
{
    // Выборка данных из памяти (до 4 байт включительно)
    // Выбираются из памяти sizeData байт с адреса idx
    // Максимум данных = 4 байта, что есть максимальный размер единицы данных в программе
    uint32_t res = 0;
    size_t counter = 0;
    while (counter < sizeData)
    {
        uint32_t tmp = getByte(static_cast<size_t>(idx));
        tmp <<= (8 * counter);
        res |= tmp;
        ++idx;
        ++counter;
    }
    return res;
}

void Processor::Execute()
{
    auto firstByte = memory_[getIP()];
    proc_union un{};
    while (firstByte != 0)
    {

        auto commandCode = (uint8_t) ((firstByte >> 1) );
        auto sizeCom = (uint8_t) getCommandSize((uint8_t)firstByte);
        std::cout << "IP = " << getIP() << std::endl;

        if (sizeCom == 1)
        {
            un.code_8.code = commandCode;
            un.code_8.b = (uint8_t) (firstByte % 2);
        }
        else
        {
            un.code_24.code = commandCode;
            un.code_24.b = (uint8_t) (firstByte % 2);
            un.code_24.adr = (address) loadData(sizeof(address), static_cast<address>(getIP() + 1));
        }

        if (commands_[commandCode] != nullptr)
        {
            commands_[commandCode]->operator()(*this, un);
            setIP(getIP() + commands_[commandCode]->getMode());
        }

        firstByte = memory_[getIP()];
    }
}

void Processor::pushData(const size_t sizeData, const uint32_t data, address currentID) noexcept
{
    // Загрузка данных из переменной data размера sizeData в память
    // Начиная с адреса currentID
    // Необходимо в некоторых командах для выгрузки значений в память
    size_t counter = 0;
    while (counter < sizeData)
    {
        auto tmp = (uint8_t) ((data >> (counter * 8)) & 0xff);
        memory_[currentID] = tmp;
        ++currentID;
        ++counter;
    }
}

void Processor::setFlags(int res_op) {
    // Установка флагов для целого значения
    // res_op = результат целочисленной арифметики
    if (res_op == 0)
        setZF(1);
    else
        setZF(0);

    if (res_op < 0)
        setSF(1);
    else
        setSF(0);
}

void Processor::setFlags(float res_float)
{
    // Установка флагов для дробного значения
    // res_op = результат дробной арифметики

    if (res_float < eps && res_float > 0)
        setZF(1);
    else
        setZF(0);

    if (res_float < 0)
        setSF(1);
    else
        setSF(0);
}

size_t getCommandSize(uint8_t code_op)
{
    // 0-63 -- Формат-8 -- 1 байт
    // 64-127 -- Формат-24 -- 3 байта
    return code_op & 128 ? 3 : 1;
}

void Uploader(const std::string &filename, Processor &proc) {
    // Формат бинарного файла
    // Префиксы (в двоичном виде)
    // c -- команды
    // i -- целые знаковые
    // f -- дробные
    // s -- IP
    // a -- адрес загрузки
    // p -- указатель (2 байта для адресного регистра)

    std::ifstream ifs(filename, std::ios::binary | std::ios::in); // для загрузки бинарного файла
    address idx = 0; // Адрес загрузки в памяти -- может отличаться от индекса в байтовой последовательности

    std::vector <char> bytes; // байты из двоичного модуля

    while (!ifs.eof()) // считываем в массив байтов (для удобства работы)
    {
        char prefix = 0;
        ifs.read(&prefix, sizeof(prefix));
        bytes.push_back(prefix);
    }

    for (size_t i = 0; i < bytes.size(); )
    {
        if (bytes[i] == 'c') // Нашли КОП
        {
            ++i; // Переходим
            proc.setByte(idx, bytes[i]);
            size_t len = getCommandSize((uint8_t)bytes[i]);
            ++idx; ++i;
            for (int k = 0; k < (int) (len - 1); ++k) // В зависимости от размера команды -- 1 или 3 байта
            {
                proc.setByte(idx, bytes[i]);
                ++idx; ++i;
            }
        }
        else if (bytes[i] == 'b')
        {
            ++i;
            proc.setByte(idx, bytes[i]);
            ++idx; ++i;
        }
        else if (bytes[i] == 'i' || bytes[i] == 'f')
        {
            ++i;
            for (size_t j = 0; j < sizeof(int); ++j) // считаем, что sizeof(int) == sizeof(float)
            {
                proc.setByte(idx, bytes[i]);
                ++idx; ++i;
            }
        }
        else if (bytes[i] == 'p')
        {
            ++i;
            for (size_t j = 0; j < sizeof(address); ++j) // Выгружаем указатель в память
            {
                proc.setByte(idx, bytes[i]);
                ++idx; ++i;
            }
        }
        else if (bytes[i] == 'a' || bytes[i] == 's')
        {

            char state = bytes[i]; // Для логики работы с адресом
            address adr = 0; size_t counter = 0;
            while (counter < sizeof(address)) // Выделяем адрес
            {
                ++i;
                address tmp = (uint8_t) bytes[i];
                tmp <<= (8 * (counter));
                adr |= tmp;

                ++counter;
            }

            if (state == 'a') // директива смены адреса загрузки
            {
                idx = adr;
            }
            else // директива указания IP
            {
                proc.setIP(adr);
            }

        }
        else
        {
            ++i;
        }
    }
    std::cout << "Program loaded with IP = " << proc.getIP() << std::endl;
    ifs.close();

}

void UploaderTxt(const std::string &filename_txt, const std::string &filename_bin) {
    // Процедура перевода текстового файла с кодом в бинарный
    // Префиксы (в двоичном виде)
    // c -- команды
    // i -- целые знаковые
    // f -- дробные
    // s -- IP
    // a -- адрес загрузки
    // p -- указатель (2 байта для адресного регистра)

    // filename_txt -- программа в текстовом виде
    // filename_bin -- двоичное представление

    std::ifstream ifs(filename_txt);
    std::ofstream ofs(filename_bin, std::ios::binary | std::ios::out);

    while (!ifs.eof())
    {
        // Считаем строку префикса, берём первый символ
        std::string prefix_str;
        ifs >> prefix_str;
        char prefix = prefix_str[0];

        // Временные переменные для хранения КОП и управляющего бита
        int code_op, b;
        address adr;
        char byte_cop;
        // Временные переменные для хранения целых, дробных и 2-байтовых целых значений
        int intval; float fltval; uint8_t bvar;
        address  adrval;

        ofs.write(&prefix, sizeof(prefix));

        switch (prefix) {
            case 'c':
                ifs >> code_op;
                ifs >> b;
                // Формируем команду, соединяя КОП и бит b, сдвигая код операции влево
                code_op <<= 1;
                code_op |= (b % 2);
                byte_cop = (char) code_op; // для записи одного байта
                ofs.write(&byte_cop, sizeof(byte_cop));
                if (getCommandSize((uint8_t)code_op) == 3) {
                    ifs >> adr;
                    ofs.write((char*)&adr, sizeof(adr));
                }
                break;
            case 'i': // Загрузка целых чисел
                ifs >> intval;
                ofs.write((char*)&intval, sizeof(intval));
                break;
            case 'f': // Загрузка дробных чисел
                ifs >> fltval;
                ofs.write((char*)&fltval, sizeof(fltval));
                break;
            case 'b':
                ifs >> bvar;
                ofs.write((char*)&bvar, sizeof(bvar));
            case 's': // Загрузка адреса
            case 'a':
            case 'p':
                ifs >> adrval;
                ofs.write((char*)&adrval, sizeof(adrval));
                break;
            default:
                std::cout << "Invalid prefix = " << prefix << std::endl;
        }

    }
    ofs.close();
    ifs.close();
}

// Загрузка адресного регистра
void AdrRegLoader::operator()(Processor &proc, proc_union un) {
    proc.setAdrReg(un.code_24.b ? proc.getAdrReg() + un.code_24.adr : un.code_24.adr);
}

// Сохранение адресного регистра
void AdrRegSaver::operator()(Processor &proc, proc_union un) {
    proc.pushData(sizeof(address), (un.code_24.b ? proc.getAdrReg() + un.code_24.adr : un.code_24.adr), un.code_24.adr);
}

// Загрузка стека
void StackLoader::operator()(Processor &proc, proc_union un) {
    auto loaded_data = proc.loadData(sizeof(float), un.code_24.adr);
    Processor::stack_value val {}; val.int_val = loaded_data;
    proc.push(val);
}

// Сохранение стека
void StackSaver::operator()(Processor &proc, proc_union un) {
    auto val = proc.peek(); // Сохранение без извлечения из стека
    proc.pushData(sizeof(float), (uint32_t) val.int_val, un.code_24.adr);
}

// Извлечение стека
void StackPop::operator()(Processor &proc, proc_union un) {
    auto val = proc.pop(); // Извлечение из стека
    proc.pushData(sizeof(float), (uint32_t) val.int_val, un.code_24.adr);
}


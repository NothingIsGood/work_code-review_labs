#include <iostream>
#include "Debugger.h"


int main()
{
    const std::string filename = "file.txt.bin";
    Debugger dbg(filename);
    dbg.Start();

    return 0;
}

/*
int main(int argc, char* argv[])
{
    Debugger dbg(string(argv[1]));
    dbg.start();
    return 0;
}
 */
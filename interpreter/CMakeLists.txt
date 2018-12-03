cmake_minimum_required(VERSION 3.12)
project(interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

add_executable(interpreter main.cpp MathInterpreter.cpp MathInterpreter.h Structures.h ParseData.cpp ParseData.h Assembly.cpp Assembly.h Processor.cpp Processor.h StructuresProcessor.h)
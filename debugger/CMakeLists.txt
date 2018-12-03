cmake_minimum_required(VERSION 3.12)
project(debugger)

set(CMAKE_CXX_STANDARD 17)

add_executable(debugger main.cpp Processor.cpp Processor.h StructuresProcessor.h Debugger.cpp Debugger.h)
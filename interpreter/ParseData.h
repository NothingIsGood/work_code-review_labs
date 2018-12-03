//
// Created on 03.10.2018.
//

#ifndef INTERPRETER_PARSEDATA_H
#define INTERPRETER_PARSEDATA_H

#include <string>
#include <cinttypes>
#include <algorithm>
#include <cmath>

#include "Structures.h"

std::string no_space_symbols(const std::string& value);
int digit(char ch, int base = 10);
int parseInt(const std::string& value, Error& err);
float parseFloat(const std::string& value, Error& err);
std::string parseString(const std::string& value, Error& err);

#endif

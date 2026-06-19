#pragma once

#include <windows.h>

#include <cstring>
#include <fstream>
#include <string>

#include <MinHook.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "il2cpp_symbols.hpp"

void hook_log(const char* message);
void hook_logf(const char* format, ...);

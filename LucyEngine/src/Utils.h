#pragma once

#include "Core/Base.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Utils {
	struct Size { int32_t width; int32_t height; };

	std::vector<std::string> Split(std::string& s, const std::string& delimiter);
	std::vector<std::string> ReadFile(const std::string& path);
	Size ReadSizeFromIni(const char* windowName);
}
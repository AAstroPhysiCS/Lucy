#include "Core/Base.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Utils {

	std::vector<std::string> Split(std::string& s, const std::string& delimiter) {

		size_t start = 0;
		size_t end = 0;
		std::string token;
		std::vector<std::string> buffer;

		while ((end = s.find(delimiter, start)) != std::string::npos) {
			token = s.substr(start, end - start);
			start = end + delimiter.length();
			buffer.push_back(token);
		}

		buffer.push_back(s.substr(start));
		return buffer;
	}

	std::vector<std::string> ReadIniFile()
	{
		std::ifstream f("D:/programming/C++/LucyEngine/LucyEditor/lucyconfig.ini");
		std::string line;
		std::vector<std::string> buffer;

		while (std::getline(f, line)) {
			buffer.push_back(line);
		}

		return buffer;
	}

	auto ReadSizeFromIni(const char* windowName) {
		struct Size { int32_t width; int32_t height; };
		
		auto& buffer = ReadIniFile();

		std::string windowNameFull = "[Window][";
		windowNameFull.append(windowName).append("]");

		auto iter = std::find(buffer.begin(), buffer.end(), windowNameFull);
		if (iter == buffer.end()) return Size{ 0, 0 };
		size_t windowNameIndex = iter - buffer.begin();

		std::string& size = buffer[windowNameIndex + 2];
		std::vector<std::string>& vec = Split(Split(size, "=")[1], ",");

		return Size{ std::atoi(vec[0].c_str()), std::atoi(vec[1].c_str()) };
	}
}
#pragma once

#include "Core/Base.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../nativefiledialog/include/nfd.h"

namespace Utils {
	struct Size { int32_t width; int32_t height; };

	std::vector<std::string> Split(std::string& s, const std::string& delimiter);
	std::vector<std::string> ReadFile(const std::string& path);
	Size ReadSizeFromIni(const char* windowName);

	struct DialogFilter {
		const char* name;
		const char* spec;
	};

	static DialogFilter MeshFilterList[1] = {
		{"Mesh Files", "fbx,obj,gltf"}
	};

	void OpenDialog(std::string& outString, const DialogFilter filterList[], size_t count, const char* defaultPath);
}
#pragma once

#include "Core/Base.h"

#include "../nativefiledialog/include/nfd.h"
#include "glm/glm.hpp"

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

namespace UIUtils {
	void TextCenter(const std::string& text, float xPadding, uint32_t indent = 0);
	void TextCenter(const char const* text, float xPadding, uint32_t indent = 0);
	void TextCenterTable(const char const* text, float xPadding, float yPadding, uint32_t indent = 0);
	void TransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed);
}

namespace Maths {

	struct Ray {
		glm::vec3 Origin;
		glm::vec3 Dir;
	};

	bool RayTriangleIntersection(const Ray& r, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v);
}
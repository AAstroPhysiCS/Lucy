#pragma once

namespace Utils {

	struct Attribute { 
		uint32_t Width; 
		uint32_t Height;
	};

	std::vector<std::string> Split(const std::string& s, const std::string& delimiter);

	template <typename Iter = std::vector<std::string>::iterator>
	std::string CombineDataToSingleBuffer(std::vector<std::string>& lines, const Iter& from, const Iter& to) {
		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}
		return buffer;
	}

	template <typename Iter = std::vector<std::string>::iterator>
	std::string CombineDataToSingleBuffer(std::vector<std::string>& lines) {
		return CombineDataToSingleBuffer(lines, lines.begin(), lines.end());
	}

	Attribute ReadAttributeFromIni(const char* windowName, const char* attributeName);

	struct DialogFilter {
		const char* name;
		const char* spec;
	};

	static DialogFilter MeshFilterList[1] = {
		{"Mesh Files", "fbx,obj,gltf"}
	};

	static DialogFilter CubemapFilterList[1] = {
		{"HDR Files", "hdr"}
	};

	void OpenDialog(std::string& outString, const DialogFilter filterList[], size_t count, const char* defaultPath);
}

namespace UI {
	void TextCenter(const std::string& text, float xPadding, uint32_t indent = 0);
	void TextCenter(const char* const text, float xPadding, uint32_t indent = 0);
	void TextCenterTable(const char* const text, float xPadding, float yPadding, uint32_t indent = 0);
	void TransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed);
}

namespace Maths {

	struct Ray {
		glm::vec3 Origin;
		glm::vec3 Dir;
	};

	glm::vec3 EulerDegreesToLightDirection(const glm::vec3& eulerDegrees);

	bool RayTriangleIntersection(const Ray& r, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v);
}
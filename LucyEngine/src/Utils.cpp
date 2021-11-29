#include "lypch.h"

#include "Utils.h"

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

	std::vector<std::string> ReadFile(const std::string& path) {
		std::ifstream f(path);

		if (!f) {
			LUCY_CRITICAL("Error opening the file! Or it can not be found");
			LUCY_ASSERT(false);
		}

		std::string line;
		std::vector<std::string> buffer;

		while (std::getline(f, line)) {
			buffer.push_back(line);
		}

		return buffer;
	}

	Size ReadSizeFromIni(const char* windowName) {
		auto& buffer = ReadFile("lucyconfig.ini");

		std::string windowNameFull = "[Window][";
		windowNameFull.append(windowName).append("]");

		auto iter = std::find(buffer.begin(), buffer.end(), windowNameFull);
		if (iter == buffer.end()) return Size{ 0, 0 };
		size_t windowNameIndex = iter - buffer.begin();

		std::string& size = buffer[windowNameIndex + 2];
		std::vector<std::string>& vec = Split(Split(size, "=")[1], ",");

		return Size{ std::atoi(vec[0].c_str()), std::atoi(vec[1].c_str()) };
	}

	void OpenDialog(std::string& outString, const DialogFilter filterList[], size_t count, const char* defaultPath) {
		char* outPath;
		nfdresult_t result = NFD_OpenDialog(&outPath, (nfdfilteritem_t*)filterList, count, defaultPath);
		if (result == NFD_OKAY) {
			outString = std::string(outPath);
			NFD_FreePath((nfdu8char_t*)outPath);
		}
	}
}

namespace Maths {

	//From: https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
	bool RayTriangleIntersection(const Ray& r, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t, float& u, float& v) {
		glm::vec3 v0v1 = v1 - v0;
		glm::vec3 v0v2 = v2 - v0;
		glm::vec3 pvec = glm::cross(r.Dir, v0v2);
		float det = glm::dot(v0v1, pvec);
		// if the determinant is negative the triangle is backfacing
		// if the determinant is close to 0, the ray misses the triangle
		if (det < 0.0001f) return false;
		// ray and triangle are parallel if det is close to 0
		if (fabs(det) < 0.0001f) return false;
		float invDet = 1 / det;

		glm::vec3 tvec = r.Origin - v0;
		u = glm::dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1) return false;

		glm::vec3 qvec = glm::cross(tvec, v0v1);
		v = glm::dot(r.Dir, qvec) * invDet;
		if (v < 0 || u + v > 1) return false;

		t = glm::dot(v0v2, qvec) * invDet;

		return true;
	}
}
#pragma once

#include "Base.h"

namespace Lucy {

	//see std::ios openmode flags
	enum class OpenMode : uint16_t {
		AtEnd		= 0x04,
		Append		= 0x08, 
		Truncate	= 0x10,
		Binary		= 0x20
	};

	inline uint16_t operator|(uint16_t a, OpenMode b) {
		return (a | (uint16_t)b);
	}

	class FileSystem {
	public:
		static void Init();
		static void Destroy();

		static std::string GetParentPath(std::string& path);
		static std::string GetParentPath(const std::string& path);
		static std::string GetFileName(std::string& file);
		static std::string GetFileName(const std::string& file);

		static std::string GetFileExtension(std::string& file);
		static std::string GetFileExtension(const std::string& file);

		template <typename T>
		inline static void WriteToFile(const std::string& path, const std::vector<T>& data, OpenMode mode) {
			uint16_t flags = 0x02 | mode; //see std::ios::out

			std::ofstream of(path, flags);
			if (!of || !of.is_open()) {
				LUCY_CRITICAL(fmt::format("Writing to {0} failed", path));
				LUCY_ASSERT(false);
			}

			of.write((const char*)data.data(), data.size() * sizeof(T));
			of.flush();
			of.close();
		}

		static void ReadFile(const char* path, std::string& data);

		template <typename T>
		inline static void ReadFile(const std::string& path, std::vector<T>& data, OpenMode mode) {
			uint16_t flags = 0x01 | mode; //see std::ios::in

			std::ifstream f(path, flags);
			if (!f || !f.is_open()) {
				LUCY_CRITICAL(fmt::format("Error opening the file! Or it can not be found {0}", path));
				LUCY_ASSERT(false);
			}

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.resize(size / sizeof(T));
			f.read((char*)data.data(), size);
			f.close();
		}

		template <typename T>
		inline static void ReadFileLine(const std::string& path, std::vector<T>& data) {
			std::ifstream f(path);

			if (!f || !f.is_open()) {
				LUCY_CRITICAL(fmt::format("Error opening the file! Or it can not be found {0}", path));
				LUCY_ASSERT(false);
			}

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.reserve(size);

			T line;
			while (std::getline(f, line)) {
				data.emplace_back(line);
			}
		}

		static bool FileExists(std::string& file);
		static bool FileExists(const std::string& file);
	private:
		FileSystem() = delete;
		~FileSystem() = delete;
	};
}

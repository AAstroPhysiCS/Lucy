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
	private:
		FileSystem() = default;
		~FileSystem();

		friend class Application;
	public:
		void Init();

		std::string GetParentPath(std::string& path);
		std::string GetParentPath(const std::string& path);
		std::string GetFileName(std::string& file);
		std::string GetFileName(const std::string& file);

		std::string GetFileExtension(std::string& file);
		std::string GetFileExtension(const std::string& file);

		template <typename T>
		inline void WriteToFile(const std::string& path, const std::vector<T>& data, OpenMode mode) {
			uint16_t flags = 0x02 | mode; //see std::ios::out

			std::ofstream of(path, flags);
			LUCY_ASSERT(of && of.is_open(), "Writing to {0} failed", path);

			of.write((const char*)data.data(), data.size() * sizeof(T));
			of.flush();
			of.close();
		}

		void ReadFile(const char* path, std::string& data);

		template <typename T>
		inline void ReadFile(const std::string& path, std::vector<T>& data, OpenMode mode) {
			uint16_t flags = 0x01 | mode; //see std::ios::in

			std::ifstream f(path, flags);
			LUCY_ASSERT(f && f.is_open(), "Error opening the file! Or it can not be found {0}", path);

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.resize(size / sizeof(T));
			f.read((char*)data.data(), size);
			f.close();
		}

		template <typename T>
		inline void ReadFileLine(const std::string& path, std::vector<T>& data) {
			std::ifstream f(path);
			LUCY_ASSERT(f && f.is_open(), "Error opening the file! Or it can not be found {0}", path);

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.reserve(size);

			T line;
			while (std::getline(f, line)) {
				data.emplace_back(line);
			}
		}

		bool FileExists(std::string& file);
		bool FileExists(const std::string& file);
	};
}

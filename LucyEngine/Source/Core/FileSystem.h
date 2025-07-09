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
		FileSystem() = delete;
		~FileSystem() = delete;
	public:
		static void Init();
		static void Destroy();

		static std::filesystem::path GetParentPath(const std::string& path);
		static std::filesystem::path GetFileExtension(const std::string& file);

		static std::string GetFileName(const std::string& file);
		static std::string GetFileName(const std::filesystem::path& file);

		static bool CreateDir(const std::string& file);
		static bool CreateDir(const std::filesystem::path& filePath);

		static bool DirectoryExists(const std::string& file);
		static bool DirectoryExists(const std::filesystem::path& filePath);

		static bool FileExists(const std::string& file);
		static bool FileExists(const std::filesystem::path& filePath);

		template <typename TData>
		static inline void WriteToFile(const std::filesystem::path& path, const std::vector<TData>& data, OpenMode mode) {
			uint16_t flags = 0x02 | mode; //see std::ios::out

			std::ofstream of(path, flags);
			LUCY_ASSERT(of && of.is_open(), "Writing to {0} failed", path.generic_string());

			of.write((const char*)data.data(), data.size() * sizeof(TData));
			of.flush();
			of.close();
		}

		static void ReadFile(const std::filesystem::path& path, std::string& data);

		template <typename TData>
		static inline void ReadFile(const std::filesystem::path&path, std::vector<TData>& data, OpenMode mode) {
			uint16_t flags = 0x01 | mode; //see std::ios::in

			std::ifstream f(path, flags);
			LUCY_ASSERT(f && f.is_open(), "Error opening the file! Or it can not be found {0}", path.generic_string());

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.resize(size / sizeof(TData));
			f.read((char*)data.data(), size);
			f.close();
		}

		template <typename TData>
		static inline void ReadFileLine(const std::filesystem::path& path, std::vector<TData>& data) {
			std::ifstream f(path);
			LUCY_ASSERT(f && f.is_open(), "Error opening the file! Or it can not be found {0}", path.generic_string());

			f.seekg(0, std::ios::end);
			std::size_t size = f.tellg();
			f.seekg(0, std::ios::beg);

			data.reserve(size);

			TData line;
			while (std::getline(f, line)) {
				data.emplace_back(line);
			}
		}
	};
}

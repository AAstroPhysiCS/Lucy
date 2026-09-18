#include "lypch.h"
#include "FileSystem.h"
#include "Utilities/Utilities.h"

#include "../nativefiledialog/include/nfd.h"

namespace Lucy {
	
	void FileSystem::Init() {
		LUCY_ASSERT(NFD_Init());
	}

	void FileSystem::Destroy() {
		NFD_Quit();
	}

	void FileSystem::ReadFile(const std::filesystem::path& path, std::string& data) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path.string());
		std::ifstream f(path);
		std::stringstream buffer;
		buffer << f.rdbuf();
		data = buffer.str();
	}

	bool FileSystem::CreateDir(const std::string& file) {
		std::error_code ec;
		bool result = std::filesystem::create_directories(file, ec);
		return !ec;
	}

	bool FileSystem::CreateDir(const std::filesystem::path& filePath) {
		std::error_code ec;
		bool result = std::filesystem::create_directories(filePath, ec);
		return !ec; // Returns true if successful or already exists
	}

	bool FileSystem::DirectoryExists(const std::string& file) {
		return std::filesystem::is_directory(file);
	}

	bool FileSystem::DirectoryExists(const std::filesystem::path& filePath) {
		return std::filesystem::is_directory(filePath);
	}

	size_t FileSystem::GetDirectoryFileCount(const std::filesystem::path& filePath) {
		using std::filesystem::directory_iterator;
		return std::count_if(directory_iterator(filePath), directory_iterator{}, (bool(*)(const std::filesystem::path&))std::filesystem::is_regular_file);
	}

	bool FileSystem::FileExists(const std::string& file) {
		return std::filesystem::exists(file);
	}

	bool FileSystem::FileExists(const std::filesystem::path& filePath) {
		return std::filesystem::exists(filePath);
	}

	std::filesystem::path FileSystem::DecodeURI(const std::filesystem::path& uri) {
		return DecodeURI(uri.string());
	}

	std::string FileSystem::DecodeURI(const std::string& uri) {
		std::string result;
		result.reserve(uri.size());

		const auto HexToInt = [](char character) -> int32_t {
			if (character >= '0' && character <= '9')
				return character - '0';
			if (character >= 'A' && character <= 'F')
				return character - 'A' + 10;
			if (character >= 'a' && character <= 'f')
				return character - 'a' + 10;
			return -1;
		};

		for (size_t i = 0; i < uri.size(); i++) {
			if (uri[i] == '%' && i + 2 < uri.size()) {
				int32_t high = HexToInt(uri[i + 1]);
				int32_t low = HexToInt(uri[i + 2]);

				if (high != -1 && low != -1) {
					result += static_cast<char>((high << 4) | low);
					i += 2;
					continue;
				}
			}

			result += uri[i];
		}

		return result;
	}

	std::filesystem::path FileSystem::WeaklyCanonical(const std::filesystem::path& filePath) {
		auto decoded = DecodeURI(filePath.string());
		return std::filesystem::weakly_canonical(decoded);
	}

	std::filesystem::path FileSystem::GetParentPath(const std::string& path) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		auto decoded = DecodeURI(path);
		std::filesystem::path relPath(decoded);
		return relPath.parent_path();
	}

	std::string FileSystem::GetFileName(const std::string& file) {
		std::filesystem::path relPath(file);
		return GetFileName(relPath);
	}

	std::string FileSystem::GetFileName(const std::filesystem::path& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file.string());
		return Utils::Split(file.filename().string(), ".")[0];
	}

	std::filesystem::path FileSystem::GetFileExtension(const std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path path(file);
		return path.extension();
	}
}
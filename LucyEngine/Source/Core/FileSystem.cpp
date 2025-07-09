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

	bool FileSystem::FileExists(const std::string& file) {
		return std::filesystem::exists(file);
	}

	bool FileSystem::FileExists(const std::filesystem::path& filePath) {
		return std::filesystem::exists(filePath);
	}

	std::filesystem::path FileSystem::GetParentPath(const std::string& path) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::filesystem::path relPath(path);
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
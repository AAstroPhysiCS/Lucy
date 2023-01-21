#include "lypch.h"
#include "FileSystem.h"
#include "Utils/Utils.h"

#include "../nativefiledialog/include/nfd.h"

namespace Lucy {
	
	FileSystem::~FileSystem() {
		NFD_Quit();
	}

	void FileSystem::Init() {
		LUCY_ASSERT(NFD_Init());
	}

	void FileSystem::ReadFile(const char* path, std::string& data) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::ifstream f(path);
		std::stringstream buffer;
		buffer << f.rdbuf();
		data = buffer.str();
	}

	bool FileSystem::FileExists(std::string& file) {
		return std::filesystem::exists(file);
	}

	bool FileSystem::FileExists(const std::string& file) {
		return std::filesystem::exists(file);
	}

	std::string FileSystem::GetParentPath(std::string& path) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string FileSystem::GetParentPath(const std::string& path) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string FileSystem::GetFileName(std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string FileSystem::GetFileName(const std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string FileSystem::GetFileExtension(std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path path(file);
		return path.extension().string();
	}

	std::string FileSystem::GetFileExtension(const std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path path(file);
		return path.extension().string();
	}
}
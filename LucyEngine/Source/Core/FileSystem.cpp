#include "lypch.h"
#include "FileSystem.h"
#include "Utils/Utils.h"

#include "../nativefiledialog/include/nfd.h"

namespace Lucy {

	void FileSystem::Init() {
		LUCY_ASSERT(NFD_Init() == NFD_OKAY);
	}

	void FileSystem::Destroy() {
		NFD_Quit();
	}

	std::string FileSystem::GetParentPath(std::string& path) {
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string FileSystem::GetFileName(std::string& file) {
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	void FileSystem::ReadFile(const char* path, std::string& data) {
		std::ifstream f(path);
		std::stringstream buffer;
		buffer << f.rdbuf();
		data = buffer.str();
	}

	bool FileSystem::FileExists(std::string& file) {
		return std::filesystem::exists(file);
	}

	std::string FileSystem::GetParentPath(const std::string& path) {
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string FileSystem::GetFileName(const std::string& file) {
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	bool FileSystem::FileExists(const std::string& file) {
		return std::filesystem::exists(file);
	}
}
#include "lypch.h"
#include "FileSystem.h"
#include "Utils.h"


namespace Lucy {

	std::string Lucy::FileSystem::GetParentPath(std::string& path) {
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string FileSystem::GetFileName(std::string& file) {
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	bool FileSystem::FileExists(std::string& file) {
		return std::filesystem::exists(file);
	}
}
#include "lypch.h"
#include "FileSystem.h"
#include "Utils/Utils.h"

#include "../nativefiledialog/include/nfd.h"

namespace Lucy {

#define FILE_EXISTS(path)	if (!FileExists(path)) {											\
								LUCY_CRITICAL(fmt::format("File cannot be found {0}", path));	\
								LUCY_ASSERT(false);												\
							}

	void Filesystem::Init() {
		LUCY_ASSERT(NFD_Init() == NFD_OKAY);
	}

	void Filesystem::Destroy() {
		NFD_Quit();
	}

	void Filesystem::ReadFile(const char* path, std::string& data) {
		FILE_EXISTS(path);
		std::ifstream f(path);
		std::stringstream buffer;
		buffer << f.rdbuf();
		data = buffer.str();
	}

	bool Filesystem::FileExists(std::string& file) {
		return std::filesystem::exists(file);
	}

	bool Filesystem::FileExists(const std::string& file) {
		return std::filesystem::exists(file);
	}

	std::string Filesystem::GetParentPath(std::string& path) {
		FILE_EXISTS(path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string Filesystem::GetParentPath(const std::string& path) {
		FILE_EXISTS(path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string Filesystem::GetFileName(std::string& file) {
		FILE_EXISTS(file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string Filesystem::GetFileName(const std::string& file) {
		FILE_EXISTS(file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string Filesystem::GetFileExtension(std::string& file) {
		FILE_EXISTS(file);
		std::filesystem::path path(file);
		return path.extension().string();
	}

	std::string Filesystem::GetFileExtension(const std::string& file) {
		FILE_EXISTS(file);
		std::filesystem::path path(file);
		return path.extension().string();
	}
}
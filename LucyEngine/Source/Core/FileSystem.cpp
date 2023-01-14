#include "lypch.h"
#include "Filesystem.h"
#include "Utils/Utils.h"

#include "../nativefiledialog/include/nfd.h"

namespace Lucy {

	void Filesystem::Init() {
		LUCY_ASSERT(NFD_Init());
	}

	void Filesystem::Destroy() {
		NFD_Quit();
	}

	void Filesystem::ReadFile(const char* path, std::string& data) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
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
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string Filesystem::GetParentPath(const std::string& path) {
		LUCY_ASSERT(FileExists(path), "File cannot be found {0}", path);
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}

	std::string Filesystem::GetFileName(std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string Filesystem::GetFileName(const std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path relPath(file);
		return Utils::Split(relPath.filename().string(), ".")[0];
	}

	std::string Filesystem::GetFileExtension(std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path path(file);
		return path.extension().string();
	}

	std::string Filesystem::GetFileExtension(const std::string& file) {
		LUCY_ASSERT(FileExists(file), "File cannot be found {0}", file);
		std::filesystem::path path(file);
		return path.extension().string();
	}
}
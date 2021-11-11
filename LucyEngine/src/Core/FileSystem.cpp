#include "FileSystem.h"

namespace Lucy {

	std::string Lucy::FileSystem::GetParentPath(std::string& path) {
		std::filesystem::path relPath(path);
		return relPath.parent_path().string();
	}
}

#pragma once

#include "Base.h"

namespace Lucy {

	class FileSystem {
	public:
		static std::string GetParentPath(std::string& path);
		static std::string GetParentPath(const std::string& path);
		static std::string GetFileName(std::string& file);
		static std::string GetFileName(const std::string& file);
		static bool FileExists(std::string& file);
		static bool FileExists(const std::string& file);

	private:
		FileSystem() = delete;
		~FileSystem() = delete;
	};
}

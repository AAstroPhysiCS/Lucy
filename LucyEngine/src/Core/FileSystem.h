#pragma once

#include <filesystem>
#include "Base.h"

namespace Lucy {
	class FileSystem
	{
	public:
		static std::string GetParentPath(std::string& path);

	private:
		FileSystem() = delete;
		~FileSystem() = delete;
	};
}

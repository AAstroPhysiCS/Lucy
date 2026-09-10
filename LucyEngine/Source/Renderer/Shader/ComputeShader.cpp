#include "lypch.h"
#include "ComputeShader.h"

namespace Lucy {
	
	ComputeShader::ComputeShader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName)
		: Shader(name, path, entryPointName) {
	}

	void ComputeShader::RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) {
		LoadInternal(device, datas[0]);
	}
}
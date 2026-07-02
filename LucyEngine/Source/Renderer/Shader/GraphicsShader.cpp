#include "lypch.h"
#include "GraphicsShader.h"

namespace Lucy {

	GraphicsShader::GraphicsShader(const std::string& name, const std::filesystem::path& path)
		: Shader(name, path) {
	}

	void GraphicsShader::RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) {
		LoadInternal(device, datas[0], datas[1]);
	}
}
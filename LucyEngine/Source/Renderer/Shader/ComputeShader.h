#pragma once

#include "Shader.h"

namespace Lucy {

	class ComputeShader : public Shader {
	public:
		ComputeShader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName);
		virtual ~ComputeShader() = default;
	protected:
		void RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) final override;

		virtual void LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& dataCompute) = 0;
	};
}
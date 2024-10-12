#pragma once

#include "Shader.h"

namespace Lucy {

	class ComputeShader : public Shader {
	public:
		ComputeShader(const std::string& name, const std::filesystem::path& path);
		virtual ~ComputeShader() = default;
	protected:
		void RTLoad(const Ref<RenderDevice>& device, bool forceReloadFromDisk = false) final override;

		virtual void LoadInternal(const Ref<RenderDevice>& device, const std::vector<uint32_t>& dataCompute) = 0;
	};
}
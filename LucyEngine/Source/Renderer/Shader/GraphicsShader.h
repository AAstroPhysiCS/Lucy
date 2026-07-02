#pragma once

#include "Shader.h"

namespace Lucy {

	class GraphicsShader : public Shader {
	public:
		GraphicsShader(const std::string& name, const std::filesystem::path& path);
		virtual ~GraphicsShader() = default;

		void RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) final override;
	protected:
		virtual void LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& dataVertex, const std::span<const uint32_t>& dataFragment) = 0;
	};
}
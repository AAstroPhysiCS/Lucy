#pragma once

#include "Shader.h"

namespace Lucy {

	class GraphicsShader : public Shader {
	public:
		GraphicsShader(const std::string& name, const std::filesystem::path& path);
		virtual ~GraphicsShader() = default;

		void RTLoad(const Ref<RenderDevice>& device, bool forceReloadFromDisk = false) final override;

		struct Extensions {
			const char* vertexExtension;
			const char* fragmentExtension;
		};
		Extensions GetCachedFileExtension() const;
	protected:
		virtual void LoadInternal(const Ref<RenderDevice>& device, const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) = 0;
	};
}
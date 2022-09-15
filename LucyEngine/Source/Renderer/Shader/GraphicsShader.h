#pragma once

#include "Shader.h"

namespace Lucy {

	class GraphicsShader : public Shader {
	public:
		GraphicsShader(const std::string& name, const std::string& path);
		virtual ~GraphicsShader() = default;

		void Load() final override;

		struct Extensions {
			const char* vertexExtension;
			const char* fragmentExtension;
		};
		const Extensions GetCachedFileExtension();
	protected:
		virtual void LoadInternal(const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) = 0;
	};
}
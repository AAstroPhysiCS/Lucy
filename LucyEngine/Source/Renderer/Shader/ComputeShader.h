#pragma once

#include "Shader.h"

namespace Lucy {

	class ComputeShader : public Shader {
	public:
		ComputeShader(const std::string& name, const std::string& path);
		virtual ~ComputeShader() = default;
	protected:
		void Load() final override;

		virtual void LoadInternal(const std::vector<uint32_t>& dataCompute) = 0;
	};
}
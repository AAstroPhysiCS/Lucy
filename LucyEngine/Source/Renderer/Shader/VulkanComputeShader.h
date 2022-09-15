#pragma once

#include "ComputeShader.h"

namespace Lucy {

	class VulkanComputeShader final : public ComputeShader {
	public:
		VulkanComputeShader(const std::string& name, const std::string& path);
		virtual ~VulkanComputeShader() = default;
		
		inline VkPipelineShaderStageCreateInfo GetShaderStageInfo() { return m_ShaderStageInfo; }

		void Destroy() final override;
	private:
		void LoadInternal(const std::vector<uint32_t>& dataCompute) final override;
		
		VkPipelineShaderStageCreateInfo m_ShaderStageInfo{};
		VkShaderModule m_ComputeShaderModule{};
	};
}
#pragma once

#include "ComputeShader.h"

namespace Lucy {

	class VulkanComputeShader final : public ComputeShader {
	public:
		VulkanComputeShader(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device);
		virtual ~VulkanComputeShader() = default;
		
		inline VkPipelineShaderStageCreateInfo GetShaderStageInfo() const { return m_ShaderStageInfo; }

		void RTDestroyResource(const Ref<RenderDevice>& device) final override;
	private:
		void LoadInternal(const Ref<RenderDevice>& device, const std::vector<uint32_t>& dataCompute) final override;
		
		VkPipelineShaderStageCreateInfo m_ShaderStageInfo{};
		VkShaderModule m_ComputeShaderModule{};
	};
}
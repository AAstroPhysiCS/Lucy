#pragma once

#include "ComputeShader.h"

namespace Lucy {

	class VulkanComputeShader final : public ComputeShader {
	public:
		VulkanComputeShader(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device, const std::span<const uint32_t>& dataCompute);
		virtual ~VulkanComputeShader() = default;
		
		inline VkPipelineShaderStageCreateInfo GetShaderInfo() const { return m_ShaderStageInfo; }

		void RTDestroyResource(const Ref<RenderDevice>& device) final override;
	private:
		void LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& dataCompute) final override;
		
		VkPipelineShaderStageCreateInfo m_ShaderStageInfo{};
		VkShaderModule m_ComputeShaderModule{};
	};
}
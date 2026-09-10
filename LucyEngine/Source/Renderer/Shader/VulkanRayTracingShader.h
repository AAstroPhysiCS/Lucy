#pragma once

#include "Shader.h"

namespace Lucy {

	class VulkanRayTracingShader final : public Shader {
	public:
		VulkanRayTracingShader(const std::string& name, const std::filesystem::path& path, const std::string& entryPointName,
			ShaderStageType stage, Ref<RenderDevice> device, const std::span<const uint32_t>& data);
		virtual ~VulkanRayTracingShader() = default;

		inline VkPipelineShaderStageCreateInfo GetShaderInfo() const { return m_ShaderStageInfo; }
		inline ShaderStageType GetStage() const { return m_Stage; }

		void RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) final override;
		void RTDestroyResource(const Ref<RenderDevice>& device) final override;
	private:
		void LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& data);

		ShaderStageType m_Stage = ShaderStageType::Unknown;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfo{};
		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
	};

}
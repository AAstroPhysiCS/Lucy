#pragma once

#include "GraphicsShader.h"

namespace Lucy {

	class VulkanRenderDevice;

	class VulkanGraphicsShader final : public GraphicsShader {
	public:
		VulkanGraphicsShader(const std::string& name, const std::filesystem::path& path, const std::string& shaderEntryPointName, Ref<RenderDevice> device,
			const std::span<const uint32_t>& dataVertex, const std::span<const uint32_t>& dataFragment);
		virtual ~VulkanGraphicsShader() = default;

		void RTDestroyResource(const Ref<RenderDevice>& device) final override;

		inline VkPipelineShaderStageCreateInfo* GetShaderStageInfos() { return m_ShaderStageInfos; }
	private:
		void LoadInternal(const Ref<RenderDevice>& device, const std::span<const uint32_t>& dataVertex, const std::span<const uint32_t>& dataFragment) final override;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfos[2] = { {}, {} };

		//Gets destroyed after pipeline creation
		VkShaderModule m_VertexShaderModule{};
		VkShaderModule m_FragmentShaderModule{};
	};
}
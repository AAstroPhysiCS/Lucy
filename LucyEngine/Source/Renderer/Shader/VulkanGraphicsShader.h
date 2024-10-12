#pragma once

#include "GraphicsShader.h"

namespace Lucy {

	class VulkanRenderDevice;

	class VulkanGraphicsShader final : public GraphicsShader {
	public:
		VulkanGraphicsShader(const std::string& name, const std::filesystem::path& path, Ref<RenderDevice> device);
		virtual ~VulkanGraphicsShader() = default;

		void RTDestroyResource(const Ref<RenderDevice>& device) final override;

		inline VkPipelineShaderStageCreateInfo* GetShaderStageInfos() { return m_ShaderStageInfos; }
	private:
		void LoadInternal(const Ref<RenderDevice>& device, const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) final override;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfos[2] = { {}, {} };

		//Gets destroyed after pipeline creation
		VkShaderModule m_VertexShaderModule{};
		VkShaderModule m_FragmentShaderModule{};
	};
}
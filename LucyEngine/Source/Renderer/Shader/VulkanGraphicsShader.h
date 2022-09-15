#pragma once

#include "GraphicsShader.h"

#include "../Context/VulkanContextDevice.h"

namespace Lucy {

	class VulkanGraphicsShader final : public GraphicsShader {
	public:
		VulkanGraphicsShader(const std::string& path, const std::string& name);
		virtual ~VulkanGraphicsShader() = default;

		void Destroy() final override;

		inline VkPipelineShaderStageCreateInfo* GetShaderStageInfos() { return m_ShaderStageInfos; }
	private:
		void LoadInternal(const std::vector<uint32_t>& dataVertex, const std::vector<uint32_t>& dataFragment) override;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfos[2] = { {}, {} };

		//Gets destroyed after pipeline creation
		VkShaderModule m_VertexShaderModule{};
		VkShaderModule m_FragmentShaderModule{};
	};
}
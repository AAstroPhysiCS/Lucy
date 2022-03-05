#pragma once

#include "Shader.h"

#include "../Context/VulkanDevice.h"

namespace Lucy {

	class VulkanShader : public Shader {
	public:
		VulkanShader(const std::string& path, const std::string& name);

		void Bind() override;
		void Unbind() override;
		void Destroy() override;

		inline VkPipelineShaderStageCreateInfo* GetShaderStageInfos() { return m_ShaderStageInfos; }
	private:
		void LoadInternal(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) override;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfos[2] = { {}, {} };

		//Gets destroyed after pipeline creation
		VkShaderModule m_VertexShaderModule{};
		VkShaderModule m_FragmentShaderModule{};
	};
}
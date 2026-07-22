#pragma once

#include "Renderer/Device/RenderDeviceResource.h"
#include "Renderer/Device/RenderDeviceHandles.h"
#include "Renderer/Renderer.h"

#include "Renderer/Shader/ShaderReflect.h"

namespace Lucy {

	struct DescriptorSetCreateInfo {
		uint32_t SetIndex = 0;
		uint32_t Count = Renderer::GetMaxFramesInFlight();
		std::vector<ShaderVariable> ShaderVariables;
	};

	class DescriptorSet : public RenderDeviceResource {
	public:
		DescriptorSet(const DescriptorSetCreateInfo& createInfo);
		virtual ~DescriptorSet() = default;

		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;
		DescriptorSet(DescriptorSet&&) = delete;
		DescriptorSet& operator=(DescriptorSet&&) = delete;
		
		virtual void RTUpdate() = 0;

		void AddUniformBuffer(const std::string& name, RenderDeviceResourceHandle bufferHandle);
		void AddSharedStorageBuffer(const std::string& name, RenderDeviceResourceHandle bufferHandle);

		inline uint32_t GetSetIndex() const { return m_CreateInfo.SetIndex; }

		inline const auto& GetAllUniformBufferHandles() const { return m_UniformBufferHandles; }
		inline const auto& GetAllSharedStorageBufferHandles() const { return m_SharedStorageBufferHandles; }
	protected:
		virtual void RTDestroyResource() = 0;

		DescriptorSetCreateInfo m_CreateInfo;
	private:
		std::unordered_map<std::string, RenderDeviceResourceHandle> m_UniformBufferHandles;
		std::unordered_map<std::string, RenderDeviceResourceHandle> m_SharedStorageBufferHandles;
	};
}


#pragma once

#include <map>

#include "VulkanDescriptorPool.h"

#include "Renderer/Device/RenderDeviceHandles.h"

#include "../Shader/Shader.h"

namespace Lucy {

	class VulkanRenderDevice;

	class VulkanDescriptorSetManager final {
	public:
		//avoiding to parse the global set... because the render device already parses it ONCE.
		static inline constexpr uint32_t TEXTURE_BINDLESS_TABLE_SET_INDEX = 0;
		static inline constexpr uint32_t GLOBAL_PER_FRAME_SET_INDEX = 1;
	public:
		VulkanDescriptorSetManager(VulkanRenderDevice* device);
		~VulkanDescriptorSetManager() = default;

		VulkanDescriptorSetManager(const VulkanDescriptorSetManager&) = delete;
		VulkanDescriptorSetManager& operator=(const VulkanDescriptorSetManager&) = delete;
		VulkanDescriptorSetManager(VulkanDescriptorSetManager&&) noexcept = delete;
		VulkanDescriptorSetManager& operator=(VulkanDescriptorSetManager&&) = delete;

		void RegisterShaderBindings(const Ref<Shader>& shader);
		void RTDestroy();

		const RenderDeviceResourceHandle& GetGlobalDescriptorSet(uint32_t setIndex) const { return m_GlobalDescriptorSets.at(setIndex); }
		const RenderDeviceResourceHandle& GetDefaultSamplerHandle() const { return m_DefaultSamplerHandle; }
		std::vector<RenderDeviceResourceHandle> GetDescriptorSetHandles(const Ref<Shader>& shader);
	private:
		VulkanRenderDevice* m_RenderDevice = nullptr;
		Ref<VulkanDescriptorPool> m_GlobalDescriptorPool = nullptr;
		Ref<VulkanDescriptorPool> m_PerShaderDescriptorPool = nullptr;

		std::unordered_map<uint32_t, RenderDeviceResourceHandle> m_GlobalDescriptorSets;
		std::unordered_map<uint32_t, RenderDeviceResourceHandle> m_DescriptorSetsPerShader;

		RenderDeviceResourceHandle m_DefaultSamplerHandle{};
	};
}

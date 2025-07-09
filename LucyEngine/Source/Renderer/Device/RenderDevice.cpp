#include "lypch.h"
#include "RenderDevice.h"
#include "VulkanRenderDevice.h"

#include "Renderer/Renderer.h"
#include "Renderer/VulkanRenderPass.h"

#include "Renderer/Pipeline/VulkanGraphicsPipeline.h"
#include "Renderer/Pipeline/VulkanComputePipeline.h"

#include "Renderer/Image/VulkanImageCube.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanVertexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanIndexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanSharedStorageBuffer.h"

namespace Lucy {

	Ref<RenderDevice> RenderDevice::Create(RendererConfiguration config) {
		switch (config.RenderArchitecture) {
			case RenderArchitecture::Vulkan: {
				return Memory::CreateRef<VulkanRenderDevice>();
				break;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the renderer!");
				break;
		}
		return nullptr;
	}

	void RenderDevice::CreatePipelineDeviceQueries(size_t pipelineCount) {
		LUCY_INFO("Creating pipeline queries. Pipeline count: {0}", pipelineCount);
		m_RenderDevicePipelineQuery = RenderDeviceQuery::Create({
			.Device = shared_from_this()->As<RenderDevice>(),
			.QueryCount = (uint32_t)pipelineCount,
			.QueryType = RenderDeviceQueryType::Pipeline
		});
	}

	void RenderDevice::CreateTimestampDeviceQueries(size_t passCount) {
		LUCY_INFO("Creating timestamp queries. Pass count: {0}", passCount);

		m_RenderDeviceTimestampQuery = RenderDeviceQuery::Create({
			.Device = shared_from_this()->As<RenderDevice>(),
			.QueryCount = (uint32_t)passCount * 2,
			.QueryType = RenderDeviceQueryType::Timestamp
		});
	}

	void RenderDevice::RTResetPipelineQuery(Ref<CommandPool> commandPool) {
		m_RenderDevicePipelineQuery->RTResetPoolByIndex(commandPool, Renderer::GetCurrentFrameIndex());
	}

	void RenderDevice::RTResetTimestampQuery(Ref<CommandPool> commandPool) {
		m_RenderDeviceTimestampQuery->RTResetPoolByIndex(commandPool, Renderer::GetCurrentFrameIndex());
	}

	uint32_t RenderDevice::RTBeginTimestamp(Ref<CommandPool> cmdPool) {
		return m_RenderDeviceTimestampQuery->RTBegin(cmdPool);
	}
	
	uint32_t RenderDevice::RTEndTimestamp(Ref<CommandPool> cmdPool) {
		return m_RenderDeviceTimestampQuery->RTEnd(cmdPool);
	}

	uint32_t RenderDevice::RTBeginPipelineQuery(Ref<CommandPool> cmdPool) {
		return m_RenderDevicePipelineQuery->RTBegin(cmdPool);
	}

	uint32_t RenderDevice::RTEndPipelineQuery(Ref<CommandPool> cmdPool) {
		return m_RenderDevicePipelineQuery->RTEnd(cmdPool);
	}

	std::vector<uint64_t> RenderDevice::GetQueryResults(RenderDeviceQueryType type) {
		switch (type) {
			case RenderDeviceQueryType::Timestamp:
				return m_RenderDeviceTimestampQuery->GetQueryResults();
			case RenderDeviceQueryType::Pipeline:
				return m_RenderDevicePipelineQuery->GetQueryResults();
			default:
				LUCY_ASSERT(false, "Unimplemented device query type!");
		};
		return {};
	}

	RenderResourceHandle RenderDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) {
		static std::mutex pipelineCreationMutex;

		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanGraphicsPipeline>(createInfo, shared_from_this()->As<VulkanRenderDevice>());

				std::unique_lock<std::mutex> lock(pipelineCreationMutex);
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) {
		static std::mutex pipelineCreationMutex;

		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanComputePipeline>(createInfo, shared_from_this()->As<VulkanRenderDevice>());

				std::unique_lock<std::mutex> lock(pipelineCreationMutex);
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateRenderPass(const RenderPassCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanRenderPass>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateImage(const ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				if (createInfo.ImageType == ImageType::TypeCube) {
					auto resource = Memory::CreateRef<VulkanImageCube>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
					auto handle = m_ResourceManager.PushResource(resource);
					return handle;
				}
				auto resource = Memory::CreateRef<VulkanImage2D>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateImage(const std::filesystem::path& path, ImageCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				if (createInfo.ImageType == ImageType::TypeCube) {
					auto resource = Memory::CreateRef<VulkanImageCube>(path, createInfo, shared_from_this()->As<VulkanRenderDevice>());
					auto handle = m_ResourceManager.PushResource(resource);
					return handle;
				}
				auto resource = Memory::CreateRef<VulkanImage2D>(path, createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateImage(const Ref<VulkanImage2D>& other) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanImage2D>(other, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateFrameBuffer(const FrameBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanFrameBuffer>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateVertexBuffer(size_t size) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanVertexBuffer>(size, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateIndexBuffer(size_t size) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanIndexBuffer>(size, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateDescriptorSet(const DescriptorSetCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanDescriptorSet>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateSharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanSharedStorageBuffer>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	RenderResourceHandle RenderDevice::CreateUniformBuffer(const UniformBufferCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan: {
				auto resource = Memory::CreateRef<VulkanUniformBuffer>(createInfo, shared_from_this()->As<VulkanRenderDevice>());
				auto handle = m_ResourceManager.PushResource(resource);
				return handle;
			}
			default:
				LUCY_ASSERT(false, "No suitable API found to create the resource!");
		}
		return InvalidRenderResourceHandle;
	}

	bool RenderDevice::IsValidResource(RenderResourceHandle handle) const {
		return m_ResourceManager.ResourceExists(handle);
	}

	void RenderDevice::RTDestroyResource(RenderResourceHandle& handle) {
		m_ResourceManager.RTDestroyResource(handle);
	}
}
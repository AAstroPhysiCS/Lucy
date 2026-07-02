#include "lypch.h"

#include "Renderer/Image/VulkanImage.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "VulkanUniformImageSampler.h"

namespace Lucy {
	
	PipelineConstant& Pipeline::GetPipelineConstants(const std::string& name) {
		for (PipelineConstant& pushConstant : m_PushConstants) {
			if (name == pushConstant.GetName()) {
				return pushConstant;
			}
		}
		LUCY_ASSERT(false, "Could not find a suitable Push Constant for the given name: {0}", name);
	}

	uint32_t Pipeline::BindImageHandleTo(const std::string& imageBufferName, const Ref<Image>& image, size_t mip) {
		LUCY_ASSERT(image != nullptr, "Binding image failed, because image '{0}' is nullptr!", imageBufferName);

		const auto& descriptorSetHandles = GetDescriptorSetHandles();

		if (Renderer::GetRenderArchitecture() == RenderArchitecture::Vulkan) {
			const Ref<VulkanImage> vulkanImage = image->As<VulkanImage>();

			for (const auto& handle : descriptorSetHandles) {
				auto descriptorSet = GetDescriptorSetFromHandle(handle)->As<VulkanDescriptorSet>();

				if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName)) {
					if (mip == (size_t)-1) {
						imageSampler->ImageInfos.push_back(
							VulkanAPI::DescriptorImageInfo(
								vulkanImage->GetCurrentLayout(),
								vulkanImage->GetImageView().GetVulkanHandle(),
								vulkanImage->GetSampler().GetVulkanHandle()
							)
						);
					} else {
						imageSampler->ImageInfos.push_back(
							VulkanAPI::DescriptorImageInfo(
								vulkanImage->GetCurrentLayout(),
								vulkanImage->GetImageView().GetMipViewVulkanHandle(mip),
								vulkanImage->GetSampler().GetVulkanHandle()
							)
						);
					}

					return (uint32_t)imageSampler->ImageInfos.size() - 1;
				}
			}
			LUCY_ASSERT(false, "Could not find a image sampler with the name {0}", imageBufferName);
		}
		return 0;
	}

	bool Pipeline::HasImageHandleBoundTo(const std::string& imageBufferName) {
		auto& descriptorSetHandles = GetDescriptorSetHandles();
		for (const auto& handle : descriptorSetHandles) {
			auto descriptorSet = GetDescriptorSetFromHandle(handle)->As<VulkanDescriptorSet>();

			if (auto imageSampler = descriptorSet->GetVulkanImageSampler(imageBufferName))
				return !imageSampler->ImageInfos.empty();
		}
		LUCY_ASSERT(false, "Could not find a image sampler with the name {0}", imageBufferName);
		return false;
	}

	void Pipeline::RTDestroyResource() {
		Renderer::EnqueueToRenderCommandQueue([=](const auto& device) {
			auto& descriptorSetHandles = GetDescriptorSetHandles();

			m_PushConstants.clear();
			for (auto handles : descriptorSetHandles)
				device->RTDestroyResource(handles);
			descriptorSetHandles.clear();
		});
	}

	void Pipeline::AddDescriptorSetHandle(RenderResourceHandle handle) { 
		m_DescriptorSetHandles.push_back(handle); 
	}

	void Pipeline::AddPushConstant(const ShaderVariable& pc) {
		m_PushConstants.emplace_back(pc.Name, pc.BufferSize, 0, pc.StageFlag);
	}
}

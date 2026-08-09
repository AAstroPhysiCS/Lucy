#include "lypch.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphPass.h"
#include "RenderGraphResource.h"
#include "RenderGraph.h"

#include "Renderer/Image/Image.h"

namespace Lucy {

	RenderGraphBuilder::RenderGraphBuilder(RenderGraph* renderGraph, RenderGraphPass* pass) 
		: m_RenderGraph(renderGraph), m_RenderGraphPass(pass) {
	}

	void RenderGraphBuilder::SetViewportArea(uint32_t width, uint32_t height) {
		m_RenderGraphPass->SetViewportArea(width, height);
	}

	void RenderGraphBuilder::SetInFlightMode(bool mode) {
		m_RenderGraphPass->SetInFlightMode(mode);
	}

	void RenderGraphBuilder::SetClearColor(ClearColor clearColor) {
		m_RenderGraphPass->SetClearColor(clearColor);
	}

	void RenderGraphBuilder::SetExecutionPolicy(RenderGraphExecutionPolicy policy) {
		m_RenderGraphPass->SetExecutionPolicy(policy);
	}

	void RenderGraphBuilder::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp) {
		m_RenderGraph->DeclareImage(rgResource, createInfo, loadStoreAccessOp);
	}

	void RenderGraphBuilder::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
													const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp) {
		m_RenderGraph->DeclareImage(rgResource, createInfo, loadStoreAccessOp, rgResourceDepth, createDepthInfo, loadStoreDepthAccessOp);
	}

	void RenderGraphBuilder::DeclareBuffer(const RenderGraphResource& rgResource, const RenderDeviceBufferCreateInfo& createInfo) {
		m_RenderGraph->DeclareBuffer(rgResource, createInfo, m_RenderGraphPass->IsInFlightMode());
	}

	void RenderGraphBuilder::ReadExternalImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access) {
		m_RenderGraph->ReadExternalImage(m_RenderGraphPass, rgResource);
		m_RenderGraphPass->AddResourceRead({
			.Resource = rgResource,
			.Type = RenderGraphResourceType::Image,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = true,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::ReadExternalTransientImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access) {
		m_RenderGraph->ReadExternalTransientImage(m_RenderGraphPass, rgResource);
		m_RenderGraphPass->AddResourceRead({
			.Resource = rgResource,
			.Type = RenderGraphResourceType::Image,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = true,
			.IsTransient = true
		});
	}

	void RenderGraphBuilder::WriteExternalImage(const RenderGraphResource& rgResource, RenderGraphResourceAccess access) {
		m_RenderGraph->WriteExternalImage(m_RenderGraphPass, rgResource);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResource,
			.Type = RenderGraphResourceType::Image,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = true,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::ReadImage(const RenderGraphResource& rgResourceToRead, RenderGraphResourceAccess access) {
		m_RenderGraph->ReadImage(m_RenderGraphPass, rgResourceToRead);
		m_RenderGraphPass->AddResourceRead({
			.Resource = rgResourceToRead,
			.Type = RenderGraphResourceType::Image,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::WriteImage(const RenderGraphResource& rgResourceToWrite, RenderGraphResourceAccess access) {
		m_RenderGraph->WriteImage(m_RenderGraphPass, rgResourceToWrite);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResourceToWrite,
			.Type = RenderGraphResourceType::Image,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::ReadBuffer(const RenderGraphResource& rgResourceToRead, RenderGraphResourceAccess access) {
		m_RenderGraph->ReadBuffer(m_RenderGraphPass, rgResourceToRead);
		m_RenderGraphPass->AddResourceRead({
			.Resource = rgResourceToRead,
			.Type = RenderGraphResourceType::Buffer,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::WriteBuffer(const RenderGraphResource& rgResourceToWrite, RenderGraphResourceAccess access) {
		m_RenderGraph->WriteBuffer(m_RenderGraphPass, rgResourceToWrite);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResourceToWrite,
			.Type = RenderGraphResourceType::Buffer,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::ReadExternalBuffer(const RenderGraphResource& rgResource, RenderGraphResourceAccess access) {
		m_RenderGraph->ReadExternalBuffer(m_RenderGraphPass, rgResource);
		m_RenderGraphPass->AddResourceRead({
			.Resource = rgResource,
			.Type = RenderGraphResourceType::Buffer,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = true,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::WriteExternalBuffer(const RenderGraphResource& rgResource, RenderGraphResourceAccess access) {
		m_RenderGraph->WriteExternalBuffer(m_RenderGraphPass, rgResource);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResource,
			.Type = RenderGraphResourceType::Buffer,
			.Access = access,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = true,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::BindRenderTarget(const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind) {
		BindRenderTarget(rgResourceToBind);
		m_RenderGraph->BindRenderTarget(m_RenderGraphPass, rgResourceDepthToBind);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResourceDepthToBind,
			.Type = RenderGraphResourceType::Image,
			.Access = RenderGraphResourceAccess::DepthAttachmentWrite,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}

	void RenderGraphBuilder::BindRenderTarget(const RenderGraphResource& rgResourceToBind) {
		m_RenderGraph->BindRenderTarget(m_RenderGraphPass, rgResourceToBind);
		m_RenderGraphPass->AddResourceWrite({
			.Resource = rgResourceToBind,
			.Type = RenderGraphResourceType::Image,
			.Access = RenderGraphResourceAccess::ColorAttachmentWrite,
			.QueueFamily = m_RenderGraphPass->GetTargetQueueFamily(),
			.IsExternal = false,
			.IsTransient = false
		});
	}
}
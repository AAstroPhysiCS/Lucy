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

	void RenderGraphBuilder::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp) {
		m_RenderGraph->DeclareImage(rgResource, createInfo, loadStoreAccessOp);
	}

	void RenderGraphBuilder::DeclareImage(const RenderGraphResource& rgResource, const ImageCreateInfo& createInfo, RenderPassLoadStoreAttachments loadStoreAccessOp,
													const RenderGraphResource& rgResourceDepth, const ImageCreateInfo& createDepthInfo, RenderPassLoadStoreAttachments loadStoreDepthAccessOp) {
		m_RenderGraph->DeclareImage(rgResource, createInfo, loadStoreAccessOp, rgResourceDepth, createDepthInfo, loadStoreDepthAccessOp);
	}

	void RenderGraphBuilder::ReadExternalImage(const RenderGraphResource& rgResource) {
		m_RenderGraph->ReadExternalImage(m_RenderGraphPass, rgResource);
	}

	void RenderGraphBuilder::ReadExternalTransientImage(const RenderGraphResource& rgResource) {
		m_RenderGraph->ReadExternalTransientImage(m_RenderGraphPass, rgResource);
	}

	void RenderGraphBuilder::WriteExternalImage(const RenderGraphResource& rgResource) {
		m_RenderGraph->WriteExternalImage(m_RenderGraphPass, rgResource);
	}

	void RenderGraphBuilder::ReadImage(const RenderGraphResource& rgResourceToRead) {
		m_RenderGraph->ReadImage(m_RenderGraphPass, rgResourceToRead);
	}

	void RenderGraphBuilder::WriteImage(const RenderGraphResource& rgResourceToWrite) {
		m_RenderGraph->WriteImage(m_RenderGraphPass, rgResourceToWrite);
	}

	void RenderGraphBuilder::BindRenderTarget(const RenderGraphResource& rgResourceToBind, const RenderGraphResource& rgResourceDepthToBind) {
		BindRenderTarget(rgResourceToBind);
		BindRenderTarget(rgResourceDepthToBind);
	}

	void RenderGraphBuilder::BindRenderTarget(const RenderGraphResource& rgResourceToBind) {
		m_RenderGraph->BindRenderTarget(m_RenderGraphPass, rgResourceToBind);
	}

	void RenderGraphBuilder::ReadBuffer(const RenderGraphResource& rgResourceToRead) {
		m_RenderGraph->ReadBuffer(m_RenderGraphPass, rgResourceToRead);
	}

	void RenderGraphBuilder::WriteBuffer(const RenderGraphResource& rgResourceToWrite) {
		m_RenderGraph->WriteBuffer(m_RenderGraphPass, rgResourceToWrite);
	}
}
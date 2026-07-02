#include "lypch.h"

#include "RenderGraphPass.h"
#include "RenderGraphResource.h"
#include "RenderGraphRegistry.h"

namespace Lucy {

	RenderGraphPass::RenderGraphPass(const RenderGraphPassCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	void RenderGraphPass::Setup(RenderGraphBuilder& build) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::Setup");
		m_ExecuteFunc = std::move(m_CreateInfo.SetupFunc(build));
	}

	void RenderGraphPass::AddRenderTarget(const RenderGraphResource& renderTargetToAdd) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::AddRenderTarget");
		m_RenderTargets.push_back(renderTargetToAdd);
	}

	void RenderGraphPass::AddResourceRead(const RenderGraphResourceAddInfo& addInfo) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::AddResourceRead");
		m_ResourceReads.push_back(addInfo);
	}

	void RenderGraphPass::AddResourceWrite(const RenderGraphResourceAddInfo& addInfo) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::AddResourceWrite");
		m_ResourceWrites.push_back(addInfo);
	}

	void RenderGraphPass::OnViewportResize(uint32_t width, uint32_t height) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::OnViewportResize");
		SetViewportArea(width, height);
	}

	void RenderGraphPass::Execute(RenderCommandList& cmdList) {
		LUCY_PROFILE_NEW_EVENT("RenderGraphPass::Execute");
		m_ExecuteFunc(m_CreateInfo.Registry, cmdList);
	}

	void RenderGraphPass::SetViewportArea(uint32_t width, uint32_t height) {
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}
	
	void RenderGraphPass::SetInFlightMode(bool mode) {
		m_PassIsInFlightMode = mode;
	}

	void RenderGraphPass::SetState(RenderGraphPassState state) {
		m_State = state;
	}

	void RenderGraphPass::SetClearColor(ClearColor clearColor) {
		m_ClearColor = clearColor;
	}
}

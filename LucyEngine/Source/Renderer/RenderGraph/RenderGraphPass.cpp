#include "lypch.h"

#include "RenderGraphPass.h"

namespace Lucy {

	RenderGraphPass::RenderGraphPass(const RenderGraphPassCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	void RenderGraphPass::Setup(RenderGraphBuilder& build) {
		m_ExecuteFunc = std::move(m_CreateInfo.SetupFunc(build));
	}

	void RenderGraphPass::AddRenderTarget(const RenderGraphResource& renderTargetToAdd) {
		m_RenderTargets.push_back(renderTargetToAdd);
	}

	void RenderGraphPass::OnViewportResize(uint32_t width, uint32_t height) {
		SetViewportArea(width, height);
	}

	void RenderGraphPass::Execute(RenderCommandList& cmdList) {
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

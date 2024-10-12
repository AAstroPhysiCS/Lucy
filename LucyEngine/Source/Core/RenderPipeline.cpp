#include "lypch.h"
#include "RenderPipeline.h"

#include "Events/EventHandler.h"

namespace Lucy {

	RenderPipeline::RenderPipeline(const RenderPipelineCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	void RenderPipeline::OnEvent(Event& e) {
		EventHandler::AddListener<WindowResizeEvent>(e, [this](const WindowResizeEvent& evt) {
			SetViewportArea(evt.GetWidth(), evt.GetHeight());
		});

		EventHandler::AddListener<ViewportAreaResizeEvent>(e, [this](const ViewportAreaResizeEvent& evt) {
			SetViewportArea(evt.GetWidth(), evt.GetHeight());
		});

		EventHandler::AddListener<CursorPosEvent>(e, [this](const CursorPosEvent& evt) {
			m_ViewportMouseX = (float)evt.GetXPos();
			m_ViewportMouseY = (float)evt.GetYPos();

			Input::MouseX = m_ViewportMouseX;
			Input::MouseY = m_ViewportMouseY;
		});
	}
}
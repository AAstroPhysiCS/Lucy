#pragma once

#include "Core/Module.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Scene/Scene.h"

#include "ImGuiOverlay.h"

namespace Lucy {

	class EditorModule : public Module {
	public:
		EditorModule(Ref<Window> window, Ref<Scene> scene);
		virtual ~EditorModule() = default;
		
		void Begin() override;
		void End() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
		void Destroy() override;
		void Wait() override;

		inline const Ref<Scene>& GetScene() const { return m_Scene; }
	private:
		ImGuiOverlay m_ImGuiOverlay;
	};
}
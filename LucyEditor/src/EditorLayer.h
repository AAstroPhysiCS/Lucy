#pragma once

#include "Core/Layer.h"
#include "Renderer/Renderer.h"

namespace Lucy {

	class EditorLayer : public Layer {

	private:
		EditorLayer() = default;
		~EditorLayer() = default;

	public:
		
		static EditorLayer* GetInstance() {
			if (!s_Instance)
				s_Instance = new EditorLayer();
			return s_Instance;
		}

		void Begin();
		void End();
		void OnRender();
		void OnEvent();
		void Destroy();

	private:
		static EditorLayer* s_Instance;
	};

}
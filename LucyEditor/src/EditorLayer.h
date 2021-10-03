#pragma once

#include "Layer.h"

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

		void OnUpdate() override;
		void OnEvent() override;
		
		void Destroy() override {
			delete s_Instance;
		}

	private:
		static EditorLayer* s_Instance;
	};

}

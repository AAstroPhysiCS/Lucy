#pragma once

#include "Core/Application.h"

namespace Lucy {

	class EditorApplication : public Application {
	public:
		EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& specs);
		virtual ~EditorApplication();

		void Run() final override;
		void OnEvent(Event* e) final override;
	private:
		Ref<Scene> m_Scene = nullptr;
	};
}
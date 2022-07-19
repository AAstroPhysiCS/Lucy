#pragma once

#include "Core/Application.h"

namespace Lucy {

	class EditorApplication : public Application {
	public:
		EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& specs);
		virtual ~EditorApplication();

		void Run() override;
		void OnEvent(Event* e) override;
	private:
		Ref<Scene> m_Scene = nullptr;
	};
}
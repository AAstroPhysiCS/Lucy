#pragma once

#include "Core/Application.h"

namespace Lucy {

	class EditorApplication : public Application {
	public:
		EditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& specs);
		virtual ~EditorApplication() = default;

		void Run() override;
		void OnEvent(Event* e) override;
	};
}
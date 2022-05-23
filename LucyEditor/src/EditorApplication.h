#pragma once

#include "Core/Application.h"
#include "Core/Base.h"
#include "Core/Metrics.h"

namespace Lucy {

	class EditorApplication : public Application {
	public:
		EditorApplication(const ApplicationArgs& args, ApplicationSpecification& specs);
		virtual ~EditorApplication();

		void Run() override;
		void OnEvent(Event* e) override;
	};
}
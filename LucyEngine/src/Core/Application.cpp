#include "lypch.h"
#include "Application.h"

namespace Lucy {

	Application::Application(const ApplicationArgs& args, ApplicationSpecification& specs)
		: m_Args(args), m_Specification(specs) {
	}
}

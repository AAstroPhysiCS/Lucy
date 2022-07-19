#include "lypch.h"
#include "Module.h"

namespace Lucy {

	Module::Module(Ref<Window> window, Ref<Scene> scene)
		: m_Window(window), m_Scene(scene) {
	}
}
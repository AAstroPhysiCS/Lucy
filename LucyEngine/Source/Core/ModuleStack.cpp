#include "lypch.h"
#include "ModuleStack.h"

namespace Lucy {

	void ModuleStack::Push(Ref<Module> m) {
		m_ModuleStack.push_back(m);
	}

	void ModuleStack::Pop() {
		m_ModuleStack.pop_back();
	}
}
#include "lypch.h"
#include "ModuleStack.h"

namespace Lucy {

	void ModuleStack::Push(std::initializer_list<Module*> list) {
		std::copy(list.begin(), list.end(), std::back_inserter(m_ModuleStack));
	}

	void ModuleStack::Pop() {
		m_ModuleStack.pop_back();
	}
}
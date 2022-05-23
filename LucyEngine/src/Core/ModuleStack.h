#pragma once
#include <vector>

#include "Base.h"
#include "Module.h"

namespace Lucy {

	class ModuleStack {
	public:
		ModuleStack() = default;
		~ModuleStack() = default;

		void Push(std::initializer_list<Module*> list);
		void Pop();

		inline std::vector<Module*>& GetStack() { return m_ModuleStack; }
	private:
		std::vector<Module*> m_ModuleStack;
	};
}
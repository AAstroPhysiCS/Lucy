#pragma once
#include <vector>

#include "Base.h"
#include "Module.h"

namespace Lucy {

	class ModuleStack {
	public:
		ModuleStack() = default;
		~ModuleStack() = default;

		void Push(RefLucy<Module> m);
		void Pop();

		inline std::vector<RefLucy<Module>>& GetStack() { return m_ModuleStack; }
	private:
		std::vector<RefLucy<Module>> m_ModuleStack;
	};
}
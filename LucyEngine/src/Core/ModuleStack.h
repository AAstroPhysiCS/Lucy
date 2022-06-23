#pragma once
#include <vector>

#include "Base.h"
#include "Module.h"

namespace Lucy {

	class ModuleStack {
	public:
		ModuleStack() = default;
		~ModuleStack() = default;

		void Push(Ref<Module> m);
		void Pop();

		inline std::vector<Ref<Module>>& GetStack() { return m_ModuleStack; }
	private:
		std::vector<Ref<Module>> m_ModuleStack;
	};
}
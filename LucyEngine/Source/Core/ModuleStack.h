#pragma once
#include <vector>

#include "Module.h"

namespace Lucy {

	class ModuleStack {
	public:
		ModuleStack() = default;
		~ModuleStack() = default;

		using BeginModuleStackIterator = std::vector<Ref<Module>>::iterator;
		using BeginConstModuleStackIterator = const std::vector<Ref<Module>>::const_iterator;
		
		using EndModuleStackIterator = std::vector<Ref<Module>>::iterator;
		using EndConstModuleStackIterator = const std::vector<Ref<Module>>::const_iterator;

		void Push(Ref<Module> m);
		void Pop();

		inline Ref<Module>& operator[](uint32_t index) {
			return m_ModuleStack[index];
		}

		inline size_t GetSize() { return m_ModuleStack.size(); }

		inline BeginModuleStackIterator begin() { return m_ModuleStack.begin(); }
		inline BeginConstModuleStackIterator begin() const { return m_ModuleStack.cbegin(); }

		inline EndModuleStackIterator end() { return m_ModuleStack.end(); }
		inline EndConstModuleStackIterator end() const { return m_ModuleStack.cend(); }
	private:
		std::vector<Ref<Module>> m_ModuleStack;
	};
}
#include "lypch.h"
#include "LayerStack.h"

namespace Lucy {

	void LayerStack::Push(std::initializer_list<Layer*> list) {
		std::copy(list.begin(), list.end(), std::back_inserter(m_LayerStack));
	}

	void LayerStack::Pop() {
		m_LayerStack.pop_back();
	}
}
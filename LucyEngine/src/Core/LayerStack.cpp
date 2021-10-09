#include "LayerStack.h"

namespace Lucy {
	
	void LayerStack::Push(std::initializer_list<Layer*> list)
	{
		for (Layer* l : list) {
			m_LayerStack.push_back(l);
		}
	}

	void LayerStack::Pop()
	{
		m_LayerStack.pop_back();
	}

	std::vector<Layer*> LayerStack::GetStack() const
	{
		return m_LayerStack;
	}

}
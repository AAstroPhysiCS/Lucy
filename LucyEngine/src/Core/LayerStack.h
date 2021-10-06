#pragma once

#include "Layer.h"

#include <vector>

namespace Lucy {
	class LayerStack
	{

	public:
		LayerStack() = default;
		~LayerStack() = default;

		void Push(Layer* layer) {
			m_LayerStack.push_back(layer);
		}

		void Pop() {
			m_LayerStack.pop_back();
		}

		std::vector<Layer*> GetStack() const { return m_LayerStack; }

	private:
		std::vector<Layer*> m_LayerStack;
	};
}



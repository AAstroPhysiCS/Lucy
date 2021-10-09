#pragma once

#include "Layer.h"

#include <vector>

namespace Lucy {
	class LayerStack
	{

	public:
		LayerStack() = default;
		~LayerStack() = default;

		void Push(std::initializer_list<Layer*> list);
		void Pop();

		std::vector<Layer*> GetStack() const;

	private:
		std::vector<Layer*> m_LayerStack;
	};
}
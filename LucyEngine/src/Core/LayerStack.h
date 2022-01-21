#pragma once

#include <vector>

#include "Base.h"
#include "Layer.h"

namespace Lucy {

	class LayerStack {
	public:
		LayerStack() = default;
		~LayerStack() = default;

		void Push(std::initializer_list<Layer*> list);
		void Pop();

		inline std::vector<Layer*>& GetStack() { return m_LayerStack; }
	private:
		std::vector<Layer*> m_LayerStack;
	};
}
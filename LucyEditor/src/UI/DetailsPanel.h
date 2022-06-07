#pragma once

#include "UI/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class DetailsPanel : public Panel
	{
	public:
		static DetailsPanel& GetInstance();
	private:
		DetailsPanel() = default;
		virtual ~DetailsPanel() = default;

		void Render();

		template <typename T>
		static void DrawComponentPanel(Entity& e, std::function<void(T&)> func) {
			if (e.HasComponent<T>())
				func(e.GetComponent<T>());
		}
	};
}


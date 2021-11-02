#pragma once

#include <functional>

#include "UI/Panel.h"
#include "Scene/Entity.h"

namespace Lucy {

	class PropertiesPanel : public Panel
	{
	public:
		static PropertiesPanel& GetInstance();

		void Render();
	private:
		template <typename T>
		static void DrawComponentPanel(Entity& e, std::function<void(T&)> func) {
			if (e.HasComponent<T>())
				func(e.GetComponent<T>());
		}

		void RenderTransformControl(const char* id, float& x, float& y, float& z, float defaultValue, float speed);
	};
}


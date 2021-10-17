#pragma once

#include "../Events/Event.h"

namespace Lucy {

	class Layer
	{
	protected:
		Layer() = default;
		virtual ~Layer() = default;
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void OnRender() = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void Destroy() = 0;
	};
}
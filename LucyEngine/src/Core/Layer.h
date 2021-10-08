#pragma once

namespace Lucy {
	class Layer
	{

	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void OnRender() = 0;
		virtual void OnEvent() = 0;
		virtual void Destroy() = 0;
	};
}
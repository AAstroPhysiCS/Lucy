#pragma once

namespace Lucy {
	class Layer
	{

	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnUpdate() = 0;
		virtual void OnEvent() = 0;
		virtual void Destroy() = 0;
	};
}
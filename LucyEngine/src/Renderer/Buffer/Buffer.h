#pragma once

namespace Lucy {

	template <typename T>
	class Buffer {
	protected:
		Buffer() = default;
		~Buffer() = default;

		uint32_t m_Id = 0;
		T* m_DataHead = nullptr;
	};
}
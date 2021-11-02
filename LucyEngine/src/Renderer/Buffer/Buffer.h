#pragma once

#include <stdint.h>

namespace Lucy {

	template <typename T>
	class Buffer
	{
	public:
		inline T* GetData() const { return m_DataHead; }

	protected:
		Buffer(uint32_t size)
			: m_AllocSize(size) {}
		Buffer() = default;
		~Buffer() = default;

		uint32_t m_AllocSize = 0;
		uint32_t m_Id = 0;
		T* m_DataHead = nullptr;
	};
}
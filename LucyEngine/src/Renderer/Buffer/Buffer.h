#pragma once

#include <stdint.h>

namespace Lucy {

	template <typename T>
	class Buffer
	{
	public:
		inline T* GetData() const { return m_Data; }

	protected:
		Buffer(uint32_t size)
			: m_Size(size)
		{
		}

		~Buffer()
		{
			delete[] m_Data;
		}
		
		Buffer() = default;

		uint32_t m_Size = 0;
		uint32_t m_Id = 0;
		T* m_Data = nullptr;
	};
}
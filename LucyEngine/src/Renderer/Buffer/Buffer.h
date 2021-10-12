#pragma once

#include <stdint.h>

namespace Lucy {
	class Buffer
	{

	protected:
		Buffer(uint32_t size);

	protected:
		uint32_t m_Size = 0;
		uint32_t m_Id = 0;
		void* m_Data = nullptr;
	};
}
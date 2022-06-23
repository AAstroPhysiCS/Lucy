#pragma once

namespace Lucy {

	template <typename T>
	class Buffer {
		using VecIterator = typename std::vector<T>::iterator;
	public:
		Buffer() = default;

		virtual ~Buffer() {
			Clear();
		}

		Buffer(const Buffer& other) {
			std::copy(other.m_Data.begin(), other.m_Data.end(), std::back_inserter(m_Data));
		}

		inline Buffer& operator=(const Buffer& other) { 
			if (this != &other) {
				Clear();
				std::copy(other.m_Data.begin(), other.m_Data.end(), std::back_inserter(m_Data));
			}
			return *this;
		}

		inline bool operator==(const Buffer& other) { return m_Data == other.m_Data; }
		inline bool operator!=(const Buffer& other) { return !(*this == other); }

		inline const T& operator[](size_t index) const { return m_Data[index]; }
		inline T& operator[](size_t index) { return m_Data[index]; }

		///Size in Bytes
		void Allocate(size_t size) {
			if (size < 0) {
				LUCY_CRITICAL("Trying to allocate negative bytes.");
				LUCY_ASSERT(false);
			}
			m_Data.resize(size);
		}

		void SetData(const std::vector<T>& data, uint32_t from = 0, uint32_t to = 0) {
			if (to == 0)
				to = data.size();

			if (to > data.size() || from < 0 || m_Data.size() < data.size()) {
				LUCY_CRITICAL("Index out of bounds");
				LUCY_ASSERT(false);
			}

			for (uint32_t i = from; i < to; i++) {
				m_Data[i] = data[i];
			}
		}

		inline void Append(const std::vector<T>& data) {
			m_Data.insert(m_Data.end(), data.begin(), data.end());
		}

		inline void Append(const Buffer& buffer) {
			Append(buffer.m_Data, buffer.m_Data.size());
		}

		inline void Clear() {
			m_Data.clear();
		}

		inline auto Begin() -> VecIterator { return m_Data.begin(); }
		inline auto End() -> VecIterator { return m_Data.end(); }

		inline bool IsEmpty() { return m_Data.empty(); }
		inline size_t GetSize() { return m_Data.size(); }
		inline size_t GetCapacity() const { return m_Data.capacity(); }
	protected:
		std::vector<T> m_Data;
	};
}
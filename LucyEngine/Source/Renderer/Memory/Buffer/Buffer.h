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

		Buffer& operator=(const Buffer& other) { 
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

		void Reserve(size_t size) {
			m_Data.reserve(size);
		}

		void Resize(size_t size) {
			m_Data.resize(size);
		}

		void SetData(const std::vector<T>& data, size_t from = 0, size_t to = 0) {
			if (to == 0)
				to = data.size();
			
			if (to > data.size() || from + to > m_Data.size()) {
				LUCY_CRITICAL("Index out of bounds");
				LUCY_ASSERT(false);
			}

			size_t indexData = 0;
			for (size_t i = from; i < to + from; i++) {
				m_Data[i] = data[indexData++];
			}
		}

		void SetData(T* data, size_t size) {
			Resize(size);
			memcpy(m_Data.data(), data, size);
		}

		void SetData(const Buffer<T>& other) {
			SetData(other.m_Data);
		}

		inline T* operator&() const { return m_Data.data(); }

		inline void Append(const std::vector<T>& data) {
			m_Data.insert(m_Data.end(), data.begin(), data.end());
		}

		inline void Append(const Buffer& buffer) {
			Append(buffer.m_Data, buffer.m_Data.size());
		}

		inline void Append(T* data, size_t size) {
			InsertPadding(alignof(T));
			m_Data.insert(m_Data.end(), data, data + size);
		}

		inline void AppendMove(T* data, size_t size) {
			InsertPadding(alignof(T));
			m_Data.insert(m_Data.end(), std::make_move_iterator(data), std::make_move_iterator(data + size));
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

	private:
		inline void InsertPadding(size_t alignment) {
			size_t offset = m_Data.size();
			size_t padding = (alignment - (offset % alignment)) % alignment;
			m_Data.insert(m_Data.end(), padding, 0);
		}
	};

	using ByteBuffer = Buffer<uint8_t>;
	using IntBuffer = Buffer<uint32_t>;
	using FloatBuffer = Buffer<float>;
}
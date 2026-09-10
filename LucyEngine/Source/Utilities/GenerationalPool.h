#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <type_traits>

#include "Core/Base.h"

namespace Lucy {

	static inline constexpr uint32_t INVALID_INDEX = 0xFFFFFFFFu;

	template<typename TTag, typename TIndex>
	struct GenerationalHandle;

	template<typename THandle>
	concept IsGenerationalHandle =
		requires(std::remove_cvref_t<THandle> handle) {
		handle.Index;
		handle.Generation;

		typename std::remove_cvref_t<THandle>::Tag;
		typename std::remove_cvref_t<THandle>::IndexType;
	} && std::same_as<std::remove_cvref_t<THandle>, GenerationalHandle<typename std::remove_cvref_t<THandle>::Tag, typename std::remove_cvref_t<THandle>::IndexType>>;

	template<typename TTag, typename TIndex = uint32_t>
	struct GenerationalHandle {
		using Tag = TTag;
		using IndexType = TIndex;

		IndexType Index = INVALID_INDEX;
		uint32_t Generation = 0;

		[[nodiscard]] operator TIndex() const { return Index; }
		[[nodiscard]] operator bool() const { return Index != INVALID_INDEX; }

		auto operator<=>(const GenerationalHandle&) const = default;
	};

	template<IsGenerationalHandle THandle, typename TData>
	class GenerationalPool final {
	public:
		using Handle = THandle;
		using IndexType = Handle::IndexType;
		using Tag = Handle::Tag;
		using Data = TData;

		GenerationalPool() = default;
		~GenerationalPool() = default;

		GenerationalPool(const GenerationalPool&) = delete;
		GenerationalPool& operator=(const GenerationalPool&) = delete;
		GenerationalPool(GenerationalPool&&) noexcept = default;
		GenerationalPool& operator=(GenerationalPool&&) noexcept = default;

	private:
		struct Slot {
			TData Data{};
			uint32_t Generation = 1;
			bool Alive = false;
		};
	public:
		template<typename... TArgs>
		[[nodiscard]] THandle Create(TArgs&&... args) {
			IndexType index;

			if (!m_FreeList.empty()) {
				index = m_FreeList.back();
				m_FreeList.pop_back();
			} else {
				index = static_cast<IndexType>(m_Slots.size());
				m_Slots.emplace_back();
			}

			Slot& slot = m_Slots[index];
			slot.Data = TData{ std::forward<TArgs>(args)... };
			slot.Alive = true;

			return THandle{
				.Index = index,
				.Generation = slot.Generation
			};
		}

		void Destroy(THandle& handle) {
			LUCY_ASSERT(IsValid(handle), "Destroying invalid or stale handle.");

			Slot& slot = m_Slots[handle.Index];

			slot.Data = {};
			slot.Alive = false;
			slot.Generation++;

			//overflow check... generation can theoretically overflow and be 0... we do not want it to be 0
			if (slot.Generation == 0)
				slot.Generation = 1;

			m_FreeList.push_back(handle.Index);
			handle = {};
		}

		[[nodiscard]] bool IsValid(const THandle& handle) const {
			size_t index = static_cast<size_t>(handle.Index);

			if (index >= m_Slots.size())
				return false;

			const Slot& slot = m_Slots[index];
			return slot.Alive && slot.Generation == handle.Generation;
		}
		
		[[nodiscard]] const TData& Get(const THandle& handle) const {
			LUCY_ASSERT(IsValid(handle), "Invalid or stale handle.");
			return m_Slots[handle.Index].Data;
		}

		[[nodiscard]] const TData& GetByIndex(IndexType index) const {
			LUCY_ASSERT(index < m_Slots.size(), "Index out of range.");
			return m_Slots[index].Data;
		}

		[[nodiscard]] const Slot& Back() { return m_Slots.back(); }

		[[nodiscard]] const bool IsEmpty() const { return m_Slots.empty(); }
		
		[[nodiscard]] TData& Get(const THandle& handle) {
			LUCY_ASSERT(IsValid(handle), "Invalid or stale handle.");
			return m_Slots[handle.Index].Data;
		}

		[[nodiscard]] TData& GetByIndex(IndexType index) {
			LUCY_ASSERT(index < m_Slots.size(), "Index out of range.");
			return m_Slots[index].Data;
		}

		[[nodiscard]] size_t GetCapacity() const { return m_Slots.size(); }

		void Reserve(size_t capacity) {
			m_Slots.reserve(capacity);
			m_FreeList.reserve(capacity);
		}

		void Clear() {
			m_Slots.clear();
			m_FreeList.clear();
		}
	public:
		using Iterator = std::vector<Slot>::iterator;
		using CIterator = std::vector<Slot>::const_iterator;

		[[nodiscard]] Iterator begin() { return m_Slots.begin(); }
		[[nodiscard]] Iterator end() { return m_Slots.end(); }

		[[nodiscard]] CIterator begin() const { return m_Slots.cbegin(); }
		[[nodiscard]] CIterator end() const { return m_Slots.cend(); }
	private:
		std::vector<Slot> m_Slots;
		std::vector<IndexType> m_FreeList; // indices of dead slots that can be reused
	};
}

template<typename TTag, typename TIndex>
struct std::hash<Lucy::GenerationalHandle<TTag, TIndex>> {
	size_t operator()(const Lucy::GenerationalHandle<TTag, TIndex>& handle) const noexcept {
		uint64_t value = 0;

		if constexpr (sizeof(TIndex) <= 4) {
			value = (static_cast<uint64_t>(handle.Generation) << 32ull) | static_cast<uint64_t>(handle.Index);
		} else {
			value = static_cast<uint64_t>(handle.Index);
			value ^= static_cast<uint64_t>(handle.Generation) + 0x9e3779b97f4a7c15ull + (value << 6ull) + (value >> 2ull);
		}

		// SplitMix64 finalizer
		// https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64

		value ^= value >> 30ull;
		value *= 0xbf58476d1ce4e5b9ull;
		value ^= value >> 27ull;
		value *= 0x94d049bb133111ebull;
		value ^= value >> 31ull;

		return static_cast<size_t>(value);
	}
};
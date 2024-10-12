#pragma once

#include "Core/Logger.h"

#include <type_traits>
#include <memory>

namespace Lucy {

#ifndef CUSTOM_MEMORY_MANAGING
	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T>
	using Unique = std::unique_ptr<T>;

	class MemoryTrackable : public std::enable_shared_from_this<MemoryTrackable> {
	private:
		template <std::derived_from<MemoryTrackable> TCasted>
		struct AsProxy final {
			Ref<MemoryTrackable> thisObj;

			inline operator Ref<TCasted>() && { return std::dynamic_pointer_cast<TCasted>(thisObj); }
			inline operator Unique<TCasted>() && = delete;
		};
	public:
		virtual ~MemoryTrackable() = default;

		template <std::derived_from<MemoryTrackable> TCasted>
		inline Ref<TCasted> As() {
			return AsProxy<TCasted>{ .thisObj = shared_from_this() };
		}
	protected:
		MemoryTrackable() = default;
	};

	//TODO: Abstract this more
	class Memory final {
	public:
		template <typename TType, typename ... TArgs>
		static Ref<TType> CreateRef(TArgs&& ... args) {
			return std::make_shared<TType>(std::forward<TArgs>(args)...);
		}

		template <typename TType, typename ... TArgs>
		static Unique<TType> CreateUnique(TArgs&& ... args) {
			return std::make_unique<TType>(std::forward<TArgs>(args)...);
		}
	};
#else
	//Lucy wrapper class for std::shared_ptr
	template <typename T>
	class Ref final {
	public:
		using ValueType = T;
		Ref() = default;
		Ref(T* ptr)
			: m_Count(new uint32_t(1)),
			m_Ptr(ptr) {

			if (!m_Ptr || !m_Count) {
				Lucy::Logger::LogCritical("Ref Mem: Allocating template argument failed!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}


			std::enable_shared_from_this < ()
		}

		~Ref() {
			if (m_Count) {
				DecRef();
				if (*m_Count == 0) {
					Release();
				}
			}
		}

		//Nullptr constructor
		Ref(std::nullptr_t)
			: m_Count(nullptr),
			m_Ptr(nullptr) {
		}

		//Nullptr assignment operator
		inline Ref& operator=(std::nullptr_t) {
			DecRef();

			m_Ptr = nullptr;
			m_Count = nullptr;

			return *this;
		}

		//Copy constructor
		Ref(const Ref& other)
			: m_Count(other.m_Count),
			m_Ptr(other.m_Ptr) {
			IncRef();
		}

		//Copy constructor with implicit casting
		template <typename TCasted>
		Ref(const Ref<TCasted>& other)
			: m_Count(other.m_Count),
			m_Ptr(static_cast<T*>(other.m_Ptr)) {
			IncRef();
		}

		//Move constructor
		Ref(Ref&& other) noexcept {
			Move<T>(std::move(other));
		}

		//Move constructor with implicit casting
		template <typename TCasted>
		Ref(Ref<TCasted>&& other) noexcept {
			Move<TCasted>(std::move(other));
		}

		//Copy assignment operator
		inline Ref& operator=(const Ref& other) {
			Copy<T>(other);
			return *this;
		}

		//Copy assignment operator with implicit casting
		template <typename TCasted>
		inline Ref& operator=(const Ref<TCasted>& other) noexcept {
			Copy<TCasted>(other);
			return *this;
		}

		//Move assignment operator
		inline Ref& operator=(Ref&& other) noexcept {
			if (this == &other)
				return *this;
			Move<T>(std::move(other));
			return *this;
		}

		//Move assignment operator with implicit casting
		template <typename TCasted>
		inline Ref& operator=(Ref<TCasted>&& other) noexcept {
			if (this == &other)
				return *this;
			Move<TCasted>(std::move(other));
			return *this;
		}

		inline bool operator==(const Ref& other) { return m_Ptr == other.m_Ptr && m_Count == other.m_Count; }
		inline bool operator!=(const Ref& other) { return !(*this == other); }

		inline T& operator*() { return *m_Ptr; }

		explicit inline operator bool() const { return m_Ptr; }

		//Explicit casting function
		template <typename TCasted>
		Ref<TCasted> As() const {
			if (!m_Ptr) {
				Lucy::Logger::LogCritical("Trying to cast a nullptr!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}

			TCasted* castedPtr = static_cast<TCasted*>(m_Ptr);
			if (!castedPtr) {
				Lucy::Logger::LogCritical("Pointer casting failed!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}

			Ref<TCasted> castedRef(std::move(*this));
			castedRef.m_Ptr = castedPtr;
			return castedRef;
		}

		template <typename TCasted>
		bool InstanceOf() const {
			return typeid(T) == typeid(TCasted);
		}

		inline T* Get() { return m_Ptr; }
		inline T* Get() const { return m_Ptr; }

		inline T* operator->() { return m_Ptr; }
		inline T* operator->() const { return m_Ptr; }

		template <typename ... TArgs>
		inline Ref& operator+=(TArgs... args) {
			ValueType::operator+=(std::forward<TArgs>(args...));
			return *this;
		}

		inline uint32_t GetCount() { return *m_Count; }
		inline uint32_t GetCount() const { return *m_Count; }
	private:
		uint32_t* m_Count = nullptr;
		T* m_Ptr = nullptr;

		template <typename U>
		void Copy(const Ref<U>& other) {
			m_Count = other.m_Count;
			m_Ptr = other.m_Ptr;

			IncRef();
		}

		template <typename U>
		void Move(Ref<U>&& other) {
			m_Ptr = static_cast<T*>(other.m_Ptr);
			m_Count = other.m_Count;

			other.m_Count = nullptr;
			other.m_Ptr = nullptr;

			//or
			//std::swap(m_Ptr, other.m_Ptr)
			//std::swap(m_Count, other.m_Count)
		}

		void IncRef() {
			if (m_Count) {
				(*m_Count)++;
			}
		}

		void DecRef() {
			if (m_Count) {
				(*m_Count)--;
			}
		}

		void Release() {
			delete m_Count;
			delete m_Ptr;

			m_Count = nullptr;
			m_Ptr = nullptr;
		}

		friend class Ref;
	};


	//Lucy wrapper class for std::unique_ptr
	template <typename T>
	class Unique final {
	public:
		using ValueType = T;
		Unique() = default;
		Unique(T* ptr)
			: m_Ptr(ptr) {

			if (!m_Ptr) {
				Lucy::Logger::LogCritical("Ref Mem: Allocating template argument failed!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}
		}

		~Unique() { delete m_Ptr; }

		Unique(const Unique& other) = delete;

		Unique(Unique&& other) noexcept {
			Move(other);
		}

		template <typename TCasted>
		Unique(Unique<TCasted>&& other) noexcept {
			Move<TCasted>(other);
		}

		inline Unique& operator=(std::nullptr_t) {
			m_Ptr = nullptr;
			return *this;
		}

		inline Unique& operator=(const Unique& other) = delete;

		inline Unique& operator=(Unique&& other) noexcept {
			if (this == &other)
				return *this;
			Move(other);
			return *this;
		}

		template <typename TCasted>
		inline Unique& operator=(Unique<TCasted>&& other) noexcept {
			if (this == &other)
				return *this;
			Move<TCasted>(other);
			return *this;
		}

		inline bool operator==(const Unique& other) { return m_Ptr == other.m_Ptr; }
		inline bool operator!=(const Unique& other) { return !(*this == other); }

		inline T& operator*() { return *m_Ptr; }

		explicit inline operator bool() const { return m_Ptr; }

		inline T* Get() { return m_Ptr; }
		inline T* Get() const { return m_Ptr; }

		inline T* operator->() { return m_Ptr; }
		inline T* operator->() const { return m_Ptr; }

		template <typename ... TArgs>
		inline Unique& operator+=(TArgs... args) {
			ValueType::operator+=(std::forward<TArgs>(args...));
			return *this;
		}
	private:
		T* m_Ptr = nullptr;

		template <typename U>
		void Move(Ref<U>&& other) {
			m_Ptr = static_cast<T*>(other.m_Ptr);
			other.m_Ptr = nullptr;

			//or
			//std::swap(m_Ptr, other.m_Ptr)
		}
	};
#endif
}

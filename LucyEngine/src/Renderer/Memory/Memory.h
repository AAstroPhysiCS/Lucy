#pragma once

#include "Core/Logger.h"

namespace Lucy {

	//Lucy wrapper class for std::shared_ptr
	template <typename T>
	class Ref {
	public:
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
			DecRef();
		}

		//Nullptr assignment operator
		inline Ref& operator=(std::nullptr_t) {
			m_Ptr = nullptr;
			m_Count = nullptr;

			DecRef();
			return *this;
		}

		//Copy constructor
		Ref(const Ref& other)
			: m_Count(other.m_Count),
			m_Ptr(other.m_Ptr) {
			IncRef();
		}

		//Copy constructor with implicit casting
		template <typename Casted>
		Ref(const Ref<Casted>& other)
			: m_Count(other.m_Count),
			m_Ptr(static_cast<T*>(other.m_Ptr)) {
			IncRef();
		}

		//Move constructor
		Ref(Ref&& other) noexcept {
			Move<T>(std::move(other));
		}

		//Move constructor with implicit casting
		template <typename Casted>
		Ref(Ref<Casted>&& other) noexcept {
			Move<Casted>(std::move(other));
		}

		//Copy assignment operator
		inline Ref& operator=(const Ref& other) {
			Copy<T>(other);
			return *this;
		}

		//Copy assignment operator with implicit casting
		template <typename Casted>
		inline Ref& operator=(const Ref<Casted>& other) noexcept {
			Copy<Casted>(other);
			return *this;
		}

		//Move assignment operator
		inline Ref& operator=(Ref&& other) noexcept {
			Move<T>(std::move(other));
			return *this;
		}

		//Move assignment operator with implicit casting
		template <typename Casted>
		inline Ref& operator=(Ref<Casted>&& other) noexcept {
			Move<Casted>(std::move(other));
			return *this;
		}

		inline bool operator==(const Ref& other) { return m_Ptr == other.m_Ptr && m_Count == other.m_Count; }
		inline bool operator!=(const Ref& other) { return !(*this == other); }

		explicit inline operator bool() const { return m_Ptr; }

		//Explicit casting function
		template <typename Casted>
		Ref<Casted> As() const {
			if (!m_Ptr) {
				Lucy::Logger::LogCritical("Trying to cast a nullptr!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}

			Casted* castedPtr = static_cast<Casted*>(m_Ptr);
			if (!castedPtr) {
				Lucy::Logger::LogCritical("Pointer casting failed!");
#ifdef LUCY_WINDOWS
				__debugbreak();
#endif
			}

			Ref<Casted> castedRef = nullptr;
			castedRef.m_Count = m_Count;
			castedRef.m_Ptr = castedPtr;
			castedRef.IncRef();
			return castedRef;
		}

		inline T* Get() { return m_Ptr; }
		inline T* Get() const { return m_Ptr; }

		inline T* operator->() { return m_Ptr; }
		inline T* operator->() const { return m_Ptr; }

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
			m_Ptr = other.m_Ptr;
			m_Count = other.m_Count;

			other.m_Count = nullptr;
			other.m_Ptr = nullptr;
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
	class Unique {
	public:
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
		Unique(Unique&& other) = delete;

		inline bool operator==(const Unique& other) { return m_Ptr == other.m_Ptr; }
		inline bool operator!=(const Unique& other) { return !(*this == other); }

		explicit inline operator bool() const { return m_Ptr; }

		template <typename Casted>
		inline Unique<Casted> As() { return Unique<Casted>(dynamic_cast<Casted*>(m_Ptr)); }

		inline Unique& operator=(Unique& other) = delete;
		inline Unique& operator=(Unique&& other) = delete;

		inline T* Get() { return m_Ptr; }
		inline T* Get() const { return m_Ptr; }

		inline T* operator->() { return m_Ptr; }
		inline T* operator->() const { return m_Ptr; }
	private:
		T* m_Ptr = nullptr;
	};

	namespace Memory {

		template <typename T, typename ... Args>
		inline Unique<T> CreateUnique(Args && ... args) {
			T* obj = new T(args...);
			return Unique<T>(obj);
		}

		template <typename T, typename ... Args>
		inline Ref<T> CreateRef(Args&& ... args) {
			T* obj = new T(args...);
			return Ref<T>(obj);
		}
	}
}
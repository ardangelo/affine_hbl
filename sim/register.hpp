#pragma once

#include <array>
#include <variant>

namespace reg
{

template <typename T, size_t address_>
class read_write
{
private:
	static volatile inline T& ref = *reinterpret_cast<T*>(address_);

public:
	constexpr static T* address = address_;

	constexpr static auto& Get() {
		return ref;
	}

	constexpr static void Set(T const& val) {
		ref = val;
	}

	template <typename U>
	constexpr operator U() const {
		return U{ref};
	}

	constexpr auto& operator=(T const& val) const {
		ref = val;
		return *this;
	}

	constexpr auto& operator |= (T const& val) const {
		ref |= val;
		return *this;
	}
};

template <typename T, size_t address_>
class read_only
{
private:
	static volatile inline T& ref = *reinterpret_cast<T*>(address_);

public:
	constexpr static T const* address = address_;

	constexpr static auto& Get() {
		return ref;
	}

	template <typename U>
	constexpr operator U() const {
		return U{ref};
	}
};

template <typename T, size_t address_>
class write_only
{
private:
	static volatile inline T& ref = *reinterpret_cast<T*>(address_);

public:
	constexpr static T* address = address_;

	constexpr static void Set(T const& val) {
		ref = val;
	}

	constexpr auto& operator=(T const& val) const {
		ref = val;
		return *this;
	}
};

} // namespace reg

namespace val
{

template <typename T>
class read_write
{
private:
	static volatile inline T value;

public:
	constexpr static T* address = &value;

	constexpr static auto& Get() {
		return value;
	}

	constexpr static void Set(T const& val) {
		value = val;
	}

	template <typename U>
	constexpr operator U() const {
		return U{value};
	}

	constexpr auto& operator=(T const& val) const {
		value = val;
		return *this;
	}

	constexpr auto& operator |= (T const& val) const {
		value |= val;
		return *this;
	}
};

template <typename T>
class write_only
{
private:
	static volatile inline T value;

public:
	constexpr static T* address = &value;

	constexpr static void Set(T const& val) {
		value = val;
	}

	constexpr auto& operator=(T const& val) const {
		value = val;
		return *this;
	}
};

} // namespace reg

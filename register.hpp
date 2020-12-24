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

template <typename T, typename Base, size_t address_>
class read_write_overlay
{
public:
	constexpr operator Base() const {
		return *(Base const*)address_;
	}

	void operator= (T const& val) const {
		*(Base volatile*)address_ = *(Base const*)&val;
	}

	void operator= (Base const& val) const {
		*(Base volatile*)address_ = val;
	}

	void operator|= (T const& val) const {
		*(Base volatile*)address_ |= *(Base const*)&val;
	}

	T operator& (T const& rhs) const {
		auto result = *(Base volatile*)address_ & *(Base const*)&rhs;
		return *(T*)(&result);
	}

	T volatile* operator -> () const {
		return (T volatile*)address_;
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

template <typename Applier, typename Value>
struct io_val
{
	Value value;

	template <typename T>
	auto& operator= (T const raw) {
		value = raw;
		Applier::apply(value);
		return *this;
	}

	template <typename T>
	auto& operator|= (T const raw) {
		value = value | raw;
		Applier::apply(value);
		return *this;
	}

	auto& Get() {
		return value;
	}

	auto const& Get() const {
		return value;
	}
};
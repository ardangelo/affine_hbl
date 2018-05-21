#ifndef STRONGTYPES_HPP_
#define STRONGTYPES_HPP_

template <typename Parameter>
class NamedInt {
public:
	NamedInt() : value_(0) {}
	NamedInt(int value) : value_(value) {}
	NamedInt(const NamedInt<Parameter>& other) : value_(other.value_) {}
	constexpr int get() { return value_; }
	constexpr operator int() { return value_; }

	NamedInt<Parameter>& operator=(const NamedInt<Parameter>& rhs) {
		value_ = rhs.value_;
		return *this;
	}

	constexpr NamedInt<Parameter> operator+(const NamedInt<Parameter>& rhs) {
		return NamedInt<Parameter>(value_ + rhs.value_);
	}
	NamedInt<Parameter>& operator+=(const NamedInt<Parameter>& rhs) {
		value_ += rhs.value_;
		return *this;
	}

	constexpr NamedInt<Parameter> operator-(const NamedInt<Parameter>& rhs) {
		return NamedInt<Parameter>(value_ - rhs.value_);
	}

	constexpr NamedInt<Parameter> operator*(const NamedInt<Parameter>& rhs) {
		return NamedInt<Parameter>(value_ * rhs.value_);
	}

	constexpr NamedInt<Parameter> operator*(int rhs) {
		return NamedInt<Parameter>(value_ * rhs);
	}

	constexpr NamedInt<Parameter> operator/(const NamedInt<Parameter>& rhs) {
		return NamedInt<Parameter>(value_ / rhs.value_);
	}
private:
	int value_;
};

#endif
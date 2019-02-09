#pragma once

#include <cstdint>
#include <type_traits>
#include <limits>

#include <array>

namespace math {
	namespace {
		struct FromRaw {};
	}

	template <std::size_t N, typename Rep>
		//std::size_t Max = std::numeric_limits<Rep>::max()>
	struct fixed_point {
		Rep x;

		constexpr explicit fixed_point(Rep const x_, FromRaw) : x{x_} {
			static_assert(N <= std::numeric_limits<Rep>::digits, "too many fractional bits for representation type");
		}

		template<class Num, typename = typename std::enable_if<std::is_arithmetic<Num>::value>::type>
		constexpr fixed_point(Num const n_) : fixed_point{static_cast<Rep>(n_ * (1 << N)), FromRaw{}} {}

		constexpr fixed_point() : fixed_point{Rep{}} {}

		static constexpr auto max() {
			return fixed_point{std::numeric_limits<Rep>::max(), FromRaw{}};
		}
		static constexpr auto unit() {
			return fixed_point{0x1, FromRaw{}};
		}

		constexpr auto to_rep() const {
			return x;
		}
		constexpr explicit operator Rep() const {
			return to_rep();
		}

		constexpr explicit operator double() const {
			return static_cast<double>(x) / (2 << (N - 1));
		}

		template <std::size_t M, typename RepP>
		constexpr explicit operator fixed_point<M, RepP>() const {
			if (M > N) {
				return fixed_point<M, RepP>{static_cast<RepP>(x << (M - N)), FromRaw{}};
			} else {
				return fixed_point<M, RepP>{static_cast<RepP>(x >> (N - M)), FromRaw{}};
			}
		}

		constexpr bool operator< (fixed_point const rhs) const {
			return x < rhs.x;
		}

		constexpr bool operator> (fixed_point const rhs) const {
			return x > rhs.x;
		}

		constexpr bool operator== (fixed_point const rhs) const {
			return x == rhs.x;
		}

		constexpr bool operator<= (fixed_point const rhs) const {
			return (x < rhs.x) || (x == rhs.x);
		}

		template <std::size_t M, typename RepP>
		constexpr auto& operator= (fixed_point<M, RepP> const rhs) {
			x = fixed_point{rhs}.x;
			return *this;
		}

		constexpr auto operator- () const {
			return fixed_point{-x, FromRaw{}};
		}

		constexpr auto operator+ (fixed_point const rhs) const {
			return fixed_point{x + rhs.x, FromRaw{}};
		}

		constexpr auto& operator+= (fixed_point const rhs) {
			x += rhs.x;
			return *this;
		}

		constexpr auto operator- (fixed_point const rhs) const {
			return fixed_point{x - rhs.x, FromRaw{}};
		}

		template <std::size_t M>
		constexpr auto operator* (fixed_point<M, Rep> const rhs) const {
			return fixed_point<N + M, Rep>{x * rhs.x, FromRaw{}};
		}

		template <std::size_t M>
		constexpr auto operator/ (fixed_point<M, Rep> const rhs) const {
			if (rhs.x == 0) {
				return fixed_point<N - M, Rep>::max();
			}
			return fixed_point<N - M, Rep>{x / rhs.x, FromRaw{}};
		}
	};

	template <typename T, std::size_t R, std::size_t C>
	struct Matrix {
		using Data = std::array<std::array<T, C>, R>;
		Data data;

		template <typename Fn>
		static constexpr auto unary(Fn const fn, Matrix const& m) {
			using V = decltype(fn(m.data[0][0]));
			Matrix<V, R, C> result{V{}};
			for (std::size_t r = 0; r < R; r++) {
				for (std::size_t c = 0; c < C; c++) {
					result[r][c] = fn(m.data[r][c]);
				}
			}
			return result;
		}

		template <typename U>
		constexpr operator Matrix<U, R, C>() const {
			return unary([](T const& x){ return U{x}; }, *this);
		}

		constexpr auto& operator[] (std::size_t const r) {
			#ifdef DEBUG
			if (r >= R) { throw std::logic_error("row out of bounds"); }
			#endif
			return data[r];
		}

		constexpr auto const& operator[] (std::size_t const r) const {
			#ifdef DEBUG
			if (r >= R) { throw std::logic_error("row out of bounds"); }
			#endif
			return data[r];
		}

		template <typename Fn, typename U>
		static constexpr auto additive(Fn const fn, Matrix const& lhs, Matrix<U, R, C> const& rhs) {
			using V = decltype(fn(lhs.data[0][0], rhs.data[0][0]));
			Matrix<V, R, C> result{V{}};
			for (std::size_t r = 0; r < R; r++) {
				for (std::size_t c = 0; c < C; c++) {
					result[r][c] = fn(lhs.data[r][c], rhs.data[r][c]);
				}
			}
			return result;
		}

		template <typename U>
		constexpr auto operator+ (Matrix<U, R, C> const& rhs) const {
			return additive(std::plus<>(), *this, rhs);
		}
		template <typename U>
		constexpr auto operator+= (Matrix<U, R, C> const& rhs) {
			*this = *this + additive(std::plus<>(), *this, rhs);
			return *this;
		}

		template <typename U>
		constexpr auto operator- (Matrix<U, R, C> const& rhs) const {
			return additive(std::minus<>(), *this, rhs);
		}

		template <typename Fn, typename U, std::size_t Cp>
		static constexpr auto multiplicative(Fn const fn, Matrix const& lhs, Matrix<U, C, Cp> const& rhs) {
			using V = decltype(fn(lhs.data[0][0], rhs.data[0][0]));
			Matrix<V, R, Cp> result{};
			for (std::size_t cp = 0; cp < Cp; cp++) {
				for (std::size_t r = 0; r < R; r++) {
					auto sum = V{};
					for (std::size_t c = 0; c < C; c++) {
						sum = sum + fn(lhs.data[r][c], rhs.data[c][cp]);
					}
					result[r][cp] = sum;
				}
			}
			return result;
		}

		template <typename U, std::size_t Cp>
		constexpr auto operator* (Matrix<U, C, Cp> const& rhs) const {
			return multiplicative(std::multiplies<>(), *this, rhs);
		}
	};
}
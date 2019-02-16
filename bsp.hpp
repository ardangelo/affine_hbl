#pragma once

#include <memory>

struct Vertex {
	int x, y;
};

template <std::size_t N>
constexpr static bool operator== (std::array<char, N> const& lhs, std::array<char, N> const& rhs) {
	for (std::size_t i = 0; i < lhs.size(); i++) {
		if (lhs[i] != rhs[i]) { return false; }
		else if ((lhs[i] == '\0') && (lhs[i] == rhs[i])) { return true; }
	}
	return true;
}

struct Sector {
	std::array<char, 8> name;
	inline constexpr bool operator== (Sector const& rhs) const {
		return name == rhs.name;
	}
};
struct Sidedef {
	std::array<char, 8> color;
	Sector const* sector;
};
struct Linedef {
	Vertex const*  begin;
	Vertex const*  end;
	Sidedef const* right;
	Sidedef const* left = nullptr;
	// linedef texture pegging: https://doomwiki.org/wiki/Texture_alignment
};

#include <array>
#include <type_traits>

namespace {
	template <typename To, typename From, typename = std::enable_if_t<std::is_constructible<To const&, From const&>::value>>
	using All = From;
}

template <typename T, typename... Args>
auto constexpr make_array(All<T, Args> const&... elems) {
	return std::array<T, sizeof...(Args)>{ { elems... } };
}

constexpr auto V = make_array<Vertex>
	( Vertex{ 0,  0} // 0
	, Vertex{ 0, 32} // 1
	, Vertex{64, 32} // 2
	, Vertex{64,  0} // 3
	, Vertex{38, 20} // 4
	, Vertex{32, 16} // 5
	, Vertex{38, 12} // 6
	, Vertex{42, 16} // 7
);

constexpr auto S = make_array<Sector>
	( Sector{"solid"} // 0
);

constexpr auto D = make_array<Sidedef>
	( Sidedef{"A", &S[0]} // 0
	, Sidedef{"B", &S[0]} // 1
	, Sidedef{"C", &S[0]} // 2
	, Sidedef{"D", &S[0]} // 3
	, Sidedef{"E", &S[0]} // 4
	, Sidedef{"F", &S[0]} // 5
	, Sidedef{"G", &S[0]} // 6
	, Sidedef{"H", &S[0]} // 7
);

constexpr auto L = make_array<Linedef>
	( Linedef{&V[0], &V[1], &D[0]} // 0
	, Linedef{&V[1], &V[2], &D[1]} // 1
	, Linedef{&V[2], &V[3], &D[2]} // 2
	, Linedef{&V[3], &V[0], &D[3]} // 3
	, Linedef{&V[4], &V[5], &D[4]} // 4
	, Linedef{&V[5], &V[6], &D[5]} // 5
	, Linedef{&V[6], &V[7], &D[6]} // 6
	, Linedef{&V[7], &V[4], &D[7]} // 7
);

#include <optional>
#include <iterator>
#include <cassert>

namespace {
	template <typename T, std::size_t N, typename Pred>
	constexpr static auto mark_filter(std::array<T, N> const& arr, Pred const& pred) {
		std::array<bool, N> result = {false};
		for (std::size_t i = 0; i < N; i++) {
			if (pred(arr[i])) {
				result[i] = true;
			}
		}
		return result;
	}

	template <typename T, std::size_t N, typename Pred>
	constexpr static auto combine_filter(std::array<T, N> const& arr, std::array<bool, N> const& picked, Pred const& pred) {
		std::array<bool, N> result = {false};
		for (std::size_t i = 0; i < N; i++) {
			if (picked[i] && pred(arr[i])) {
				result[i] = true;
			}
		}
		return result;
	}

	template <typename T, std::size_t N>
	struct filtered {
		struct filtered_iterator {
			std::array<T, N> const& arr;
			std::array<bool, N> const& picked;
			using MaybeIdx = std::optional<std::size_t>;
			MaybeIdx cursor;

			constexpr static auto next_idx(std::array<bool, N> const& picked, MaybeIdx const maybeIdx) {
				if (maybeIdx) {
					auto idx = *maybeIdx + 1;
					for (; idx < N; idx++) {
						if (picked[idx]) { return MaybeIdx{idx}; }
					}
				}
				return MaybeIdx{std::nullopt};
			}

			constexpr filtered_iterator(std::array<T, N> const& arr_,
				std::array<bool, N> const& picked_, std::optional<std::size_t> const cursor_)
				: arr{arr_}, picked{picked_}, cursor{cursor_}
			{}

			constexpr filtered_iterator(std::array<T, N> const& arr_,
				std::array<bool, N> const& picked_)
				: arr{arr_}, picked{picked_}, cursor{picked[0] ? 0 : next_idx(picked_, 0)}
			{}

			struct EndTag {};
			constexpr filtered_iterator(std::array<T, N> const& arr_,
				std::array<bool, N> const& picked_, EndTag)
				: arr{arr_}, picked{picked_}, cursor{std::nullopt}
			{}

			constexpr bool at_end() const { return cursor != std::nullopt; }

			constexpr auto next() const {
				return filtered_iterator{arr, picked, next_idx(picked, cursor)};
			}

			using iterator_category = std::input_iterator_tag;
			using value_type = T const&;
			using difference_type = std::ptrdiff_t;
			using pointer = T const*;
			using reference = T const&;
			constexpr auto operator++(int) { cursor = next_idx(picked, cursor); }
			constexpr auto operator++() { (*this)++; }
			constexpr value_type const& operator*() const { return arr[*cursor]; }
			constexpr pointer operator->() const { return &arr[*cursor]; }
			constexpr bool operator== (filtered_iterator const& rhs) const {
				return (&arr == &rhs.arr)
					&& (&picked == &rhs.picked)
					&& (cursor == rhs.cursor);
			}
			constexpr bool operator!= (filtered_iterator const& rhs) const {
				return !(*this == rhs);
			}
		};

		std::array<T, N> const& arr;
		std::array<bool, N> const& picked;

		struct AlreadyPicked {}; // tag dispatch needed to allow Pred template
		constexpr filtered(std::array<T, N> const& arr_, std::array<bool, N> const& picked_, AlreadyPicked)
			: arr{arr_}, picked{picked_}
		{}

		template <typename Pred>
		constexpr filtered(std::array<T, N> const& arr_, Pred&& pred)
			: arr{arr_}, picked{mark_filter(arr_, pred)}
		{}

		template <typename Pred>
		constexpr filtered(filtered const& filt, Pred&& pred)
			: arr{filt.arr}, picked{combine_filter(filt.arr, filt.picked, pred)}
		{}

		constexpr filtered operator& (filtered const& rhs) const {
			assert(&arr == &rhs.arr);
			std::array<bool, N> result_picked = {false};
			for (std::size_t i = 0; i < N; i++) {
				result_picked[i] = picked[i] && rhs.picked[i];
			}
			return filtered{arr, result_picked, AlreadyPicked{}};
		}

		constexpr auto begin() const {
			return filtered_iterator{arr, picked};
		}
		constexpr auto end() const {
			using EndTag = typename filtered_iterator::EndTag; // apparently filtered_iterator is in a dependent scope?
			return filtered_iterator{arr, picked, EndTag{}};
		}
	};
}

#if 0
template <typename T, std::size_t N>
using filtered_iterator = typename filtered<T, N>::filtered_iterator;

template <typename T, std::size_t N, std::size_t ResultSize>
constexpr auto set_filter_helper(filtered_iterator<T, N> const iter, std::array<T const*, ResultSize> const result) {
	if (!iter.at_end()) {
		return set_filter_helper<T, N, ResultSize + 1>(iter.next(), append(result, &(*iter)));
	} else {
		return append(result, (Sidedef const*)nullptr);
	}
}

template <typename T, std::size_t N>
constexpr auto set_filter(filtered<T, N> const& filt) {
	auto const iter = filt.begin();
	std::array<T const*, 1> result = { &(*iter) };
	return set_filter_helper<T, N, 1>(iter, result);
}
#endif

template <typename T, std::size_t N, std::size_t M>
constexpr auto operator+ (std::array<T, N> const& lhs, std::array<T, M> const& rhs) {
	std::array<T, N + M> result = {};
	for (std::size_t i = 0; i < N; i++) { result[i]     = lhs[i]; }
	for (std::size_t i = 0; i < M; i++) { result[N + i] = rhs[i]; }
	return result;
}

template <typename T, std::size_t N>
constexpr auto operator+ (std::array<T, N> const& arr, T const& with) {
	return arr + std::array<T, 1>{with};
}

constexpr auto sidedefs_in(Sector const& sector) {
	return filtered(D, [&sector](Sidedef const& d) {
		return *(d.sector) == sector;
	});
}

constexpr auto const& linedef_of(Sidedef const& sidedef) {
	for (auto const& linedef : L) {
		if ((linedef.left == &sidedef) || (linedef.right == &sidedef)) {
			return linedef;
		}
	}
}

using MaybeVertex = std::optional<Vertex>;
constexpr auto sidedef_intersection(Linedef const& ray, Sidedef const& with) {
	auto const slope = [](Linedef const& linedef) constexpr {
		if (auto const denom = double{linedef.end->x - linedef.begin->x}) {
			return std::optional<double>{double{linedef.end->y - linedef.begin->y} / denom };
		} else {
			return std::optional<double>{std::nullopt};
		}
	};
	auto const y_intercept = [](Linedef const& linedef, double const slope) constexpr {
		return double{linedef.begin->y} - slope * double{linedef.begin->x};
	};

	auto const line_intersection = [&](Linedef const& ray, Linedef const& with_linedef) constexpr {
		// y=ax+c, y=bx+d: { (d - c) / (a - b) , (ad - bc) / (a - b) }
		auto const maybe_a = slope(ray);
		auto const maybe_b = slope(with_linedef);

		if (maybe_a && maybe_b) {
			auto const a = *maybe_a;
			auto const c = y_intercept(ray, a);
			auto const b = *maybe_b;
			auto const d = y_intercept(with_linedef, b);

			if (auto const denom = a - b) {
				return MaybeVertex{Vertex
					{ .x = (d - c) / (a - b)
					, .y = (a*d - b*c) / (a - b)
				}};
			} else {
				return MaybeVertex{std::nullopt};
			}
		} else if (maybe_a && !maybe_b) {
			auto const a = *maybe_a;
			auto const c = y_intercept(ray, a);
			auto const x = with_linedef.begin->x;
			return MaybeVertex{Vertex
				{ .x = x
				, .y = a * x + c
			}};
		} else if (!maybe_a && maybe_b) {
			auto const b = *maybe_b;
			auto const d = y_intercept(with_linedef, b);
			auto const x = ray.begin->x;
			return MaybeVertex{Vertex
				{ .x = x
				, .y = b * x + d
			}};
		} else {
			return MaybeVertex{std::nullopt};
		}
	};

	auto const vert_lies_on = [&](Linedef const& linedef, Vertex const& to) {
		if (auto const maybe_slope = slope(linedef)) {
			return false;
		}

		return false;
	};

	auto const intersects_left_side = [&](Linedef const& linedef, Vertex const& to) {
		return false;
	};

	auto const& with_linedef = linedef_of(with);
	if (auto const intersection_point = line_intersection(ray, with_linedef)) {
		if (vert_lies_on(with_linedef, *intersection_point)) {
			return intersection_point;
		}
	}

	return MaybeVertex{std::nullopt};
};

constexpr auto sidedefs_intersecting_with(Sector const& sector, Linedef const& linedef) {
	return filtered(sidedefs_in(sector), [&linedef](Sidedef const& sidedef) {
		return bool{sidedef_intersection(linedef, sidedef)};
	});
}

template <typename Filtered>
constexpr auto count(Filtered const& filt) {
	std::size_t count = 0;
	for (auto const& elem : filt) {
		count++;
	}
	return count;
}

constexpr auto split_on(Linedef const& linedef) {

}

constexpr auto s0 = count(sidedefs_intersecting_with(S[0], L[7]));

#pragma once

#include <variant>

template <typename T, size_t N>
struct circular_buffer {
	T buf[N];
	size_t end;

	constexpr circular_buffer()
		: buf{}
		, end{0}
	{}

	constexpr void push_back(T val) {
		buf[end] = val;
		end = (end + 1) % N;
	}

	constexpr T& at(size_t idx) {
		return buf[idx % N];
	}
};

template <typename T, size_t N>
struct circular_queue {
	T buf[N];
	size_t begin, end;

	constexpr circular_queue() : begin{0}, end{0} {}

	constexpr auto empty() const { return begin == end; }

	constexpr void push_back(T val) {
		buf[end] = val;
		end = (end + 1) % N;
	}

	constexpr T pop_front() {
		auto const result = buf[begin];
		begin = (begin + 1) % N;
		return result;
	}
};

namespace event {
	struct Key {
		enum class Type : uint32_t
			{ Pause = 0
			, Left
			, Right
		};
		enum class State : bool
			{ Down = 1
			, Up = 0
		};

		Type type;
		State state;
	};

	using event_type = std::variant<Key>;
	using queue_type = circular_queue<event_type, 16>;

	queue_type queue;
};

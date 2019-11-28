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

	constexpr bool empty() const { return end == begin; }
	constexpr bool full()  const { return (end - begin) == N; }

	constexpr void push_back(T val) {
		buf[end % N] = val;
		end++;
	}

	constexpr T pop_front() {
		auto const result = buf[begin % N];
		begin++;
		return result;
	}
};

namespace event {
	struct Key {
		enum Type : uint32_t
			{ Pause = 0
			, Right
			, Left
			, Up
			, Down
		};
		enum State : bool
			{ On  = 1
			, Off = 0
		};

		Type type;
		State state;
	};

	using type = std::variant<Key>;
	using queue_type = circular_queue<type, 16>;

	queue_type queue;
};

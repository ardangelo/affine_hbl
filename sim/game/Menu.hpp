#pragma once

#include "World.hpp"

struct Menu
{
private: // members
	bool active;

public: // interface
	constexpr Menu()
		: active{false}
	{}

public: // responder
	bool Respond(event::Key key);

	template <typename Any>
	bool Respond(Any&&) { return false; }

public: // ticker
	void Tick();
};

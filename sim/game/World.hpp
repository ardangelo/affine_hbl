#pragma once

#include "math.hpp"
#include "event.hpp"

namespace
{
	using coord_fp = math::fixed_point<4, uint32_t>;
}

struct World
{
public: // types
	struct Cmd
	{
		coord_fp da_x, da_y;
	};

	enum class Mode : uint32_t
	{
		InGame
	};

	struct Camera
	{
	public: // members
		coord_fp a_x, a_y;

	public: // interface
		constexpr Camera()
			: a_x{0}
			, a_y{0}
		{}

		void Think(Cmd const& cmd);

	public: // ticker
		void Tick(Cmd const& cmd);
	};

private: // members
	bool keydown[5];
	Mode mode;

public: // tickers
	Camera camera;

public: // interface
	constexpr World()
		: keydown{}
		, mode{Mode::InGame}
		, camera{}
	{}

	Cmd BuildCmd() const;

public: // responder
	bool Respond(event::Key key);

	template <typename Any>
	bool Respond(Any&&) { return false; }

public: // ticker
	void Tick(Cmd const& cmd);
};

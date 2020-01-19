#pragma once

#include "math.hpp"
#include "event.hpp"

#include "BSP.hpp"

namespace
{
	using coord_fp = math::fixed_point<4, uint32_t>;
}

class World
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
	bool m_keydown[5];
	Mode m_mode;

public: // tickers
	Camera m_camera;

public: // interface
	constexpr World()
		: m_keydown{}
		, m_mode{Mode::InGame}
		, m_camera{}
	{}

	Cmd BuildCmd() const;

public: // responder
	bool Respond(event::Key key);

	template <typename Any>
	bool Respond(Any&&) { return false; }

public: // ticker
	void Tick(Cmd const& cmd);
};

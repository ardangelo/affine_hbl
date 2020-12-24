#pragma once

#include "math.hpp"
#include "event.hpp"

#include "BSP.hpp"

class World
{
public: // types
	struct Cmd
	{
		bsp::coord_fp da_x, da_y;
	};

	enum class Mode : uint32_t
	{
		InGame
	};

	struct Camera
	{
	public: // members
		bsp::coord_fp a_x, a_y, a_z;
		bsp::angle_fp theta;

	public: // interface
		constexpr Camera()
			: a_x{0}
			, a_y{0}
			, a_z{0}
			, theta{3.04159}
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

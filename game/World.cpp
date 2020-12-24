#include "World.hpp"

// struct Camera

void World::Camera::Think(Cmd const& cmd)
{
	a_x += cmd.da_x;
	a_y += cmd.da_y;
}

void World::Camera::Tick(Cmd const& cmd)
{
	Think(cmd);
}

// struct World

World::Cmd World::BuildCmd() const
{
	auto constexpr speed = bsp::coord_fp{0x1};

	auto cmd = Cmd{};

	using Ty = event::Key::Type;
	if (m_keydown[Ty::Right]) {
		cmd.da_x += speed;
	}
	if (m_keydown[Ty::Left]) {
		cmd.da_x -= speed;
	}
	if (m_keydown[Ty::Up]) {
		cmd.da_y -= speed;
	}
	if (m_keydown[Ty::Down]) {
		cmd.da_y += speed;
	}

	return cmd;
}

bool World::Respond(event::Key key)
{
	using Ty = event::Key::Type;
	using St = event::Key::State;

	switch (key.state) {

	case St::On:
		switch (key.type) {
			case Ty::Pause:
				return true;

			default:
				m_keydown[key.type] = true;
				return true;
		}

	case St::Off:
		m_keydown[key.type] = false;
		return false;

	}
}

void World::Tick(World::Cmd const& cmd)
{
	if (m_mode == Mode::InGame) {
		m_camera.Tick(cmd);
	}
}

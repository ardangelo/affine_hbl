#pragma once

#include <memory>

#include <SDL.h>

#include "PC.hpp"

class SDL : public PC<SDL>
{
public: // types

	struct LibSDLState
	{
		using unique_SDL_Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
		unique_SDL_Renderer renderer{nullptr, SDL_DestroyRenderer};

		using unique_SDL_Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
		unique_SDL_Window window{nullptr, SDL_DestroyWindow};

		LibSDLState()
		{
			SDL_Init(SDL_INIT_VIDEO);

			{ SDL_Renderer *rawRenderer = nullptr;
			  SDL_Window   *rawWindow   = nullptr;

				SDL_CreateWindowAndRenderer(PC::screenWidth, PC::screenHeight, 0, &rawWindow, &rawRenderer);
				renderer = unique_SDL_Renderer{rawRenderer, SDL_DestroyRenderer};
				window   = unique_SDL_Window{rawWindow, SDL_DestroyWindow};
			}

			SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
			SDL_RenderClear(renderer.get());
		}

		~LibSDLState()
		{
			SDL_Quit();
		}
	};

public: // members
	static inline auto libSDLState = LibSDLState{};

public: // DrawImpl interface

	static void waitNextFrame()
	{}

	static void pumpEvents(event::queue_type& queue)
	{
		SDL_Event event;

		if (!SDL_PollEvent(&event)) {
			return;
		}
		if (event.type == SDL_QUIT) {
			exit(0);
		}

		queue.push_back(event::Key
			{ .type  = event::Key::Type::Right
			, .state = event::Key::State::On
		});
		queue.push_back(event::Key
			{ .type  = event::Key::Type::Down
			, .state = event::Key::State::On
		});
	}

	static void drawPx(int colPx, int rowPx, int r, int g, int b)
	{
		SDL_SetRenderDrawColor(libSDLState.renderer.get(), 8 * r, 8 * g, 8 * b, 0xff);
		SDL_RenderDrawPoint(libSDLState.renderer.get(), colPx, rowPx);
	}

	static void updateDisplay()
	{
		SDL_RenderPresent(libSDLState.renderer.get());
	}

};
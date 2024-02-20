#include "Controller.hpp"

Controller::Controller(Game &_game) : m_game(&_game) {
}

Controller::~Controller() {
}

void Controller::update() {
	SDL_Event event;

	// Mouse input
	Uint32 mouseState = SDL_GetMouseState(&m_game->m_mousex, &m_game->m_mousey);
	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		std::cout << m_game->m_mousex << ", " << m_game->m_mousey << std::endl;
	}

	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type) {
		case SDL_KEYDOWN:
			keyDown(&event.key);
			break;
		case SDL_KEYUP:
			keyUp(&event.key);
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
				m_view->m_stopRendering = true;
			if (event.window.event == SDL_WINDOWEVENT_RESTORED)
				m_view->m_stopRendering = false;
			break;
		case SDL_QUIT:
			exit(0);
			break;
		}
	}
}

void Controller::keyDown(SDL_KeyboardEvent *_event) {
	if (_event->repeat == 0) {
		if (_event->keysym.scancode == SDL_SCANCODE_W) {
			m_game->m_px += m_game->m_pdx;
			m_game->m_py += m_game->m_pdy;
		}

		if (_event->keysym.scancode == SDL_SCANCODE_S) {
			m_game->m_px -= m_game->m_pdx;
			m_game->m_py -= m_game->m_pdy;
		}

		if (_event->keysym.scancode == SDL_SCANCODE_A) {
			m_game->m_pa -= 0.1f;
			if (m_game->m_pa < 0.0f) {
				m_game->m_pa += 2 * PI;
			}
			m_game->m_pdx = glm::cos(m_game->m_pa) * 5;
			m_game->m_pdy = glm::sin(m_game->m_pa) * 5;
		}

		if (_event->keysym.scancode == SDL_SCANCODE_D) {
			m_game->m_pa += 0.1f;
			if (m_game->m_pa > 2 * PI) {
				m_game->m_pa -= 2 * PI;
			}
			m_game->m_pdx = glm::cos(m_game->m_pa) * 5;
			m_game->m_pdy = glm::sin(m_game->m_pa) * 5;
		}
	}
}

void Controller::keyUp(SDL_KeyboardEvent *_event) {
	if (_event->repeat == 0) {
		if (_event->keysym.scancode == SDL_SCANCODE_UP) {
			std::cout << "up released" << std::endl;
		}
	}
}

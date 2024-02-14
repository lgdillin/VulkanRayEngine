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
		switch (event.type) {
		case SDL_KEYDOWN:
			keyDown(&event.key);
			break;
		case SDL_KEYUP:
			keyUp(&event.key);
			break;
		case SDL_QUIT:
			exit(0);
			break;
		}
	}
}

void Controller::keyDown(SDL_KeyboardEvent *_event) {
	if (_event->repeat == 0) {
		if (_event->keysym.scancode == SDL_SCANCODE_UP) {
			std::cout << "Up pressed" << std::endl;
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

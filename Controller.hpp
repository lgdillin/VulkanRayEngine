#pragma once

#include <SDL2/SDL.h>

#include "Game.hpp"

class Controller {
public:
	Controller(Game &_game);
	~Controller();

	void initialize() {}

	void update();

	//void mouseDown(SDL_)
	void keyDown(SDL_KeyboardEvent *_event);
	void keyUp(SDL_KeyboardEvent *_event);
private:

	Game *m_game;

};
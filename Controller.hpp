#pragma once

#include <glm/glm.hpp>

#include "View.hpp"
#include "Game.hpp"

constexpr float PI = 3.1415926;

class Controller {
public:
	Controller(Game &_game);
	~Controller();
	void setView(View &_view) { m_view = &_view; }

	void initialize() {}

	void update();

	//void mouseDown(SDL_)
	void keyDown(SDL_KeyboardEvent *_event);
	void keyUp(SDL_KeyboardEvent *_event);
private:

	Game *m_game;
	View *m_view;
};
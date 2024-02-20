#include "Game.hpp"

Game::Game() {
	m_px = 400.0f;
	m_py = 300.0f;
	m_pa = 0.0f;
	m_mousex = 0.0f;
	m_mousey = 0.0f;

	m_triangles = std::vector<Triangle>();
}

Game::~Game() {
}

void Game::initialize() {
	//Triangle t(*m_device);
	//m_triangles.push_back(t);
}

void Game::update() {
	// update the data model
}

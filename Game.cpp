#include "Game.hpp"

Game::Game() {
	m_triangles = std::vector<Triangle>();
}

Game::~Game() {
}

void Game::initialize() {
	Triangle t(*m_device);
	m_triangles.push_back(t);
}

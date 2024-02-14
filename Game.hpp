#pragma once

#include <vector>

#include "Triangle.hpp"

class Game {
public:
	Game();
	~Game();

	void initialize();

	void update();

	void setDevice(VkDevice &_device) { m_device = &_device; }

	VkDevice *m_device;
	std::vector<Triangle> m_triangles;

	int m_mousex;
	int m_mousey;
private:
};
#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Triangle.hpp"

constexpr int m_map[] = {
		1,1,1,1,1,1,1,1,
		1,0,1,0,0,0,0,1,
		1,0,1,0,0,0,0,1,
		1,0,1,0,0,0,0,1,
		1,0,0,0,0,0,0,1,
		1,0,0,0,0,1,0,1,
		1,0,0,0,0,0,0,1,
		1,1,1,1,1,1,1,1,
};

class Game {
public:
	Game();
	~Game();

	void initialize();

	void update();

	//void setDevice(VkDevice &_device) { m_device = &_device; }

	std::vector<Triangle> m_triangles;

	int m_mousex;
	int m_mousey;
	float m_px;
	float m_py;
	float m_pa;
	float m_pdx;
	float m_pdy;
private:
};
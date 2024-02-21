#pragma once

#include <iostream>
#include <vector>
#include <fstream>

namespace vre {
	class Pipeline {
	public:
		Pipeline() { }
		
		void createGraphicsPipeline();

		static std::vector<char> readFile(const std::string &_file);

	};
}
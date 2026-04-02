#ifndef MENUEPAGE_HPP
# define MENUEPAGE_HPP

#include "interface.hpp"
#include <unordered_map>

class MenuePage {
	private:
		std::unordered_map<std::string, sf::Text> text_map;
		std::unordered_map<std::string, sf::RectangleShape> rectangle_map;
		std::unordered_map<std::string, Item> item_map;

}


#endif

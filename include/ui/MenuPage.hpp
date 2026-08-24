#ifndef MENUPAGE_HPP
#define MENUPAGE_HPP

#include "interface.hpp"
#include <unordered_map>
#include <functional>

class MenuPage
{
private:
	std::unordered_map<std::string, sf::Text>           _textMap;
	std::unordered_map<std::string, sf::RectangleShape> _rectMap;
	std::unordered_map<std::string, FonctionItem>       _itemMap;
	std::function<void(MenuPage&, sf::RenderWindow&)>   _drawFunc;

public:
	MenuPage()  = default;
	~MenuPage() = default;

	void addText(const std::string& key, const sf::Text& text);
	void addRectangle(const std::string& key, const sf::RectangleShape& rect);
	void addItem(const std::string& key, const FonctionItem item);

	void removeText(const std::string& key);
	void removeRectangle(const std::string& key);
	void removeItem(const std::string& key);

	sf::Text*           getText(const std::string& key);
	sf::RectangleShape* getRectangle(const std::string& key);
	FonctionItem*       getItem(const std::string& key);

	bool hasText(const std::string& key) const;
	bool hasRectangle(const std::string& key) const;
	bool hasItem(const std::string& key) const;

	void clear();
	void updateHover(sf::Vector2f mouse);
	void handleClick(sf::Vector2f mouse);
	void setDrawFunction(std::function<void(MenuPage&, sf::RenderWindow&)> func);
	void draw(sf::RenderWindow& window);
};

#endif

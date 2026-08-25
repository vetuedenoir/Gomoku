#include "ui/MenuPage.hpp"

void MenuPage::addText(const std::string& key, const sf::Text& text)
{
	_textMap[key] = text;
}

void MenuPage::addRectangle(const std::string& key, const sf::RectangleShape& rect)
{
	_rectMap[key] = rect;
}

void MenuPage::addItem(const std::string& key, const FonctionItem item)
{
	_itemMap.emplace(key, item);
}

void MenuPage::removeText(const std::string& key)
{
	_textMap.erase(key);
}
void MenuPage::removeRectangle(const std::string& key)
{
	_rectMap.erase(key);
}
void MenuPage::removeItem(const std::string& key)
{
	_itemMap.erase(key);
}

sf::Text* MenuPage::getText(const std::string& key)
{
	auto it = _textMap.find(key);
	return (it != _textMap.end()) ? &it->second : nullptr;
}

sf::RectangleShape* MenuPage::getRectangle(const std::string& key)
{
	auto it = _rectMap.find(key);
	return (it != _rectMap.end()) ? &it->second : nullptr;
}

FonctionItem* MenuPage::getItem(const std::string& key)
{
	auto it = _itemMap.find(key);
	return (it != _itemMap.end()) ? &it->second : nullptr;
}

bool MenuPage::hasText(const std::string& key) const
{
	return _textMap.find(key) != _textMap.end();
}
bool MenuPage::hasRectangle(const std::string& key) const
{
	return _rectMap.find(key) != _rectMap.end();
}
bool MenuPage::hasItem(const std::string& key) const
{
	return _itemMap.find(key) != _itemMap.end();
}

void MenuPage::clear()
{
	_textMap.clear();
	_rectMap.clear();
	_itemMap.clear();
}

void MenuPage::updateHover(sf::Vector2f mouse)
{
	for (auto& [key, fi] : _itemMap)
		fi.item.setHovered(fi.item.contains(mouse));
}

void MenuPage::handleClick(sf::Vector2f mouse)
{
	for (auto& [key, fi] : _itemMap)
	{
		if (fi.item.contains(mouse) && fi.onclick)
		{
			fi.onclick();
			return;
		}
	}
}

void MenuPage::setDrawFunction(std::function<void(MenuPage&, sf::RenderWindow&)> func)
{
	_drawFunc = func;
}

void MenuPage::draw(sf::RenderWindow& window)
{
	if (_drawFunc)
		_drawFunc(*this, window);
}

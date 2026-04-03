#ifndef INTERFACE_HPP
# define INTERFACE_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional> // pour std::function

#include "MenuePage.hpp"


constexpr unsigned int WIN_W = 800;
constexpr unsigned int WIN_H = 600;

// Colors
inline const sf::Color BG        (18,  18,  32);
inline const sf::Color GOLD      (220, 175,  80);
inline const sf::Color WHITE     (230, 230, 230);
inline const sf::Color DIM       (100, 100, 120);

// ── helpers ─────────────────────────────────────────────────────────────────

void centerOrigin(sf::Text &t);

sf::Text makeText(const std::string &str, const sf::Font &font,
						  unsigned int size, sf::Color color);

void drawGrid(sf::RenderWindow &window);

// ── menu item ─────────────────────────────────────────────────────────────────

struct Item
{
	sf::Text        label;
	sf::RectangleShape bg;

	Item(const std::string &str, const sf::Font &font, float x, float y)
		: label(makeText(str, font, 38, WHITE))
	{
		label.setPosition(x, y);

		bg.setSize(sf::Vector2f(260.f, 58.f));
		bg.setOrigin(130.f, 29.f);
		bg.setPosition(x, y);
		bg.setFillColor(sf::Color::Transparent);
		bg.setOutlineThickness(2.f);
		bg.setOutlineColor(DIM);
	}

	bool contains(sf::Vector2f pt) const
	{
		return bg.getGlobalBounds().contains(pt);
	}

	void setHovered(bool hovered)
	{
		label.setFillColor(hovered ? GOLD : WHITE);
		bg.setOutlineColor(hovered ? GOLD : DIM);
	}

	void draw(sf::RenderWindow &w) const
	{
		w.draw(bg);
		w.draw(label);
	}
};

struct	FonctionItem
{
	struct Item	item;
	std::function<void()>	onclick;

	FonctionItem(const Item& i, std::function<void()> f)
        : item(i), onclick(f) {}
};



#endif
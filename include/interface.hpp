#ifndef INTERFACE_HPP
#define INTERFACE_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

constexpr unsigned int WIN_W = 1000;
constexpr unsigned int WIN_H = 800;

constexpr float CX = WIN_W / 2.f;

constexpr unsigned FONT_XL   = static_cast<unsigned>(WIN_H * 0.100f); //  ~80
constexpr unsigned FONT_LG   = static_cast<unsigned>(WIN_H * 0.075f); //  ~60
constexpr unsigned FONT_ML   = static_cast<unsigned>(WIN_H * 0.069f); //  ~55
constexpr unsigned FONT_ITEM = static_cast<unsigned>(WIN_H * 0.048f); //  ~38 (menu item labels)
constexpr unsigned FONT_CC   = static_cast<unsigned>(WIN_H * 0.028f); //  ~22 (color-choice title)
constexpr unsigned FONT_MD   = static_cast<unsigned>(WIN_H * 0.025f); //  ~20
constexpr unsigned FONT_SM   = static_cast<unsigned>(WIN_H * 0.023f); //  ~18
constexpr unsigned FONT_XS   = static_cast<unsigned>(WIN_H * 0.020f); //  ~16

// Colors
inline const sf::Color BG(18, 18, 32);
inline const sf::Color GOLD(220, 175, 80);
inline const sf::Color WHITE(230, 230, 230);
inline const sf::Color DIM(100, 100, 120);

// ── helpers ─────────────────────────────────────────────────────────────────

void centerOrigin(sf::Text& t);

sf::Text makeText(const std::string& str, const sf::Font& font, unsigned int size, sf::Color color);

void drawGrid(sf::RenderWindow& window);

// ── menu item ─────────────────────────────────────────────────────────────────

struct Item
{
	sf::Text           label;
	sf::RectangleShape bg;

	Item(const std::string& str, const sf::Font& font, float x, float y) : label(makeText(str, font, FONT_ITEM, WHITE))
	{
		label.setPosition(x, y);

		constexpr float W = WIN_W * 0.26f;
		constexpr float H = WIN_H * 0.0725f;
		bg.setSize(sf::Vector2f(W, H));
		bg.setOrigin(W / 2.f, H / 2.f);
		bg.setPosition(x, y);
		bg.setFillColor(sf::Color::Transparent);
		bg.setOutlineThickness(2.f);
		bg.setOutlineColor(DIM);
	}

	bool contains(sf::Vector2f pt) const { return bg.getGlobalBounds().contains(pt); }

	void setHovered(bool hovered)
	{
		label.setFillColor(hovered ? GOLD : WHITE);
		bg.setOutlineColor(hovered ? GOLD : DIM);
	}

	void draw(sf::RenderWindow& w) const
	{
		w.draw(bg);
		w.draw(label);
	}
};

struct FonctionItem
{
	struct Item           item;
	std::function<void()> onclick;

	FonctionItem(const Item& i, std::function<void()> f) : item(i), onclick(f) {}
};

#endif
#ifndef STATUS_BANNER_HPP
#define STATUS_BANNER_HPP

#include <SFML/Graphics.hpp>
#include <string>

class StatusBanner
{
public:
	void setPersistent(std::string text, sf::Color color);
	void flash(std::string text, sf::Color color, float seconds);
	void clear();

	bool blocking() const;
	void draw(sf::RenderWindow& w, const sf::Font& font) const;

private:
	bool flashActive() const;

	std::string _persistent;
	sf::Color   _persistentColor{ 100, 100, 120 };

	std::string _flashText;
	sf::Color   _flashColor{ 220, 175, 80 };
	sf::Clock   _flashClock;
	float       _flashSeconds = 0.f;
	bool        _flashing     = false;
};

#endif

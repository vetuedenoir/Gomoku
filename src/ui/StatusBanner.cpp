#include "ui/StatusBanner.hpp"
#include "interface.hpp"

void StatusBanner::setPersistent(std::string text, sf::Color color)
{
	_persistent      = std::move(text);
	_persistentColor = color;
}

void StatusBanner::flash(std::string text, sf::Color color, float seconds)
{
	_flashText    = std::move(text);
	_flashColor   = color;
	_flashSeconds = seconds;
	_flashing     = true;
	_flashClock.restart();
}

void StatusBanner::clear()
{
	_persistent.clear();
	_flashText.clear();
	_flashing = false;
}

bool StatusBanner::flashActive() const
{
	return _flashing && _flashClock.getElapsedTime().asSeconds() < _flashSeconds;
}

bool StatusBanner::blocking() const
{
	return flashActive();
}

void StatusBanner::draw(sf::RenderWindow& w, const sf::Font& font) const
{
	const bool         flashing = flashActive();
	const std::string& text     = flashing ? _flashText : _persistent;
	if (text.empty())
		return;

	sf::Text tip = makeText(text, font, flashing ? FONT_SM : FONT_XS, flashing ? _flashColor : _persistentColor);
	if (flashing)
		tip.setStyle(sf::Text::Bold);
	tip.setPosition(CX, WIN_H * 0.975f);
	w.draw(tip);
}

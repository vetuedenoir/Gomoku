#ifndef SFML_COMPAT_HPP
#define SFML_COMPAT_HPP

// Petite couche de compatibilité SFML 2.x / SFML 3.x.
// Le projet doit compiler avec les deux (SFML 3 sur Linux, SFML 2 sur certains macOS).

#include <SFML/Graphics.hpp>
#include <string>

#if SFML_VERSION_MAJOR >= 3
#define GOMOKU_SFML3 1
#else
#define GOMOKU_SFML3 0
#endif

namespace sfc
{
// sf::Rect : left/top/width/height (SFML 2) <-> position/size (SFML 3)
inline float left(const sf::FloatRect& r)
{
#if GOMOKU_SFML3
	return r.position.x;
#else
	return r.left;
#endif
}

inline float top(const sf::FloatRect& r)
{
#if GOMOKU_SFML3
	return r.position.y;
#else
	return r.top;
#endif
}

inline float width(const sf::FloatRect& r)
{
#if GOMOKU_SFML3
	return r.size.x;
#else
	return r.width;
#endif
}

inline float height(const sf::FloatRect& r)
{
#if GOMOKU_SFML3
	return r.size.y;
#else
	return r.height;
#endif
}

// sf::Text : Text(str, font, size) (SFML 2) <-> Text(font, str, size) (SFML 3)
inline sf::Text newText(const std::string& str, const sf::Font& font, unsigned int size)
{
#if GOMOKU_SFML3
	return sf::Text(font, str, size);
#else
	return sf::Text(str, font, size);
#endif
}
} // namespace sfc

#endif

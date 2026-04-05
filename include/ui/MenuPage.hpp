#ifndef MENUPAGE_HPP
# define MENUPAGE_HPP

#include "interface.hpp"
#include <unordered_map>
#include <functional>

class MenuPage
{
public:
    MenuPage() = default;
    ~MenuPage() = default;

    void addText     (const std::string &key, const sf::Text &text);
    void addRectangle(const std::string &key, const sf::RectangleShape &rect);
    void addItem     (const std::string &key, const FonctionItem item);

    void removeText     (const std::string &key);
    void removeRectangle(const std::string &key);
    void removeItem     (const std::string &key);

    sf::Text           *getText     (const std::string &key);
    sf::RectangleShape *getRectangle(const std::string &key);
    FonctionItem       *getItem     (const std::string &key);

    bool hasText     (const std::string &key) const;
    bool hasRectangle(const std::string &key) const;
    bool hasItem     (const std::string &key) const;

    void clear();

    // Updates hover state on all buttons based on current mouse position.
    void updateHover(sf::Vector2f mouse);

    // Fires the onclick of whichever button the mouse is over.
    void handleClick(sf::Vector2f mouse);

    void setDrawFunction(std::function<void(MenuPage &, sf::RenderWindow &)> func);
    void draw(sf::RenderWindow &window);

private:
    std::unordered_map<std::string, sf::Text>          text_map;
    std::unordered_map<std::string, sf::RectangleShape> rectangle_map;
    std::unordered_map<std::string, FonctionItem>       item_map;
    std::function<void(MenuPage &, sf::RenderWindow &)> drawFunc;
};

#endif

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

static const unsigned int WIN_W  = 800;
static const unsigned int WIN_H  = 600;
#ifdef __APPLE__
static const char        *FONT   = "/System/Library/Fonts/Helvetica.ttc";
#else
static const char        *FONT   = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
#endif

// Colors
static const sf::Color BG        (18,  18,  32);
static const sf::Color GOLD      (220, 175,  80);
static const sf::Color WHITE     (230, 230, 230);
static const sf::Color DIM       (100, 100, 120);

// ── helpers ──────────────────────────────────────────────────────────────────

static void centerOrigin(sf::Text &t)
{
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
}

static sf::Text makeText(const std::string &str, const sf::Font &font,
                          unsigned int size, sf::Color color)
{
    sf::Text t(str, font, size);
    t.setFillColor(color);
    centerOrigin(t);
    return t;
}

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

// ── decorative grid (faint board pattern) ─────────────────────────────────────

static void drawGrid(sf::RenderWindow &window)
{
    const int    LINES = 12;
    const float  STEP  = 48.f;
    const float  OX    = (WIN_W - STEP * (LINES - 1)) / 2.f;
    const float  OY    = (WIN_H - STEP * (LINES - 1)) / 2.f;
    sf::Color    c(255, 255, 255, 12);

    sf::RectangleShape line;
    line.setFillColor(c);

    // vertical
    line.setSize(sf::Vector2f(1.f, STEP * (LINES - 1)));
    for (int i = 0; i < LINES; ++i)
    {
        line.setPosition(OX + i * STEP, OY);
        window.draw(line);
    }
    // horizontal
    line.setSize(sf::Vector2f(STEP * (LINES - 1), 1.f));
    for (int i = 0; i < LINES; ++i)
    {
        line.setPosition(OX, OY + i * STEP);
        window.draw(line);
    }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "Gomoku",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile(FONT))
        return 1;

    // Title
    sf::Text title = makeText("GOMOKU", font, 80, GOLD);
    title.setStyle(sf::Text::Bold);
    title.setPosition(WIN_W / 2.f, 160.f);

    // Divider line under title
    sf::RectangleShape divider(sf::Vector2f(320.f, 2.f));
    divider.setFillColor(GOLD);
    divider.setOrigin(160.f, 1.f);
    divider.setPosition(WIN_W / 2.f, 215.f);

    // Subtitle
    sf::Text sub = makeText("Five in a row", font, 20, DIM);
    sub.setPosition(WIN_W / 2.f, 240.f);

    // Menu items
    std::vector<Item> items;
    items.push_back(Item("Play",     font, WIN_W / 2.f, 360.f));
    items.push_back(Item("Quit",     font, WIN_W / 2.f, 440.f));

    while (window.isOpen())
    {
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left)
            {
                if (items[0].contains(mouse))   // Play
                { /* TODO: switch to game state */ }
                else if (items[1].contains(mouse)) // Quit
                    window.close();
            }
        }

        for (std::size_t i = 0; i < items.size(); ++i)
            items[i].setHovered(items[i].contains(mouse));

        window.clear(BG);
        drawGrid(window);
        window.draw(divider);
        window.draw(title);
        window.draw(sub);
        for (std::size_t i = 0; i < items.size(); ++i)
            items[i].draw(window);
        window.display();
    }

    return 0;
}

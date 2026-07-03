#include "ui/UIRenderer.hpp"
#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "game/controller/IGameController.hpp"
#include "interface.hpp"
#include <sstream>
#include <iomanip>

static std::string fmtMs(double ms)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << ms << " ms";
    return os.str();
}

static void hudRow(sf::RenderWindow& w, const sf::Font& font, unsigned size,
                   const std::string& label, const std::string& value,
                   float x, float right, float y)
{
    sf::Text l(label, font, size);
    l.setFillColor(DIM);
    l.setOrigin(l.getLocalBounds().left, l.getLocalBounds().top);
    l.setPosition(x, y);
    w.draw(l);

    sf::Text v(value, font, size);
    v.setFillColor(WHITE);
    const sf::FloatRect vb = v.getLocalBounds();
    v.setOrigin(vb.left + vb.width, vb.top);
    v.setPosition(right, y);
    w.draw(v);
}

void UIRenderer::renderMenu(sf::RenderWindow& w, MenuPage& page)
{
    page.draw(w);
}

void UIRenderer::renderGame(sf::RenderWindow& w, Board& board,
                             const IGameController& ctrl, CellStatus ghost)
{
    board.draw(w, ctrl.visualBoard(), ghost);
}

void UIRenderer::renderStats(sf::RenderWindow& w, const sf::Font& font,
                             const IGameController& ctrl)
{
    const float pad    = WIN_H * 0.016f;
    const float lineH  = FONT_XS * 1.45f;
    const float panelW = WIN_W * 0.19f;
    const float panelH = pad * 2.f + lineH * 2.f;
    const float panelX = WIN_W - panelW - WIN_H * 0.02f;
    const float panelY = WIN_H * 0.02f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(10, 10, 20, 180));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(DIM);
    w.draw(panel);

    const float left  = panelX + pad;
    const float right = panelX + panelW - pad;
    const float y0    = panelY + pad;

    hudRow(w, font, FONT_XS, "AI last", fmtMs(ctrl.aiMoveLastMs()),    left, right, y0);
    hudRow(w, font, FONT_XS, "AI avg",  fmtMs(ctrl.aiMoveAverageMs()), left, right, y0 + lineH);
}

void UIRenderer::renderColorChoice(sf::RenderWindow& w, MenuPage& colorChoice)
{
    const bool  threeOptions = colorChoice.hasItem("place2");
    const float panelW  = WIN_W * 0.32f;
    const float panelH  = threeOptions ? WIN_H * 0.304f : WIN_H * 0.235f;
    const float panelCY = threeOptions ? WIN_H * 0.581f : WIN_H * 0.546f;

    sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
    panel.setOrigin(panelW / 2.f, panelH / 2.f);
    panel.setPosition(CX, panelCY);
    panel.setFillColor(sf::Color(10, 10, 20, 218));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(GOLD);
    w.draw(panel);

    colorChoice.draw(w);
}

void UIRenderer::renderWinScreen(sf::RenderWindow& w, MenuPage& winScreen)
{
    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(WIN_W),
                                            static_cast<float>(WIN_H)));
    overlay.setFillColor(sf::Color(0, 0, 0, 190));
    w.draw(overlay);

    winScreen.draw(w);
}

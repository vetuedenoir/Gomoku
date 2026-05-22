#include "ui/UIRenderer.hpp"
#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "game/controller/IGameController.hpp"
#include "interface.hpp"

void UIRenderer::renderMenu(sf::RenderWindow& w, MenuPage& page)
{
    page.draw(w);
}

void UIRenderer::renderGame(sf::RenderWindow& w, Board& board,
                             const IGameController& ctrl, CellStatus ghost)
{
    board.draw(w, ctrl.visualBoard(), ghost);
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

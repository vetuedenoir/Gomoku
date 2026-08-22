#include "ui/UIRenderer.hpp"
#include "ui/MenuPage.hpp"
#include "ui/Board.hpp"
#include "game/controller/IGameController.hpp"
#include "interface.hpp"
#include <sstream>
#include <iomanip>

static const sf::Color STONE_BLACK(30, 30, 30);
static const sf::Color STONE_WHITE(230, 230, 230);

static std::string fmtMs(double ms)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << ms << " ms";
    return os.str();
}

// Draws one vertical "player card": a stone swatch, the colour name, the
// You / AI role, the capture progress, and (for the AI) its move timings.
static void drawPlayerCard(sf::RenderWindow& w, const sf::Font& font,
                           float cx, float cy, float cardW, float cardH,
                           const std::string& colorName, const std::string& role,
                           sf::Color stoneColor, int capturePairs, int maxPairs,
                           bool isActive, bool isAI,
                           double aiLastMs, double aiAvgMs)
{
    sf::RectangleShape panel(sf::Vector2f(cardW, cardH));
    panel.setOrigin(cardW / 2.f, cardH / 2.f);
    panel.setPosition(cx, cy);
    panel.setFillColor(sf::Color(10, 10, 20, 200));
    panel.setOutlineThickness(isActive ? 2.f : 1.f);
    panel.setOutlineColor(isActive ? GOLD : DIM);
    w.draw(panel);

    const float top = cy - cardH / 2.f;

    // Stone swatch
    const float stoneR = cardW * 0.16f;
    sf::CircleShape stone(stoneR);
    stone.setOrigin(stoneR, stoneR);
    stone.setPosition(cx, top + cardH * 0.14f);
    stone.setFillColor(stoneColor);
    stone.setOutlineThickness(1.f);
    stone.setOutlineColor(DIM);
    w.draw(stone);

    // Colour name
    sf::Text name = makeText(colorName, font, FONT_SM, WHITE);
    name.setPosition(cx, top + cardH * 0.30f);
    w.draw(name);

    // Role badge (You / AI)
    sf::Text roleText = makeText(role, font, FONT_XS, isActive ? GOLD : DIM);
    roleText.setStyle(sf::Text::Bold);
    roleText.setPosition(cx, top + cardH * 0.40f);
    w.draw(roleText);

    // Captures
    sf::Text capLabel = makeText("Captures", font, FONT_XS, DIM);
    capLabel.setPosition(cx, top + cardH * 0.56f);
    w.draw(capLabel);

    sf::Text caps = makeText(std::to_string(capturePairs) + " / " +
                             std::to_string(maxPairs), font, FONT_MD, WHITE);
    caps.setPosition(cx, top + cardH * 0.66f);
    w.draw(caps);

    // AI move timings, folded into the AI card only
    if (isAI)
    {
        sf::Text last = makeText("last " + fmtMs(aiLastMs), font, FONT_XS, DIM);
        last.setPosition(cx, top + cardH * 0.82f);
        w.draw(last);

        sf::Text avg = makeText("avg " + fmtMs(aiAvgMs), font, FONT_XS, DIM);
        avg.setPosition(cx, top + cardH * 0.90f);
        w.draw(avg);
    }
}

void UIRenderer::renderMenu(sf::RenderWindow& w, MenuPage& page)
{
    page.draw(w);
}

void UIRenderer::renderGame(sf::RenderWindow& w, Board& board,
                             const IGameController& ctrl, CellStatus ghost,
                             std::optional<Move> suggestion)
{
    board.draw(w, ctrl.visualBoard(), ghost);

    if (suggestion.has_value())
        board.drawHighlight(w, suggestion->col, suggestion->row);
}

void UIRenderer::renderStats(sf::RenderWindow& w, const sf::Font& font,
                             const Board& board, const IGameController& ctrl)
{
    const sf::FloatRect b = board.bounds();
    const float leftGutter  = b.left;
    const float rightGutter = WIN_W - (b.left + b.width);

    const float cardW = std::min(leftGutter, rightGutter) * 0.86f;
    const float cardH = std::min(b.height * 0.5f, WIN_H * 0.36f);
    const float cy    = b.top + b.height / 2.f;

    const float leftCX  = leftGutter / 2.f;
    const float rightCX = b.left + b.width + rightGutter / 2.f;

    // Captures are tracked as individual stones; Gomoku scoring is by pairs.
    const int maxPairs   = CAPTURES_TO_WIN / 2;
    const int blackPairs = ctrl.blackCaptureCount() / 2;
    const int whitePairs = ctrl.whiteCaptureCount() / 2;

    const bool aiVsAi        = ctrl.aiVsAi();
    const bool hotseat       = !ctrl.aiOpponent() && !aiVsAi;
    const bool playerIsBlack = ctrl.playerActor().color == Color::Black;
    const Color turn         = ctrl.currentColor();

    // Role badges: hotseat → P1/P2; AI vs AI → AI/AI; else YOU/AI by seat.
    const std::string blackRole = aiVsAi  ? "AI"
                                : hotseat ? "P1"
                                : (playerIsBlack ? "YOU" : "AI");
    const std::string whiteRole = aiVsAi  ? "AI"
                                : hotseat ? "P2"
                                : (playerIsBlack ? "AI" : "YOU");

    // Engine timings on every AI-controlled seat.
    const bool blackIsAI = aiVsAi || (!hotseat && !playerIsBlack);
    const bool whiteIsAI = aiVsAi || (!hotseat && playerIsBlack);

    // Black on the left
    drawPlayerCard(w, font, leftCX, cy, cardW, cardH,
                   "Black", blackRole,
                   STONE_BLACK, blackPairs, maxPairs,
                   turn == Color::Black, blackIsAI,
                   ctrl.aiMoveLastMs(Color::Black), ctrl.aiMoveAverageMs(Color::Black));

    // White on the right
    drawPlayerCard(w, font, rightCX, cy, cardW, cardH,
                   "White", whiteRole,
                   STONE_WHITE, whitePairs, maxPairs,
                   turn == Color::White, whiteIsAI,
                   ctrl.aiMoveLastMs(Color::White), ctrl.aiMoveAverageMs(Color::White));
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

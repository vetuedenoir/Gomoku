#include "MenuPage.hpp"

        void MenuPage::addText(const std::string& key, const sf::Text& text) {
			text_map[key] = text;
		}

		void MenuPage::addRectangle(const std::string& key, const sf::RectangleShape& rect) {
			rectangle_map[key] = rect;
		}

		void MenuPage::addItem(const std::string& key, const FonctionItem ftItem) {
			item_map.emplace(key, ftItem);
		}

		// ===== SUPPRESSION =====
		void MenuPage::removeText(const std::string& key) {
			text_map.erase(key);
		}

		void MenuPage::removeRectangle(const std::string& key) {
			rectangle_map.erase(key);
		}

		void MenuPage::removeItem(const std::string& key) {
			item_map.erase(key);
		}

		// ===== ACCÈS =====
		sf::Text* MenuPage::getText(const std::string& key) {
			auto it = text_map.find(key);
			return (it != text_map.end()) ? &it->second : nullptr;
		}

		sf::RectangleShape* MenuPage::getRectangle(const std::string& key) {
			auto it = rectangle_map.find(key);
			return (it != rectangle_map.end()) ? &it->second : nullptr;
		}

		FonctionItem* MenuPage::getItem(const std::string& key) {
			auto it = item_map.find(key);
			return (it != item_map.end()) ? &it->second : nullptr;
		}

		// ===== VÉRIFICATION =====
		bool MenuPage::hasText(const std::string& key) const {
			return text_map.find(key) != text_map.end();
		}

		bool MenuPage::hasRectangle(const std::string& key) const {
			return rectangle_map.find(key) != rectangle_map.end();
		}

		bool MenuPage::hasItem(const std::string& key) const {
			return item_map.find(key) != item_map.end();
		}

		// ===== CLEAR =====
		void MenuPage::clear() {
			text_map.clear();
			rectangle_map.clear();
			item_map.clear();
		}

		void MenuPage::setDrawFunction(std::function<void(MenuPage&, sf::RenderWindow&)> func) {
			drawFunc = func;
		}

		void MenuPage::draw(sf::RenderWindow& window) {
			if (drawFunc)
				drawFunc(*this, window);
		}
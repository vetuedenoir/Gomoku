#include "../include/MenuePage.hpp"

        void MenuePage::addText(const std::string& key, const sf::Text& text) {
			text_map[key] = text;
		}

		void MenuePage::addRectangle(const std::string& key, const sf::RectangleShape& rect) {
			rectangle_map[key] = rect;
		}

		void MenuePage::addItem(const std::string& key, const FonctionItem ftItem) {
			item_map.emplace(key, ftItem);
		}

		// ===== SUPPRESSION =====
		void MenuePage::removeText(const std::string& key) {
			text_map.erase(key);
		}

		void MenuePage::removeRectangle(const std::string& key) {
			rectangle_map.erase(key);
		}

		void MenuePage::removeItem(const std::string& key) {
			item_map.erase(key);
		}

		// ===== ACCÈS =====
		sf::Text* MenuePage::getText(const std::string& key) {
			auto it = text_map.find(key);
			return (it != text_map.end()) ? &it->second : nullptr;
		}

		sf::RectangleShape* MenuePage::getRectangle(const std::string& key) {
			auto it = rectangle_map.find(key);
			return (it != rectangle_map.end()) ? &it->second : nullptr;
		}

		FonctionItem* MenuePage::getItem(const std::string& key) {
			auto it = item_map.find(key);
			return (it != item_map.end()) ? &it->second : nullptr;
		}

		// ===== VÉRIFICATION =====
		bool MenuePage::hasText(const std::string& key) const {
			return text_map.find(key) != text_map.end();
		}

		bool MenuePage::hasRectangle(const std::string& key) const {
			return rectangle_map.find(key) != rectangle_map.end();
		}

		bool MenuePage::hasItem(const std::string& key) const {
			return item_map.find(key) != item_map.end();
		}

		// ===== CLEAR =====
		void MenuePage::clear() {
			text_map.clear();
			rectangle_map.clear();
			item_map.clear();
		}

		void MenuePage::setDrawFunction(std::function<void(MenuePage&, sf::RenderWindow&)> func) {
			drawFunc = func;
		}

		void MenuePage::draw(sf::RenderWindow& window) {
			if (drawFunc)
				drawFunc(*this, window);
		}
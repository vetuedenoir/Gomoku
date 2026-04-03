#ifndef MENUEPAGE_HPP
# define MENUEPAGE_HPP

#include "interface.hpp"
#include <unordered_map>

class MenuePage {
	private:
		std::unordered_map<std::string, sf::Text> text_map;
		std::unordered_map<std::string, sf::RectangleShape> rectangle_map;
		std::unordered_map<std::string, struct FonctionItem> item_map;

		std::function<void(MenuePage&, sf::RenderWindow&)> drawFunc;
	
	public:
		MenuePage() = default;
		~MenuePage() = default;

		// ===== AJOUT =====
		void addText(const std::string& key, const sf::Text& text);

		void addRectangle(const std::string& key, const sf::RectangleShape& rect);

		void addItem(const std::string& key, const FonctionItem ftItem);

		// ===== SUPPRESSION =====
		void removeText(const std::string& key);

		void removeRectangle(const std::string& key);

		void removeItem(const std::string& key);

		// ===== ACCÈS =====
		sf::Text* getText(const std::string& key);

		sf::RectangleShape* getRectangle(const std::string& key);

		FonctionItem* getItem(const std::string& key);

		// ===== VÉRIFICATION =====
		bool hasText(const std::string& key) const;

		bool hasRectangle(const std::string& key) const;

		bool hasItem(const std::string& key) const;

		// ===== CLEAR =====
		void clear();

		void setDrawFunction(std::function<void(MenuePage&, sf::RenderWindow&)> func);
		void draw(sf::RenderWindow& window);
};


#endif

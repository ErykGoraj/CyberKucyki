#include <iostream>
#include <SFML/Graphics.hpp>

using namespace std;

int FRAME_RATE {60};
sf::Vector2u WINDOWS_SIZE {1920, 1080};

enum Scene {
    MAIN_MENU,
    MENU,
    SETTINGS,
    CREDITS,
    GAME
};

Scene scene = MAIN_MENU;

void drawBoard(int tiles, float size, int offset_x, int offset_y, sf::RenderWindow &window);
void buttons(sf::RenderWindow &window);
int rollDice();

// Napraw tekstury MAPY
// Dodaj zegar gry
// Synchronizuj zegar z przyciskami i rzutami
// Dodaj efekty dzwiekowe

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOWS_SIZE),"Cyber Kucyki");
    window.setFramerateLimit(FRAME_RATE);

    int button_width = 264;
    int button_height = 68;

    sf::Texture settings_basic_texture("resources/buttons/settings.png");
    sf::Texture settings_pressed_texture("resources/buttons/settings_pressed.png");
    sf::Texture settings_onmouse_texture("resources/buttons/settings_onmouse.png");
    sf::Sprite settings_basic(settings_basic_texture);

    sf::Texture play_basic_texture("resources/buttons/play.png");
    sf::Texture play_onmouse_texture("resources/buttons/play_onmouse.png");
    sf::Texture play_pressed_texture("resources/buttons/play_pressed.png");
    sf::Sprite play_basic(play_basic_texture);

    sf::Texture authors_basic_texture("resources/buttons/authors.png");
    sf::Texture authors_onmouse_texture("resources/buttons/authors_onmouse.png");
    sf::Texture authors_pressed_texture("resources/buttons/authors_pressed.png");
    sf::Sprite authors_basic(authors_basic_texture);

    play_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f});
    settings_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f + 3 * button_height/2});
    authors_basic.setPosition({WINDOWS_SIZE.x/2.0f - (button_width/2.0f),500.0f  + 6 * button_height/2});

    sf::RectangleShape kostka({200,200});

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear(sf::Color(30, 30, 40));

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(mousePixel);
        bool settings_hovered = settings_basic.getGlobalBounds().contains(mousePos);
        bool play_hovered = play_basic.getGlobalBounds().contains(mousePos);
        bool authors_hovered = authors_basic.getGlobalBounds().contains(mousePos);

        bool kostka_naj = kostka.getGlobalBounds().contains(mousePos);

        if (scene == MAIN_MENU)
        {
            if (settings_hovered && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
                settings_basic.setTexture(settings_pressed_texture, true);
            }
            else if (settings_hovered)
            {
                settings_basic.setTexture(settings_onmouse_texture, true);
            }
            else
            {
                settings_basic.setTexture(settings_basic_texture, true);
            }

            if (play_hovered && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
                play_basic.setTexture(play_pressed_texture, true);
                scene = GAME;
            }
            else if (play_hovered)
            {
                play_basic.setTexture(play_onmouse_texture, true);
            }
            else
            {
                play_basic.setTexture(play_basic_texture, true);
            }

            if (authors_hovered && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
                authors_basic.setTexture(authors_pressed_texture, true);
            }
            else if (authors_hovered)
            {
                authors_basic.setTexture(authors_onmouse_texture, true);
            }
            else
            {
                authors_basic.setTexture(authors_basic_texture, true);
            }

            window.draw(settings_basic);
            window.draw(play_basic);
            window.draw(authors_basic);
        }else if (scene == GAME)
        {
            if (kostka_naj && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            {
                std::cout << rollDice();
            }

            window.draw(kostka);
            drawBoard(12, 90, 420, 0, window);
            drawBoard(8, 90, 600, 180, window);
        }
        window.display();
    }
}

void drawBoard(int tiles, float size, int offset_x, int offset_y, sf::RenderWindow &window)
{
    sf::RectangleShape trj;
    trj.setSize({size, size});
    trj.setPosition({0.0f, 0.0f});

    for (int i = 0; i < tiles; i++)
    {
        trj.setPosition({offset_x + i * size, 0.0f + offset_y});
        window.draw(trj);

        trj.setPosition({offset_x + i * size, size * (tiles - 1) + offset_y});
        window.draw(trj);

        if (i != 0 && i != tiles - 1)
        {
            trj.setPosition({offset_x + 0.0f, size * i + offset_y});
            window.draw(trj);

            trj.setPosition({offset_x + size * (tiles - 1), size * i + offset_y});
            window.draw(trj);
        }
    }
}

int rollDice()
{
    return std::rand() % 11;
}
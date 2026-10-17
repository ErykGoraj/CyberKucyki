#include <iostream>
#include <SFML/Graphics.hpp>

using namespace std;

int FRAME_RATE {60};
sf::Vector2u WINDOWS_SIZE {1920, 1080};

void draw_board(int tiles, float size, int offset_x, int offset_y, sf::RenderWindow &window);

int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOWS_SIZE),"Cyber Kucyki");
    window.setFramerateLimit(FRAME_RATE);

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
        draw_board(12, 90, 420, 0, window);
        draw_board(8, 90, 600, 180, window);
        window.display();
    }
}

void draw_board(int tiles, float size, int offset_x, int offset_y, sf::RenderWindow &window)
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

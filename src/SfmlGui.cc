
#include "FakeChip8.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <bitset>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>

#include "SfmlGui.h"
namespace fakers
{

void Gui::run() {
    using namespace std::chrono_literals;
    renderWindow_ = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(windowSizeX, windowSizeY), "FakeChip8 - Emulator");
    sf::Font font;
    if (!font.loadFromFile("C:\\Windows\\Fonts\\consola.ttf")) {
        throw std::runtime_error("cannot load font");
    }
    constexpr float fontSize = 20;
    sf::Text text(L"\u00a9 HalfsInner", font, fontSize);
    text.setOutlineThickness(2.f);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Blue);
    while (renderWindow_->isOpen()) {
        // Process events
        sf::Event event;
        while (renderWindow_->pollEvent(event)) {
            // Close window: exit
            if (event.type == sf::Event::Closed) {
                onExitHandler();
                renderWindow_->close();
            }
            if (event.type == sf::Event::Resized) {
                sf::Vector2u windowSize = renderWindow_->getSize();
                sf::FloatRect visibleArea;
                visibleArea.left = 0;
                visibleArea.top = 0;
                visibleArea.width = windowSize.x;
                visibleArea.height = windowSize.y;

                renderWindow_->setView(sf::View{ visibleArea });

                windowSizeX = windowSize.x;
                windowSizeY = windowSize.y;
            }
            if (event.type == sf::Event::KeyPressed) {
                handleKeyPressed(event, true);
            }
            if (event.type == sf::Event::KeyReleased) {
                handleKeyPressed(event, false);
            }
        }

        renderWindow_->clear();
        {
            const std::lock_guard lock{ drawingMutex_ };
            for (auto const& rect : pixelRects_) {
                renderWindow_->draw(rect);
            }
        }
        renderWindow_->draw(text);
        renderWindow_->display();
        std::this_thread::sleep_for(15ms);
    }
}

void Gui::handleKeyPressed(const sf::Event& event, bool isPressed) {
    const std::lock_guard lock{ readingKeyMutex_ };
    switch (event.key.code) {
    case sf::Keyboard::Num1:
        keyPressedState[1] = isPressed;
        break;
    case sf::Keyboard::Num2:
        keyPressedState[2] = isPressed;
        break;
    case sf::Keyboard::Num3:
        keyPressedState[3] = isPressed;
        break;
    case sf::Keyboard::Q:
        keyPressedState[4] = isPressed;
        break;
    case sf::Keyboard::W:
        keyPressedState[5] = isPressed;
        break;
    case sf::Keyboard::E:
        keyPressedState[6] = isPressed;
        break;
    case sf::Keyboard::A:
        keyPressedState[7] = isPressed;
        break;
    case sf::Keyboard::S:
        keyPressedState[8] = isPressed;
        break;
    case sf::Keyboard::D:
        keyPressedState[9] = isPressed;
        break;
    case sf::Keyboard::Z:
        keyPressedState[0xa] = isPressed;
        break;
    case sf::Keyboard::X:
        keyPressedState[0] = isPressed;
        break;
    case sf::Keyboard::C:
        keyPressedState[0xb] = isPressed;
        break;
    case sf::Keyboard::Num4:
        keyPressedState[0xc] = isPressed;
        break;
    case sf::Keyboard::R:
        keyPressedState[0xd] = isPressed;
        break;
    case sf::Keyboard::F:
        keyPressedState[0xe] = isPressed;
        break;
    case sf::Keyboard::V:
        keyPressedState[0xf] = isPressed;
        break;
    default:
        break;
    }
}

void Gui::draw(std::vector<uint64_t> const& graphic) {
    std::vector<sf::RectangleShape> updated;
    constexpr size_t maxSquaresOnAxisX = 64;
    constexpr size_t maxSquaresOnAxisY = 32;

    constexpr float outlineSize = 4;
    constexpr float padding = outlineSize / 2.f;
    constexpr float squareSizeBase = 100;
    float ratioX = (float)windowSizeX / (float)windowSizeY;
    float ratioY = (float)windowSizeY / (float)windowSizeX;
    float squareSizeX = windowSizeX / ((float)maxSquaresOnAxisX);
    float squareSizeY = windowSizeY / ((float)maxSquaresOnAxisY);
    auto rectSize =
        sf::Vector2f{ squareSizeX - outlineSize, squareSizeY - outlineSize };

    sf::RectangleShape rectangle;
    rectangle.setOutlineColor(sf::Color::Cyan);
    rectangle.setFillColor(sf::Color::Blue);
    rectangle.setOutlineThickness(outlineSize);
    rectangle.setSize(rectSize);

    for (size_t i = 0; i < graphic.size(); ++i) {
        std::bitset<64u> line{ graphic[i] };
        for (size_t j = 0; j < maxSquaresOnAxisX; ++j) {
            if (line[j]) {
                auto x = (rectSize.x + outlineSize) * (float)(maxSquaresOnAxisX - j - 1);
                auto y = (rectSize.y + outlineSize) * (float)(i);

                rectangle.setPosition(sf::Vector2f(padding + x, padding + y));
                updated.push_back(rectangle);
            }
        }
    }
    const std::lock_guard lock{ drawingMutex_ };
    std::swap(updated, pixelRects_);
}

std::bitset<16> Gui::read() {
    const std::lock_guard lock{ readingKeyMutex_ };
    return keyPressedState;
}

void Gui::onExit(std::function<void()>&& handler) { onExitHandler = handler; }

} // namespace fakers 
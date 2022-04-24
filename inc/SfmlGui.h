#pragma once 

#include "FakeChip8.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <bitset>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
namespace fakers
{
class Gui : public DisplayIO, public InputIO {
public:
    void run();

    void handleKeyPressed(const sf::Event& event, bool isPressed);

    void draw(std::vector<uint64_t> const& graphic) override;

    virtual std::bitset<16> read() override;

    void onExit(std::function<void()>&& handler);
private:
    std::mutex drawingMutex_;
    std::mutex readingKeyMutex_;

    std::unique_ptr<sf::RenderWindow> renderWindow_;
    std::vector<sf::RectangleShape> pixelRects_;
    int windowSizeX = 800;
    int windowSizeY = 640;

    std::bitset<16> keyPressedState;
    std::function<void()> onExitHandler;
};

} // namespace fakers
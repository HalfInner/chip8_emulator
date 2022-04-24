#pragma once
// #include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
// #include <iostream>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <future>
#include <iterator>
#include <iostream>
#include <memory>
#include <mutex>

#include "FakeChip8.h"
#include "SfmlGui.h"
namespace fakers
{

std::vector<uint8_t> readRom(std::string_view romPath) {
    std::cout << "Loading " << romPath << '\n';
    auto f = std::ifstream{ romPath.data(), std::ios::binary};
    std::ostringstream ss;
    ss << f.rdbuf();
    auto const& s = ss.str();
    return { begin(s), end(s) };
}

class FakeChip8Runner {
public:
    void run(std::string_view romPath) {
        using namespace std::chrono_literals;

        Gui gui;
        FakeChip8 chip8;
        gui.onExit([&chip8]() { chip8.stop(); });

        auto g = std::async(std::launch::async, &Gui::run, &gui);
        chip8.attachDisplay(&gui);
        chip8.attachIO(&gui);
        chip8.load(readRom(romPath));

        try {
            auto start = std::chrono::steady_clock::now();
            bool isRunning = true;
            while (isRunning) {
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_seconds = end - start;
                if (elapsed_seconds > 8ms) {
                    start = end;
                    isRunning = chip8.step();
                }
                std::this_thread::sleep_for(7ms);
            }
        } catch (std::exception& e) {
            std::cout << "ERROR:" << e.what() << "\n";
        }
        chip8.stop();
    }
};

} // namesapce fakers
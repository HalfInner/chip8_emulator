#pragma once

#include <bitset>
#include <sstream>
#include <vector>

namespace fakers
{
struct DisplayIO {
    virtual void draw(std::vector<uint64_t> const& graphic) {};
    virtual ~DisplayIO() {}
};

struct AudioIO {

};

struct InputIO {
    virtual std::bitset<16> read() = 0;
};

class FakeChip8 {
public:
    ~FakeChip8();
    void load(const std::vector<uint8_t>& program);

    void attachDisplay(DisplayIO* display);
    void attachIO(InputIO* inputIO);

    void stop();
    bool step();

private:
    constexpr int arg(int opcode, int n) const;
    void handleStep();
    bool handleGetKey();
    int readOpCode();
    void flushDebugToStdout();

    bool debug_{};

    std::stringstream debugPrint_;

    bool toStop_ = false;
    int pc_ = 0;
    int regI_ = 0;
    int timer_ = 0;
    int sound_ = 0;

    int blockKeyVar_ = 0;
    bool pendingKeyRead_ = false;


    std::vector<uint8_t> vars_;
    std::vector<uint8_t> memory_;
    std::vector<uint64_t> rawDisplay_;

    std::bitset<16> keyStates_;

    std::vector<int> stack_;

    DisplayIO* display_{ nullptr };
    InputIO* inputIO_{ nullptr };
};

} // namespace fakers
#pragma once

#include "FakeChip8.h"

#include <bitset>
#include <iostream>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace fakers
{
namespace
{
constexpr unsigned char CHIP8_FONT_SET[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

static constexpr size_t MEM_SIZE = 0x1000;
static constexpr size_t MEM_START = 0x200;
static constexpr size_t FLAG_REG = 0xf;

constexpr bool ENABLE_DEBUGGING = true;

} // namespace

FakeChip8::~FakeChip8() {
    flushDebugToStdout();
}

void FakeChip8::load(const std::vector<uint8_t>& program) {
    std::cout << "Loading Program... ";
    vars_.resize(16);
    memory_.resize(MEM_SIZE);
    rawDisplay_.resize(32);
    std::fill(begin(rawDisplay_), end(rawDisplay_), 0x00);
    pc_ = MEM_START;
    std::copy(std::begin(CHIP8_FONT_SET), std::end(CHIP8_FONT_SET), std::begin(memory_));
    std::copy(std::begin(program), std::end(program), std::begin(memory_) + MEM_START);

    std::cout << "program=" << program.size() << " mem=" << memory_.size() << "\n";
    debug_ = ENABLE_DEBUGGING;
    if (debug_) {
        std::cout << "DEBUG ON";
    }
}

void FakeChip8::attachDisplay(DisplayIO* display) {
    display_ = display;
}

void FakeChip8::attachIO(InputIO* inputIO) {
    inputIO_ = inputIO;
}

void FakeChip8::stop() {
    toStop_ = true;
}

bool FakeChip8::step() {
    handleStep();
    if (handleGetKey()) {
        return !toStop_;
    }

    debugPrint_.str("");
    if (debug_) debugPrint_ << "0x" << std::hex << pc_ << "\t0x";
    int opcode = readOpCode();
    if (debug_) debugPrint_ << std::hex << opcode << ":\t";
    auto calls = std::unordered_map<int, std::function<int(int)>>{ {
        { 0, [&](int opcode) {
        if (opcode == 0x00E0) {
            if (debug_) debugPrint_ << "cls";
            rawDisplay_.clear();
            rawDisplay_.resize(32);
            return 0;
        }
        if (opcode == 0x00EE) {
            pc_ = stack_.back();
            if (debug_) debugPrint_ << "ret  pc=0x" << std::hex << pc_;
            stack_.pop_back();
            return 0;
        }
        pc_ = opcode & 0xfff;
        if (debug_) debugPrint_ << "exec pc=0x" << std::hex << pc_;
        return 0;
    } },
    { 1, [&](int opcode) {
        auto oldpc = pc_ - 2;
        pc_ = opcode & 0xfff;
        if (oldpc == pc_) {
            if (debug_) debugPrint_ << "stop";
            toStop_ = true;
        } else {
            if (debug_) debugPrint_ << "goto pc=0x" << std::hex << pc_;
        }
        return 0;
    } },
    { 2, [&](int opcode) {
        stack_.push_back(pc_);
        pc_ = opcode & 0xfff;
        if (debug_) debugPrint_ << "call pc=0x" << std::hex << pc_;
        return 0;
    } },
    { 3, [&](int opcode) {
        if (debug_) debugPrint_ << "cmp  V" << (int)arg(opcode, 1) << " 0x"
            << std::hex << (int)vars_.at(arg(opcode, 1)) << "==0x" << (int)(opcode & 0xff);
        if ((vars_.at(arg(opcode, 1)) == (opcode & 0xff))) {
            if (debug_) debugPrint_ << " skip pc=" << pc_;
            pc_ += 2;
        }
        return 0;
    } },
    { 4, [&](int opcode) {
        if (debug_) debugPrint_ << "cmp  V" << (int)arg(opcode, 1) << " 0x"
            << std::hex << (int)vars_.at(arg(opcode, 1)) << "!=0x" << (int)(opcode & 0xff);
        if ((vars_.at(arg(opcode, 1)) != (opcode & 0xff))) {
            if (debug_) debugPrint_ << " skip pc=" << pc_;
            pc_ += 2;
        }
        return 0;
    } },
    { 5, [&](int opcode) {
        if (debug_) debugPrint_ << "cmp  V" << (int)arg(opcode, 1) << "==V"
            << (int)arg(opcode, 2) << " 0x" << std::hex << (int)vars_.at(arg(opcode, 1)) << " == 0x" << (int)(opcode & 0xff);
        if ((vars_.at(arg(opcode, 1)) == vars_.at(arg(opcode, 2)))) {
            if (debug_) debugPrint_ << " skip pc=" << pc_;
            pc_ += 2;
        }
        return 0;
    } },
    { 6, [&](int opcode) {
        if (debug_) debugPrint_ << "asgn V" << std::hex << (int)arg(opcode, 1)
            << "=0x" << (int)(opcode & 0xff);
        vars_.at(arg(opcode, 1)) = opcode & 0xff;
        vars_.at(arg(opcode, 1)) = vars_.at(arg(opcode, 1));
        if (debug_) debugPrint_ << "\t =>" << (int)vars_.at(arg(opcode, 1));
        return 0;
    } },
    { 7, [&](int opcode) {
        if (debug_) debugPrint_ << "inc  V" << std::hex << (int)arg(opcode, 1) << "+=0x" << (int)(opcode & 0xff);
        vars_.at(arg(opcode, 1)) += opcode & 0xff;
        if (debug_) debugPrint_ << "\t =>" << (int)vars_.at(arg(opcode, 1));
        return 0;
    } },
    { 8, [&](int opcode) {
        if (debug_) debugPrint_ << "asgn V" << (int)arg(opcode, 1);
        switch (arg(opcode, 3)) {
        case 0x0:
            vars_.at(arg(opcode, 1)) = vars_.at(arg(opcode, 2));
            if (debug_) debugPrint_ << "=V" << (int)arg(opcode, 2);
            break;
        case 0x1:
            vars_.at(arg(opcode, 1)) |= vars_.at(arg(opcode, 2));
            if (debug_) debugPrint_ << "|=V" << (int)arg(opcode, 2);
            break;
        case 0x2:
            if (debug_) debugPrint_ << "&=V" << (int)arg(opcode, 2) << "V" << (int)arg(opcode, 1);
            vars_.at(arg(opcode, 1)) &= vars_.at(arg(opcode, 2));
            break;
        case 0x3:
            if (debug_) debugPrint_ << "^=V" << (int)arg(opcode, 2);
            vars_.at(arg(opcode, 1)) ^= vars_.at(arg(opcode, 2));
            break;
        case 0x4:
            if (debug_) debugPrint_ << "+=V" << (int)arg(opcode, 2);
            if (vars_.at(arg(opcode, 1)) + vars_.at(arg(opcode, 2)) > 0xff) {
                vars_.at(FLAG_REG) = 1;
            } else {
                vars_.at(FLAG_REG) = 0;
            }
            vars_.at(arg(opcode, 1)) += vars_.at(arg(opcode, 2));
            break;
        case 0x5:
            if (debug_) debugPrint_ << "-=V" << (int)arg(opcode, 2);
            if (vars_.at(arg(opcode, 1)) >= vars_.at(arg(opcode, 2))) {
                vars_.at(FLAG_REG) = 1;
            } else {
                vars_.at(FLAG_REG) = 0;
            }
            vars_.at(arg(opcode, 1)) -= vars_.at(arg(opcode, 2));
            break;
        case 0x6:
            if (debug_) debugPrint_ << ">>=1";
            vars_.at(FLAG_REG) = vars_.at(arg(opcode, 1)) & 0x1;
            vars_.at(arg(opcode, 1)) >>= 1;
            break;
        case 0x7:
            if (debug_) debugPrint_ << "=V" << (int)arg(opcode, 2) <<
                "-V" << (int)arg(opcode, 1);
            vars_.at(arg(opcode, 1)) = vars_.at(arg(opcode, 2)) - vars_.at(arg(opcode, 1));
            break;
        case 0xe:
            if (debug_) debugPrint_ << "<<=1";
            vars_.at(FLAG_REG) = !!(vars_.at(arg(opcode, 1)) & 0x80);
            vars_.at(arg(opcode, 1)) <<= 1;
            break;
        default:
            if (debug_) debugPrint_ << "NOT HANDLED";
            throw std::runtime_error("NOT HANDLED" + std::to_string(opcode));
            break;
        }
        if (debug_) debugPrint_ << "\t =>" << (int)vars_.at(arg(opcode, 1));
        return 0;
    } },
    { 9, [&](int opcode) {
        if (vars_.at(arg(opcode, 1)) != vars_.at(arg(opcode, 2))) {
            pc_ += 2;
        }
        return 0;
    } },
    { 0xa, [&](int opcode) {
        regI_ = opcode & 0xfff;
        if (debug_) debugPrint_ << "reg  I=0x" << std::hex << (int)regI_;
        return 0;
    } },
    { 0xb, [&](int opcode) {
        pc_ = vars_.at(0) + (opcode & 0xfff);
        return 0;
    } },
    { 0xc, [&](int opcode) {
        if (debug_) debugPrint_ << "rnd  V" << std::hex << (int)arg(opcode, 1)
            << "= rand() & 0x" << std::hex << (int)(opcode & 0xff);
        vars_.at(arg(opcode, 1)) = rand() & (opcode & 0xff);
        return 0;
    } },
    { 0xd, [&](int opcode) {
        int x = vars_.at(arg(opcode, 1)) % 64;
        int y = vars_.at(arg(opcode, 2)) % 32;
        int n = arg(opcode, 3);
        if (debug_) debugPrint_ << "draw I=0x" << regI_
            << std::dec << ":(" << x << ";" << y << ";" << n << ")";

        constexpr bool collision = true;
        constexpr size_t byteSize = 8;
        vars_.at(FLAG_REG) = !collision;
        for (size_t i = 0; i < n && (regI_ + i) < memory_.size() && y + i < rawDisplay_.size(); ++i) {
            uint64_t lineGraphic = memory_.at(regI_ + i);
            lineGraphic <<= byteSize * (sizeof(uint64_t) - 1);
            if (rawDisplay_.at(y + i) & (lineGraphic >> x)) {
                vars_.at(FLAG_REG) = collision;
            }
            rawDisplay_.at(y + i) ^= lineGraphic >> x;
        }
        display_->draw(rawDisplay_);
        return 0;
    } },
    { 0xe, [&](int opcode) {
        if (debug_) debugPrint_ << "key" << std::hex << (int)vars_.at(arg(opcode, 1));
        bool keyPressed = keyStates_[vars_.at(arg(opcode, 1))];
        switch (opcode & 0xff) {
        case 0x9E:
            if (debug_) debugPrint_ << " ON ";
            if (keyPressed) {
                if (debug_) debugPrint_ << " skip pc=" << pc_;
                pc_ += 2;
            }
            break;
        case 0xA1:
            if (debug_) debugPrint_ << " OFF";
            if (!keyPressed) {
                if (debug_) debugPrint_ << " skip pc=" << pc_;
                pc_ += 2;
            }
            break;
        }
        return 0;
    } },
    { 0xf, [&](int opcode) {
        int val = vars_.at(arg(opcode, 1));
        int type = opcode & 0xff;
        switch (type) {
        case 0x07:
            if (debug_) debugPrint_ << "time V" << (int)arg(opcode, 1) << "=" << timer_;
            vars_.at(arg(opcode, 1)) = timer_;
            break;
        case 0x0a:
            if (debug_) debugPrint_ << "key  V" << (int)arg(opcode, 1);
            blockKeyVar_ = arg(opcode, 1);
            pendingKeyRead_ = true;
            break;
        case 0x15:
            if (debug_) debugPrint_ << "time t=" << (int)arg(opcode, 1);
            timer_ = vars_.at(arg(opcode, 1));
            break;
        case 0x18:
            if (debug_) debugPrint_ << "snd  s=" << (int)arg(opcode, 1);
            sound_ = vars_.at(arg(opcode, 1));
            break;
        case 0x1e:
            if (debug_)
                debugPrint_ << "inc  I+=V" << std::hex << (int)arg(opcode, 1)
                << "\t =>" << (int)regI_;
            regI_ += vars_.at(arg(opcode, 1));
            break;
        case 0x29:
        {
            size_t fontSize = 5;
            regI_ = static_cast<size_t>(vars_.at(arg(opcode, 1)) * fontSize);
            if (debug_) debugPrint_ << "reg  I=0x" << std::hex << regI_ << "(font)";
            break;
        }
        case 0x33:
            if (debug_) debugPrint_ << "bcd  load 0x" << val;
            memory_.at(regI_ + 2) = val % 10;
            val /= 10;
            memory_.at(regI_ + 1) = val % 10;
            val /= 10;
            memory_.at(regI_ + 0) = val % 10;
            if (debug_) debugPrint_ << " (" << (int)(memory_.at(regI_ + 0))
                << ";" << (int)(memory_.at(regI_ + 1))
                << ";" << (int)(memory_.at(regI_ + 2)) << ")";
            break;
        case 0x55:
            if (debug_) debugPrint_ << "reg  dump V[0;" << arg(opcode, 1) << "]";
            for (size_t i = 0; i <= arg(opcode, 1); ++i) {
                memory_.at(regI_ + i) = vars_.at(i);
            }
            break;
        case 0x65:
            if (debug_) debugPrint_ << "reg  load V[0;" << arg(opcode, 1) << "]";
            for (size_t i = 0; i <= arg(opcode, 1); ++i) {
                vars_.at(i) = memory_.at(regI_ + i);
            }
            break;
        }
        return 0;
    } }
        } };

    calls[arg(opcode, 0)](opcode);
    flushDebugToStdout();
    return pc_ < memory_.size() && !toStop_;
}


bool FakeChip8::handleGetKey() {
    keyStates_ = inputIO_->read();
    if (keyStates_.any() && pendingKeyRead_) {
        for (size_t i = 0; i < keyStates_.size(); ++i) {
            if (keyStates_[i]) {
                vars_.at(blockKeyVar_) = i;
            }
        }
        pendingKeyRead_ = false;
    }
    return pendingKeyRead_;
}

constexpr int FakeChip8::arg(int opcode, int n) const {
    return opcode >> (4 * (3 - n)) & 0xf;
}

void FakeChip8::handleStep() {
    if (timer_) --timer_;
    if (sound_) --sound_;
}

int FakeChip8::readOpCode() {
    int val = (memory_.at(pc_) << 8) | (memory_.at(pc_ + 1));
    pc_ += 2;
    return val;
}

void  FakeChip8::flushDebugToStdout() {
    if (debug_) {
        auto const& debugOutput = debugPrint_.str();
        if (!debugOutput.empty()) {
            std::cout << debugPrint_.str() << '\n';
        }
    }
    debugPrint_.str("");
}

} // namespace fakers
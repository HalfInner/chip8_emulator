#!/usr/bin/sh
cmake -S . -B out && cmake --build out && out/Debug/FakeChip8.exe roms/MERLIN
# 65x02 MCU family emulator

bare6502 is a flexible and extensible emulator for the 65x02 family of MCUs.

# Features
- Supported 6502 & 65C02 CPUs (can be easily extended)
- Supported peripherals: ACIA/Display/Memory(RAM/ROM)/Timer
- 'BUS' logic for flexible connections
- Flexible configuration via JSON

# References
- Original
    - [JSON parser](https://github.com/whyisitworking/C-Simple-JSON-Parser)
    - [6502 tests](https://github.com/Klaus2m5/6502_65C02_functional_tests)
    - [MS BASIC for 6502](https://github.com/mist64/msbasic)
- Modified (with bare6502 support)
    - [MS BASIC for 6502](https://github.com/TheB1t/msbasic/tree/bare6502_support)
    - [llvm-mos-sdk](https://github.com/TheB1t/llvm-mos-sdk/tree/bare6502_support)

# Build

```bash
git clone https://github.com/TheB1t/bare6502.git
cd bare6502
mkdir build
cd build
cmake ..
make
```

# Run

There is several default configuratins available, 6502 & 65C02 functional tests and MS BASIC. For use just run `./bare6502 -c <configuration file>` with the desired configuration file. For start just press any key.

If you want to use emulated console you should attach to PTY (you can find path to PTY in program output) using picocom or similar tool.
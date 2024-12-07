#include "../includes/CPU.h"
#include <fstream>
#include <iostream>

#include <chrono>
#include <thread>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <binary file>\n";
        return 1;
    }

    CPU cpu;

    // Load binary file into memory
    std::ifstream file(argv[1], std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open binary file: " << argv[1] << "\n";
        return 1;
    }
    else
    {
        std::cout << "Binary file opened successfully!\n";
    }

    // Load binary into memory starting at address 0x8000
    uint16_t startAddress = 0x8000;
    uint8_t byte;
    while (file.read(reinterpret_cast<char *>(&byte), 1))
    {
        if (startAddress > 0xFFFF)
        {
            std::cerr << "Error: Attempted to write beyond memory limit (0xFFFF).\n";
            break;
        }

        cpu.memory[startAddress++] = byte;
    }

    // Check if any data was loaded
    if (startAddress == 0x8000)
    {
        std::cerr << "No data loaded into memory!\n";
    }
    else
    {
        std::cout << "Loaded binary into memory, total bytes: " << (startAddress - 0x8000) << "\n";
    }

    // Set the program counter to the start of the loaded program
    cpu.pc = 0x8000;

    // Execute instructions in a loop
    while (cpu.memory[0xF001] == 0)
    {
        cpu.execute();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    int result = static_cast<int>(cpu.memory[0xF001]);
    std::cout << "Summation result: " << std::dec << result << std::endl;

    return 0;
}

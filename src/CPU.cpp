#include "CPU.h"

#include <iostream>
#include <stdexcept>

// 플래그 정의
constexpr uint8_t FLAG_CARRY = 0;
constexpr uint8_t FLAG_ZERO = 1;
constexpr uint8_t FLAG_INTERRUPT = 2;
constexpr uint8_t FLAG_DECIMAL = 3;
constexpr uint8_t FLAG_BRK = 4;
constexpr uint8_t FLAG_UNUSED = 5;
constexpr uint8_t FLAG_OVERFLOW = 6;
constexpr uint8_t FLAG_NEGATIVE = 7;

#define SET_FLAG(status, flag, condition)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (condition)                                                                                                 \
            (status) |= (1 << (flag));                                                                                 \
        else                                                                                                           \
            (status) &= ~(1 << (flag));                                                                                \
    } while (0)

#define CLEAR_FLAG(status, flag) ((status) &= ~(1 << (flag)))
#define CHECK_FLAG(status, flag) ((status) & (1 << (flag)))

CPU::CPU()
{
    memory.reserve(0x10000);      // 최대 64KB
    memory.resize(0x10000, 0x00); // 초기화
    setupInstructionSet();
}

uint8_t CPU::read(uint16_t address)
{
    return memory[address];
}

uint16_t CPU::read16(uint16_t address, bool wrapAround = false)
{
    // little endian
    if (wrapAround)
        return read(address & 0xFF) | (read((address + 1) & 0xFF) << 8);
    return read(address) | (read(address + 1) << 8);
}

void CPU::write(uint16_t address, uint8_t value)
{
    memory[address] = value;
}

void CPU::execute()
{
    uint8_t opcode = fetch();

    // if (opcode != 0x00)
    // {
    //     std::cout << "Executing opcode: 0x" << std::hex << std::uppercase << +opcode << " " << pc << std::endl;
    // }

    if (instructionSet.find(opcode) != instructionSet.end())
    {
        auto &instruction = instructionSet[opcode];
        uint16_t address = fetchAddress(instruction.mode);
        instruction.operation(address);
    }
    else
        std::cerr << "Unknown opcode: " << std::hex << +opcode << "\n";
}

uint8_t CPU::fetch()
{
    return read(pc++);
}

uint16_t CPU::fetchAbsolute()
{
    return read(pc++) | (read(pc++) << 8); // little endian
}

uint8_t CPU::fetchZeroPage(uint8_t offset = 0)
{
    return (read(pc++) + offset) & 0xFF; // Zero Page에서 랩 어라운드
}

uint16_t CPU::fetchAddress(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::Implied: return 0;
    case AddressMode::Accumulator: return -1;
    case AddressMode::Immediate: return pc++;
    case AddressMode::ZeroPage: return fetchZeroPage();
    case AddressMode::ZeroPageXIndexed: return fetchZeroPage(x);
    case AddressMode::ZeroPageYIndexed: return fetchZeroPage(y);
    case AddressMode::Absolute: return fetchAbsolute();
    case AddressMode::AbsoluteXIndexed: return fetchAbsolute() + x;
    case AddressMode::AbsoluteYIndexed: return fetchAbsolute() + y;
    case AddressMode::Indirect: return read16(fetchAbsolute());
    case AddressMode::IndexedIndirect: return read16(fetchZeroPage(x), true);
    case AddressMode::IndirectIndexed: return read16(fetchZeroPage(), true) + y;
    case AddressMode::Relative: return pc + 1 + static_cast<int8_t>(read(pc++));
    default: throw std::runtime_error("Unknown addressing mode");
    }
}
void CPU::setZNFlag(uint8_t value)
{
    SET_FLAG(stat, FLAG_ZERO, value == 0);
    SET_FLAG(stat, FLAG_NEGATIVE, value & 0x80);
}

/* instruction set */
void CPU::setupInstructionSet()
{
    // Access
    instructionSet[0xA9] = { [this](uint16_t address) { LDA(address); }, AddressMode::Immediate };
    instructionSet[0xA5] = { [this](uint16_t address) { LDA(address); }, AddressMode::ZeroPage };
    instructionSet[0xB5] = { [this](uint16_t address) { LDA(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xAD] = { [this](uint16_t address) { LDA(address); }, AddressMode::Absolute };
    instructionSet[0xBD] = { [this](uint16_t address) { LDA(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0xB9] = { [this](uint16_t address) { LDA(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0xA1] = { [this](uint16_t address) { LDA(address); }, AddressMode::IndexedIndirect };
    instructionSet[0xB1] = { [this](uint16_t address) { LDA(address); }, AddressMode::IndirectIndexed };
    instructionSet[0x85] = { [this](uint16_t address) { STA(address); }, AddressMode::ZeroPage };
    instructionSet[0x95] = { [this](uint16_t address) { STA(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x8D] = { [this](uint16_t address) { STA(address); }, AddressMode::Absolute };
    instructionSet[0x9D] = { [this](uint16_t address) { STA(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x99] = { [this](uint16_t address) { STA(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x81] = { [this](uint16_t address) { STA(address); }, AddressMode::IndexedIndirect };
    instructionSet[0x91] = { [this](uint16_t address) { STA(address); }, AddressMode::IndirectIndexed };
    instructionSet[0xA2] = { [this](uint16_t address) { LDX(address); }, AddressMode::Immediate };
    instructionSet[0xA6] = { [this](uint16_t address) { LDX(address); }, AddressMode::ZeroPage };
    instructionSet[0xB6] = { [this](uint16_t address) { LDX(address); }, AddressMode::ZeroPageYIndexed };
    instructionSet[0xAE] = { [this](uint16_t address) { LDX(address); }, AddressMode::Absolute };
    instructionSet[0xBE] = { [this](uint16_t address) { LDX(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x86] = { [this](uint16_t address) { STX(address); }, AddressMode::ZeroPage };
    instructionSet[0x96] = { [this](uint16_t address) { STX(address); }, AddressMode::ZeroPageYIndexed };
    instructionSet[0x8E] = { [this](uint16_t address) { STX(address); }, AddressMode::Absolute };
    instructionSet[0xA0] = { [this](uint16_t address) { LDY(address); }, AddressMode::Immediate };
    instructionSet[0xA4] = { [this](uint16_t address) { LDY(address); }, AddressMode::ZeroPage };
    instructionSet[0xB4] = { [this](uint16_t address) { LDY(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xAC] = { [this](uint16_t address) { LDY(address); }, AddressMode::Absolute };
    instructionSet[0xBC] = { [this](uint16_t address) { LDY(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x84] = { [this](uint16_t address) { STY(address); }, AddressMode::ZeroPage };
    instructionSet[0x94] = { [this](uint16_t address) { STY(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x8C] = { [this](uint16_t address) { STY(address); }, AddressMode::Absolute };

    // Transfer
    instructionSet[0xAA] = { [this](uint16_t) { TAX(); }, AddressMode::Implied };
    instructionSet[0x8A] = { [this](uint16_t) { TXA(); }, AddressMode::Implied };
    instructionSet[0xA8] = { [this](uint16_t) { TAY(); }, AddressMode::Implied };
    instructionSet[0x98] = { [this](uint16_t) { TYA(); }, AddressMode::Implied };

    // Arithmetic
    instructionSet[0x69] = { [this](uint16_t address) { ADC(address); }, AddressMode::Immediate };
    instructionSet[0x65] = { [this](uint16_t address) { ADC(address); }, AddressMode::ZeroPage };
    instructionSet[0x75] = { [this](uint16_t address) { ADC(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x6D] = { [this](uint16_t address) { ADC(address); }, AddressMode::Absolute };
    instructionSet[0x7D] = { [this](uint16_t address) { ADC(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x79] = { [this](uint16_t address) { ADC(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x61] = { [this](uint16_t address) { ADC(address); }, AddressMode::IndexedIndirect };
    instructionSet[0x71] = { [this](uint16_t address) { ADC(address); }, AddressMode::IndirectIndexed };
    instructionSet[0xE9] = { [this](uint16_t address) { SBC(address); }, AddressMode::Immediate };
    instructionSet[0xE5] = { [this](uint16_t address) { SBC(address); }, AddressMode::ZeroPage };
    instructionSet[0xF5] = { [this](uint16_t address) { SBC(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xED] = { [this](uint16_t address) { SBC(address); }, AddressMode::Absolute };
    instructionSet[0xFD] = { [this](uint16_t address) { SBC(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0xF9] = { [this](uint16_t address) { SBC(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0xE1] = { [this](uint16_t address) { SBC(address); }, AddressMode::IndexedIndirect };
    instructionSet[0xF1] = { [this](uint16_t address) { SBC(address); }, AddressMode::IndirectIndexed };
    instructionSet[0xE6] = { [this](uint16_t address) { INC(address); }, AddressMode::ZeroPage };
    instructionSet[0xF6] = { [this](uint16_t address) { INC(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xEE] = { [this](uint16_t address) { INC(address); }, AddressMode::Absolute };
    instructionSet[0xFE] = { [this](uint16_t address) { INC(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0xC6] = { [this](uint16_t address) { DEC(address); }, AddressMode::ZeroPage };
    instructionSet[0xD6] = { [this](uint16_t address) { DEC(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xCE] = { [this](uint16_t address) { DEC(address); }, AddressMode::Absolute };
    instructionSet[0xDE] = { [this](uint16_t address) { DEC(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0xE8] = { [this](uint16_t) { INX(); }, AddressMode::Implied };
    instructionSet[0xCA] = { [this](uint16_t) { DEX(); }, AddressMode::Implied };
    instructionSet[0xC8] = { [this](uint16_t) { INY(); }, AddressMode::Implied };
    instructionSet[0x88] = { [this](uint16_t) { DEY(); }, AddressMode::Implied };

    // Shift
    instructionSet[0x0A] = { [this](uint16_t address) { ASL(address); }, AddressMode::Accumulator };
    instructionSet[0x06] = { [this](uint16_t address) { ASL(address); }, AddressMode::ZeroPage };
    instructionSet[0x16] = { [this](uint16_t address) { ASL(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x0E] = { [this](uint16_t address) { ASL(address); }, AddressMode::Absolute };
    instructionSet[0x1E] = { [this](uint16_t address) { ASL(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x4A] = { [this](uint16_t address) { LSR(address); }, AddressMode::Accumulator };
    instructionSet[0x46] = { [this](uint16_t address) { LSR(address); }, AddressMode::ZeroPage };
    instructionSet[0x56] = { [this](uint16_t address) { LSR(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x4E] = { [this](uint16_t address) { LSR(address); }, AddressMode::Absolute };
    instructionSet[0x5E] = { [this](uint16_t address) { LSR(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x2A] = { [this](uint16_t address) { ROL(address); }, AddressMode::Accumulator };
    instructionSet[0x26] = { [this](uint16_t address) { ROL(address); }, AddressMode::ZeroPage };
    instructionSet[0x36] = { [this](uint16_t address) { ROL(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x2E] = { [this](uint16_t address) { ROL(address); }, AddressMode::Absolute };
    instructionSet[0x3E] = { [this](uint16_t address) { ROL(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x6A] = { [this](uint16_t address) { ROR(address); }, AddressMode::Accumulator };
    instructionSet[0x66] = { [this](uint16_t address) { ROR(address); }, AddressMode::ZeroPage };
    instructionSet[0x76] = { [this](uint16_t address) { ROR(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x6E] = { [this](uint16_t address) { ROR(address); }, AddressMode::Absolute };
    instructionSet[0x7E] = { [this](uint16_t address) { ROR(address); }, AddressMode::AbsoluteXIndexed };

    // Bitwise
    instructionSet[0x29] = { [this](uint16_t address) { AND(address); }, AddressMode::Immediate };
    instructionSet[0x25] = { [this](uint16_t address) { AND(address); }, AddressMode::ZeroPage };
    instructionSet[0x35] = { [this](uint16_t address) { AND(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x2D] = { [this](uint16_t address) { AND(address); }, AddressMode::Absolute };
    instructionSet[0x3D] = { [this](uint16_t address) { AND(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x39] = { [this](uint16_t address) { AND(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x21] = { [this](uint16_t address) { AND(address); }, AddressMode::IndexedIndirect };
    instructionSet[0x31] = { [this](uint16_t address) { AND(address); }, AddressMode::IndirectIndexed };
    instructionSet[0x09] = { [this](uint16_t address) { ORA(address); }, AddressMode::Immediate };
    instructionSet[0x05] = { [this](uint16_t address) { ORA(address); }, AddressMode::ZeroPage };
    instructionSet[0x15] = { [this](uint16_t address) { ORA(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x0D] = { [this](uint16_t address) { ORA(address); }, AddressMode::Absolute };
    instructionSet[0x1D] = { [this](uint16_t address) { ORA(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x19] = { [this](uint16_t address) { ORA(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x01] = { [this](uint16_t address) { ORA(address); }, AddressMode::IndexedIndirect };
    instructionSet[0x11] = { [this](uint16_t address) { ORA(address); }, AddressMode::IndirectIndexed };
    instructionSet[0x49] = { [this](uint16_t address) { EOR(address); }, AddressMode::Immediate };
    instructionSet[0x45] = { [this](uint16_t address) { EOR(address); }, AddressMode::ZeroPage };
    instructionSet[0x55] = { [this](uint16_t address) { EOR(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0x4D] = { [this](uint16_t address) { EOR(address); }, AddressMode::Absolute };
    instructionSet[0x5D] = { [this](uint16_t address) { EOR(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0x59] = { [this](uint16_t address) { EOR(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0x41] = { [this](uint16_t address) { EOR(address); }, AddressMode::IndexedIndirect };
    instructionSet[0x51] = { [this](uint16_t address) { EOR(address); }, AddressMode::IndirectIndexed };
    instructionSet[0x24] = { [this](uint16_t address) { BIT(address); }, AddressMode::ZeroPage };
    instructionSet[0x2C] = { [this](uint16_t address) { BIT(address); }, AddressMode::Absolute };

    // Compare
    instructionSet[0xC9] = { [this](uint16_t address) { CMP(address); }, AddressMode::Immediate };
    instructionSet[0xC5] = { [this](uint16_t address) { CMP(address); }, AddressMode::ZeroPage };
    instructionSet[0xD5] = { [this](uint16_t address) { CMP(address); }, AddressMode::ZeroPageXIndexed };
    instructionSet[0xCD] = { [this](uint16_t address) { CMP(address); }, AddressMode::Absolute };
    instructionSet[0xDD] = { [this](uint16_t address) { CMP(address); }, AddressMode::AbsoluteXIndexed };
    instructionSet[0xD9] = { [this](uint16_t address) { CMP(address); }, AddressMode::AbsoluteYIndexed };
    instructionSet[0xC1] = { [this](uint16_t address) { CMP(address); }, AddressMode::IndexedIndirect };
    instructionSet[0xD1] = { [this](uint16_t address) { CMP(address); }, AddressMode::IndirectIndexed };
    instructionSet[0xE0] = { [this](uint16_t address) { CPX(address); }, AddressMode::Immediate };
    instructionSet[0xE4] = { [this](uint16_t address) { CPX(address); }, AddressMode::ZeroPage };
    instructionSet[0xEC] = { [this](uint16_t address) { CPX(address); }, AddressMode::Absolute };
    instructionSet[0xC0] = { [this](uint16_t address) { CPY(address); }, AddressMode::Immediate };
    instructionSet[0xC4] = { [this](uint16_t address) { CPY(address); }, AddressMode::ZeroPage };
    instructionSet[0xCC] = { [this](uint16_t address) { CPY(address); }, AddressMode::Absolute };

    // branch
    instructionSet[0x90] = { [this](uint16_t address) { BCC(address); }, AddressMode::Relative };
    instructionSet[0xB0] = { [this](uint16_t address) { BCS(address); }, AddressMode::Relative };
    instructionSet[0xF0] = { [this](uint16_t address) { BEQ(address); }, AddressMode::Relative };
    instructionSet[0xD0] = { [this](uint16_t address) { BNE(address); }, AddressMode::Relative };
    instructionSet[0x10] = { [this](uint16_t address) { BPL(address); }, AddressMode::Relative };
    instructionSet[0x30] = { [this](uint16_t address) { BMI(address); }, AddressMode::Relative };
    instructionSet[0x50] = { [this](uint16_t address) { BVC(address); }, AddressMode::Relative };
    instructionSet[0x70] = { [this](uint16_t address) { BVS(address); }, AddressMode::Relative };

    // jump
    instructionSet[0x4C] = { [this](uint16_t address) { JMP(address); }, AddressMode::Absolute };
    instructionSet[0x6C] = { [this](uint16_t address) { JMP(address); }, AddressMode::Indirect };
    instructionSet[0x20] = { [this](uint16_t address) { JSR(address); }, AddressMode::Absolute };
    instructionSet[0x60] = { [this](uint16_t) { RTS(); }, AddressMode::Implied };
    instructionSet[0x00] = { [this](uint16_t) { BRK(); }, AddressMode::Implied };
    instructionSet[0x40] = { [this](uint16_t) { RTI(); }, AddressMode::Implied };

    // stack
    instructionSet[0x48] = { [this](uint16_t) { PHA(); }, AddressMode::Implied };
    instructionSet[0x68] = { [this](uint16_t) { PLA(); }, AddressMode::Implied };
    instructionSet[0x08] = { [this](uint16_t) { PHP(); }, AddressMode::Implied };
    instructionSet[0x28] = { [this](uint16_t) { PLP(); }, AddressMode::Implied };
    instructionSet[0x9A] = { [this](uint16_t) { TXS(); }, AddressMode::Implied };
    instructionSet[0xBA] = { [this](uint16_t) { TSX(); }, AddressMode::Implied };

    // flag
    instructionSet[0x18] = { [this](uint16_t) { CLC(); }, AddressMode::Implied };
    instructionSet[0x38] = { [this](uint16_t) { SEC(); }, AddressMode::Implied };
    instructionSet[0x58] = { [this](uint16_t) { CLI(); }, AddressMode::Implied };
    instructionSet[0x78] = { [this](uint16_t) { SEI(); }, AddressMode::Implied };
    instructionSet[0xD8] = { [this](uint16_t) { CLD(); }, AddressMode::Implied };
    instructionSet[0xF8] = { [this](uint16_t) { SED(); }, AddressMode::Implied };
    instructionSet[0xB8] = { [this](uint16_t) { CLV(); }, AddressMode::Implied };

    // other
    instructionSet[0xEA] = { [this](uint16_t) { NOP(); }, AddressMode::Implied };
}

// Access
void CPU::LDA(uint16_t address)
{
    a = read(address);
    setZNFlag(a);
}

void CPU::STA(uint16_t address)
{
    write(address, a);
}

void CPU::LDX(uint16_t address)
{
    x = read(address);
    setZNFlag(x);
}

void CPU::STX(uint16_t address)
{
    write(address, x);
}

void CPU::LDY(uint16_t address)
{
    y = read(address);
    setZNFlag(y);
}

void CPU::STY(uint16_t address)
{
    write(address, y);
}

// Transfer
void CPU::TAX()
{
    x = a;
    setZNFlag(x);
}

void CPU::TXA()
{
    a = x;
    setZNFlag(a);
}

void CPU::TAY()
{
    y = a;
    setZNFlag(y);
}

void CPU::TYA()
{
    a = y;
    setZNFlag(a);
}

// Arithmetic
void CPU::ADC(uint16_t address)
{
    uint8_t memory = read(address);
    uint8_t result = a + memory + (CHECK_FLAG(stat, FLAG_CARRY) ? 1 : 0);
    SET_FLAG(stat, FLAG_CARRY, result > 0xFF);
    SET_FLAG(stat, FLAG_OVERFLOW, (~(a ^ memory) & (a ^ result) & 0x80));
    a = result & 0xFF;
    setZNFlag(a);
}

void CPU::SBC(uint16_t address)
{
    uint8_t memory = read(address);
    uint16_t result = a - memory - (CHECK_FLAG(stat, FLAG_CARRY) ? 0 : 1);
    SET_FLAG(stat, FLAG_CARRY, result < 0x100);
    SET_FLAG(stat, FLAG_OVERFLOW, ((a ^ result) & (a ^ memory) & 0x80));
    a = result & 0xFF;
    setZNFlag(a);
}

void CPU::INC(uint16_t address)
{
    uint8_t result = read(address) + 1;
    write(address, result);
    setZNFlag(result);
}

void CPU::DEC(uint16_t address)
{
    uint8_t result = read(address) - 1;
    write(address, result);
    setZNFlag(result);
}

void CPU::INX()
{
    setZNFlag(++x);
}

void CPU::DEX()
{
    setZNFlag(--x);
}

void CPU::INY()
{
    setZNFlag(++y);
}

void CPU::DEY()
{
    setZNFlag(--y);
}

// Shift
void CPU::ASL(uint16_t address)
{
    if (address == -1)
    {
        SET_FLAG(stat, FLAG_CARRY, a & 0x80);
        a <<= 1;
        setZNFlag(a);
    }
    else
    {
        uint8_t value = read(address);
        SET_FLAG(stat, FLAG_CARRY, value & 0x80);
        write(address, value <<= 1);
        setZNFlag(value);
    }
}

void CPU::LSR(uint16_t address)
{
    if (address == -1)
    {
        SET_FLAG(stat, FLAG_CARRY, a & 0x01);
        a >>= 1;
        setZNFlag(a);
    }
    else
    {
        uint8_t value = read(address);
        SET_FLAG(stat, FLAG_CARRY, value & 0x01);
        write(address, value >>= 1);
        setZNFlag(value);
    }
}

void CPU::ROL(uint16_t address)
{
    if (address == -1)
    {
        uint8_t carry = CHECK_FLAG(stat, FLAG_CARRY) ? 1 : 0;
        SET_FLAG(stat, FLAG_CARRY, a & 0x80);
        a = (a << 1) | carry;
        setZNFlag(a);
    }
    else
    {
        uint8_t value = read(address);
        uint8_t carry = CHECK_FLAG(stat, FLAG_CARRY) ? 1 : 0;
        SET_FLAG(stat, FLAG_CARRY, value & 0x80);
        write(address, value = (value << 1) | carry);
        setZNFlag(value);
    }
}

void CPU::ROR(uint16_t address)
{
    if (address == -1)
    {
        uint8_t carry = CHECK_FLAG(stat, FLAG_CARRY) ? 0x80 : 0;
        SET_FLAG(stat, FLAG_CARRY, a & 0x01);
        a = (a >> 1) | carry;
        setZNFlag(a);
    }
    else
    {
        uint8_t value = read(address);
        uint8_t carry = CHECK_FLAG(stat, FLAG_CARRY) ? 0x80 : 0;
        SET_FLAG(stat, FLAG_CARRY, value & 0x01);
        write(address, value = (value >> 1) | carry);
        setZNFlag(value);
    }
}

void CPU::AND(uint16_t address)
{
    a &= read(address);
    setZNFlag(a);
}

void CPU::ORA(uint16_t address)
{
    a |= read(address);
    setZNFlag(a);
}

void CPU::EOR(uint16_t address)
{
    a ^= read(address);
    setZNFlag(a);
}

void CPU::BIT(uint16_t address)
{
    int8_t value = read(address);
    SET_FLAG(stat, FLAG_ZERO, (a & value) == 0);
    SET_FLAG(stat, FLAG_NEGATIVE, value & 0x80);
    SET_FLAG(stat, FLAG_OVERFLOW, value & 0x40);
}

// Compare
void CPU::CMP(uint16_t address)
{
    uint8_t value = read(address);
    SET_FLAG(stat, FLAG_CARRY, a >= value);
    setZNFlag(a - value);
}

void CPU::CPX(uint16_t address)
{
    uint8_t value = read(address);
    SET_FLAG(stat, FLAG_CARRY, x >= value);
    setZNFlag(x - value);
}

void CPU::CPY(uint16_t address)
{
    uint8_t value = read(address);
    SET_FLAG(stat, FLAG_CARRY, y >= value);
    setZNFlag(y - value);
}

// Branch
void CPU::BCC(uint16_t address)
{
    if (!CHECK_FLAG(stat, FLAG_CARRY))
        pc = address;
}

void CPU::BCS(uint16_t address)
{
    if (CHECK_FLAG(stat, FLAG_CARRY))
        pc = address;
}

void CPU::BEQ(uint16_t address)
{
    if (CHECK_FLAG(stat, FLAG_ZERO))
        pc = address;
}

void CPU::BNE(uint16_t address)
{
    if (!CHECK_FLAG(stat, FLAG_ZERO))
        pc = address;
}

void CPU::BPL(uint16_t address)
{
    if (!CHECK_FLAG(stat, FLAG_NEGATIVE))
        pc = address;
}
void CPU::BMI(uint16_t address)
{
    if (CHECK_FLAG(stat, FLAG_NEGATIVE))
        pc = address;
}

void CPU::BVC(uint16_t address)
{
    if (!CHECK_FLAG(stat, FLAG_OVERFLOW))
        pc = address;
}

void CPU::BVS(uint16_t address)
{
    if (CHECK_FLAG(stat, FLAG_OVERFLOW))
        pc = address;
}

void CPU::JMP(uint16_t address)
{
    pc = address;
}

void CPU::JSR(uint16_t address)
{
    write(0x100 + sp--, (pc - 1) >> 8);
    write(0x100 + sp--, (pc - 1) & 0xFF);
    pc = address;
}

void CPU::RTS()
{
    pc = (read(++sp + 0x100) | (read(++sp + 0x100) << 8)) + 1;
}

void CPU::BRK()
{
    write(0x100 + sp--, (pc - 1) >> 8);
    write(0x100 + sp--, (pc - 1) & 0xFF);
    write(0x100 + sp--, stat | (1 << 4) | (1 << 5));
    pc = read(0xFFFE) | (read(0xFFFF) << 8);
    SET_FLAG(stat, FLAG_INTERRUPT, true);
}

void CPU::RTI()
{
    stat = read(0x0100 + ++sp);
    pc = read(0x0100 + ++sp) | (read(0x0100 + ++sp) << 8);
}

// Stack
void CPU::PHA()
{
    write(0x100 + sp--, a);
}

void CPU::PLA()
{
    a = read(++sp + 0x100);
    setZNFlag(a);
}

void CPU::PHP()
{
    write(0x100 + sp--, stat | (1 << 4) | (1 << 5));
}

void CPU::PLP()
{
    stat = read(++sp + 0x100);
}

void CPU::TXS()
{
    sp = x;
}

void CPU::TSX()
{
    x = sp;
    setZNFlag(x);
}

// Flag
void CPU::CLC()
{
    SET_FLAG(stat, FLAG_CARRY, false);
}

void CPU::SEC()
{
    SET_FLAG(stat, FLAG_CARRY, true);
}

void CPU::CLI()
{
    SET_FLAG(stat, FLAG_INTERRUPT, false);
}

void CPU::SEI()
{
    SET_FLAG(stat, FLAG_INTERRUPT, true);
}

void CPU::CLD()
{
    SET_FLAG(stat, FLAG_DECIMAL, false);
}

void CPU::SED()
{
    SET_FLAG(stat, FLAG_DECIMAL, true);
}

void CPU::CLV()
{
    SET_FLAG(stat, FLAG_OVERFLOW, false);
}

// Other
void CPU::NOP() {}

// Debug
void CPU::debugStack()
{
    std::cout << "SP: 0x" << std::hex << +sp << std::endl;
    for (uint16_t i = sp + 1; i <= 0xFF; ++i)
    {
        std::cout << "Stack[0x" << std::hex << (0x100 + i) << "] = 0x" << +memory[0x100 + i] << "\n";
    }
    std::cout << "-----------------------------" << std::endl;
}
#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

enum class AddressMode
{ // 설명 (명령어 길이)
    Implied,
    Accumulator,
    Immediate,
    ZeroPage,         // 첫 번째 페이지(0 ~ 0xFF) 참조 (2바이트)
    ZeroPageXIndexed, // 첫 번째 페이지(0 ~ 0xFF) + X (2바이트)
    ZeroPageYIndexed, // 첫 번째 페이지(0 ~ 0xFF) + Y (2바이트)
    Absolute,         // 16비트(64KB) 주소           (3바이트)
    AbsoluteXIndexed, // 16비트(64KB) 주소 + X       (3바이트)
    AbsoluteYIndexed, // 16비트(64KB) 주소 + Y       (3바이트)
    Indirect,         // 간접 주소(포인터)             (3바이트)
    IndexedIndirect,  // Zero Page 주소 + X -> 간접 주소 (2바이트)
    IndirectIndexed,  // Zero Page 주소 -> 간접 주소 + Y (2바이트)
    Relative
};

struct Instruction
{
    std::function<void(uint16_t)> operation;
    AddressMode mode;
};

class CPU
{
public:
    uint8_t a = 0x00;  // Accumulator
    uint8_t x = 0x00;  // X Register
    uint8_t y = 0x00;  // Y Register
    uint8_t sp = 0xFF; // Stack Pointer

    uint8_t stat = 0x00;  // Processor Status Register
    uint16_t pc = 0x0000; // Program Counter

    std::vector<uint8_t> memory;                             // 64KB 메모리
    std::unordered_map<uint8_t, Instruction> instructionSet; // Opcode 테이블

    CPU();

    uint8_t read(uint16_t address);
    uint16_t read16(uint16_t address, bool wrapAround);
    void write(uint16_t address, uint8_t value);
    void execute();
    uint8_t fetch();
    uint16_t fetchAbsolute();
    uint8_t fetchZeroPage(uint8_t offset);
    uint16_t fetchAddress(AddressMode mode);
    void setZNFlag(uint8_t value);

    /* Instruction set */
    void setupInstructionSet();

    // Access
    void LDA(uint16_t address);
    void STA(uint16_t address);
    void LDX(uint16_t address);
    void STX(uint16_t address);
    void LDY(uint16_t address);
    void STY(uint16_t address);

    // Transfer
    void TAX();
    void TXA();
    void TAY();
    void TYA();

    // Arithmetic
    void ADC(uint16_t address);
    void SBC(uint16_t address);
    void INC(uint16_t address);
    void DEC(uint16_t address);
    void INX();
    void DEX();
    void INY();
    void DEY();

    // Shift
    void ASL(uint16_t address);
    void LSR(uint16_t address);
    void ROL(uint16_t address);
    void ROR(uint16_t address);

    // Bitwise
    void AND(uint16_t address);
    void ORA(uint16_t address);
    void EOR(uint16_t address);
    void BIT(uint16_t address);

    // compare
    void CMP(uint16_t address);
    void CPX(uint16_t address);
    void CPY(uint16_t address);

    // branch
    void BCC(uint16_t address);
    void BCS(uint16_t address);
    void BEQ(uint16_t address);
    void BNE(uint16_t address);
    void BPL(uint16_t address);
    void BMI(uint16_t address);
    void BVC(uint16_t address);
    void BVS(uint16_t address);

    // jump
    void JMP(uint16_t address);
    void JSR(uint16_t address);
    void RTS();
    void BRK();
    void RTI();

    /// stack
    void PHA();
    void PLA();
    void PHP();
    void PLP();
    void TXS();
    void TSX();

    // Flag
    void CLC();
    void SEC();
    void CLI();
    void SEI();
    void CLD();
    void SED();
    void CLV();

    // Other
    void NOP();

    /* debug */
    void debugStack();
};

#endif

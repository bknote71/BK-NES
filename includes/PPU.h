#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <functional>
#include <vector>

enum PipelineState
{
    PreRender,
    VisibleRender,
    PostRender,
    VBlank,
};

/**
 * NES PPU 하드웨어 특성상 렌더링 중에 VRAM/OAM을 쓰는 행위는 매우 위험
 * - CPU는 이론상 언제든 IO 레지스터에 쓰기 가능하지만,
 * - 안정적인 그래픽 출력을 위해서는 vblank 중(또는 렌더링 꺼진 상태)에 PPU 관련 IO 레지스터 접근을 하는 것이 관례.
 */

class PPU
{
public:
    // PPUCTRL
    uint16_t baseNTAddr;  // 베이스 네임테이블주소 (0, 1, 2, 3)
    uint8_t vIncrement;   // vram 주소 증가량 (0: 1, 1: 32)
    uint16_t sprPTAddr;   // 스프라이트 패턴테이블 주소 (0: $0000, 1: $1000) (8x8 일때 유효)
    uint16_t bgPTAddr;    // 배경 패턴 테이블 주소 (0: $0000, 1: $1000)
    uint8_t sprSize;      // 스프라이트 크기 (0: 8x8, 1: 8x16)
    bool masterSlave;     // EXT 핀 관련
    bool enableVblankNMI; // vblank NMI enable

    // PPUMASK: 렌더링 제어 및 컬러 효과 제어
    bool graycale;
    bool showBgInLeftmost;
    bool showSprInLeftmost;
    bool enableBgRendering;
    bool enableSprRendering;
    uint8_t emphasizeRGB;

    // PPUSTATUS
    bool spriteOverflow;
    bool sprZeroHit; // 특정 픽셀에서 스프라이트0과 배경이 겹치면 set.
    bool vblankFlag;

    // OAM
    uint8_t oamAddr; // 스프라이트 evaluation 시작 주소
    std::vector<uint32_t> oam;
    std::vector<uint8_t> soam;
    std::vector<uint8_t> sprShifters;

    // Internal registers
    uint16_t v; // [14-12]: fine Y, [11-10]: nametable, [9-5]: coarseY(타일 y축 위치), [4-0]: coarseX(타일 x축 위치)
    uint16_t t; // for setting scroll value
    uint8_t x;  // fine X scroll
    bool w;     // first or second write toggle

    uint32_t cycle;
    uint32_t scanline;
    bool oddFrame;

    uint16_t bgShifterLow;
    uint16_t bgShifterHigh;
    uint8_t bgPaletteShifter;

    PipelineState pipelineState;

    std::vector<uint8_t> palette;
    std::vector<std::vector<uint32_t>> pBuffer;

    // vblank
    std::function<void(void)> vblankNMI;

    // methods

    // IORegisters (called by CPU)
    void setPPUCtrl(uint8_t ctrl);
    void setPPUMask(uint8_t mask);
    void setOAMAddr(uint8_t oamAddr);
    void setOAMData(uint8_t data);
    void setPPUSCroll(uint8_t scroll);
    void setPPUAddr(uint8_t addr);
    void setPPUData(uint8_t data);

    uint8_t getPPUStatus();

    uint8_t read(uint16_t address);

    void render();
    void preRender();
    void visibleRender();
    void postRender();
    void vblank();

    void renderPixel();
    void renderBackgroundPixel(uint8_t &bgPixel, bool &bgOpaque);
    void renderSpritePixel(int x, int y, bool bgOpaque, uint8_t &sprPixel, bool &sprOpaque, bool &sprForeground);
    uint8_t compositePixel(int x, int y, uint8_t bgPixel, uint8_t sprPixel, bool bgOpaque, bool sprOpaque, bool sprForeground);

    void clearFlags();

    // fetch
    uint8_t fetchPatternTablePixelData(uint16_t ptAddr, uint8_t tileX);
    uint8_t fetchNameTableData();
    uint8_t fetchAttributeTableData(int tileX = -1, int tileY = -1);

    uint8_t fetchPatternTableLow(uint8_t tile, uint8_t tileX);
    uint8_t fetchPatternTableHigh(uint8_t tile, uint8_t tileX);

    void loadBgShiftersForNextScanline();
    void loadNextTileIntoShifters();

    void evaluateSprites(int y);
    void fetchSpriteTitleByte();
    void fetchSpriteAttributeByte();
    void fetchSpritePatternLow();
    void fetchSpritePatternHigh();

    void incrementHoriV();
    void incrementVertV();
    void resetHorizontalScroll();
    void resetVerticalScroll();
};

#endif
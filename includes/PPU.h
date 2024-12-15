#ifndef PPU_H
#define PPU_H

#include <cstdint>

enum PipelineState
{
    PreRender,
    VisibleRender,
    PostRender,
};

class PPU
{
public:
    // [14-12]: fine Y, [11-10]: nametable, [9-5]: coarseY(타일 y축 위치), [4-0]: coarseX(타일 x축 위치)
    uint16_t v;
    uint16_t t;
    uint8_t bgPage;
    uint8_t fineX; // pixel 단위의 세밀한 수평 스크롤(0~7)

    uint32_t cycle;
    uint32_t scanline;

    PipelineState pipelineState;

    uint8_t read(uint16_t address);

    void render();
    void preRender();
    void visibleRender();
    void postRender();

    void renderPixel();
    void fetchBackgroundPixel(int x, int y, uint8_t &bgPixel, bool &bgOpaque);
    void fetchSpritePixel(int x, int y, uint8_t &sprPixel, bool &sprOpaque, bool &sprForeground);
    void compositePixel(int x, int y, uint8_t &bgPixel, uint8_t &sprPixel, bool &bgOpaque, bool &sprOpaque, bool &sprForeground);

    void clearFlags();
    uint8_t fetchNameTableData();
    uint8_t fetchAttributeTableData();
    uint8_t fetchPatternTableLow(uint8_t tile, uint8_t fine_x);
    uint8_t fetchPatternTableHigh(uint8_t tile, uint8_t fine_x);

    void evaluateSpritesForNextScanline();
    void fetchSpriteTitleByte();
    void fetchSpriteAttributeByte();
    void fetchSpritePatternLow();
    void fetchSpritePatternHigh();

    void incrementHoriV();
    void incrementVertV();
    void resetHorizontalScroll();
};

#endif
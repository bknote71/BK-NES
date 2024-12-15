#include "PPU.h"

static const int scanlineVisibleCycle = 256;
static const int scanlineEndCycle = 256;

void PPU::render()
{
    switch (pipelineState)
    {
    case PreRender: preRender(); break;
    case VisibleRender: visibleRender(); break;

    default: break;
    }
    ++cycle;
}

void PPU::preRender()
{
    if (cycle == 1)
    {
        clearFlags();
    }

    // TODO
}

void PPU::visibleRender()
{
    // idle
    if (cycle == 0)
        return;

    if (cycle <= scanlineVisibleCycle)
        renderPixel();
    else if (cycle == scanlineVisibleCycle + 1)
        resetHorizontalScroll();
    else if (cycle >= scanlineVisibleCycle + 2 && cycle <= 320)
        evaluateSpritesForNextScanline();
    else if (cycle >= 321 && cycle <= 336)
        return; // TODO
    else if (cycle <= scanlineEndCycle)
        fetchNameTableData();
}

void PPU::renderPixel()
{
    uint8_t bgPixel = 0;
    uint8_t sprPixel = 0;

    bool bgOpaque = false;
    bool sprOpaque = false;
    bool sprForeground = false;

    int x = cycle - 1;
    int y = scanline;

    fetchBackgroundPixel(x, y, bgPixel, bgOpaque);
    fetchSpritePixel(x, y, sprPixel, sprOpaque, sprForeground);
    compositePixel(x, y, bgPixel, sprPixel, bgOpaque, sprOpaque, sprForeground);
}

void PPU::fetchBackgroundPixel(int x, int y, uint8_t &bgPixel, bool &bgOpaque)
{
    uint8_t fine_x = (this->fineX + x) & 0x07;
    uint8_t tile = fetchNameTableData();
    uint8_t palette = fetchAttributeTableData();
    uint8_t plow = fetchPatternTableLow(tile, fine_x);
    uint8_t phigh = fetchPatternTableHigh(tile, fine_x);

    uint8_t tilePixel = (phigh << 1 | plow) & 0x03;
    bgPixel = ((palette & 0x03) << 2) | tilePixel;
    bgOpaque = bgPixel != 0;

    if (fine_x == 7)
        incrementHoriV();
}

void PPU::fetchSpritePixel(int x, int y, uint8_t &sprPixel, bool &sprOpaque, bool &sprForeground) {}
void PPU::compositePixel(int x, int y, uint8_t &bgPixel, uint8_t &sprPixel, bool &bgOpaque, bool &sprOpaque, bool &sprForeground) {}

uint8_t PPU::fetchNameTableData()
{
    uint16_t ntAddr = 0x2000 | (v & 0x0FFF);
    return read(ntAddr);
}

uint8_t PPU::fetchAttributeTableData()
{
    uint16_t attrBase = 0x23C0 | (v & 0x0C00);
    uint16_t coarseY = (v >> 5) & 0x001F; // attrtable에서의 타일 y축 위치 (32(30)개를 8개에 매핑. 즉 상위 3개비트만)
    uint16_t coarseX = v & 0x001F;        // attrtable에서의 타일 x축 위치 (32개를 8개에 매핑. 즉 상위 3개비트만)
    uint16_t atAddr = attrBase | ((coarseY & 0x001C) << 1) | ((coarseX >> 2) & 0x07);
    return read(atAddr);
}

/*
 * 예) Tile ID 0:
 * - Low Plane:  8 bytes @ $0000 - $0007
 * 0 High Plane: 8 bytes @ $0008 - $000F
 */
uint8_t PPU::fetchPatternTableLow(uint8_t tile, uint8_t fine_x)
{
    uint16_t ptBase = bgPage << 12; // 0x0000 or 0x1000
    uint16_t fineY = (v >> 12) & 0x07;
    uint16_t ptAddr = ptBase + tile * 16 + fineY;
    return (read(ptAddr) >> (7 - fine_x)) & 0x01;
}

uint8_t PPU::fetchPatternTableHigh(uint8_t tile, uint8_t fine_x)
{
    uint16_t ptBase = bgPage << 12;
    uint16_t fineY = (v >> 12) & 0x07;
    uint16_t ptAddr = ptBase + tile * 16 + fineY + 8;
    return (read(ptAddr) >> (7 - fine_x)) & 0x01;
}

/*
 * increment coarse X (타일 x값 증가)
 *
 * 만약 coarse X가 31(0x1F) 이라면 끝에 다다른다면?
 * - X to 0
 * - 수평 네임 테이블 토글 (^=0x0400)
 *
 * 참고: hori(v): 타일 단위의 수평 스크롤
 */
void PPU::incrementHoriV()
{
    if (v & 0x001F == 0x001F)
    {
        v &= ~0x001F;
        v ^= 0x0400;
        return;
    }

    v += 1;
}

void PPU::resetHorizontalScroll()
{
    v &= ~0x041F;
    v |= t & 0x041F;
}
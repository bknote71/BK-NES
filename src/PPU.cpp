#include "PPU.h"

static const int visibleCycle = 256;
static const int endCycle = 256;

// temp
static const uint32_t colors[] = {
    0x666666ff, 0x002a88ff, 0x1412a7ff, 0x3b00a4ff, 0x5c007eff, 0x6e0040ff, 0x6c0600ff, 0x561d00ff, 0x333500ff, 0x0b4800ff, 0x005200ff,
    0x004f08ff, 0x00404dff, 0x000000ff, 0x000000ff, 0x000000ff, 0xadadadff, 0x155fd9ff, 0x4240ffff, 0x7527feff, 0xa01accff, 0xb71e7bff,
    0xb53120ff, 0x994e00ff, 0x6b6d00ff, 0x388700ff, 0x0c9300ff, 0x008f32ff, 0x007c8dff, 0x000000ff, 0x000000ff, 0x000000ff, 0xfffeffff,
    0x64b0ffff, 0x9290ffff, 0xc676ffff, 0xf36affff, 0xfe6eccff, 0xfe8170ff, 0xea9e22ff, 0xbcbe00ff, 0x88d800ff, 0x5ce430ff, 0x45e082ff,
    0x48cddeff, 0x4f4f4fff, 0x000000ff, 0x000000ff, 0xfffeffff, 0xc0dfffff, 0xd3d2ffff, 0xe8c8ffff, 0xfbc2ffff, 0xfec4eaff, 0xfeccc5ff,
    0xf7d8a5ff, 0xe4e594ff, 0xcfef96ff, 0xbdf4abff, 0xb3f3ccff, 0xb5ebf2ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

void PPU::setPPUCtrl(uint8_t ctrl)
{
    baseNTAddr = 0x2000 + ((ctrl & 0x03) << 10);
    vIncrement = ctrl & 0x04;
    sprPTAddr = ((ctrl >> 3) & 0x01) << 12;
    bgPTAddr = ((ctrl >> 4) & 0x01) << 12;
    sprSize = (ctrl & 0x20) ? 16 : 8;
    enableVblankNMI = ctrl & 0x80;
}

void PPU::setPPUMask(uint8_t mask)
{
    graycale = mask & 0x01;
    showBgInLeftmost = mask & 0x02;
    showSprInLeftmost = mask & 0x04;
    enableBgRendering = mask & 0x08;
    enableSprRendering = mask & 0x10;
    emphasizeRGB = (mask >> 5) & 0x07;
}

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

    if (cycle <= visibleCycle)
        renderPixel();
    else if (cycle == visibleCycle + 1)
        resetHorizontalScroll();
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

    if (enableBgRendering)
    {
        int fine_x = (this->x + x) & 0x07;
        if (x > 7) // (cycle 9이상일 때부터) 1사이클 당 1픽셀 출력 가능
            renderBackgroundPixel(fine_x, y, bgPixel, bgOpaque);

        if (fine_x == 7)
            incrementHoriV();
    }

    if (enableSprRendering && x > 7)
    {
        renderSpritePixel(x, y, bgOpaque, sprPixel, sprOpaque, sprForeground);
    }

    // palette의 idx (< 32)
    uint8_t pixel = compositePixel(x, y, bgPixel, sprPixel, bgOpaque, sprOpaque, sprForeground);
    pBuffer[x][y] = colors[palette[pixel]];
}

void PPU::renderBackgroundPixel(int x, int y, uint8_t &bgPixel, bool &bgOpaque)
{
    uint8_t tile = fetchNameTableData();
    uint8_t palette = fetchAttributeTableData();
    uint8_t ptLow = fetchPatternTableLow(tile, x);
    uint8_t ptHigh = fetchPatternTableHigh(tile, x);
    uint8_t tilePixel = (ptHigh << 1 | ptLow) & 0x03;

    bgPixel = ((palette & 0x03) << 2) | tilePixel;
    bgOpaque = bgPixel != 0;
}

void PPU::renderSpritePixel(int x, int y, bool bgOpaque, uint8_t &sprPixel, bool &sprOpaque, bool &sprForeground)
{
    /**
     * spr:
     * - [31:24] X좌표
     * - [23:16] 속성(attribute) (하위 2 비트: 팔레트, 그 외 비트: 수직/수평 플립, 우선순위)
     * - [15:8]  타일번호(tile)
     * - [7:0]   Y좌표
     */
    for (uint8_t sprIdx : secondaryOAM)
    {
        uint32_t spr = oam[sprIdx];
        uint8_t spry = (uint8_t)((spr >> 0) & 0xFF);
        uint8_t tile = (uint8_t)((spr >> 8) & 0xFF);
        uint8_t attr = (uint8_t)((spr >> 16) & 0xFF);
        uint8_t sprx = (uint8_t)((spr >> 24) & 0xFF);

        int xOffset = x - sprx;
        int yOffset = y - spry;

        if (xOffset < 0 || xOffset > 7)
            continue;
        if (yOffset < 0 || yOffset > sprSize)
            continue;

        bool flipVertical = attr & 0x80;
        bool flipHorizontal = attr & 0x40;
        bool behindBG = attr & 0x20; // Priority
        uint8_t paletteIdx = attr & 0x03;

        int tileY = flipVertical ? (sprSize - 1 - yOffset) : yOffset;
        int tileX = flipHorizontal ? xOffset : (7 - xOffset);

        uint16_t tilePTAddr;
        if (sprSize == 16) // 8x16 스프라이트 처리
        {
            // TODO
        }
        else
            tilePTAddr = sprPTAddr + (tile * 16) + tileY;

        // fetch pt low-plain / high-plain
        uint8_t pixel = fetchPatternTablePixelData(tilePTAddr, tileX);

        if (!(sprOpaque = sprPixel))
            continue;

        sprPixel = 0x10 | (paletteIdx << 2) | (pixel & 0x03);
        sprForeground = !behindBG;

        // set sprite zero hit flag
        // - true: spr가 0이고 현재 pixel이 0이 아니어야 함 + 배경 역시 불투명하고 렌더링 가능해야 함.
        if (sprIdx == 0 && bgOpaque && enableBgRendering)
            sprZeroHit = true;

        // priority: 가장 낮은 인덱스를 그린다.
        return;
    }
}

uint8_t PPU::compositePixel(int x, int y, uint8_t bgPixel, uint8_t sprPixel, bool bgOpaque, bool sprOpaque, bool sprForeground)
{
    if (!bgOpaque && !sprOpaque)
        return 0;

    if (!bgOpaque && sprOpaque)
        return sprPixel;

    if (bgOpaque && !sprOpaque)
        return bgOpaque;

    if (sprForeground)
        return sprPixel;
    return bgPixel;
}

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
uint8_t PPU::fetchPatternTablePixelData(uint16_t ptAddr, uint8_t tileX)
{
    // uint16_t ptAddr = bgPTAddr + tile * 16 + tileY;
    uint8_t ptLow = read(ptAddr);
    uint8_t ptHigh = read(ptAddr + 8);
    uint8_t lowBit = ((ptLow >> tileX) & 1) & 0x01;
    uint8_t highBit = ((ptLow >> tileX) & 1) & 0x01;
    return (highBit << 1) | lowBit;
}

uint8_t PPU::fetchPatternTableLow(uint8_t tile, uint8_t tileX)
{
    uint16_t fineY = (v >> 12) & 0x07;
    uint16_t ptAddr = bgPTAddr + tile * 16 + fineY;
    return (read(ptAddr) >> (7 - tileX)) & 0x01;
}

uint8_t PPU::fetchPatternTableHigh(uint8_t tile, uint8_t tileX)
{
    uint16_t fineY = (v >> 12) & 0x07;
    uint16_t ptAddr = bgPTAddr + tile * 16 + fineY + 8;
    return (read(ptAddr) >> (7 - tileX)) & 0x01;
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
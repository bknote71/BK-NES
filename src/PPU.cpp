#include "PPU.h"

static const int visibleCycle = 256;
static const int endCycle = 340;
static const int visibleScanlines = 240;

// temp
static const uint32_t colors[] = {
    0x666666ff, 0x002a88ff, 0x1412a7ff, 0x3b00a4ff, 0x5c007eff, 0x6e0040ff, 0x6c0600ff, 0x561d00ff, 0x333500ff, 0x0b4800ff, 0x005200ff,
    0x004f08ff, 0x00404dff, 0x000000ff, 0x000000ff, 0x000000ff, 0xadadadff, 0x155fd9ff, 0x4240ffff, 0x7527feff, 0xa01accff, 0xb71e7bff,
    0xb53120ff, 0x994e00ff, 0x6b6d00ff, 0x388700ff, 0x0c9300ff, 0x008f32ff, 0x007c8dff, 0x000000ff, 0x000000ff, 0x000000ff, 0xfffeffff,
    0x64b0ffff, 0x9290ffff, 0xc676ffff, 0xf36affff, 0xfe6eccff, 0xfe8170ff, 0xea9e22ff, 0xbcbe00ff, 0x88d800ff, 0x5ce430ff, 0x45e082ff,
    0x48cddeff, 0x4f4f4fff, 0x000000ff, 0x000000ff, 0xfffeffff, 0xc0dfffff, 0xd3d2ffff, 0xe8c8ffff, 0xfbc2ffff, 0xfec4eaff, 0xfeccc5ff,
    0xf7d8a5ff, 0xe4e594ff, 0xcfef96ff, 0xbdf4abff, 0xb3f3ccff, 0xb5ebf2ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

// set IORegisters (called by CPU)

void PPU::setPPUCtrl(uint8_t ctrl)
{
    baseNTAddr = 0x2000 + ((ctrl & 0x03) << 10);
    vIncrement = (ctrl & 0x04) ? 32 : 1;
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

void PPU::setOAMAddr(uint8_t oamAddr)
{
    this->oamAddr = oamAddr;
}

void PPU::setPPUSCroll(uint8_t scroll)
{
    if (!w) // set (coarseX and fineX) of t
    {
        t &= ~0x1f;
        t |= (scroll >> 3) & 0x1F;
        x = scroll & 0x07;
        w = 1;
    }
    else // set coarseY of t
    {
        // FGH..AB CDE..... <- d: ABCDEFGH (scroll)
        t &= ~0x73e0;
        t |= ((scroll & 0x07) << 12) | ((scroll & 0xF8) << 2);
        w = 0;
    }
}

void PPU::setPPUAddr(uint8_t addr)
{
    if (!w) // t의 상위 8비트 세팅 (0_CDEFGH)
    {
        t &= ~0xFF00;
        t |= (addr & 0x3F) << 8;
        w = 1;
    }
    else // t의 하위 8비트 세팅 및 v에 쓰기
    {
        t &= ~0xFF;
        t |= addr;
        v = t;
        w = 0;
    }
}

uint8_t PPU::getPPUStatus()
{
    vblankFlag = 0;
}

// rendering
void PPU::render()
{
    switch (pipelineState)
    {
    case PreRender: preRender(); break;
    case VisibleRender: visibleRender(); break;
    case PostRender: postRender(); break;
    case VBlank: vblank(); break;
    default: break;
    }
    ++cycle;
}

void PPU::preRender()
{
    if (cycle == 1)
    { // clear flags
        vblankFlag = false;
        sprZeroHit = false;
        spriteOverflow = false;
    }
    // 1 ~ 256: unused tile fetch
    else if (cycle == visibleCycle + 1)
        resetHorizontalScroll();
    else if (280 == cycle) // 280-304
        resetVerticalScroll();
    else if (cycle == 321)
        loadBgShiftersForNextScanline();
    else if (cycle >= endCycle - (oddFrame))
    {
        pipelineState = VisibleRender;
        cycle = -1;
        scanline = 0;
    }
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
    else if (cycle == visibleCycle + 2)
    {
        sprShifters = soam;
    }
    else if (cycle == 321) // 321 ~ 336
    {
        loadBgShiftersForNextScanline();
    }
    else if (cycle >= endCycle)
    {
        cycle = -1;
        if (++scanline >= visibleScanlines)
            pipelineState = PostRender;
    }
}

void PPU::postRender()
{
    if (cycle < endCycle)
        return;

    cycle = -1;
    pipelineState = VBlank;

    // virtual screen?
}

void PPU::vblank()
{
    if (cycle == 1 && scanline == 241)
    {
        vblankFlag = true;
        if (enableVblankNMI)
            vblankNMI();
    }

    if (cycle >= endCycle)
    {
        cycle = -1;

        if (++scanline >= 261)
        {
            pipelineState = PreRender;
            oddFrame = !oddFrame;
        }
    }
}

void PPU::renderPixel()
{
    if (scanline == 0 && cycle <= 16)
    {
        // 16이 될 때까지 그리지 않음
        if (cycle == 16)
            loadBgShiftersForNextScanline();
        return;
    }

    uint8_t bgPixel = 0;
    uint8_t sprPixel = 0;

    bool bgOpaque = false;
    bool sprOpaque = false;
    bool sprForeground = false;

    int x = cycle - 1;
    int y = scanline;

    if (enableBgRendering)
    {
        renderBackgroundPixel(bgPixel, bgOpaque);

        if ((x + this->x) % 8 == 7) // 8픽셀 마다 다음 타일 데이터를 쉬프터에 로드
        {
            loadNextTileIntoShifters();
        }
    }

    if (enableSprRendering) //  && x > 7
    {
        renderSpritePixel(x, y, bgOpaque, sprPixel, sprOpaque, sprForeground);
    }

    // palette의 idx (< 32)
    uint8_t pixel = compositePixel(x, y, bgPixel, sprPixel, bgOpaque, sprOpaque, sprForeground);
    pBuffer[x][y] = colors[palette[pixel]];

    // sprite evaluation
    if (cycle == 65)
        evaluateSprites(y);

    if (cycle == visibleCycle)
        incrementVertV();
}

void PPU::renderBackgroundPixel(uint8_t &bgPixel, bool &bgOpaque)
{
    // TODO: fineX 적용하기 (픽셀 단위 스크롤)
    // - 읽을 때 (7 - (x + this->x(fineX)) % 8) 값 위치를 읽어야 한다.
    uint8_t bgPixelLow = (bgShifterLow & 0x8000) ? 1 : 0;
    uint8_t bgPixelHigh = (bgShifterHigh & 0x8000) ? 1 : 0;
    uint8_t paletteIdx = (bgPaletteShifter & 0x0C) >> 2;

    bgPixel = (bgPixelHigh << 1) | bgPixelLow;
    bgPixel = (paletteIdx << 2) | bgPixel;
    bgOpaque = (bgPixel != 0);

    // shift
    bgShifterLow <<= 1;
    bgShifterHigh <<= 1;
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
    for (uint8_t sprIdx : sprShifters)
    {
        uint32_t spr = oam[sprIdx];
        uint8_t spry = static_cast<uint8_t>((spr >> 0) & 0xFF);
        uint8_t tile = static_cast<uint8_t>((spr >> 8) & 0xFF);
        uint8_t attr = static_cast<uint8_t>((spr >> 16) & 0xFF);
        uint8_t sprx = static_cast<uint8_t>((spr >> 24) & 0xFF);

        int xOffset = x - sprx;
        int yOffset = y - spry;

        if (xOffset < 0 || xOffset > 7)
            continue;
        if (yOffset < 0 || yOffset >= sprSize)
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

        if (!(sprOpaque = (pixel != 0)))
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
        return bgPixel;

    if (sprForeground)
        return sprPixel;

    return bgPixel;
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

uint8_t PPU::fetchNameTableData()
{
    uint16_t ntAddr = 0x2000 | (v & 0x0FFF);
    return read(ntAddr);
}

uint8_t PPU::fetchAttributeTableData(int tileX = -1, int tileY = -1)
{
    uint16_t attrBase = 0x23C0 | (v & 0x0C00);
    int x = (tileX != -1) ? tileX : v & 0x001F;
    int y = (tileY != -1) ? tileY : (v >> 5) & 0x001F;

    int attrX = x >> 2;
    int attrY = y >> 2;

    uint16_t blockAddr = attrBase + (attrY * 8) + attrX;
    uint8_t attrData = read(blockAddr);

    int shift = (((x & 0x02) >> 1) + ((y & 0x02) >> 1) * 2) * 2;
    uint8_t paletteIdx = (attrData >> shift) & 0x03;
    return paletteIdx;
}

void PPU::loadBgShiftersForNextScanline()
{
    uint8_t fineY = (v >> 12) & 0x07;
    uint8_t tile1 = fetchNameTableData();
    uint8_t tile2 = tile1 + 1;

    // uint8_t palette = fetchAttributeTableData();
    uint16_t tile1Addr = bgPTAddr + tile1 * 16 + fineY;
    uint16_t tile2Addr = bgPTAddr + tile2 * 16 + fineY;

    uint8_t ptLow1 = read(tile1Addr);
    uint8_t ptHigh1 = read(tile1Addr + 8);
    uint8_t ptLow2 = read(tile2Addr);
    uint8_t ptHigh2 = read(tile2Addr + 8);

    bgShifterLow = (ptLow1 << 8) | ptLow2;
    bgShifterHigh = (ptHigh1 << 8) | ptHigh2;
    bgShifterLow <<= this->x;
    bgShifterHigh <<= this->x;

    uint8_t paletteIdx1 = fetchAttributeTableData(0, fineY) & 0x03;
    uint8_t paletteIdx2 = fetchAttributeTableData(1, fineY) & 0x03;

    bgPaletteShifter = (paletteIdx1 << 2) | paletteIdx2;

    // increment hori v 2번
    incrementHoriV();
    incrementHoriV();
}

void PPU::loadNextTileIntoShifters()
{
    uint8_t tile = fetchNameTableData();
    uint8_t paletteIdx = fetchAttributeTableData() & 0x03;
    uint16_t tileAddr = bgPTAddr + tile * 16 + (v >> 12) & 0x07;

    uint8_t ptLow = read(tileAddr);
    uint8_t ptHigh = read(tileAddr + 8);

    bgShifterLow |= ptLow;
    bgShifterHigh |= ptHigh;
    bgPaletteShifter = (bgPaletteShifter << 2) | paletteIdx;

    incrementHoriV();
}

void PPU::evaluateSprites(int y)
{
    soam.resize(0);
    uint8_t cnt = 0;
    for (uint8_t sprIdx = oamAddr / 4; sprIdx < 64; ++sprIdx)
    {
        uint8_t spry = static_cast<uint8_t>(oam[sprIdx] & 0xFF);
        int yOffset = y - spry;

        if (yOffset < 0 || yOffset >= sprSize)
            continue;

        if (cnt >= 8)
        {
            spriteOverflow = true;
            break;
        }
        soam.push_back(sprIdx);
        ++cnt;
    }
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
    if ((v & 0x001F) != 0x001F)
    {
        v += 1;
        return;
    }

    v &= ~0x001F;
    v ^= 0x0400;
}

void PPU::incrementVertV()
{
    // Fine Y (bits 12-14)가 최대값(7)이 아닌 경우 Fine Y를 증가시킴
    if ((v & 0x7000) != 0x7000)
    {
        v += 0x1000;
        return;
    }

    v &= ~0x7000;
    uint16_t coarseY = (v & 0x03E0) >> 5; // Coarse Y (bits 5-9)를 추출

    if (coarseY == 29)
    {
        v &= ~0x03E0;
        v ^= 0x0800; // 수직 네임테이블 비트 토글
    }
    else if (coarseY == 31)
    {
        v &= ~0x03E0;
    }
    else
    {
        coarseY += 1;
        v = (v & ~0x03E0) | (coarseY << 5);
    }
}

void PPU::resetHorizontalScroll()
{ // reset coarse X, nametable 10(hori)

    v &= ~0x041F;
    v |= t & 0x041F;
}

void PPU::resetVerticalScroll()
{ // reset coarse Y(5-9), nametable 11(vert), fine Y(14-12)
    v &= ~0x7BE0;
    v |= t & 0x7BE0;
}

// 1픽셀 씩 로드하는 방법
// void PPU::renderBackgroundPixel(int x, int y, uint8_t &bgPixel, bool &bgOpaque)
// {
//     uint8_t tile = fetchNameTableData();
//     uint8_t paletteIdx = fetchAttributeTableData();
//     uint8_t ptLow = fetchPatternTableLow(tile, x);
//     uint8_t ptHigh = fetchPatternTableHigh(tile, x);
//     uint8_t tilePixel = (ptHigh << 1 | ptLow) & 0x03;

//     bgPixel = ((paletteIdx & 0x03) << 2) | tilePixel;
//     bgOpaque = bgPixel != 0;
// }
//
// uint8_t PPU::fetchPatternTableLow(uint8_t tile, uint8_t tileX)
// {
//     uint16_t fineY = (v >> 12) & 0x07;
//     uint16_t ptAddr = bgPTAddr + tile * 16 + fineY;
//     return (read(ptAddr) >> (7 - tileX)) & 0x01;
// }

// uint8_t PPU::fetchPatternTableHigh(uint8_t tile, uint8_t tileX)
// {
//     uint16_t fineY = (v >> 12) & 0x07;
//     uint16_t ptAddr = bgPTAddr + tile * 16 + fineY + 8;
//     return (read(ptAddr) >> (7 - tileX)) & 0x01;
// }

#ifndef __IOREGS_H__
#define __IOREGS_H__

#define OAM_START (0xFE00)

#define R_JOYPAD (0xFF00)
#define R_JOYPAD_CONTROL (5)
#define R_JOYPAD_DIRECTION (4)

#define R_DIV (0xFF04)

#define R_TIMA (0xFF05)
#define R_TMA (0xFF06)
#define R_TAC (0xFF07)

#define R_IF (0xFF0F)

#define R_TAC_ENABLED (0x4)
#define R_TAC_TYPE (0x3)

#define R_LCDC (0xFF40)
#define R_LCDC_ENABLED (7)
#define R_LCDC_WINDOW_TILE_MAP (6)
#define R_LCDC_WINDOW_ENABLED (5)
#define R_LCDC_BG_TILE_MAP (3)
#define R_LCDC_TILE_DATA (4)

#define R_STAT (0xFF41)
#define R_STAT_LYC_ENABLE (6)
#define R_STAT_LY_COINCIDENCE (2)

#define R_SCY (0xFF42)
#define R_SCX (0xFF43)

#define R_LY   (0xFF44)
#define R_LYC  (0xFF45)

#define R_DMA  (0xFF46)

#define R_BGP (0xFF47)

#define R_WY (0xFF4A)
#define R_WX (0xFF4B)

#define R_IE (0xFFFF)

#define BIT_SET(x, n) {x |= (1 << (n));}
#define BIT_RESET(x, n) {x &= ~(1 << (n));}
#define BIT_ISSET(x, n) (x & (1 << (n)))

#endif//__IOREGS_H__

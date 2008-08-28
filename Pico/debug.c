// some debug code, just for fun of it
// (c) Copyright 2008 notaz, All rights reserved.

#include "pico_int.h"
#include "debug.h"

#define bit(r, x) ((r>>x)&1)
#define MVP dstrp+=strlen(dstrp)
void z80_debug(char *dstr);

static char dstr[1024*8];

char *PDebugMain(void)
{
  struct PicoVideo *pv=&Pico.video;
  unsigned char *reg=pv->reg, r;
  extern int HighPreSpr[];
  int i, sprites_lo, sprites_hi;
  char *dstrp;

  sprites_lo = sprites_hi = 0;
  for (i = 0; HighPreSpr[i] != 0; i+=2)
    if (HighPreSpr[i+1] & 0x8000)
         sprites_hi++;
    else sprites_lo++;

  dstrp = dstr;
  sprintf(dstrp, "mode set 1: %02x       spr lo: %2i, spr hi: %2i\n", (r=reg[0]), sprites_lo, sprites_hi); MVP;
  sprintf(dstrp, "display_disable: %i, M3: %i, palette: %i, ?, hints: %i\n", bit(r,0), bit(r,1), bit(r,2), bit(r,4)); MVP;
  sprintf(dstrp, "mode set 2: %02x                            hcnt: %i\n", (r=reg[1]), pv->reg[10]); MVP;
  sprintf(dstrp, "SMS/gen: %i, pal: %i, dma: %i, vints: %i, disp: %i, TMS: %i\n",bit(r,2),bit(r,3),bit(r,4),bit(r,5),bit(r,6),bit(r,7)); MVP;
  sprintf(dstrp, "mode set 3: %02x\n", (r=reg[0xB])); MVP;
  sprintf(dstrp, "LSCR: %i, HSCR: %i, 2cell vscroll: %i, IE2: %i\n", bit(r,0), bit(r,1), bit(r,2), bit(r,3)); MVP;
  sprintf(dstrp, "mode set 4: %02x\n", (r=reg[0xC])); MVP;
  sprintf(dstrp, "interlace: %i%i, cells: %i, shadow: %i\n", bit(r,2), bit(r,1), (r&0x80) ? 40 : 32, bit(r,3)); MVP;
  sprintf(dstrp, "scroll size: w: %i, h: %i  SRAM: %i; eeprom: %i (%i)\n", reg[0x10]&3, (reg[0x10]&0x30)>>4,
  	bit(Pico.m.sram_reg, 4), bit(Pico.m.sram_reg, 2), SRam.eeprom_type); MVP;
  sprintf(dstrp, "sram range: %06x-%06x, reg: %02x\n", SRam.start, SRam.end, Pico.m.sram_reg); MVP;
  sprintf(dstrp, "pend int: v:%i, h:%i, vdp status: %04x\n", bit(pv->pending_ints,5), bit(pv->pending_ints,4), pv->status); MVP;
  sprintf(dstrp, "pal: %i, hw: %02x, frame#: %i\n", Pico.m.pal, Pico.m.hardware, Pico.m.frame_count); MVP;
#if defined(EMU_C68K)
  sprintf(dstrp, "M68k: PC: %06x, st_flg: %x, cycles: %u\n", SekPc, PicoCpuCM68k.state_flags, SekCyclesDoneT()); MVP;
  sprintf(dstrp, "d0=%08x, a0=%08x, osp=%08x, irql=%i\n", PicoCpuCM68k.d[0], PicoCpuCM68k.a[0], PicoCpuCM68k.osp, PicoCpuCM68k.irq); MVP;
  sprintf(dstrp, "d1=%08x, a1=%08x,  sr=%04x\n", PicoCpuCM68k.d[1], PicoCpuCM68k.a[1], CycloneGetSr(&PicoCpuCM68k)); dstrp+=strlen(dstrp); MVP;
  for(r=2; r < 8; r++) {
    sprintf(dstrp, "d%i=%08x, a%i=%08x\n", r, PicoCpuCM68k.d[r], r, PicoCpuCM68k.a[r]); MVP;
  }
#elif defined(EMU_M68K)
  sprintf(dstrp, "M68k: PC: %06x, cycles: %u, irql: %i\n", SekPc, SekCyclesDoneT(), PicoCpuMM68k.int_level>>8); MVP;
#elif defined(EMU_F68K)
  sprintf(dstrp, "M68k: PC: %06x, cycles: %u, irql: %i\n", SekPc, SekCyclesDoneT(), PicoCpuFM68k.interrupts[0]); MVP;
#endif
  sprintf(dstrp, "z80Run: %i, z80_reset: %i, z80_bnk: %06x\n", Pico.m.z80Run, Pico.m.z80_reset, Pico.m.z80_bank68k<<15); MVP;
  z80_debug(dstrp); MVP;
  if (strlen(dstr) > sizeof(dstr))
    elprintf(EL_STATUS, "warning: debug buffer overflow (%i/%i)\n", strlen(dstr), sizeof(dstr));

  return dstr;
}

char *PDebugSpriteList(void)
{
  struct PicoVideo *pvid=&Pico.video;
  int table=0,u,link=0;
  int max_sprites = 80;
  char *dstrp;

  if (!(pvid->reg[12]&1))
    max_sprites = 64;

  dstr[0] = 0;
  dstrp = dstr;

  table=pvid->reg[5]&0x7f;
  if (pvid->reg[12]&1) table&=0x7e; // Lowest bit 0 in 40-cell mode
  table<<=8; // Get sprite table address/2

  for (u=0; u < max_sprites; u++)
  {
    unsigned int *sprite;
    int code, code2, sx, sy, height;

    sprite=(unsigned int *)(Pico.vram+((table+(link<<2))&0x7ffc)); // Find sprite

    // get sprite info
    code = sprite[0];

    // check if it is on this line
    sy = (code&0x1ff)-0x80;
    height = ((code>>24)&3)+1;

    // masking sprite?
    code2 = sprite[1];
    sx = ((code2>>16)&0x1ff)-0x80;

    sprintf(dstrp, "#%02i x:%4i y:%4i %ix%i %s\n", u, sx, sy, ((code>>26)&3)+1, height,
      (code2&0x8000)?"hi":"lo");
    MVP;

    link=(code>>16)&0x7f;
    if(!link) break; // End of sprites
  }

  return dstr;
}

#define GREEN1  0x0700
#ifdef USE_BGR555
 #define YELLOW1 0x071c
 #define BLUE1   0xf000
 #define RED1    0x001e
#else
 #define YELLOW1 0xe700
 #define BLUE1   0x001e
 #define RED1    0xf000
#endif

static void set16(unsigned short *p, unsigned short d, int cnt)
{
  while (cnt-- > 0)
    *p++ = d;
}

void PDebugShowSpriteStats(unsigned short *screen, int stride)
{
  int lines, i, u, step;
  unsigned short *dest;
  unsigned char *p;

  step = (320-4*4-1) / MAX_LINE_SPRITES;
  lines = 240;
  if (!Pico.m.pal || !(Pico.video.reg[1]&8))
    lines = 224, screen += stride*8;

  for (i = 0; i < lines; i++)
  {
    dest = screen + stride*i;
    p = &HighLnSpr[i][0];

    // sprite graphs
    for (u = 0; u < (p[0] & 0x7f); u++) {
      set16(dest, (p[3+u] & 0x80) ? YELLOW1 : GREEN1, step);
      dest += step;
    }

    // flags
    dest = screen + stride*i + 320-4*4;
    if (p[1] & 0x40) set16(dest+4*0, GREEN1,  4);
    if (p[1] & 0x80) set16(dest+4*1, YELLOW1, 4);
    if (p[1] & 0x20) set16(dest+4*2, BLUE1,   4);
    if (p[1] & 0x10) set16(dest+4*3, RED1,    4);
  }

  // draw grid
  for (i = step*5; i <= 320-4*4-1; i += step*5) {
    for (u = 0; u < lines; u++)
      screen[i + u*stride] = 0x182;
  }
}

void PDebugShowPalette(unsigned short *screen, int stride)
{
  unsigned int *spal=(void *)Pico.cram;
  unsigned int *dpal=(void *)HighPal;
  int x, y, i;

  for (i = 0x3f/2; i >= 0; i--)
#ifdef USE_BGR555
    dpal[i] = ((spal[i]&0x000f000f)<< 1)|((spal[i]&0x00f000f0)<<3)|((spal[i]&0x0f000f00)<<4);
#else
    dpal[i] = ((spal[i]&0x000f000f)<<12)|((spal[i]&0x00f000f0)<<3)|((spal[i]&0x0f000f00)>>7);
#endif
  for (i = 0x3f; i >= 0; i--)
    HighPal[0x40|i] = (unsigned short)((HighPal[i]>>1)&0x738e);
  for (i = 0x3f; i >= 0; i--) {
    int t=HighPal[i]&0xe71c;t+=0x4208;if(t&0x20)t|=0x1c;if(t&0x800)t|=0x700;if(t&0x10000)t|=0xe000;t&=0xe71c;
    HighPal[0x80|i] = (unsigned short)t;
  }

  screen += 16*stride+8;
  for (y = 0; y < 8*4; y++)
    for (x = 0; x < 8*16; x++)
      screen[x + y*stride] = HighPal[x/8 + (y/8)*16];

  screen += 160;
  for (y = 0; y < 8*4; y++)
    for (x = 0; x < 8*16; x++)
      screen[x + y*stride] = HighPal[(x/8 + (y/8)*16) | 0x40];

  screen += stride*48;
  for (y = 0; y < 8*4; y++)
    for (x = 0; x < 8*16; x++)
      screen[x + y*stride] = HighPal[(x/8 + (y/8)*16) | 0x80];

  Pico.m.dirtyPal = 1;
}

#if defined(DRAW2_OVERRIDE_LINE_WIDTH)
#define DRAW2_LINE_WIDTH DRAW2_OVERRIDE_LINE_WIDTH
#else
#define DRAW2_LINE_WIDTH 328
#endif

void PDebugShowSprite(unsigned short *screen, int stride, int which)
{
  struct PicoVideo *pvid=&Pico.video;
  int table=0,u,link=0,*sprite=0,*fsprite,oldsprite[2];
  int x,y,max_sprites = 80, oldcol, oldreg;

  if (!(pvid->reg[12]&1))
    max_sprites = 64;

  table=pvid->reg[5]&0x7f;
  if (pvid->reg[12]&1) table&=0x7e; // Lowest bit 0 in 40-cell mode
  table<<=8; // Get sprite table address/2

  for (u=0; u < max_sprites && u <= which; u++)
  {
    sprite=(int *)(Pico.vram+((table+(link<<2))&0x7ffc)); // Find sprite

    link=(sprite[0]>>16)&0x7f;
    if (!link) break; // End of sprites
  }
  if (u >= max_sprites) return;

  fsprite = (int *)(Pico.vram+(table&0x7ffc));
  oldsprite[0] = fsprite[0];
  oldsprite[1] = fsprite[1];
  fsprite[0] = (sprite[0] & ~0x007f01ff) | 0x000080;
  fsprite[1] = (sprite[1] & ~0x01ff8000) | 0x800000;
  oldreg = pvid->reg[7];
  oldcol = Pico.cram[0];
  pvid->reg[7] = 0;
  Pico.cram[0] = 0;
  PicoDrawMask = PDRAW_SPRITES_LOW_ON;

  PicoFrameFull();
  for (y = 0; y < 8*4; y++)
  {
    unsigned char *ps = PicoDraw2FB + DRAW2_LINE_WIDTH*y + 8;
    for (x = 0; x < 8*4; x++)
      if (ps[x]) screen[x] = HighPal[ps[x]], ps[x] = 0;
    screen += stride;
  }

  fsprite[0] = oldsprite[0];
  fsprite[1] = oldsprite[1];
  pvid->reg[7] = oldreg;
  Pico.cram[0] = oldcol;
  PicoDrawMask = -1;
}

#define dump_ram(ram,fname) \
{ \
  unsigned short *sram = (unsigned short *) ram; \
  FILE *f; \
  int i; \
\
  for (i = 0; i < sizeof(ram)/2; i++) \
    sram[i] = (sram[i]<<8) | (sram[i]>>8); \
  f = fopen(fname, "wb"); \
  if (f) { \
    fwrite(ram, 1, sizeof(ram), f); \
    fclose(f); \
  } \
  for (i = 0; i < sizeof(ram)/2; i++) \
    sram[i] = (sram[i]<<8) | (sram[i]>>8); \
}

#define dump_ram_noswab(ram,fname) \
{ \
  FILE *f; \
  f = fopen(fname, "wb"); \
  if (f) { \
    fwrite(ram, 1, sizeof(ram), f); \
    fclose(f); \
  } \
}

void PDebugDumpMem(void)
{
  dump_ram(Pico.ram,  "dumps/ram.bin");
  dump_ram_noswab(Pico.zram, "dumps/zram.bin");
  dump_ram(Pico.vram, "dumps/vram.bin");
  dump_ram(Pico.cram, "dumps/cram.bin");
  dump_ram(Pico.vsram,"dumps/vsram.bin");

  if (PicoAHW & PAHW_MCD)
  {
    dump_ram(Pico_mcd->prg_ram, "dumps/prg_ram.bin");
    if (Pico_mcd->s68k_regs[3]&4) // 1M mode?
      wram_1M_to_2M(Pico_mcd->word_ram2M);
    dump_ram(Pico_mcd->word_ram2M, "dumps/word_ram_2M.bin");
    wram_2M_to_1M(Pico_mcd->word_ram2M);
    dump_ram(Pico_mcd->word_ram1M[0], "dumps/word_ram_1M_0.bin");
    dump_ram(Pico_mcd->word_ram1M[1], "dumps/word_ram_1M_1.bin");
    if (!(Pico_mcd->s68k_regs[3]&4)) // 2M mode?
      wram_2M_to_1M(Pico_mcd->word_ram2M);
    dump_ram_noswab(Pico_mcd->pcm_ram,"dumps/pcm_ram.bin");
    dump_ram_noswab(Pico_mcd->bram,   "dumps/bram.bin");
  }
}

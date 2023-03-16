/*
 * (C) Copyright 2023 S. V. Nickolas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 *
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL.h>

#include "glasstty.h"

static uint32_t bg, fg;

/*
 * SDL2 structure pointers.
 *
 * display is a dynamically allocated offscreen buffer used for rendering the
 * VDP data, or the generated NTSC noise.
 */
static SDL_Window *screen;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_AudioDeviceID audio_device;
static SDL_AudioSpec audio_spec;
static SDL_TimerID tick;

static uint32_t *display;

/* 80x24 tty. */
static uint8_t vram[1920], oldvram[1920];
static uint8_t csrx, csry;
static uint8_t blink=0;
static uint8_t keyring[256], keybufr=0, keybufw=0;

static uint8_t font[1344]=
{
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,
 0x00,0x66,0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x6C,0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x6C,0x00,0x00,0x00,
 0x18,0x18,0x7C,0xC6,0xC2,0xC0,0x7C,0x06,0x86,0xC6,0x7C,0x18,0x18,0x00,
 0x00,0x00,0x00,0x00,0xC2,0xC6,0x0C,0x18,0x30,0x66,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x38,0x6C,0x6C,0x38,0x76,0xDC,0xCC,0xCC,0x76,0x00,0x00,0x00,
 0x00,0x30,0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x0C,0x18,0x30,0x30,0x30,0x30,0x30,0x18,0x0C,0x00,0x00,0x00,
 0x00,0x00,0x30,0x18,0x0C,0x0C,0x0C,0x0C,0x0C,0x18,0x30,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x30,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,
 0x00,0x00,0x02,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0x06,0x0C,0x18,0x30,0x60,0xC6,0xFE,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0x06,0x06,0x3C,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x1E,0x00,0x00,0x00,
 0x00,0x00,0xFE,0xC0,0xC0,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x38,0x60,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0xFE,0xC6,0x06,0x0C,0x18,0x30,0x30,0x30,0x30,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7E,0x06,0x06,0x0C,0x78,0x00,0x00,0x00,
 0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x30,0x00,0x00,0x00,
 0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0x0C,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0xDE,0xDE,0xDE,0xDC,0xC0,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x10,0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0xFC,0x00,0x00,0x00,
 0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xC0,0xC2,0x66,0x3C,0x00,0x00,0x00,
 0x00,0x00,0xF8,0x6C,0x66,0x66,0x66,0x66,0x66,0x6C,0xF8,0x00,0x00,0x00,
 0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x62,0x66,0xFE,0x00,0x00,0x00,
 0x00,0x00,0xFE,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0xF0,0x00,0x00,0x00,
 0x00,0x00,0x3C,0x66,0xC2,0xC0,0xC0,0xDE,0xC6,0x66,0x3A,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,
 0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,0x00,0x00,
 0x00,0x00,0xE6,0x66,0x6C,0x6C,0x78,0x6C,0x6C,0x66,0xE6,0x00,0x00,0x00,
 0x00,0x00,0xF0,0x60,0x60,0x60,0x60,0x60,0x62,0x66,0xFE,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0xC6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0xC6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x38,0x6C,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00,0x00,0x00,
 0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0C,0x0E,0x00,0x00,
 0x00,0x00,0xFC,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0xE6,0x00,0x00,0x00,
 0x00,0x00,0x7C,0xC6,0xC6,0x60,0x38,0x0C,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x7E,0x7E,0x5A,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xC6,0xC6,0xC6,0xD6,0xD6,0xFE,0x7C,0x6C,0x00,0x00,0x00,
 0x00,0x00,0xC6,0xC6,0x6C,0x38,0x38,0x38,0x6C,0xC6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,
 0x00,0x00,0xFE,0xC6,0x8C,0x18,0x30,0x60,0xC2,0xC6,0xFE,0x00,0x00,0x00,
 0x00,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,0x00,0x00,
 0x00,0x00,0x80,0xC0,0xE0,0x70,0x38,0x1C,0x0E,0x06,0x02,0x00,0x00,0x00,
 0x00,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,0x00,0x00,
 0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,
 0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0xCC,0x76,0x00,0x00,0x00,
 0x00,0x00,0xE0,0x60,0x60,0x78,0x6C,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC0,0xC0,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x1C,0x0C,0x0C,0x3C,0x6C,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xFE,0xC0,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x38,0x6C,0x64,0x60,0xF0,0x60,0x60,0x60,0xF0,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0xCC,0x78,0x00,
 0x00,0x00,0xE0,0x60,0x60,0x6C,0x76,0x66,0x66,0x66,0xE6,0x00,0x00,0x00,
 0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,
 0x00,0x00,0x06,0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x00,
 0x00,0x00,0xE0,0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00,0x00,0x00,
 0x00,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xD6,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xDC,0x66,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,
 0x00,0x00,0x00,0x00,0x00,0x76,0xCC,0xCC,0xCC,0x7C,0x0C,0x0C,0x1E,0x00,
 0x00,0x00,0x00,0x00,0x00,0xDC,0x76,0x66,0x60,0x60,0xF0,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x7C,0xC6,0x70,0x1C,0xC6,0x7C,0x00,0x00,0x00,
 0x00,0x00,0x10,0x30,0x30,0xFC,0x30,0x30,0x30,0x36,0x1C,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0x76,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xD6,0xD6,0xFE,0x6C,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x06,0x0C,0xF8,0x00,
 0x00,0x00,0x00,0x00,0x00,0xFE,0xCC,0x18,0x30,0x66,0xFE,0x00,0x00,0x00,
 0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,
 0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x00,
 0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x70,0x00,0x00,0x00,
 0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

int tty_alive=0;

static void pset (uint16_t x, uint16_t y, uint32_t c)
{
 c&=0x00FFFFFF;
 
 if (x>719) return;
 if (y>335) return;
 display[(y*720)+x]=c|0xFF000000;
}

static void paintchar (uint8_t x, uint8_t y, uint8_t c)
{
 uint16_t xx, yy, cc, t;
 
 if ((x>79)||(y>23)) return;
 
 xx=x*9;
 yy=y*14;
 if ((c&0x7F)<0x20) cc=0; else cc=((c&0x7F)-32)*14;
 
 for (t=0; t<14; t++)
 {
  uint8_t b, c9;
  
  b=font[cc+t];
  c9=((csrx==x)&&(csry==y)&&(blink<25));
  if (c9) b=255-b;
  if (c&0x80)
  {
   b=255-b;
   c9=!c9;
  }
  pset (xx+0, yy+t, (b&0x80)?fg:bg);
  pset (xx+1, yy+t, (b&0x40)?fg:bg);
  pset (xx+2, yy+t, (b&0x20)?fg:bg);
  pset (xx+3, yy+t, (b&0x10)?fg:bg);
  pset (xx+4, yy+t, (b&0x08)?fg:bg);
  pset (xx+5, yy+t, (b&0x04)?fg:bg);
  pset (xx+6, yy+t, (b&0x02)?fg:bg);
  pset (xx+7, yy+t, (b&0x01)?fg:bg);
  pset (xx+8, yy+t, c9?fg:bg);
 }
}

static void refresh (void)
{
 uint8_t x, y;
 uint16_t yoff;
 
 for (y=0; y<24; y++)
 {
  yoff=y*80;
  for (x=0; x<80; x++)
  {
   if (vram[yoff]!=oldvram[yoff+x])
   {
    oldvram[yoff+x]=vram[yoff+x];
   }
   paintchar (x, y, vram[yoff+x]);
  }
 }
 blink++;
 while (blink>=50) blink-=50;

 /* Update window */
 SDL_UpdateTexture(texture, 0, display, 720*sizeof(uint32_t));
 SDL_RenderClear(renderer);
 SDL_RenderCopy(renderer, texture, 0, 0);
 SDL_RenderPresent(renderer);
}

static void keypoll (void)
{
 SDL_Event event;
 
 if (!tty_alive) return;
 
 while (SDL_PollEvent(&event))
 {
  int k;
  
  k=-1;
  switch (event.type)
  {
   static char *shiftnums = ")!@#$%^&*(";
   SDL_Keymod m;
   
   case SDL_QUIT:
    tty_deinit();
   case SDL_KEYDOWN:
    k=event.key.keysym.sym;
    
    m=SDL_GetModState();
    if (m&KMOD_CTRL)
    {
     if (k=='[')  k=0x1B;
     if (k=='\\') k=0x1C;
     if (k==']')  k=0x1D;
     if (k=='-')  k=0x1F;
    }
    if (m&KMOD_SHIFT)
    {
     if (k=='`') k='~';
     if (k=='-') k='_';
     if (k=='=') k='+';
     if (k=='[') k='{';
     if (k==']') k='}';
     if (k=='\\') k='|';
     if (k==';') k=':';
     if (k=='\'') k='"';
     if (k==',') k='<';
     if (k=='.') k='>';
     if (k=='/') k='?';
    }
    if ((k>='a') && (k<='z'))
    {
     if (m&KMOD_CAPS) k^=32;
     if (m&KMOD_SHIFT) k^=32;
     if (m&KMOD_CTRL) k&=0x1F;
    }
    else if ((k>='0')&&(k<='9'))
    {
     if (m&KMOD_CTRL)
     {
      if (k=='2') k=0x00; else if (k=='6') k=0x1E;
     }
     else
     {
      if (m&KMOD_SHIFT) k=shiftnums[k&0x0F];
     }
    }
    
    /* Nope. */
    if (k>255) k=-1;
    break;
  }
  
  if (k>=0) keyring[keybufw++]=k;
 }
}

uint8_t tty_getch (void)
{
 /* Timer interrupt will take care of this */
 while (keybufr==keybufw) keypoll();
 
 return keyring[keybufr++];
}

int tty_keypoll (void)
{
 return (keybufr!=keybufw);
}

static unsigned tock (unsigned interval, void *timerid)
{
 /* In case we are called before initialization is complete */
 if (!tty_alive) return 20;

 keypoll(); 
 if (!tty_alive) return 0; /* We got the axe. */
 refresh();
 
 return 20;
}

static void tty_cr (void)
{
 csrx=0;
}

static void tty_lf (void)
{
 csry++;

 if (csry==24) /* Scroll */
 {
  csry--;
  memmove(vram, vram+80, 1840);
  memset(vram+1840, 32, 80);
 }
}

static void tty_clr (void)
{
 csrx=csry=0;
 memset(oldvram, 0, 1920);
 memset(vram, 32, 1920);
}

void tty_putch (uint8_t c)
{
 if (!tty_alive) return;
 
 if ((c&0x7F)<0x20)
 {
  switch (c&0x7F)
  {
   case 0x08: /* BS */
    if (csrx)
    {
     csrx--;
     return;
    }
    csrx=79;
    if (csry) csry--;
    return;
   case 0x09: /* TAB */
    do {csrx++;} while (csrx&7);
    if (csrx>79)
    {
     tty_cr();
     tty_lf();
    }
    return;
   case 0x0A: /* LF */
    tty_lf();
    return;
   case 0x0C: /* FF */
    tty_clr();
   case 0x0D: /* CR */
    tty_cr();
    return;
   case 0x1E: /* HOME */
    csrx=csry=0;
    return;
  }
  
  return;
 }
 
 vram[(csry*80)+(csrx++)]=c;
 if (csrx>80) /* Wrap */
 {
  tty_cr();
  tty_lf();
 }
}

void tty_deinit (void)
{
 if (!tty_alive) return;
 
 SDL_RemoveTimer(tick);
 SDL_DestroyWindow(screen);
 if (display) free(display);
 tty_alive=0;
}

int tty_init (const char *title, uint32_t f, uint32_t b)
{
 /* Don't do it twice. */
 if (tty_alive) return 0;
 
 display=calloc(720*336, 4);
 
 tick=SDL_AddTimer(20, tock, "50 Hz tick");
 if (!tick) return -1;
 
 screen=SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 720, 336, 0);
 if (!screen) return -1;
 
 renderer=SDL_CreateRenderer(screen, -1, 0);
 if (!renderer) return -1;
 
 texture=SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                           SDL_TEXTUREACCESS_STREAMING, 720, 336);
 if (!texture) return -1;
 
 tty_alive=1;
 fg=f?f:0xCCCCCC;
 bg=b;
 tty_clr();
 refresh();
 return 0;
}

#ifdef TEST
int main (int argc, char **argv)
{
 int e;
 
 e=SDL_Init(SDL_INIT_EVERYTHING);
 if (e) return -1;

 e=tty_init("Terminal",0,0);
 if (e) return -1;
 
 while (tty_alive)
 {
  e=tty_getch();
  if (e=='\n') tty_putch('\r');
  tty_putch(e);
 }
}
#endif

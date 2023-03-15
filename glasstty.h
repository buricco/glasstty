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

#ifndef INC_H_GLASSTTY
#define INC_H_GLASSTTY
#include <stdint.h>

/* Some common colors to pass to tty_init() */
#define TTY_BLACK  0x000000
#define TTY_WHITE  0xFFFFFF
#define TTY_BLUE   0xDDFFFF
#define TTY_GREEN  0x00CC00
#define TTY_AMBER  0xCC7700

/* 
 * Initially set to 0; set to 1 while the window is open, then back to 0 if
 * user closes the window.
 */
extern int tty_alive;

/* Returns whether there is a key waiting in the ring buffer. */
int tty_keypoll (void);

/* If there is a key waiting, returns it.  If not, returns 0. */
uint8_t tty_getch (void);

/* Outputs a character on the terminal, processing it as necessary. */
void tty_putch (uint8_t);

/*
 * Initializes the terminal window.  Returns 0 on success, -1 on failure.
 * Pass a foreground and background color.  Alternatively, if both are set to
 * 0, then 75% grey on black (IBM PC) is used.
 */
int tty_init (const char *, uint32_t, uint32_t);

/* Shuts down the terminal window. */
void tty_deinit (void);

#endif /* INC_H_GLASSTTY */

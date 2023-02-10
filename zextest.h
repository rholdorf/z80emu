/* zextest.h
 * Header for zextest example.
 *
 * Copyright (c) 2012, 2016 Lin Ke-Fong
 *
 * This code is free, do whatever you want with it.
 */

#ifndef __ZEXTEST_INCLUDED__
#define __ZEXTEST_INCLUDED__
#include "z80emu.h"
typedef struct ZEXTEST
{
    Z80_STATE state;
    unsigned char memory[1 << 16];
    int is_done;
} ZEXTEST;

extern void SystemCall(ZEXTEST *zextest);
extern void LogWriteByte(unsigned short address, unsigned char data);
extern void LogWriteWord(unsigned short address, int data);
extern void LogReadByte(unsigned short address, unsigned char data);
extern void LogReadWord(unsigned short address, int data);

#endif

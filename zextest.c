/* zextest.c
 * Example program using z80emu to run the zexall and zexdoc tests. This will
 * check if the Z80 is correctly emulated.
 *
 * Copyright (c) 2012, 2016 Lin Ke-Fong
 * Copyright (c) 2012 Chris Pressey
 *
 * This code is free, do whatever you want with it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "zextest.h"
#include "z80emu.h"
#define Z80_CPU_SPEED 4000000 /* In Hz. */
#define CYCLES_PER_STEP (Z80_CPU_SPEED / 50)
#define MAXIMUM_STRING_LENGTH 100

static void emulate(char *filename, long beginAt, long endAt, int skip);
static void LogState(ZEXTEST context);
static int BreakPoint(ZEXTEST context, int pc, unsigned short af, unsigned short bc, unsigned short de, unsigned short hl, unsigned short ix, unsigned short iy, unsigned short sp, int r);

int main(int argc, char *argv[])
{
    long beginAt = 0;
    long endAt = 0;
    int skip = 0;
    time_t start, stop;
    if (argc >= 3)
    {
        beginAt = strtol(argv[1], NULL, 10);
        endAt = strtol(argv[2], NULL, 10);
    }
    if(argc == 4)
        skip = strtol(argv[3], NULL, 10);

    printf("DEBUG: %ld %ld\n", beginAt, endAt);
    start = time(NULL);
    emulate("testfiles/zexall.com", beginAt, endAt, skip);
    stop = time(NULL);
    printf("Emulating zexdoc and zexall took a total of %d second(s).\n", (int)(stop - start));

    return EXIT_SUCCESS;
}

/* Emulate "zexdoc.com" or "zexall.com". */

static void emulate(char *filename, long beginAt, long endAt, int skip)
{
    FILE *file;
    long l;
    ZEXTEST context;
    double total;
    long counter;
    unsigned short newTestAddress;
    unsigned char low;
    unsigned char high;
    int i;

    if ((file = fopen(filename, "rb")) == NULL)
    {
        fprintf(stderr, "Can't open file!\n");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    l = ftell(file);
    fseek(file, 0, SEEK_SET);
    fread(context.memory + 0x100, 1, l, file);
    fclose(file);

    /* Patch the memory of the program. Reset at 0x0000 is trapped by an
     * OUT which will stop emulation. CP/M bdos call 5 is trapped by an IN.
     * See Z80_INPUT_BYTE() and Z80_OUTPUT_BYTE() definitions in z80user.h.
     */
    context.memory[0] = 0xd3; /* OUT N, A */
    context.memory[1] = 0x00;
    context.memory[5] = 0xdb; /* IN A, N */
    context.memory[6] = 0x00;
    context.memory[7] = 0xc9; /* RET */

    context.is_done = 0;

    /* pula o numero de testes especificado */
    newTestAddress = (0x013A + (skip * 2)) & 0xffff;
    high = (unsigned char)((newTestAddress >> 8) & 0xff);
    low = (unsigned char)(newTestAddress & 0x00ff);
    context.memory[0x0120] = low;
    context.memory[0x0121] = high;

    /* Emulate. */
    Z80Reset(&context.state);
    context.state.pc = 0x100;
    total = 0.0;
    counter = 0;

    do
    {
        /* 1e52 f008 1086 0003 1e88 7acc 9dfc c8e4 00 77 00 */
        if (endAt > 0 && counter >= beginAt)
        {
            /*if(BreakPoint(context, 0x1e52, 0xf008, 0x1086, 0x0003, 0x1e88, 0x7acc, 0x9dfc, 0xc8e4, 0x77) == 1)
            {
            }*/

            LogState(context);
            total += Z80Emulate(&context.state, 1, &context);
            printf("|");
            LogState(context);
            printf("\n");
        }
        else
        {
            total += Z80Emulate(&context.state, 1, &context);
        }

        counter++;
        if (endAt > 0 && counter >= endAt)
            exit(0);

    } while (!context.is_done);

    printf("\n%.0f cycle(s) emulated.\n"
        "For a Z80 running at %.2fMHz, "
        "that would be %d second(s) or %.2f hour(s).\n",
        total,
        Z80_CPU_SPEED / 1000000.0,
        (int)(total / Z80_CPU_SPEED),
        total / ((double)3600 * Z80_CPU_SPEED));
}

static int BreakPoint(ZEXTEST context, int pc, unsigned short af, unsigned short bc, unsigned short de, unsigned short hl, unsigned short ix, unsigned short iy, unsigned short sp, int r)
{
    if (context.state.pc == pc
        && context.state.registers.word[Z80_AF] == af
        && context.state.registers.word[Z80_BC] == bc
        && context.state.registers.word[Z80_DE] == de
        && context.state.registers.word[Z80_HL] == hl
        && context.state.registers.word[Z80_IX] == ix
        && context.state.registers.word[Z80_IY] == iy
        && context.state.registers.word[Z80_SP] == sp
        && context.state.r == r)
        return 1;

    return 0;
}

static void LogState(ZEXTEST context)
{
    /*      M1  M2  M3  M4  M5   PC   AF   BC   DE   HL   IX   IY   SP   I    R    IM   */
    int i, j, val;
    int address = context.state.pc;
    for(i = 0; i < 5; i++)
    {
        printf("%02x", context.memory[address]);
        address++;
    }

    printf(" %04x %04x %04x %04x %04x %04x %04x %04x %02x %02x %02x ",
        context.state.pc,
        context.state.registers.word[Z80_AF],
        context.state.registers.word[Z80_BC],
        context.state.registers.word[Z80_DE],
        context.state.registers.word[Z80_HL],
        context.state.registers.word[Z80_IX],
        context.state.registers.word[Z80_IY],
        context.state.registers.word[Z80_SP],
        context.state.i,
        context.state.r,
        context.state.im);
        val = context.state.registers.byte[Z80_F];
        for (j = 7; 0 <= j; j--) 
        {
            printf("%c", (val & (1 << j)) ? '1' : '0');
        }
}


/* Emulate CP/M bdos call 5 functions 2 (output character on screen) and 9
 * (output $-terminated string to screen).
 */
void SystemCall(ZEXTEST *zextest)
{
    if (zextest->state.registers.byte[Z80_C] == 2)
        printf("%c", zextest->state.registers.byte[Z80_E]);
    else if (zextest->state.registers.byte[Z80_C] == 9)
    {
        int i, c;

        for (i = zextest->state.registers.word[Z80_DE], c = 0; zextest->memory[i] != '$'; i++)
        {
            printf("%c", zextest->memory[i & 0xffff]);
            if (c++ > MAXIMUM_STRING_LENGTH)
            {
                fprintf(stderr, "String to print is too long!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void LogWriteByte(unsigned short address, unsigned char data)
{
    printf("[%04x %02x]", address, data);
}

void LogWriteWord(unsigned short address, int data)
{
    printf("[%04x %04x]", address, data);
}

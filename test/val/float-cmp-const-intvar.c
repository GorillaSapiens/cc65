
// test comparison operations

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <_float.h>

#ifdef CONIO
#include <conio.h>
#define WAIT()  cgetc()
#else
#define WAIT()
#endif

float fp1, fp2, fp3, fp4;

char buf[0x30];
char buf2[0x30];
char buf3[0x30];

signed char var_schar;
unsigned char var_uchar;
signed int var_sint;
unsigned int var_uint;
signed long var_slong;
unsigned long var_ulong;

int result = 0;

#define expect(msg, exp, val) \
    printf("%s %s%s\n", \
        msg, \
        val ? "true" : "false", \
        (exp != val) ? " (failed)" : ""); \
    if (exp != val) { \
        result++; \
    }

int i1 = 10;
int i2 = 20;

void constintvar(void)
{
    printf("const vs int var\n");

#if 1
    expect("10.0f == 20 is", 0, (10.0f == i2));
    expect("20.0f == 10 is", 0, (20.0f == i1));
    expect("20.0f == 20 is", 1, (20.0f == i2));

    expect("10.0f != 20 is", 1, (10.0f != i2));
    expect("20.0f != 10 is", 1, (20.0f != i1));
    expect("20.0f != 20 is", 0, (20.0f != i2));

    expect("10.0f < 20 is", 1, (10.0f < i2));
    expect("20.0f < 10 is", 0, (20.0f < i1));
    expect("20.0f < 20 is", 0, (20.0f < i2));

    expect("10.0f > 20 is", 0, (10.0f > i2));
    expect("20.0f > 10 is", 1, (20.0f > i1));
    expect("20.0f > 20 is", 0, (20.0f > i2));

    expect("10.0f <= 20 is", 1, (10.0f <= i2));
    expect("20.0f <= 10 is", 0, (20.0f <= i1));
    expect("20.0f <= 20 is", 1, (20.0f <= i2));

    expect("10.0f >= 20 is", 0, (10.0f >= i2));
    expect("20.0f >= 10 is", 1, (20.0f >= i1));
    expect("20.0f >= 20 is", 1, (20.0f >= i2));
#endif
}

//-------------------------------------------------------------------------

int main(void)
{
    printf("float-cmp-const-intvar\n");

    constintvar();

    printf("float-cmp-const-intvar (res: %d)\n", result);
    return result;
}

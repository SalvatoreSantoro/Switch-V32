#include <stdint.h>
#include <stdio.h>


#define IM_RD_MASK (0b11111 << 7)
#define IM2_F7_MASK (0b1111111 << 25)
#define RD(x) ((x & IM_RD_MASK) >> 7)

#define B2_IMM(x) ((RD(x)) | ((signed)(x & IM2_F7_MASK) >> 20)) // 19?

#define SIGN_EXTEND12(x) ((((int32_t)(x)) << 20) >> 20)
#define B_IMM(x) SIGN_EXTEND12(((((x) >> 25) & 0x7F) << 5) | (((x) >> 7) & 0x1F))

int main(int argc, char* argv[])
{
    printf("%d, %d\n", B_IMM(3000), B2_IMM(3000));
    printf("%d, %d\n", B_IMM(1234), B2_IMM(1234));
    printf("%d, %d\n", B_IMM(-100), B2_IMM(-100));
    printf("%d, %d\n", B_IMM(467), B2_IMM(467));
    printf("%d, %d\n", B_IMM(999999998), B2_IMM(999999998));
    printf("%d, %d\n", B_IMM(104), B2_IMM(104));
    printf("%d, %d\n", B_IMM(-99), B2_IMM(-99));
    printf("%d, %d\n", B_IMM(1999), B2_IMM(1999));
}

ENTRY(_entry)
MEMORY
{
    ROM (rx)  : ORIGIN = 0x10000000, LENGTH = 64K
    RAM (rw)  : ORIGIN = 0x10010000, LENGTH = 136K
}
SECTIONS
{
    .text :
    {
        *(.entry .text .text*)
    } > ROM

    .rodata :
    {
        *(.rodata*)
    } > ROM

    .data :
    {
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.data*)
    } > RAM

    .bss :
    {
        *(.bss*)
        *(COMMON)
    } > RAM

    . = ALIGN(16);
    _stack_top = .;
    . += 4096;
    _stack_base = .;
}

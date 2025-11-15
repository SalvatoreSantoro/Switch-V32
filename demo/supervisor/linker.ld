ENTRY(_entry)
MEMORY
{
   RAM (rwx) : ORIGIN = 0x80400000, LENGTH = 16M
}
SECTIONS
{
    .text :
    {
        *(.text .text*)
    } > RAM

    .rodata :
    {
        *(.rodata*)
    } > RAM

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

SECTIONS
{
  /* Tell the linker to insert the section but keep everything else default */
  .sad_callbacks : ALIGN(16) {
    __start_sad_callbacks = .;
    KEEP(*(.sad_callbacks))
    __stop_sad_callbacks = .;
  }
}
INSERT AFTER .data;

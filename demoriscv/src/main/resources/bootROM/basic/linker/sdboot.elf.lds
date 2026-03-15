OUTPUT_ARCH("riscv")
ENTRY(_prog_start)

INCLUDE memory.lds

PHDRS
{
  text PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}

SECTIONS
{
  PROVIDE(_ram = ORIGIN(memory_mem));
  PROVIDE(_ram_end = _ram + LENGTH(memory_mem));

  .text ALIGN((ORIGIN(bootrom_mem) + 0x0), 8) : AT(ALIGN((ORIGIN(bootrom_mem) + 0x0), 8)) {
    PROVIDE(_ftext = .);
    *(.text.init)
    *(.text.unlikely .text.unlikely.*)
    *(.text .text.* .gnu.linkonce.t.*)
    PROVIDE(_etext = .);
    . += 0x40; /* to create a gap between .text and .data b/c ifetch can fetch ahead from .data */
  } >bootrom_mem  :text
}

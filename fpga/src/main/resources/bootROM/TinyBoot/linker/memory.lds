MEMORY
{
  bootrom_mem (rx) : ORIGIN = 0x10000, LENGTH = 0x1000 /* 4kB */
  memory_mem (rwx) : ORIGIN = 0x80000000, LENGTH = 0x100000 /* 1MB */
}

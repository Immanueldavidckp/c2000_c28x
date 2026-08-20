/*
 * f280049c_flash.cmd - linker command file, run from flash
 *
 * Minimal memory map for the TMS320F280049C.  Only the regions this example
 * actually needs are declared; the device has considerably more flash and
 * RAM than is mapped here.
 *
 * Flash bank 0 starts at 0x080000.  0x10000 words (128 KB) are mapped, which
 * is well inside the 256 KB the F280049C provides.
 */

MEMORY
{
   /* --- program (flash) -------------------------------------------------- */
   BEGIN           : origin = 0x080000, length = 0x000002  /* reset branch  */
   FLASH           : origin = 0x080002, length = 0x00FFFE

   /* --- data (RAM) ------------------------------------------------------- */
   BOOT_RSVD       : origin = 0x000002, length = 0x0000F3  /* boot ROM use  */
   RAMM0           : origin = 0x0000F5, length = 0x00030B
   RAMM1           : origin = 0x000400, length = 0x0003F8
   RAMLS0          : origin = 0x008000, length = 0x000800
   RAMLS1          : origin = 0x008800, length = 0x000800

   /* --- reset vector (read only, owned by the boot ROM) ------------------ */
   RESET           : origin = 0x3FFFC0, length = 0x000002
}

SECTIONS
{
   codestart        : > BEGIN,   ALIGN(4)
   .text            : > FLASH,   ALIGN(4)
   .cinit           : > FLASH,   ALIGN(4)
   .switch          : > FLASH,   ALIGN(4)
   .init_array      : > FLASH,   ALIGN(4)
   .const           : > FLASH,   ALIGN(4)

   .stack           : > RAMM1
   .bss             : > RAMLS0
   .data            : > RAMLS0
   .sysmem          : > RAMLS0

   /* older C2000 compiler section names - harmless if unused */
   .ebss            : > RAMLS0
   .esysmem         : > RAMLS0
   .econst          : > FLASH,   ALIGN(4)

   .reset           : > RESET,   TYPE = DSECT
}

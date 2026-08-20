/*
 * f280049c_ram.cmd - linker command file, run entirely from RAM
 *
 * Use this one when loading through the CCS/XDS debugger.  Nothing is
 * programmed into flash, so the load is fast and the flash endurance is
 * untouched.  The code is lost on power cycle - use f280049c_flash.cmd for
 * a standalone board.
 */

MEMORY
{
   BEGIN           : origin = 0x000000, length = 0x000002  /* reset branch  */
   BOOT_RSVD       : origin = 0x000002, length = 0x0000F3  /* boot ROM use  */
   RAMM0           : origin = 0x0000F5, length = 0x00030B
   RAMM1           : origin = 0x000400, length = 0x0003F8

   RAMLS0          : origin = 0x008000, length = 0x000800
   RAMLS1          : origin = 0x008800, length = 0x000800
   RAMLS2          : origin = 0x009000, length = 0x000800
   RAMLS3          : origin = 0x009800, length = 0x000800

   RESET           : origin = 0x3FFFC0, length = 0x000002
}

SECTIONS
{
   codestart        : > BEGIN
   .text            : >> RAMLS0 | RAMLS1
   .cinit           : > RAMLS2
   .switch          : > RAMLS2
   .init_array      : > RAMLS2
   .const           : > RAMLS2

   .stack           : > RAMM1
   .bss             : > RAMLS3
   .data            : > RAMLS3
   .sysmem          : > RAMLS3

   /* older C2000 compiler section names - harmless if unused */
   .ebss            : > RAMLS3
   .esysmem         : > RAMLS3
   .econst          : > RAMLS2

   .reset           : > RESET,   TYPE = DSECT
}

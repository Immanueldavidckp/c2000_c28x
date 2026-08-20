;=============================================================================
; codestart.asm - reset entry point for TMS320F280049C
;
; The boot ROM finishes and jumps to the start of flash (0x080000) when the
; device is configured to boot from flash.  The linker command file places
; the "codestart" section there, so the single long branch below hands
; control to the C runtime start-up routine _c_int00 (provided by the TI
; RTS library), which sets up the stack, runs .cinit and calls main().
;
; No header files, no macros from device support - just the branch.
;=============================================================================

    .global code_start
    .global _c_int00

    .sect "codestart"

code_start:
    LB _c_int00                 ; long branch to the C entry point

    .end

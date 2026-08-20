# Bare-metal LED blink - TMS320F280049C

Blinks the two on-board LEDs of the **LAUNCHXL-F280049C** by writing directly
to the memory-mapped peripheral registers. No header files are included by
`main.c`: no `driverlib.h`, no `device.h`, no `F28004x_Device.h`, no bit-field
register structures. Every register is reached through its raw address.

## Files

| File | Purpose |
| --- | --- |
| `main.c` | The whole application: watchdog off, GPIO mux/direction, toggle loop |
| `codestart.asm` | Reset entry point - a single `LB _c_int00` placed at the boot address |
| `f280049c_flash.cmd` | Linker command file for standalone boot from flash |
| `f280049c_ram.cmd` | Linker command file for a debugger load into RAM |
| `Makefile` | Command-line build with the TI C2000 code generation tools |

## LEDs

| LED | Pin | Notes |
| --- | --- | --- |
| LD4 | GPIO23 | port A, bit 23 |
| LD5 | GPIO34 | port B, bit 2 |

Both are active low - the pin sits between the LED and ground, so driving the
pin low turns the LED on. The two LEDs are toggled together from opposite
starting states, so they alternate.

On the **F280049C controlCARD** the first LED is GPIO31 rather than GPIO23.
Change `USE_LED1` at the top of `main.c` to `31` for that board; the port A
code needs no other edit because GPIO31 also lives in `GPAMUX2`.

## Registers used

All addresses are 16-bit word addresses, and the 32-bit registers occupy two
consecutive words.

| Register | Address | Use |
| --- | --- | --- |
| `WDKEY` | `0x7025` | Kick the watchdog before switching it off |
| `WDCR` | `0x7029` | `0x0068` = WDDIS set, WDCHK = 101b |
| `GPAMUX2` / `GPAGMUX2` | `0x7C08` / `0x7C22` | Select plain GPIO for GPIO16..31 |
| `GPADIR` | `0x7C0A` | 1 = output |
| `GPAPUD` | `0x7C0C` | 1 = internal pull-up disabled |
| `GPAODR` | `0x7C12` | 0 = push-pull |
| `GPBMUX1` / `GPBGMUX1` | `0x7C46` / `0x7C60` | Same for GPIO32..47 |
| `GPBDIR` / `GPBPUD` / `GPBODR` | `0x7C4A` / `0x7C4C` / `0x7C52` | |
| `GPASET` / `GPACLEAR` / `GPATOGGLE` | `0x7F02` / `0x7F04` / `0x7F06` | Atomic port A output writes |
| `GPBSET` / `GPBCLEAR` / `GPBTOGGLE` | `0x7F0A` / `0x7F0C` / `0x7F0E` | Atomic port B output writes |

The GPIO control registers and the watchdog control register are EALLOW
protected, so `main.c` brackets those writes with the `EALLOW` / `EDIS`
instructions emitted inline through `__asm()`.

## Clocking and blink rate

The reset clock configuration is left untouched: the F28004x comes out of
reset running from INTOSC2, so SYSCLK is 10 MHz. There is no PLL setup, no
crystal, and no flash wait-state configuration to get wrong.

Timing comes from the software loop `delay_loop()`, which is approximate by
design - roughly half a second per toggle at 10 MHz. Adjust `DELAY_LOOPS` in
`main.c` to change the rate. If you later configure the PLL for 100 MHz,
scale `DELAY_LOOPS` by the same factor.

## Building in Code Composer Studio

1. **File -> New -> CCS Project**, device `TMS320F280049C`, empty project.
2. Delete any generated `main.c` and the linker command file CCS added.
3. Add `main.c`, `codestart.asm` and one of the two `.cmd` files to the
   project (`f280049c_ram.cmd` while debugging, `f280049c_flash.cmd` for a
   standalone build).
4. **Project Properties -> Build -> C2000 Linker -> Basic Options**: set the
   entry point to `code_start` and the stack size to `0x200`.
5. No include paths and no libraries beyond the automatic RTS library are
   needed - the project pulls in nothing from C2000Ware.
6. Build, then **Run -> Debug**.

## Building from the command line

```sh
make CG_TOOL_ROOT=/path/to/ti-cgt-c2000_x.y.z            # flash image
make CG_TOOL_ROOT=/path/to/ti-cgt-c2000_x.y.z TARGET=ram # RAM image
```

The resulting `.out` file is loaded with CCS or with UniFlash.

## Notes

* The C runtime start-up routine `_c_int00` from the TI RTS library still
  sets up the stack and runs `.cinit` before `main()`. That is the normal
  boot path for a C project; the application code itself touches nothing but
  hardware registers.
* Interrupts are never enabled, so no PIE vector table is initialised and
  `.reset` is left as a `DSECT` - the boot ROM owns the real reset vector.
* The `BOOT_RSVD` region (`0x000002`-`0x0000F3`) is declared but never used
  for placement, because the boot ROM uses that part of M0 as its own stack.

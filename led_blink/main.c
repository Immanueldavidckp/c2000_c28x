/*
 * ============================================================================
 *  main.c - Bare-metal LED blink for TMS320F280049C (LAUNCHXL-F280049C)
 * ============================================================================
 *
 *  Pure bare metal:
 *    - no #include of any header file
 *    - no TI driverlib, no bit-field register structures, no device support
 *      files (F28004x_Device.h / device.h / driverlib.h are NOT used)
 *    - every peripheral register is written through its raw memory address
 *
 *  Board LEDs (LAUNCHXL-F280049C):
 *      LD4 (red)  -> GPIO23   (active low: pin low = LED on)
 *      LD5 (red)  -> GPIO34   (active low: pin low = LED on)
 *  F280049C controlCARD uses GPIO31 / GPIO34 instead - see USE_LED1 below.
 *
 *  Clocking: this file deliberately leaves the reset clock configuration
 *  alone.  After reset the F28004x runs from INTOSC2 at 10 MHz, so SYSCLK
 *  is 10 MHz and no PLL / XTAL / flash-wait-state setup is required.  The
 *  blink rate therefore comes from a plain software delay loop.
 *
 * ============================================================================
 */

/* ---------------------------------------------------------------------------
 *  Register access helpers
 *
 *  The C28x is word addressed: one address holds 16 bits.
 *  With the TI C2000 compiler  'unsigned int' = 16 bit, 'unsigned long' = 32
 *  bit, so a 32-bit access at word address N covers registers N and N+1.
 * ------------------------------------------------------------------------ */
#define HWREG16(addr)   (*((volatile unsigned int  *)(addr)))
#define HWREG32(addr)   (*((volatile unsigned long *)(addr)))

/* ---------------------------------------------------------------------------
 *  Watchdog (WD_REGS base 0x7020) - EALLOW protected
 * ------------------------------------------------------------------------ */
#define WDCR            0x7029UL    /* watchdog control register (16 bit)    */
#define WDKEY           0x7025UL    /* watchdog reset key       (16 bit)     */

/* WDCR: bit6 = WDDIS (1 = disabled), bits5:3 = WDCHK must be written 101b.
 * Any other value written to WDCHK triggers an immediate reset.            */
#define WDCR_DISABLE    0x0068U

/* ---------------------------------------------------------------------------
 *  GPIO control registers (GPIO_CTRL_REGS base 0x7C00)
 *  Each port occupies 0x40 words:  GPA 0x7C00, GPB 0x7C40, ...
 *  All of these are EALLOW protected.
 * ------------------------------------------------------------------------ */
#define GPACTRL         0x7C00UL    /* qualification sampling period        */
#define GPAQSEL1        0x7C02UL    /* qual type, GPIO0..15                 */
#define GPAQSEL2        0x7C04UL    /* qual type, GPIO16..31                */
#define GPAMUX1         0x7C06UL    /* mux,  GPIO0..15   (2 bits per pin)   */
#define GPAMUX2         0x7C08UL    /* mux,  GPIO16..31  (2 bits per pin)   */
#define GPADIR          0x7C0AUL    /* 0 = input, 1 = output                */
#define GPAPUD          0x7C0CUL    /* 0 = pull-up enabled, 1 = disabled    */
#define GPAINV          0x7C10UL    /* input inversion                      */
#define GPAODR          0x7C12UL    /* 0 = normal push-pull, 1 = open drain */
#define GPAGMUX1        0x7C20UL    /* group mux, GPIO0..15                 */
#define GPAGMUX2        0x7C22UL    /* group mux, GPIO16..31                */

#define GPBCTRL         0x7C40UL
#define GPBQSEL1        0x7C42UL    /* qual type, GPIO32..47                */
#define GPBQSEL2        0x7C44UL    /* qual type, GPIO48..63                */
#define GPBMUX1         0x7C46UL    /* mux,  GPIO32..47                     */
#define GPBMUX2         0x7C48UL    /* mux,  GPIO48..63                     */
#define GPBDIR          0x7C4AUL
#define GPBPUD          0x7C4CUL
#define GPBINV          0x7C50UL
#define GPBODR          0x7C52UL
#define GPBGMUX1        0x7C60UL    /* group mux, GPIO32..47                */
#define GPBGMUX2        0x7C62UL    /* group mux, GPIO48..63                */

/* ---------------------------------------------------------------------------
 *  GPIO data registers (GPIO_DATA_REGS base 0x7F00)
 *  8 words per port: DAT, SET, CLEAR, TOGGLE - each 32 bit.
 *  These are NOT EALLOW protected.
 * ------------------------------------------------------------------------ */
#define GPADAT          0x7F00UL
#define GPASET          0x7F02UL
#define GPACLEAR        0x7F04UL
#define GPATOGGLE       0x7F06UL

#define GPBDAT          0x7F08UL
#define GPBSET          0x7F0AUL
#define GPBCLEAR        0x7F0CUL
#define GPBTOGGLE       0x7F0EUL

/* ---------------------------------------------------------------------------
 *  Board selection
 *
 *  USE_LED1 = 23 -> LAUNCHXL-F280049C LD4   (default)
 *  USE_LED1 = 31 -> F280049C controlCARD D9
 *  LED2 is GPIO34 on both boards.
 * ------------------------------------------------------------------------ */
#define USE_LED1        23U         /* port A pin */
#define USE_LED2        34U         /* port B pin */

#define LED1_MASK       (1UL << (USE_LED1))          /* bit in port A */
#define LED2_MASK       (1UL << ((USE_LED2) - 32U))  /* bit in port B */

/* ---------------------------------------------------------------------------
 *  Blink period
 *
 *  SYSCLK = 10 MHz after reset.  The loop below burns roughly 10 SYSCLK
 *  cycles per iteration when running from RAM, more from flash (default
 *  wait states), so this is an approximate half-second.  Tune to taste.
 * ------------------------------------------------------------------------ */
#define DELAY_LOOPS     500000UL

/* ---------------------------------------------------------------------------
 *  EALLOW / EDIS
 *
 *  EALLOW and EDIS are C28x instructions, not macros from a header, so they
 *  are emitted inline.  They open / close write access to the protected
 *  registers (watchdog, GPIO mux and direction, ...).
 * ------------------------------------------------------------------------ */
#define EALLOW()        __asm(" EALLOW")
#define EDIS()          __asm(" EDIS")
#define NOP()           __asm(" NOP")

/* ---------------------------------------------------------------------------
 *  disable_watchdog
 *
 *  The watchdog is enabled out of reset and will reset the device after
 *  about 100 ms if it is never serviced.  Kick it once, then disable it.
 * ------------------------------------------------------------------------ */
static void disable_watchdog(void)
{
    EALLOW();
    HWREG16(WDKEY) = 0x0055U;       /* the 55h / AAh sequence clears the    */
    HWREG16(WDKEY) = 0x00AAU;       /* counter before we switch it off      */
    HWREG16(WDCR)  = WDCR_DISABLE;  /* WDDIS = 1, WDCHK = 101b              */
    EDIS();
}

/* ---------------------------------------------------------------------------
 *  init_leds
 *
 *  Drive both LED pins as general purpose outputs:
 *      GMUX = 00 and MUX = 00  -> the pin is plain GPIO
 *      DIR  = 1                -> output
 *      PUD  = 1                -> internal pull-up off (not needed on an
 *                                 output, and it saves a little current)
 *      ODR  = 0                -> push-pull, so the pin can source and sink
 *
 *  The mux fields are 2 bits wide, so pin p inside a 16-pin mux register
 *  occupies bits (2*p+1 : 2*p).
 * ------------------------------------------------------------------------ */
static void init_leds(void)
{
    unsigned long led1_mux_mask = 3UL << (2U * ((USE_LED1) - 16U)); /* MUX2 */
    unsigned long led2_mux_mask = 3UL << (2U * ((USE_LED2) - 32U)); /* MUX1 */

    EALLOW();

    /* ---- LED1 : port A ------------------------------------------------ */
    HWREG32(GPAGMUX2) &= ~led1_mux_mask;    /* group mux 0 */
    HWREG32(GPAMUX2)  &= ~led1_mux_mask;    /* mux 0 -> GPIO */
    HWREG32(GPAODR)   &= ~LED1_MASK;        /* push-pull */
    HWREG32(GPAPUD)   |=  LED1_MASK;        /* pull-up disabled */
    HWREG32(GPADIR)   |=  LED1_MASK;        /* output */

    /* ---- LED2 : port B ------------------------------------------------ */
    HWREG32(GPBGMUX1) &= ~led2_mux_mask;
    HWREG32(GPBMUX1)  &= ~led2_mux_mask;
    HWREG32(GPBODR)   &= ~LED2_MASK;
    HWREG32(GPBPUD)   |=  LED2_MASK;
    HWREG32(GPBDIR)   |=  LED2_MASK;

    EDIS();

    /* Both LEDs are wired to 3V3 through a resistor, so a high output
     * turns them off.  Start from a known state: LED1 on, LED2 off, which
     * makes the alternating pattern below obvious straight away.          */
    HWREG32(GPACLEAR) = LED1_MASK;          /* LED1 on  */
    HWREG32(GPBSET)   = LED2_MASK;          /* LED2 off */
}

/* ---------------------------------------------------------------------------
 *  delay_loop
 *
 *  Software delay.  'i' is volatile so the compiler cannot optimise the
 *  loop away at -O2 / -O3, and the NOP keeps the body from collapsing to
 *  nothing but the counter update.
 * ------------------------------------------------------------------------ */
static void delay_loop(unsigned long loops)
{
    volatile unsigned long i;

    for (i = 0UL; i < loops; i++)
    {
        NOP();
    }
}

/* ---------------------------------------------------------------------------
 *  main
 * ------------------------------------------------------------------------ */
int main(void)
{
    disable_watchdog();
    init_leds();

    for (;;)
    {
        /* GPxTOGGLE flips only the bits written as 1 and is atomic, so no
         * read-modify-write on the data register is needed.               */
        HWREG32(GPATOGGLE) = LED1_MASK;
        HWREG32(GPBTOGGLE) = LED2_MASK;

        delay_loop(DELAY_LOOPS);
    }
}

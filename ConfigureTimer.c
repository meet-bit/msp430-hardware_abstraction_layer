#include "msp430fr5739.h"

void configure_timer_b() {
    // Configure P3.4 for TB1.1 output (LED5)
    P3DIR |= BIT4;            // Set P3.4 as output
    P3SEL1 &= ~BIT4;          // Set function for Timer B output
    P3SEL0 |= BIT4;

    // Configure P3.5 for TB1.2 output (LED6)
    P3DIR |= BIT5;            // Set P3.5 as output
    P3SEL1 &= ~BIT5;          // Set function for Timer B output
    P3SEL0 |= BIT5;

    // Stop the timer during setup
    TB1CTL = TBSSEL_2 | MC_1 | TBCLR | ID__8 ;  // SMCLK as clock source, stop mode, clear timer

    // Set up Timer B with a period to generate 500 Hz square wave
    TB1CCR0 = (2000 - 1);    // Set timer period (16000 ticks for 500 Hz)

    // Set up TB1.1 for 50% duty cycle
    TB1CCTL1 = OUTMOD_7;    // Set/reset mode for TB1.1 (50% duty cycle)
    TB1CCR1 = 1000;         // TB1.1 high for half of the period

    // Set up TB1.2 for 25% duty cycle
    TB1CCTL2 = OUTMOD_7;    // Set/reset mode for TB1.2 (25% duty cycle)
    TB1CCR2 = 500;         // TB1.2 high for a quarter of the period

    // Start the timer in up mode
    TB1CTL = TBSSEL_2 | MC_1 | TBCLR;  // SMCLK as clock source, up mode
}

void configure_clocks() {
    // Unlock CS registers
    CSCTL0 = CSKEY;

    // Set DCO to 8 MHz (DCOFSEL_3 with DCORSEL = 0)
    CSCTL1 |= DCOFSEL_3;      // DCO frequency select: 8 MHz, DCORSEL = 0

    // Set DCO as the source for SMCLK
    CSCTL2 |= SELS__DCOCLK;

    // Set SMCLK divider to 32
    CSCTL3 |= DIVS__1;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
    configure_clocks();
    configure_timer_b();        // Configure Timer B to produce PWM

    while (1) {
        __no_operation();       // Main loop, do nothing
    }
}

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
    TB1CCR0 = (2000 - 1);    // Set timer period

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

void configure_timer_a_capture() {
    // Configure P1.2 as Timer A input (TA1.1) for capturing PWM signal from P3.4
    P1DIR &= ~BIT2;         // Set P1.2 as input
    P1SEL1 |= BIT2;         // Select TA1.1 function (capture input)
    P1SEL0 &= ~BIT2;        // Clear P1SEL0 to set as Timer A capture

    // Stop Timer A during setup
    TA1CTL = TASSEL_2 | MC_0 | TACLR;  // SMCLK as clock source, stop mode, clear timer

    // Set up Timer A for capture on both rising and falling edges
    TA1CCTL1 = CM_3 | CCIS_0 | CAP | SCS | CCIE; // Capture both edges, capture mode, enable interrupt

    // Start Timer A in continuous mode
    TA1CTL = TASSEL_2 | MC_2 | TACLR;  // SMCLK as clock source, continuous mode, clear timer
}

// Timer A interrupt service routine to handle capture events
#pragma vector = TIMER1_A1_VECTOR
__interrupt void Timer_A_Capture_ISR(void) {
    static unsigned int rising_edge = 0, falling_edge = 0, pulse_width = 0;

    if (TA1CCTL1 & CCI) {  // Check if it's a rising edge
        rising_edge = TA1CCR1;  // Capture rising edge time
    } else {  // Falling edge
        falling_edge = TA1CCR1;  // Capture falling edge time
        pulse_width = falling_edge - rising_edge;  // Calculate pulse width (in clock cycles)

        // Breakpoint here to check the value of pulse_width in the debugger
        __no_operation();  // Placeholder for debugger to check pulse_width
    }

    TA1CCTL1 &= ~CCIFG;  // Clear capture interrupt flag
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer

    configure_clocks();         // Configure SMCLK to 8 MHz
    configure_timer_b();        // Configure Timer B to produce PWM
    configure_timer_a_capture(); // Configure Timer A for pulse capture

    __bis_SR_register(GIE);     // Enable global interrupts

    while (1) {
        __no_operation();       // Main loop, do nothing
    }
}

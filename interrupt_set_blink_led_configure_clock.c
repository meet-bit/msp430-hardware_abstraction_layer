#include <msp430.h> 

void configure_clocks() {
    // Unlock CS registers
    CSCTL0 = CSKEY;

    // Set DCO to 8 MHz (DCOFSEL_3 with DCORSEL = 0)
    CSCTL1 |= DCOFSEL_3;      // DCO frequency select: 8 MHz, DCORSEL = 0

    // Set DCO as the source for SMCLK
    CSCTL2 |= SELS__DCOCLK;

    // Set SMCLK divider to 32
    CSCTL3 |= DIVS__32;

    // Set P3.4 as output for SMCLK
    P3DIR |= BIT4;           // Set P3.4 as an output
    P3SEL1 |= BIT4;          // Select SMCLK function for P3.4
    P3SEL0 |= BIT4;
}


void set_leds() {
    // Set the LEDs 1 to 8 to output 10010011 (0x93)
    // LEDs 1-4: PJ.0 to PJ.3, LEDs 5-8: P3.4 to P3.7
    PJOUT = (PJOUT & ~(BIT0 | BIT1 | BIT2 | BIT3)) | (BIT0 | BIT1);  // Set PJOUT to 0011 for PJ.0 and PJ.1, reset others
    P3OUT = (P3OUT & ~(BIT4 | BIT5 | BIT6 | BIT7)) | (BIT7 | BIT4);  // Set P3OUT to 1001 for P3.4 and P3.7, reset others
}

void delay() {
    volatile unsigned int i;
    for (i = 50000; i > 0; i--);  // Simple delay loop
}


void configure_pins(){
    // Set PJ.0, PJ.1, PJ.2, and PJ.3 as GPIO outputs and clear their function select bits
    PJDIR |= (BIT0 | BIT1 | BIT2 | BIT3);  // Set PJ.0 to PJ.3 as outputs
    PJSEL1 &= ~(BIT0 | BIT1 | BIT2 | BIT3);  // Clear PJSEL1 for PJ.0 to PJ.3
    PJSEL0 &= ~(BIT0 | BIT1 | BIT2 | BIT3);  // Clear PJSEL0 for PJ.0 to PJ.3


     P3DIR |= (BIT4 | BIT5 | BIT6 | BIT7);
     P3SEL1 &= ~(BIT4 | BIT5 | BIT6 | BIT7);
     P3SEL0 &= ~(BIT4 | BIT5 | BIT6 | BIT7);



}


void configure_pins_for_interrupt() {
    // Set P4.0 and P4.1 as inputs
    P4DIR &= ~(BIT0 | BIT1);   // Clear P4.0 and P4.1 bits to configure as input

    // Disable function selection for P4.0 and P4.1 (set as GPIO)
    P4SEL1 &= ~(BIT0 | BIT1);  // Clear P4SEL1 for P4.0 and P4.1
    P4SEL0 &= ~(BIT0 | BIT1);  // Clear P4SEL0 for P4.0 and P4.1

    // Enable internal pull-up resistors for P4.0 and P4.1
    P4REN |= (BIT0 | BIT1);    // Enable pull-up/pull-down resistors on P4.0 and P4.1
    P4OUT |= (BIT0 | BIT1);    // Select pull-up resistors (set bits in P4OUT)

    // Enable interrupt on P4.0 and P4.1 for rising edge
    P4IES &= ~(BIT0 | BIT1);   // Interrupt on rising edge (clear bits in P4IES)
    P4IE |= (BIT0 | BIT1);     // Enable interrupt for P4.0 and P4.1 (set bits in P4IE)

    // Clear any pending interrupts
    P4IFG &= ~(BIT0 | BIT1);   // Clear interrupt flags for P4.0 and P4.1

    // Configure P3.6 (LED7) and P3.7 (LED8) as outputs
    P3DIR |= (BIT6 | BIT7);    // Set P3.6 and P3.7 as outputs
    P3OUT &= ~(BIT6 | BIT7);   // Initialize LEDs as off (set P3.6 and P3.7 low)
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer

    __bis_SR_register(GIE);             // Enable global interrupts

    while (1) {
        __delay_cycles(800000);  // Simple delay (~0.1 second at 8 MHz)
    }

}



// Port 4 interrupt service routine (ISR)
#pragma vector = PORT4_VECTOR
__interrupt void Port_4_ISR(void) {
    if (P4IFG & BIT0) {         // Check if P4.0 (S1) caused the interrupt
        P4IFG &= ~BIT0;         // Clear P4.0 interrupt flag
        P3OUT ^= BIT6;          // Toggle LED7 (P3.6)
    }
    if (P4IFG & BIT1) {         // Check if P4.1 (S2) caused the interrupt
        P4IFG &= ~BIT1;         // Clear P4.1 interrupt flag
        P3OUT ^= BIT7;          // Toggle LED8 (P3.7)
    }
}




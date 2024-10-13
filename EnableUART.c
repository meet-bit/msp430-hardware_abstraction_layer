
#include "msp430fr5739.h"
#include "msp430fr57xxgeneric.h"

// Clock initialization (SMCLK = 1 MHz)
void clkInit() {
    CSCTL0 = 0xA500;                  // Write password to modify CS registers
    CSCTL1 = DCOFSEL_3;               // Set DCO to 8 MHz
    CSCTL2 = SELM__DCOCLK | SELS__DCOCLK | SELA__DCOCLK;  // Set MCLK, SMCLK, ACLK to DCO
    CSCTL3 = DIVA__8 | DIVS__8;       // Divide SMCLK and ACLK by 8 (1 MHz)
    CSCTL0_H = 0;                     // Lock CS registers
}


// Function to configure UART with correct baud rate and settings
void configure_UART() {
    // Select SMCLK for UART and configure UART pins
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |= (BIT0 | BIT1);

    UCA0CTLW0 |= UCSWRST;              // Put UART in reset mode
    UCA0CTLW0 |= UCSSEL__SMCLK;        // Use SMCLK (1 MHz after division)

    UCA0BRW = 104;                     // Set baud rate for 9600 (SMCLK 1 MHz)
    UCA0MCTLW = 0xD600;                // Set modulation UCBRSx=0xD6, UCOS16=1

    UCA0CTLW0 &= ~UCSWRST;             // Release UART from reset
    UCA0IE |= UCRXIE;                  // Enable UART Rx interrupt
}

void configure_LED() {
    // Configure P1.0 (LED1) as output
    PJDIR |= BIT0;                     // Set P1.0 as output
    PJOUT &= ~BIT0;                    // Initialize LED1 as off
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;          // Stop watchdog timer


    clkInit();
    configure_UART();                  // Configure UART
    configure_LED();                   // Configure LED1 (P1.0)

    __bis_SR_register(GIE);            // Enable global interrupts

    while (1) {
        __no_operation();              // Idle loop
    }
}

// UART ISR to echo received byte, send the next byte, and control LED1
#pragma vector = USCI_A0_VECTOR
__interrupt void uart_ISR(void) {
    if (UCA0IFG & UCRXIFG) {            // Check if RX interrupt occurred
        unsigned char RxByte = UCA0RXBUF;   // Read received byte


        // Wait for the transmit buffer to be ready before sending the next byte
        while (!(UCA0IFG & UCTXIFG));   // Ensure previous byte is fully sent
        UCA0TXBUF = RxByte;         // Transmit next byte in ASCII table

        // Wait for the transmit buffer to be ready before sending the next byte
        while (!(UCA0IFG & UCTXIFG));   // Ensure previous byte is fully sent
        UCA0TXBUF = RxByte + 1;         // Transmit next byte in ASCII table

        // Control LED1 based on the received byte
        if (RxByte == 'j') {
            PJOUT |= BIT0;              // Turn on LED1 (P1.0) when 'j' is received
        } else if (RxByte == 'k') {
            PJOUT &= ~BIT0;             // Turn off LED1 (P1.0) when 'k' is received
        }
    }
}

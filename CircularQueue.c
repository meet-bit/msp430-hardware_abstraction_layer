#include "msp430fr5739.h"
#include "msp430fr57xxgeneric.h"

// Circular buffer parameters
#define BUFFER_SIZE 50

// Circular buffer variables
unsigned char circular_buffer[BUFFER_SIZE]; // Circular buffer
volatile unsigned int head = 0;             // Head index
volatile unsigned int tail = 0;             // Tail index
volatile unsigned int count = 0;            // Current count of elements in the buffer

// Add data to the circular buffer
void circular_buffer_add(unsigned char data) {
    if (count < BUFFER_SIZE) {
        circular_buffer[head] = data; // Add the new data
        head = (head + 1) % BUFFER_SIZE; // Move head pointer
        count++; // Increase the count
    } else {
        // Buffer overrun error
        while (!(UCA0IFG & UCTXIFG)); // Wait for transmit buffer to be ready
        UCA0TXBUF = 'E'; // Send error message (example: 'E' for overrun)
        return 0;
    }
}

// Remove data from the circular buffer
unsigned char circular_buffer_remove() {
    if (count > 0) {
        unsigned char data = circular_buffer[tail]; // Get the data
        tail = (tail + 1) % BUFFER_SIZE; // Move tail pointer
        count--; // Decrease the count
        return data; // Return the data
    } else {
        // Buffer underrun error
        while (!(UCA0IFG & UCTXIFG)); // Wait for transmit buffer to be ready
        UCA0TXBUF = 'U'; // Send error message (example: 'U' for underrun)
        return 0;
    }
}

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
    UCA0IE |= UCRXIE; // Enable UART Rx interrupt
}




void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    clkInit();
    configure_UART();

    __bis_SR_register(GIE); // Enable global interrupts

    while (1) {
        UCA0TXBUF = 'A'; // Continuously transmit 'A'
    }
}

// UART ISR to handle received data
#pragma vector = USCI_A0_VECTOR
__interrupt void uart_ISR(void) {
    unsigned char RxByte = UCA0RXBUF; // Read received byte

    if (RxByte != 13) { // Ignore carriage return (ASCII 13)
        circular_buffer_add(RxByte); // Add received byte to the circular buffer
    }

    if (RxByte == 13) { // Check for carriage return
        unsigned char removedByte = circular_buffer_remove(); // Remove a byte from the buffer
        while (!(UCA0IFG & UCTXIFG)); // Wait for transmit buffer to be ready
        UCA0TXBUF = removedByte; // Transmit the removed byte back
    }
}

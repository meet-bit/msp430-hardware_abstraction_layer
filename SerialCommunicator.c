#include "msp430fr5739.h"

// Circular buffer parameters
#define BUFFER_SIZE 50

// Circular buffer variables
unsigned char circular_buffer[BUFFER_SIZE]; // Circular buffer
volatile unsigned int head = 0;             // Head index
volatile unsigned int tail = 0;             // Tail index
volatile unsigned int count = 0;            // Current count of elements in the buffer

// Function Prototypes
void circular_buffer_add(unsigned char data);
unsigned char circular_buffer_remove();
unsigned char circular_buffer_peek(unsigned int index);
void clkInit();
void configure_UART();
void configure_LED1();
void control_LED1(unsigned char state);
void configure_timer_b(unsigned int period);
void transmit_response(unsigned int data);

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
        return 0;  // No underrun check
    }
}

// Peek data from the circular buffer without removing
unsigned char circular_buffer_peek(unsigned int index) {
    if (index < count) {
        unsigned int pos = (tail + index) % BUFFER_SIZE;
        return circular_buffer[pos];
    } else {
        return 0; // Index out of bounds
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
    UCA0IE |= UCRXIE;                  // Enable UART Rx interrupt
}

// Function to configure LED1 (PJ.0)
void configure_LED1() {
    PJDIR |= BIT0;  // Set PJ.0 as output for LED1
    PJOUT &= ~BIT0; // Initialize LED1 as off
}

// Function to control LED1 state (on/off)
void control_LED1(unsigned char state) {
    if (state == 1) {
        PJOUT |= BIT0;  // Turn on LED1
    } else {
        PJOUT &= ~BIT0; // Turn off LED1
    }
}

// Configure Timer B with period and duty cycle dynamically
void configure_timer_b(unsigned int period) {
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

    // Set Timer B with the dynamic period
    TB1CCR0 = period - 1;     // Timer period (from the message)
    TB1CCTL1 = OUTMOD_7;      // Set/reset mode for TB1.1 (50% duty cycle)
    TB1CCR1 = period / 2;     // 50% duty cycle for TB1.1

    TB1CCTL2 = OUTMOD_7;      // Set/reset mode for TB1.2 (25% duty cycle)
    TB1CCR2 = period / 4;     // 25% duty cycle for TB1.2

    // Start the timer in up mode
    TB1CTL = TBSSEL_2 | MC_1 | TBCLR;  // SMCLK as clock source, up mode
}

void main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    clkInit();
    configure_UART();
    configure_LED1();         // Set up LED1 (PJ.0)

    __bis_SR_register(GIE); // Enable global interrupts

    while (1) {
        // Check if a complete packet (5 bytes) is available
        if (count >= 5) {
            // Look for the start byte (0xFF)
            unsigned int i;
            for (i = 0; i <= (count - 5); i++) {
                if (circular_buffer_peek(i) == 0xFF) {
                    // Potential start of packet found
                    unsigned char command = circular_buffer_peek(i + 1);
                    unsigned char data_byte1 = circular_buffer_peek(i + 2);
                    unsigned char data_byte2 = circular_buffer_peek(i + 3);
                    unsigned char escape_byte = circular_buffer_peek(i + 4);

                    // Handle LED control commands (0x02 = turn on LED1, 0x03 = turn off LED1)
                    if (command == 0x02) {
                        control_LED1(1);  // Turn on LED1
                    } else if (command == 0x03) {
                        control_LED1(0);  // Turn off LED1
                    }

                    // Validate command byte for timer (0x01)
                    if (command == 0x01) {
                        // Handle escape bytes
                        if (escape_byte & 0x01) {
                            data_byte1 = 0xFF;
                        }
                        if (escape_byte & 0x02) {
                            data_byte2 = 0xFF;
                        }

                        // Combine data bytes into a 16-bit number
                        unsigned int received_data = ((unsigned int)data_byte1 << 8) | data_byte2;

                        // Remove processed bytes from the buffer
                        unsigned int j;
                        for (j = 0; j < (i + 5); j++) {
                            circular_buffer_remove();
                        }

                        // Transmit the response and configure Timer B
                        transmit_response(received_data);
                        configure_timer_b(received_data);

                        // Break out of the loop to process next packet
                        break;
                    } else {
                        // Invalid command byte, discard the start byte
                        circular_buffer_remove(); // Remove the start byte
                    }
                } else {
                    // Not a start byte, discard the byte
                    circular_buffer_remove();
                }
            }
        }
    }
}

// UART ISR to handle received data
#pragma vector = USCI_A0_VECTOR
__interrupt void uart_ISR(void) {
    unsigned char RxByte = UCA0RXBUF; // Read received byte
    circular_buffer_add(RxByte);      // Add received byte to the circular buffer
}

// Function to transmit a response (for testing purposes)
void transmit_response(unsigned int data) {
    // Example: Echo back the received 16-bit data
    unsigned char upper_byte = (data >> 8) & 0xFF;
    unsigned char lower_byte = data & 0xFF;

    // Transmit start byte
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = 0xFF;

    // Transmit command byte (e.g., 0x02 for response)
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = 0x02;

    // Transmit data bytes
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = upper_byte;

    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = lower_byte;

    // Transmit escape byte (set bits if any data byte is 0xFF)
    unsigned char escape_byte = 0x00;
    if (upper_byte == 0xFF) escape_byte |= 0x01;
    if (lower_byte == 0xFF) escape_byte |= 0x02;

    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = escape_byte;
}

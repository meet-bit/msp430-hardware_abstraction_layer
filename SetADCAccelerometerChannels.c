#include "msp430fr5739.h"

// Define the data packet start byte
#define START_BYTE 255

// ADC channels for accelerometer (X: A12, Y: A13, Z: A14)
#define ADC_X_CHANNEL ADC10INCH_12
#define ADC_Y_CHANNEL ADC10INCH_13
#define ADC_Z_CHANNEL ADC10INCH_14

unsigned int adc_value;  // Store raw 10-bit ADC result
unsigned char x_axis, y_axis, z_axis;  // Store 8-bit ADC results
unsigned char current_axis = 0;        // Track which axis is currently being sampled
volatile unsigned char transmit = 0;   // Flag to trigger data transmission

// Function Prototypes
void configure_UART();
void configure_ADC10();
void configure_timer_interrupt();
void configure_p2_7();
void transmit_data();
void clkInit();
unsigned char sampleADC(unsigned int channel);

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
}

// Function to configure ADC for sampling accelerometer values
void configure_ADC10() {
    // Enable ADC10, set sample and hold time to 16 ADC10CLK cycles, and turn ADC on
    ADC10CTL0 = ADC10SHT_2 | ADC10ON;  // Sample and hold time = 16 ADC10CLK cycles, ADC on
    ADC10CTL1 = ADC10SHP | ADC10SSEL_3; // Use sampling timer, SMCLK source
    ADC10CTL2 = ADC10RES;              // 10-bit resolution
}

// Function to configure Timer A for periodic interrupts (every 40 ms, 25 Hz)
void configure_timer_interrupt() {
    TA0CCR0 = 40000 - 1;               // Timer period for 40 ms (SMCLK 1 MHz / 25Hz)
    TA0CCTL0 = CCIE;                   // Enable Timer A interrupt
    TA0CTL = TASSEL_2 | MC_1 | TACLR;  // SMCLK, up mode, clear timer
}

// Function to power accelerometer via P2.7
void configure_p2_7() {
    P2DIR |= BIT7;                     // Set P2.7 as output
    P2OUT |= BIT7;                     // Set P2.7 high to power accelerometer
}

// Function to transmit data via UART (start byte, X, Y, Z)
void transmit_data() {
    transmitChar(START_BYTE);           // Transmit start byte (255)
    transmitChar(x_axis);               // Transmit X-axis data
    transmitChar(y_axis);               // Transmit Y-axis data
    transmitChar(z_axis);               // Transmit Z-axis data
}

// Helper function to transmit a character via UART
void transmitChar(unsigned char data) {
    while (!(UCA0IFG & UCTXIFG));       // Wait for transmit buffer to be ready
    UCA0TXBUF = data;                   // Transmit character
}

// Function to sample ADC for given channel (X, Y, or Z)
unsigned char sampleADC(unsigned int channel) {
    ADC10CTL0 &= ~ADC10ENC;            // Disable ADC before changing channels
    ADC10MCTL0 = channel;              // Select the channel (X, Y, or Z axis)
    ADC10CTL0 |= ADC10ENC | ADC10SC;   // Enable and start conversion
    while (ADC10CTL1 & ADC10BUSY);     // Wait until conversion completes
    return ADC10MEM0 >> 2;             // Return 8-bit result (shifted 10-bit)
}

// Timer A0 ISR (triggered every 40 ms)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
    switch (current_axis) {
        case 0:  // Sample X-axis (A12)
            x_axis = sampleADC(ADC_X_CHANNEL);  // Sample and store X-axis
            current_axis = 1;                   // Move to Y-axis next
            break;
        case 1:  // Sample Y-axis (A13)
            y_axis = sampleADC(ADC_Y_CHANNEL);  // Sample and store Y-axis
            current_axis = 2;                   // Move to Z-axis next
            break;
        case 2:  // Sample Z-axis (A14)
            z_axis = sampleADC(ADC_Z_CHANNEL);  // Sample and store Z-axis
            current_axis = 0;                   // Move back to X-axis
            transmit = 1;                       // Set transmit flag
            break;
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

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;         // Stop watchdog timer

    clkInit();                        // Initialize clocks
    configure_p2_7();                 // Power accelerometer using P2.7
    configure_ADC10();                // Set up ADC for accelerometer
    configure_UART();                 // Set up UART for 9600 baud
    configure_timer_interrupt();      // Set up Timer A interrupt

    __bis_SR_register(GIE);           // Enable global interrupts

    while (1) {
        if (transmit == 1) {          // Check if transmit flag is set
            transmit_data();          // Transmit data via UART
            transmit = 0;             // Clear transmit flag
        }
    }
}

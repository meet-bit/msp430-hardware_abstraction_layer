#include "msp430fr5739.h"

// ADC channel for the NTC temperature sensor
#define ADC_NTC_CHANNEL ADC10INCH_1
#define START_BYTE 255

unsigned int adc_value;  // Store raw 10-bit ADC result
unsigned char temperature;  // Store 8-bit temperature result
volatile unsigned char transmit = 0;   // Flag to trigger data transmission

// Function Prototypes
void configure_UART();
void configure_ADC10();
void configure_timer_interrupt();
void configure_p2_7();
void transmit_data();
void clkInit();
unsigned char sampleADC(unsigned int channel);
void update_LEDs(unsigned char temp);

// Function to configure UART with correct baud rate and settings
void configure_UART() {
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |= (BIT0 | BIT1);

    UCA0CTLW0 |= UCSWRST;              // Put UART in reset mode
    UCA0CTLW0 |= UCSSEL__SMCLK;        // Use SMCLK (1 MHz after division)

    UCA0BRW = 104;                     // Set baud rate for 9600 (SMCLK 1 MHz)
    UCA0MCTLW = 0xD600;                // Set modulation UCBRSx=0xD6, UCOS16=1

    UCA0CTLW0 &= ~UCSWRST;             // Release UART from reset
}

// Function to configure ADC for sampling temperature sensor (NTC)
void configure_ADC10() {
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

// Function to power the NTC sensor via P2.7
void configure_p2_7() {
    P2DIR |= BIT7;                     // Set P2.7 as output
    P2OUT |= BIT7;                     // Set P2.7 high to power NTC sensor
}

// Function to transmit temperature data via UART
void transmit_data() {
    transmitChar(START_BYTE);           // Transmit start byte (255)
    transmitChar(temperature);          // Transmit temperature data
}

// Helper function to transmit a character via UART
void transmitChar(unsigned char data) {
    while (!(UCA0IFG & UCTXIFG));       // Wait for transmit buffer to be ready
    UCA0TXBUF = data;                   // Transmit character
}

// Function to sample ADC for given channel (NTC)
unsigned char sampleADC(unsigned int channel) {
    ADC10CTL0 &= ~ADC10ENC;            // Disable ADC before changing channels
    ADC10MCTL0 = channel;              // Select the channel (NTC sensor)
    ADC10CTL0 |= ADC10ENC | ADC10SC;   // Enable and start conversion
    while (ADC10CTL1 & ADC10BUSY);     // Wait until conversion completes
    return ADC10MEM0 >> 2;             // Return 8-bit result (shifted 10-bit)
}

// Function to update LEDs based on temperature value
void update_LEDs(unsigned char temp) {
    // Set P3.4 to P3.7 and PJ.0 to PJ.3 as outputs for the LEDs
    PJDIR |= (BIT0 | BIT1 | BIT2 | BIT3);    // PJ.0 - PJ.3 as output
    P3DIR |= (BIT4 | BIT5 | BIT6 | BIT7);    // P3.4 - P3.7 as output

    PJOUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);   // Turn off all PJ LEDs
    P3OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);   // Turn off all P3 LEDs

    // Light up LEDs based on temperature value
    if (temp > 190) {
        PJOUT |= BIT0; // LED1 (PJ.0)
    } else if (temp > 189) {
        PJOUT |= BIT0 | BIT1; // LED1 & LED2 (PJ.0, PJ.1)
    } else if (temp > 188) {
        PJOUT |= BIT0 | BIT1 | BIT2; // LED1 to LED3 (PJ.0, PJ.1, PJ.2)
    } else if (temp > 185) {
        PJOUT |= BIT0 | BIT1 | BIT2 | BIT3; // LED1 to LED4 (PJ.0 to PJ.3)
    } else if (temp > 182) {
        PJOUT |= BIT0 | BIT1 | BIT2 | BIT3; // PJ LEDs
        P3OUT |= BIT4; // LED5 (P3.4)
    } else if (temp > 180) {
        PJOUT |= BIT0 | BIT1 | BIT2 | BIT3; // PJ LEDs
        P3OUT |= BIT4 | BIT5; // LED5 & LED6 (P3.4, P3.5)
    } else if (temp > 179) {
        PJOUT |= BIT0 | BIT1 | BIT2 | BIT3; // PJ LEDs
        P3OUT |= BIT4 | BIT5 | BIT6; // LED5 to LED7 (P3.4 to P3.6)
    } else {
        PJOUT |= BIT0 | BIT1 | BIT2 | BIT3; // PJ LEDs
        P3OUT |= BIT4 | BIT5 | BIT6 | BIT7; // All LEDs on
    }
}

// Timer A0 ISR (triggered every 40 ms)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_ISR(void) {
    temperature = sampleADC(ADC_NTC_CHANNEL);  // Sample temperature from NTC sensor
    update_LEDs(temperature);                  // Update LED display based on temperature
    transmit = 1;                              // Set transmit flag
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
    configure_p2_7();                 // Power NTC sensor using P2.7
    configure_ADC10();                // Set up ADC for NTC sensor
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

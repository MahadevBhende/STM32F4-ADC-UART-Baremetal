#include <stdint.h>
#include <stdio.h>

unsigned int ADC_Val, ADC_Val1;

// RCC Base address
#define RCC_B         0x40023800
#define RCC_AHB1EN    (*(volatile uint32_t*)(RCC_B + 0x30))
#define RCC_APB2EN    (*(volatile uint32_t*)(RCC_B + 0x44))

// GPIOA Address
#define GPIOA_B       0x40020000
#define GPIOA_MODER   (*(volatile uint32_t*)(GPIOA_B + 0x00))
#define GPIOA_AFRH    (*(volatile uint32_t*)(GPIOA_B + 0x24))

// USART1 registers
#define USART1_BASE   0x40011000
#define USART1_SR     (*(volatile uint32_t*)(USART1_BASE + 0x00))
#define USART1_DR     (*(volatile uint32_t*)(USART1_BASE + 0x04))
#define USART1_BRR    (*(volatile uint32_t*)(USART1_BASE + 0x08))
#define USART1_CR1    (*(volatile uint32_t*)(USART1_BASE + 0x0C))

// ADC1 registers
#define ADC1_B        0x40012000
#define ADC1_SR       (*(volatile uint32_t*)(ADC1_B + 0x00))
#define ADC1_CR2      (*(volatile uint32_t*)(ADC1_B + 0x08))
#define ADC1_SMPR2    (*(volatile uint32_t*)(ADC1_B + 0x10))
#define ADC1_SQR3     (*(volatile uint32_t*)(ADC1_B + 0x34))
#define ADC1_DR       (*(volatile uint32_t*)(ADC1_B + 0x4C))

// Utility function: Send a byte via UART1
void USART1_Write(uint8_t ch) {
    while (!(USART1_SR & (1 << 7)));  // Wait until TXE = 1
    USART1_DR = ch;
}

// Print a string via UART1
void USART1_Print(char *str) {
    while (*str) {
        USART1_Write(*str++);
    }
}

// Redirect printf to UART1
int __io_putchar(int ch) {
    USART1_Write(ch);
    return ch;
}

// Initialize UART1 @ 9600 baud
void UART1_Init(void) {
    RCC_APB2EN |= (1 << 4);      // Enable USART1 clock
    RCC_AHB1EN |= (1 << 0);      // Enable GPIOA clock

    // Configure PA9 as Alternate Function (USART1_TX)
    GPIOA_MODER &= ~(3 << (9 * 2));
    GPIOA_MODER |=  (2 << (9 * 2));      // AF mode
    GPIOA_AFRH &= ~(0xF << ((9 - 8) * 4));
    GPIOA_AFRH |=  (0x7 << ((9 - 8) * 4)); // AF7 for USART1

    // Baud rate for 16 MHz PCLK, 9600 baud
    USART1_BRR = 0x0683;  // Mantissa and fraction

    // Enable USART, Transmitter
    USART1_CR1 |= (1 << 13) | (1 << 3);
}

void ADC_Init_Chan2(void) {
    // Enable clock to GPIOA and ADC1 Clock
    RCC_AHB1EN |= (1 << 0); // GPIOA clock
    RCC_APB2EN |= (1 << 8); // ADC1 clock

    // Configure PA2 for Analog Mode
    GPIOA_MODER &= ~(3 << (2 * 2));
    GPIOA_MODER |=  (3 << (2 * 2)); // Analog mode

    // Set the Sampling Time for channel 2
    ADC1_SMPR2 |= (0x7 << 6); // Max sampling time

    // Configure ADC regular sequence register channel 2
    ADC1_SQR3 = 2;

    // Continuous Conversion mode
    ADC1_CR2 |= (1 << 1);

    // Enable ADC
    ADC1_CR2 |= (1 << 0);

    // Start ADC Conversion
    ADC1_CR2 |= (1 << 30);
}

unsigned int ADC1_Read(void) {
    while (!(ADC1_SR & (1 << 1))); // Wait for EOC
    return ADC1_DR;                // Read ADC value
}

int main(void) {
    char buffer[64];

    UART1_Init();
    ADC_Init_Chan2();

    while (1) {
        ADC_Val = ADC1_Read();
        sprintf(buffer, "ADC : %d \r\n", ADC_Val);
        USART1_Print(buffer);
        for (volatile int i = 0; i < 1000000; i++); // crude delay

    }
    return 0;
}

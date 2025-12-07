/*
 * uart.c
 *
 *  Created on: Nov 24, 2025
 *      Author: dafe2002
 */

#include "uart.h"
#include <stdarg.h>
#include <stdio.h>

/////////////// UART

// Blocking RX
int getchar_polled(void) {
    while (!(USART2->SR & USART_SR_RXNE));
    return USART2->DR;
}

// Blocking TX
void putchar_polled(int c) {
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = c;
}

// Non-blocking RX - returns -1 if no data available
int getchar_nonblocking(void) {
    if (USART2->SR & USART_SR_RXNE) {
        return USART2->DR;
    }
    return -1;
}

/////////// SENDING STRINGS OUT

void sendString(const char *str) {
    while (*str) {
    	putchar_polled(*str++);
    }
}

void sendStringGreen(const char *str){
	sendString(ANSI_GREEN_BOLD);
	sendString(str);
	sendString(ANSI_RESET);
}

void sendCharGreen(uint8_t ch) {
    sendString(ANSI_GREEN_BOLD);
    putchar_polled(ch);
    sendString(ANSI_RESET);
}

void UART_Init(void)
{
    // clock enable for USART2 and GPIOA
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA2 and PA3 setting to alternate functions
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);
    GPIOA->AFR[0] &= ~((0xF << 8) | (0xF << 12)); // clear the AFs
    GPIOA->AFR[0] |= (7 << 8) | (7 << 12); // set AF7

    GPIOA->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR3);
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT_2 | GPIO_OTYPER_OT_3);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);
    GPIOA->PUPDR |= (GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0);  // Pull-up

    // stop UART for configuring
    USART2->CR1 &= ~USART_CR1_UE;

    // Set baud rate to 115200
    // USART2 is on APB1
    USART2->BRR = 0xD9;

    // UART settings
    // 8 data bits, 1 stop bit, no parity
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    // enable UART, TX, RX, RXNE interrupts
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
}

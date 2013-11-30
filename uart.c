/*
 * This file is part of the MSP430 SPI example.
 *
 * Copyright (C) 2012 Stefan Wendler <sw@kaltpost.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
 * SPI example for MSP430
 *
 * Initialize UART on USCI_A at 9600 bauds.
 *
 * Stefan Wendler
 * sw@kaltpost.de
 * http://gpio.kaltpost.de
 ******************************************************************************/

#include <msp430.h>
#include <legacymsp430.h>

#include "uart.h"

/**
 * Receive Data (RXD) at P1.1
 */
#define RXD  BIT1

/**
 * Transmit Data (TXD) at P1.2
 */
#define TXD  BIT2

char* command_list[NO_OF_COMMANDS] = {"help", "set ", "read ", "restart"};
// types: n = no param, s - string parameter, d - digits
char* command_list_type[NO_OF_COMMANDS] = {'n', "sd", 's', 'n'};
char* command_list_desc[NO_OF_COMMANDS] = {"______list available commannds", "______set [register] [value] via spi", "_____read [register]", "__restart the program"};


static volatile char command_available = 0;
char data[MAX_COMMAND_LENGTH];
char command[MAX_COMMAND_LENGTH];

void uart_init(void)
{
  P1SEL  |= RXD + TXD;                       
  P1SEL2 |= RXD + TXD;                       
  UCA0CTL1 |= UCSSEL_2;    // SMCLK
  UCA0BR0 = 0x45;          // 16MHz 112500 baud  
  UCA0BR1 = 0;             // 16MHz 112500 baud
  UCA0MCTL = UCBRS_4 + UCBRF_0;             
  UCA0CTL1 &= ~UCSWRST;    // Initialize USCI state machine
  IE2 |= UCA0RXIE;         // Enable USCI_A0 RX interrupt
}

void uart_putc(unsigned char c)
{
  while (!(IFG2&UCA0TXIFG));    // USCI_A0 TX buffer ready?
  UCA0TXBUF = c;                // TX
}

void uart_puts(const char *str)
{
  while(*str) uart_putc(*str++);
}


//TODO add commands without params, add params string / number
char command_check(char command[])
{
  char i;
  for (i = 0; i < NO_OF_COMMANDS; i++)
  {
    char* p = command;
    char* r = command_list[i];
    while(*r)
    {
      // debug info
      //uart_putc('\n');  // debug
      //uart_putc(*r);    // debug
      //uart_putc(' ');   // debug
      //uart_putc(*p);    // debug
      //uart_putc('\n');  // debug
      //uart_putc('\r');  // debug
      if(*p++ != *r++)
        break;
      else if (*r == '\0')
      {
        //uart_puts((char *)"valid command entered \n\r");  // debug
        return i;
      }
    }
  }
  return -1;
}


void command_execution (char command)
{
  switch(command)
  {
    case 0:
    {
      uart_puts((char *)"Available commands: \n\r");
      int i;
      for (i=0; i<NO_OF_COMMANDS; i++)
      {
        uart_puts(command_list[i]);
        uart_puts(command_list_desc[i]);
        uart_puts((char *)"\r\n");
      } 
      uart_puts((char *)"\n");
      break; 
    }
    case 1:
    {
      uart_puts((char *)"set command executed\n\r");
      break;
    }
    case 2:
    {
      uart_puts((char *)"read command executed\n\r");
      break;
    }
    case 3:
    {
      uart_puts((char *)"restart command executed\n\r");
      break;
    }
    default :
    {
        uart_puts((char *)"invalid command. type 'help' for the list of available commands. \n\r");
    }
  }  
}

//receive interrupt
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
  static char i = 0;
  while (!(IFG2&UCA0TXIFG));  // USCI_A0 TX buffer ready?
  data[i] = UCA0RXBUF;        // put incoming char to a buffer
  uart_putc(data[i++]);       // print incoming char

  // check if received command
  switch(data[i-1])
  {
    // enter
    case '\r':
    {
       command_available = 1;
       uart_putc('\n');
       i-=2;
       command[i+1] = '\0';
       while(i+1)
       {
         command[i] = data[i--];
       }
       i = 0;
       
       //uart_puts((char *)"string entered: ");  // debug
       //uart_puts(command);                     // debug
       //uart_puts((char *)"\n\r");              // debug
       command_execution(command_check(command));
       //uart_puts("command entered: ");
       //uart_putc(command_check(command)+'0');
       //uart_puts("\n\r");
       break; 
    }
    // backspace
    case 0x7F:
    {
       i-=2;
       break; 
    }    
  }

  if (i == MAX_COMMAND_LENGTH) i = 0;
}

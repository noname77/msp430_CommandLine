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
 * MSP430 command line
 *
 * Initialize UART on USCI_A at 115200 bauds.
 *
 * Wiktor Grajkowski
 * wiktor.grajkowski@gmail.com
 * http://flemingsociety.wordpress.com
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

char* command_list[NO_OF_COMMANDS] = {"help", "set", "read", "restart"};
// types: n = no param, s - string parameter, d - digits
char* command_list_type[NO_OF_COMMANDS] = {'n', "sd", 's', 'n'};
char* command_list_desc[NO_OF_COMMANDS] = {"______list available commannds", "______set [register] [value] via spi", "_____read [register]", "__restart the program"};


static volatile char command_available = 0;
char data[MAX_COMMAND_LENGTH];      // receive buffer
char command[MAX_COMMAND_LENGTH];   // command entered
char payload[MAX_PARAMS][MAX_PARAM_LEN];
char* pl;

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

// put char
void uart_putc(unsigned char c)
{
  while (!(IFG2&UCA0TXIFG));    // USCI_A0 TX buffer ready?
  UCA0TXBUF = c;                // TX
}

// put string
void uart_puts(const char *str)
{
  while(*str) uart_putc(*str++);
}

void command_type_check (char command, char* payload)
{
  char* p = payload;
  char* r = command_list_type[command];
  
  while (*r)
  {
    switch(*r++)
    {
      case 'n':
      {
        command_execution(command);
        break;
      }
      case 's':
      {
        
        break;
      }
      case 'd':
      {
        
        break;
      }      
    }
  }
}

void payload_split(char* p)
{
  char i = 0;
  char cnt = 0;
  while(*p)
  {
    uart_putc(*p);    // debug          
    if(*p == ' ' )
    {
      payload[cnt++][i] = '\0';
      i = 0;
    }
    else if(*p == '\r')
    {
      payload[cnt++][i++] = '\0';
      while(cnt < MAX_PARAMS)
        payload[cnt++][0] = '\0';
    }
    else
      payload[cnt][i++] = *p;   
    p++;
  }
        
  uart_puts((char *)"\n\rcommand payload[0]: ");  // debug
  uart_puts(payload[0]);                          // debug
  uart_puts((char *)"\n\rcommand payload[1]: ");  // debug
  uart_puts(payload[1]);                          // debug
  uart_puts((char *)"\n\r");                      // debug  
}

char command_check(char command_[])
{
  char i;
  for (i = 0; i < NO_OF_COMMANDS; i++)
  {
    char* p = command_;
    char* r = command_list[i];
    while(*r)
    {
      // debug info
      /* 
      uart_putc('\n');  // debug
      uart_putc(*r);    // debug
      uart_putc(' ');   // debug
      uart_putc(*p);    // debug
      uart_putc('\n');  // debug
      uart_putc('\r');  // debug
      */
      if(*p++ != *r++)
        break;
      else if (*r == '\0')
      {
        //uart_puts((char *)"valid command entered \n\r");  // debug
        pl = p;
        payload_split(++p);
        command_type_check(i, p);
        return i;
      }
    }
  }
  return -1;
}

void command_execution (char command_)
{
  switch(command_)
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
  int i;
  int ii;
  for(i=0; i<MAX_PARAMS; i++)
    for(ii=0; ii<MAX_PARAM_LEN; ii++)
      payload[i][ii] = '\0';
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
      if(i > 1)
        i-=2;
        break; 
    }
    default:
    {
      //data buffer overflow prevention
      if (i-1 == MAX_COMMAND_LENGTH-1)
      {
        uart_putc(0x7F);
        i--;
      }
    }
  }
}

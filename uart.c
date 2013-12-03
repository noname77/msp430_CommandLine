/*
 * Copyright (C) 2013 Wiktor Grajkowski <wiktor.grajkowski AT gmail.com>
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
 * This program lets you control your MSP430 in an easy way through terminal
 * like commands. Syntax: command param1 param2 ... You can specify the
 * commands, number and type of parameters, display simple help with commands'
 * descriptions. It can handle string, decimal (signed long), 
 * hex(only positive signed long) or binary (8 bit) parameters. 
 *
 * For a tutorial and demo visit http://flemingsociety.wordpress.com
 *
 *
 * Wiktor Grajkowski
 * wiktor.grajkowski AT gmail.com
 * http://flemingsociety.wordpress.com
 ******************************************************************************/

#include <msp430.h>
#include <legacymsp430.h>

#include "uart.h"
#include "spi.h"


// Receive Data (RXD) at P1.1
#define RXD  BIT1

// Transmit Data (TXD) at P1.2
#define TXD  BIT2

// commands
char* command_list[NO_OF_COMMANDS] = {"help", "set", "read", "restart"};

// types: n = no param, s - string parameter, d - digits
char* command_list_type[NO_OF_COMMANDS] = {"n", "sd", "s", "n"};

// descriptions of comands for help command
char* command_list_desc[NO_OF_COMMANDS] = {"______list available commannds", "______set [register] [value] via spi", "_____read [register]", "__restart the program"};

// strings to be replaced
char* param_strings[NO_OF_PARAM_STRINGS] = {"val1", "val2", "val3", "register"};

// values to replace strings (can't be '-1' as its reserved for invalid value detection) 
char param_strings_replace_vals[NO_OF_PARAM_STRINGS] = {1, 2, 3, 100};     


char data[MAX_COMMAND_LENGTH];            // receive buffer
char command[MAX_COMMAND_LENGTH];         // command entered
char payload[MAX_PARAMS][MAX_PARAM_LEN];  // payload parameters
char* pls[MAX_PARAMS];                    // pointer to payload parameters (maybe not needed?)

long vals[MAX_PARAMS];    // array for storing processed payload values

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

// print integer
void uart_printi(long n)
{
  int divided;                 // digit to be printed
  long division = 1000000000;  // 1000000000 as highest integer can be 2^(16) = 65536
  char reached = 0;            // used as a flag not to print leading zeros
  
  // if n = 0 don't bother with calculating stuff
  if(n == 0)
  { 
    uart_putc('0');
    return;
  }
  // if negative put minus sign and make positive
  else if(n<0)
  {
    uart_putc('-');
    n = -n;
  }  

  while(division != 0)
  {
    divided = n/division;      // digit to be printed is the integer part left after division
    if(divided != 0)           // detect first non zero digit
      reached = 1;             // set flag to print digits from now on
    if(reached)
      uart_putc(divided+'0');  // print the digit (see ascii table to know why +'0')
      
    n %= division;             // set new value for n (reminder from division)
    division /= 10;            // set new division value
  }
}

// process payload depending on the type of params and store processed values in vals[] array
void command_type_check (char command)
{
  char* r = command_list_type[command];  
  int cnt=0;              // counter for parameters, '0' being the first parameter
  int i;
  for (i=0; i<MAX_PARAMS; i++)
    vals[i] = -1;      // set all values to invalid (-1)
    
  while (*r)           // for all the parameters
  {
    char* p = pls[cnt];  // temporary pointer to payload parameter string
    switch(*r++)         // check parameter type
    {
      case 'n':          // just execute if no parameters
      {
        break;
      }
      case 's':
      {
        //uart_puts((char*)"case s: \n\r");    // debug
        int i;
        for(i=0; i<NO_OF_PARAM_STRINGS; i++)
        {
          char* s = param_strings[i];
          char* p_temp = p;
          while(*s)
          {
            if(*p_temp++ != *s++)
              break;
            else if (*s == '\0')
            {
              vals[cnt] = param_strings_replace_vals[i];    // 
              i = NO_OF_PARAM_STRINGS;                      // end searching
            }
          }
        }
        break;
      }
      //TODO add enetering too large/small number prevention
      case 'd':
      {
        static signed char hextable[] = {
                                         [0 ... 255] = -1, // bit aligned access into this table is considerably
                                         ['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, // faster for most modern processors,
                                         ['A'] = 10, 11, 12, 13, 14, 15,       // for the space conscious, reduce to
                                         ['a'] = 10, 11, 12, 13, 14, 15        // signed char.
                                        };

        //uart_puts((char*)"case d: \n\r");  // debug

        long number = 0;
        char valid = 1;

        // hex
        if(*p++ == '0' && *p-- == 'x')
        {
          //uart_puts((char*)"hex: \n\r");  // debug
          p+=2;
          unsigned long number_ = 0;
          char temp_hex[8];
          char* hex = temp_hex;
          char i=0;
          while(*p)
            temp_hex[i++] = *p++;
            
          while (i--)
          {
            number_ = (number_ << 4) | hextable[*hex++];          
            //uart_printi(number_);      // debug
            //uart_puts((char*)"\n\r");  // debug
          }
          if(number_ > ((MAX_NUM_AS_PARAM << 1) - 1))  // this is max signed int value
            valid = 0;
          else
            number = number_;
        }
        // bin
        else if(*p++ == '0' && *p-- == 'b')
        {
          //uart_puts((char*)"bin: \n\r");  // debug
          char temp_bin[8];
          p+=2;
          char i=0;
          while(*p)
          {
            uart_putc(*p);    // debug
            if(*p != '0' && *p!= '1')
            {
              //uart_putc('z');  // debug
              valid = 0;
              break;
            }
            else
              temp_bin[i++] = *p++ - '0';  
          }
          while (i--)
          {
            number = (number << 1) | temp_bin[7-i];
            uart_printi(number);         // debug
            uart_puts((char*)"\n\r");    // debug
          }
        }
        // dec
        else
        {
          //uart_puts((char*)"dec: \n\r");  // debug
          p-=2;
          char positive = 1;
          char temp_digits[10];
          char i=0;
          long scaler = 1;
          if(*p == '-')
          {
            positive = 0;
            p++;
          }
          while(*p)
          {
            temp_digits[i] = *p++ - '0';            
            if(temp_digits[i] < 0 || temp_digits[i++] > 9)
              valid = 0;
          }
          while(i>0)
          {
            number += temp_digits[i-1]*scaler;
            scaler *= 10;
            i--;
          }
          if(!positive)
            number = -number;
        }      
        if(valid)
          vals[cnt] = number;
        else
        {
          uart_puts((char*)"\n\rinvalid number in parameter: ");
          uart_printi(cnt);
        }  
        break;
      }
    }
    cnt++;
  }
  /*
  uart_puts((char*)"\n\rvals[0] = ");  // debug
  uart_printi(vals[0]);                // debug
  uart_puts((char*)"\n\rvals[1] = ");  // debug
  uart_printi(vals[1]);                // debug
  uart_puts((char*)"\n\r");            // debug
  */
  command_execution(command);
}

// TODO add payload overflow prevention
void payload_split(char* p)
{
  char i = 0;
  char cnt = 0;
  while(*p)
  {
    //uart_putc(*p);    // debug          
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
  for(i=0; i<MAX_PARAMS; i++)
    pls[i] = payload[i];
  /*  
  uart_puts((char *)"\n\rcommand payload[0]: ");  // debug
  uart_puts(payload[0]);                          // debug
  uart_puts((char *)"\n\rcommand payload[1]: ");  // debug
  uart_puts(payload[1]);                          // debug
  uart_puts((char *)"\n\r");                      // debug  
  */
}

void command_check(char command_[])
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
        payload_split(++p);
        command_type_check(i);
        return;
      }
    }
  }
  command_execution(-1);
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
      WDTCTL = 0xffff; // Set Watchdog Timer interval to ~32ms
      break;
    }
    default :
    {
        uart_puts((char *)"invalid command. type 'help' for the list of available commands. \n\r");
        break;
    }
  }
  int i;
  int ii;
  for(i=0; i<MAX_PARAMS; i++)
    for(ii=0; ii<MAX_PARAM_LEN; ii++)
      payload[i][ii] = '\0';
  uart_puts((char *)"payloads cleared. \n\r");
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
      command_check(command);
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

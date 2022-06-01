#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/dispatch/nvic.h>
#include <stdio.h>

#include "clock.h"
#include "usb.h"
#include "elog.h"
#include "keyboard.h"
#include "keymap.h"
#include "usb_keycode.h"


#define DEBOUNCE 10000

// Khai bao cac ham
void UART1_Init_A9A10(void);
int _write(int file, char *ptr, int len);
void RCC_Init(void);
void GPIO_Init(void);
void key_event(bool pressed);
void delay_us(uint32_t micros);
void usb_wakeup_isr(void);
void usb_lp_can_rx0_isr(void);

volatile uint32_t flag = 0;


//==========================================================
void delay_us(uint32_t micros)
{
  uint32_t t0 = dwt_read_cycle_counter();
  uint32_t delta = 72 * micros;
  while ((dwt_read_cycle_counter() - t0) < delta);
}

event_t key[1] =
{
  _K(A)
};

void key_event(bool pressed)
{
  event_t *event = &key[0];
  keyboard_event(event, pressed);
}

//==========================================================
static void mcu_init(void)
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

void RCC_Init(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
}
void GPIO_Init(void)
{
	// A8 OutPP High
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
    gpio_set(GPIOA, GPIO8);

	// A0 Input Float
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO0);

	// B2 OutPP_2Mhz
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
	gpio_set(GPIOB, GPIO2);

}

int main(void)
{
	uint16_t count;
	mcu_init();
	clock_init();
	serial_active = false;
	dwt_enable_cycle_counter();
	UART1_Init_A9A10();
	usb_init();

	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
	nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);

	RCC_Init();
	GPIO_Init();
	//printf("Hello World!\n");
  while (1)
  {
    //usb_poll();
    uint16_t btnState = (GPIOA_IDR & (1 << 0));
    if (btnState != 0)
    {
      count ++;
      if(count == DEBOUNCE)
      {
        GPIOB_ODR ^= 0x04;
        key_event(1);

		//delay_us(10000);
		for(int i = 0; i < 8; i++)
		{
			flag = 0;
			while(flag == 0);
		}



        key_event(0);
		printf("Hello World!\n");
      }
      if(count >= DEBOUNCE) count = DEBOUNCE + 1;
    }
    else count = 0;
	//key_event(0);
  }
}

//==========================================================
void usb_wakeup_isr(void) {

  usb_poll();
  flag = 1;
}

void usb_lp_can_rx0_isr(void) {

  usb_poll();
  flag = 1;
}
//==========================================================
void UART1_Init_A9A10()
{
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_GPIOA);

	/* Setup GPIO pin GPIO_USART1_TX and GPIO_USART1_RX. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
                       GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                       GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 250000);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);

	/* Finally enable the USART. */
	usart_enable(USART1);

}

//==========================================================

//==========================================================
int _write(int file, char *ptr, int len)
{
  for (int i = len; i != 0; i--)
  {
    while ((USART1_SR & USART_CR1_TCIE) == 0)
    {
      //usb_poll();
    }
    USART1_DR = *ptr++;
  }
  return len;
}

//==========================================================

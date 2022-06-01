/*******************************************************************************************************
  Step    TIM2 event    CCR_VALUE       DMA1    DMA transfer

  1       _UP	          0	              _CH2	  ADC1_JSQR_mem         []  -->  ADC1_JSQR
  2       _CH2	        10	            _CH7	  TIM2_SR_mem           []  -->  TIM2_SR 	        tim2_isr()
  3       _CH1	        200	            _CH5	  GPIOA_CRL_mem         []  -->  GPIOA_CRL
  4       _CH3	        201	            _CH1	  tim2_isr_channel_x_mem[]  -->  tim2_isr_func

                        ARR = 180*2 - 1

                                        _CH4	  channelAccSum         []  -->   channelValue[]
*********************************************************************************************************/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>

#include <stdio.h>
#include <math.h>

#include "clock.h"
#include "usb.h"
#include "elog.h"
#include "keyboard.h"
#include "keymap.h"
#include "usb_keycode.h"


#define         NOP()                       __asm("nop")
#define         NOP_A()  { /*NOP(); /*NOP(); NOP(); NOP();*/}
#define         NOP_B()  { /*NOP(); NOP(); NOP(); NOP();*/ }
#define         NOP_C()  { /*NOP(); NOP(); NOP(); NOP();*/ }

#define         _FOR_CHANNEL_(inst)         for (int channel = 0; channel < CHANNEL_N; channel++) { inst }

// GPIO Mode define
#define         GPIO_PINMODE(pin, mode)     ((mode) << (((pin) & (8-1))*4))
#define         GPIO_PINMODE_AIN(pin)       GPIO_PINMODE(pin, 0b0000)       // Analog Input
#define         GPIO_PINMODE_IPUD(pin)      GPIO_PINMODE(pin, 0b1000)       // Input Pull-Up or -Down

#define         GPIO_PINMODE_OPP2M(pin)     GPIO_PINMODE(pin, 0b0010)       // Output Push-Pull  2 MHz
#define         GPIO_PINMODE_OPP10M(pin)    GPIO_PINMODE(pin, 0b0001)       // Output Push-Pull 10 MHz
#define         GPIO_PINMODE_OPP50M(pin)    GPIO_PINMODE(pin, 0b0011)       // Output Push-Pull 50 MHz

#define         GPIO_PINMODE_OPP2M_ALL  \
(               GPIO_PINMODE_OPP2M(0)   \
              | GPIO_PINMODE_OPP2M(1)   \
              | GPIO_PINMODE_OPP2M(2)   \
              | GPIO_PINMODE_OPP2M(3)   \
              | GPIO_PINMODE_OPP2M(4)   \
              | GPIO_PINMODE_OPP2M(5)   \
              | GPIO_PINMODE_OPP2M(6)   \
              | GPIO_PINMODE_OPP2M(7)   \
)

#define  GPIO_PINMODE_MASK(pin)      (0b1111 << (((pin) & (8-1))*4))
#define  GPIO_MEASURE_PIN(pin)       (GPIO_PINMODE_AIN(pin) | (GPIO_PINMODE_OPP2M_ALL & ~GPIO_PINMODE_MASK(pin)))

// ADC1 Register define
#define         ADC1_CR2_BASE                  \
(                                              \
                ADC_CR2_ADON                   \
              | ADC_CR2_CONT                   \
              | ADC_CR2_ALIGN_LEFT             \
              | ADC_CR2_EXTSEL_SWSTART         \
              | ADC_CR2_JEXTSEL_JSWSTART       \
              | ADC_CR2_JEXTTRIG               \
)

#define         STAT_N                           (1024L/2)
#define         CALIBRATE_N                      (1024L)
#define         AVERAGE                          (498) // 498
#define         SCALE                            (AVERAGE*8)
#define         CHANNEL_N                        8

#define         THRESHOLD_HI                         10*SCALE       // stm32 pin -> wire -> res 10k -> cap 104 -> thLED
#define         THRESHOLD_LOW                        9*SCALE       // stm32 pin -> wire -> res 10k -> cap 104 -> thLED

// For Timer2
#define         ARR_VALUE                        (180*2-1)        // Thoi gian giua 2 lan nhay vao ngat
#define         PSC_VALUE                        (1*1-1)          // CK_CNT = PCLK / (PSC[15:0] + 1)
// TIM2_CH1
#define         CCR1_VALUE                       200


// TIM2_CH2
#define         CCR2_VALUE                       10
// TIM2_CH3
#define         CCR3_VALUE                       201


//----------------------------------For DMA-------------------------------------
#define         DMA_CCR_SRC_PERIPHERAL            ((uint32_t)(0x00000000))
#define         DMA1_CH1                          DMA1, DMA_CHANNEL1
#define         DMA1_CH2                          DMA1, DMA_CHANNEL2
#define         DMA1_CH4                          DMA1, DMA_CHANNEL4
#define         DMA1_CH5                          DMA1, DMA_CHANNEL5
#define         DMA1_CH7                          DMA1, DMA_CHANNEL7

// DMAy Channelx CCR - Chon cac cau hinh
#define         DMA1_CHANNEL4_CCR_BASE  \
(                                       \
                DMA_CCR_SRC_PERIPHERAL  \
              | DMA_CCR_PINC            \
              | DMA_CCR_MINC            \
              | DMA_CCR_PSIZE_32BIT     \
              | DMA_CCR_MSIZE_32BIT     \
              | DMA_CCR_PL_MEDIUM       \
              | DMA_CCR_MEM2MEM         \
)

//==========================================================
// Khai bao cac ham
void UART1_Init_A9A10();
void PrintADC1Info();
void PrintSystemInfo();

volatile uint32_t flag = 0;

// Global variable
int32_t channelData[CHANNEL_N*3];
#define  channelValue       (channelData + 0*CHANNEL_N)
#define  channelAccSum      (channelData + 1*CHANNEL_N)
#define  channelBaseline    (channelData + 2*CHANNEL_N)

uint32_t GPIOA_CRL_mem[1] = { GPIO_PINMODE_OPP2M_ALL };
uint32_t TIM2_SR_mem[1] = { 0 };
uint32_t ADC1_JSQR_mem[CHANNEL_N] =
{
  (ADC_CHANNEL0 << (5*3)),
  (ADC_CHANNEL1 << (5*3)),
  (ADC_CHANNEL2 << (5*3)),
  (ADC_CHANNEL3 << (5*3)),
  (ADC_CHANNEL4 << (5*3)),
  (ADC_CHANNEL5 << (5*3)),
  (ADC_CHANNEL6 << (5*3)),
  (ADC_CHANNEL7 << (5*3)),
};

vector_table_t __attribute__((aligned(512))) VectorTableRAM;

//==========================================================
event_t key[8] =
{
  //_K(A), _K(B), _K(C), _K(D), _K(E), _K(F), _K(G), _K(H)
  //_K(L), _K(H), _K(E), _K(O), _K(I), _K(T), _K(O), _K(E)

  //_K(SPACE), _K(BACKSPACE), _K(SPACE), _K(BACKSPACE), _K(E), _K(BACKSPACE), _K(SPACE), _K(LEFT)
  _K(W), _K(A), _K(DOWN), _K(LEFT), _K(RIGHT), _K(S), _K(UP), _K(SPACE)
};

void keyboard_event_touch(event_t *event, bool pressed)
{
  uint8_t key = event->key.code;

  elog("key %02x %d\n", key, pressed);

  switch (key) {
  case KEY_LCTRL ... KEY_RGUI:
      if (pressed) {
          keyboard_add_modifier(key);
      } else {
          keyboard_del_modifier(key);
      }
      break;

  default:
      if (pressed) {
          keyboard_add_key(key);
      } else {
          keyboard_del_key(key);
      }
      break;
  }

//  if (keyboard_dirty) {
//      usb_update_keyboard(&keyboard_state);
//      keyboard_dirty = false;
//  }

  if (nkro_dirty) {
      usb_update_nkro(&nkro_state);
      nkro_dirty = false;
  }
}


//==========================================================
void delay_us(uint32_t micros)
{
  uint32_t t0 = dwt_read_cycle_counter();
  uint32_t delta = 72 * micros;
  while ((dwt_read_cycle_counter() - t0) < delta);
}

void delay_ms(unsigned long ms)
{
  for (; ms; ms--)
  {
    delay_us(1000);
  }
}


uint64_t dwt_CycleCount64()
{
  static uint64_t last_cycle_count_64 = 0;
  last_cycle_count_64 += DWT_CYCCNT - (uint32_t)(last_cycle_count_64);
  return last_cycle_count_64;
}

uint64_t dwt_Millis()
{
  return dwt_CycleCount64() / 72000;
}

uint64_t dwt_Micros()
{
  return dwt_CycleCount64() / 72;
}

//==========================================================

void RCC_Config(void)
{
  // System clock
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
  flash_set_ws(FLASH_ACR_LATENCY_1WS);

  // Enable clock GPIO
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_AFIO);

   // Enable clock for Timer 2
  rcc_periph_clock_enable(RCC_TIM2);

  // Enable clock for ADC
  rcc_periph_clock_enable(RCC_ADC1);
  adc_power_off(ADC1);
  rcc_periph_reset_pulse(RST_ADC1);
  rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV2);

  // Enable clock DMA
  rcc_periph_clock_enable(RCC_DMA1);
}

//==========================================================

void GPIO_Config()
{
  //*************** Used pins *****************
  // GPIOA
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG,
                GPIO0 | GPIO1 | GPIO2 | GPIO3 |
                GPIO4 | GPIO5 | GPIO6 | GPIO7);
  GPIOA_ODR |= GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7;
  // A8 OutPP High
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
  gpio_set(GPIOA, GPIO8);
  // GPIOB
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);
  GPIOB_ODR |= 0x04;    // Bat Led B2
  GPIOB_ODR &= ~0x01;   // Xuat muc 0 o chan B0
  // GPIOC
  //gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
  //              GPIO14);
  //*************** Unused pins ****************
  // GPIOA - Input Pull Down Pins
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
  // GPIOB - Input Pull Down Pins
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                GPIO3  | GPIO4  | GPIO5  |
                GPIO6  | GPIO7  | GPIO8  | GPIO9  |
                GPIO10 | GPIO11 | GPIO12 | GPIO13 |
                GPIO14 | GPIO15);
  // GPIOC - Input Pull Down Pins
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                GPIO13 | GPIO14|  GPIO15);
}

//==========================================================

void UART1_Init_A9A10(int baudrate)
{
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_GPIOA);

	// Setup GPIO pin GPIO_USART1_TX and GPIO_USART1_RX.
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
                       GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                       GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	// Setup UART parameters.
	usart_set_baudrate(USART1, baudrate);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);

	// Finally enable the USART.
	usart_enable(USART1);

	// Disable buffering on stdout by using setbuf
	// fix printf not flush after the call unless a newline is in format tring
	setbuf(stdout, NULL);

}

//==========================================================

int _write(int file, char *ptr, int len)
{
  for (int i = len; i > 0; i--)
  {
    while ((USART1_SR & USART_CR1_TXEIE) == 0);
    USART1_DR = *ptr++;
  }
  return len;
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

//====================================================================

void DMAchannel4_config()
{
  dma_channel_reset(DMA1_CH4);                                  // Reset to default values

  dma_set_peripheral_address(DMA1_CH4, (int32_t)channelAccSum); // SRC: int32_t channelAccSum[++]
  dma_set_peripheral_size(DMA1_CH4, DMA_CCR_PSIZE_32BIT);       //      int32_t
  dma_enable_peripheral_increment_mode(DMA1_CH4);               //                           [++]

  dma_set_memory_address(DMA1_CH4, (int32_t)channelValue);      // DST: int32_t  channelValue[++]
  dma_set_memory_size(DMA1_CH4, DMA_CCR_MSIZE_32BIT);           //      int32_t
  dma_enable_memory_increment_mode(DMA1_CH4);                   //                           [++]

  dma_set_number_of_data(DMA1_CH4, CHANNEL_N*2);                // Transfer Count: CHANNEL_N*2
  dma_set_read_from_peripheral(DMA1_CH4);                       // Periph --> Mem
  dma_set_priority(DMA1_CH4, DMA_CCR_PL_MEDIUM);                // Medium priority
  dma_enable_mem2mem_mode(DMA1_CH4);                            // Mem to Mem copy
  //dma_enable_circular_mode(DMA1_CH4);                         // non-circular mode
}

void DMAchannel2_TIM2_UP_config()
{
  dma_channel_reset(DMA1_CH2);                                  // Reset to default values

  dma_set_memory_address(DMA1_CH2, (uint32_t)ADC1_JSQR_mem);    // SRC: uint32_t ADC1_JSQR_mem[++]
  dma_set_memory_size(DMA1_CH2, DMA_CCR_MSIZE_32BIT);           //      uint32_t
  dma_enable_memory_increment_mode(DMA1_CH2);                   //                            [++]

  dma_set_peripheral_address(DMA1_CH2, (uint32_t)&ADC1_JSQR);   // DST: uint32_t &ADC1_JSQR
  dma_set_peripheral_size(DMA1_CH2, DMA_CCR_PSIZE_32BIT);       //      uint32_t
  dma_disable_peripheral_increment_mode(DMA1_CH2);              //

  dma_set_number_of_data(DMA1_CH2, CHANNEL_N);                  // Transfer Count: CHANNEL_N
  dma_set_read_from_memory(DMA1_CH2);                           // Mem --> Peripheral
  dma_set_priority(DMA1_CH2, DMA_CCR_PL_MEDIUM);                // Medium priority

  //dma_enable_mem2mem_mode(DMA1_CH2);                          // Mem to Mem copy disable
  dma_enable_circular_mode(DMA1_CH2);                           // circular mode

  dma_enable_channel(DMA1_CH2);                                 // Enable DMA1 Channel 2 transfer

}

void DMAchannel7_TIM2_CH2_config()
{
  dma_channel_reset(DMA1_CH7);                                  // Reset to default values

  dma_set_memory_address(DMA1_CH7, (uint32_t)TIM2_SR_mem);      // SRC: uint32_t TIM2_SR_mem[]
  dma_set_memory_size(DMA1_CH7, DMA_CCR_MSIZE_32BIT);           //      uint32_t
  dma_disable_memory_increment_mode(DMA1_CH7);

  dma_set_peripheral_address(DMA1_CH7, (uint32_t)&TIM2_SR);     // DST: uint32_t &TIM2_SR
  dma_set_peripheral_size(DMA1_CH7, DMA_CCR_PSIZE_32BIT);       //      uint32_t
  dma_disable_peripheral_increment_mode(DMA1_CH7);              //

  dma_set_number_of_data(DMA1_CH7, 1);                          // Transfer Count: 1
  dma_set_read_from_memory(DMA1_CH7);                           // Mem --> Peripheral
  dma_set_priority(DMA1_CH7, DMA_CCR_PL_MEDIUM);                // Medium priority

  //dma_enable_mem2mem_mode(DMA1_CH7);                          // Mem to Mem copy disable
  dma_enable_circular_mode(DMA1_CH7);                           // circular mode

  dma_enable_channel(DMA1_CH7);                                 // Enable DMA1 Channel 7 transfer

}

void DMAchannel5_TIM2_CH1_config()
{
  dma_channel_reset(DMA1_CH5);                                  // Reset to default values

  dma_set_memory_address(DMA1_CH5, (uint32_t)GPIOA_CRL_mem);    // SRC: uint32_t GPIOA_CRL_mem[]
  dma_set_memory_size(DMA1_CH5, DMA_CCR_MSIZE_32BIT);           //      uint32_t
  dma_disable_memory_increment_mode(DMA1_CH5);

  dma_set_peripheral_address(DMA1_CH5, (uint32_t)&GPIOA_CRL);   // DST: uint32_t &GPIOA_CRL
  dma_set_peripheral_size(DMA1_CH5, DMA_CCR_PSIZE_32BIT);       //      uint32_t
  dma_disable_peripheral_increment_mode(DMA1_CH5);

  dma_set_number_of_data(DMA1_CH5, 1);                          // Transfer Count: 1
  dma_set_read_from_memory(DMA1_CH5);                           // Mem --> Peripheral
  dma_set_priority(DMA1_CH5, DMA_CCR_PL_MEDIUM);                // Medium priority

  //dma_enable_mem2mem_mode(DMA1_CH5);                          // Mem to Mem copy disable
  dma_enable_circular_mode(DMA1_CH5);                           // circular mode

  dma_enable_channel(DMA1_CH5);                                 // Enable DMA1 Channel 5 transfer
}

void (*tim2_isr_func)();
uint32_t tim2_isr_channel_x_mem[CHANNEL_N];
void DMAchannel1_TIM2_CH3_config()
{
  dma_channel_reset(DMA1_CH1);                                          // Reset to default values

  dma_set_memory_address(DMA1_CH1, (uint32_t)tim2_isr_channel_x_mem);   // SRC: uint32_t tim2_isr_channel_x_mem[++]
  dma_set_memory_size(DMA1_CH1, DMA_CCR_MSIZE_32BIT);                   //      uint32_t
  dma_enable_memory_increment_mode(DMA1_CH1);                           //                                     [++]

  dma_set_peripheral_address(DMA1_CH1, (uint32_t)&tim2_isr_func);       // DST: uint32_t &tim2_isr_func
  dma_set_peripheral_size(DMA1_CH1, DMA_CCR_PSIZE_32BIT);               //      uint32_t
  dma_disable_peripheral_increment_mode(DMA1_CH1);                      //

  dma_set_number_of_data(DMA1_CH1, CHANNEL_N);                          // Transfer Count: CHANNEL_N
  dma_set_read_from_memory(DMA1_CH1);                                   // Mem --> Peripheral
  dma_set_priority(DMA1_CH1, DMA_CCR_PL_MEDIUM);                        // Medium priority

  //dma_enable_mem2mem_mode(DMA1_CH1);                                  // Mem to Mem copy disable
  dma_enable_circular_mode(DMA1_CH1);                                   // circular mode

  dma_enable_channel(DMA1_CH1);                                         // Enable DMA1 Channel 1 transfer

}

void ADC_Config()
{
  adc_set_dual_mode(ADC_CR1_DUALMOD_IND);                           // ADC1 and ADC2 operate independently
  adc_set_left_aligned(ADC1);                                       // Data Align Left

  // Regular channel
  uint8_t regularChannels[] = {ADC_CHANNEL9};
  adc_set_regular_sequence(ADC1, 1, regularChannels);
  adc_set_sample_time(ADC1, regularChannels[0], ADC_SMPR_SMP_239DOT5CYC); // Set Sample Time Regular Channel 9
  adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_SWSTART);
  adc_set_continuous_conversion_mode(ADC1);                         // Continuous Conversion mode

  // Injected channel
  uint8_t injectedChannels[] = {ADC_CHANNEL0};
  adc_set_injected_sequence(ADC1, 1, injectedChannels);             // Set sequence length = 1 injected Channel

  _FOR_CHANNEL_( adc_set_sample_time(ADC1, channel, ADC_SMPR_SMP_1DOT5CYC); ) // Set Sample Time Injected Channel 0 - 7

  adc_enable_external_trigger_injected(ADC1, ADC_CR2_JEXTSEL_JSWSTART);

  adc_power_on(ADC1);                                               // Enable ADC1
  adc_reset_calibration(ADC1);                                      // Reset Calibration & wait reset finish
  adc_calibration(ADC1);                                            // Calibration & wait Calibration finish

  adc_start_conversion_regular(ADC1);
}

void NVIC_Congfig()
{
  scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);

  nvic_enable_irq(NVIC_TIM2_IRQ);
  nvic_set_priority(NVIC_TIM2_IRQ, 0 << 4);

 	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
  nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 4 << 4);

	nvic_enable_irq(NVIC_USB_WAKEUP_IRQ);
	nvic_set_priority(NVIC_USB_WAKEUP_IRQ, 5 << 4);

}
//==========================================================

void TIM2_Config()
{
  TIM2_PSC    = PSC_VALUE;                             // Prescaler value
  TIM2_ARR    = ARR_VALUE;                             // Auto - reload value

  //------------- TIM2_CH1------------
  TIM2_CCR1   = CCR1_VALUE;                            // gia tri so sanh Capture/cpmpare 1
  TIM2_DIER  |= TIM_DIER_CC1DE;

  //-------------TIM2_CH2-------------
  TIM2_CCR2   = CCR2_VALUE;                            // gia tri so sanh Capture/cpmpare 2
  TIM2_DIER  |= TIM_DIER_CC2DE;
  timer_enable_irq(TIM2, TIM_SR_CC2IF);                // Cho phep ngat compare CH2

  //------------- TIM2_CH3------------
  TIM2_CCR3   = CCR3_VALUE;                            // gia tri so sanh Capture/cpmpare 3
  TIM2_DIER  |= TIM_DIER_CC3DE;

  //------------- TIM2_UP------------
  TIM2_DIER  |= TIM_DIER_UDE;

  TIM2_CR1    = TIM_CR1_DIR_UP | TIM_CR1_CEN;          // Upcounting mode, Counter enable
}

void __attribute__((aligned(16))) tim2_isr_channel_0();
void (*tim2_isr_func)() = tim2_isr_channel_0;


uint32_t TCNT_value_min = 1000000;
uint32_t TCNT_value_max = 0;
uint32_t sumTCNT_value;
uint32_t count_TCNT = 0;

void __attribute__((aligned(16))) tim2_isr_channel_7()
{
  // Da thay bang DMA_TIM2_CH2 copy
  //ADC1_JSQR = ADC_CHANNEL7 << (5*3);          // Injected channel (7) = A7

  channelAccSum[6] += ADC1_JDR1;                // Read channel 6 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A7
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(7);              // A7 = AnalogInput, {A0 - A7} \ {A7} = OutPP HIGH
  NOP_C();

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;

  // Da thay bang DMA_TIM2_UP copy
  //TIM2_SR = 0;                                // Xoa tat ca cac co TIM2 (bao gom ca co TIM_SR_CC2IF)

  // Da thay bang DMA_TIM2_CH1 copy
  //GPIOA_CRL = GPIO_PINMODE_OPP2M_ALL;         // {A0 - A7} = OutPP HIGH - DMA

  static int step = 0;
  if ((--step) < 0)
  {
    step = AVERAGE - 1;
    DMA1_CCR4 = DMA1_CHANNEL4_CCR_BASE | DMA_CCR_EN;  // Enable DMA1 Channel 1 transfer
  }

  // Da thay bang DMA_TIM2_CH3 copy
  //tim2_isr_func = &tim2_isr_channel_0;
}

void __attribute__((aligned(16))) tim2_isr_channel_6()
{
  channelAccSum[5] += ADC1_JDR1;                // Read channel 5 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A6
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(6);              // A6 = AnalogInput, {A0 - A7} \ {A6} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_5()
{
  channelAccSum[4] += ADC1_JDR1;                // Read channel 4 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A5
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(5);              // A5 = AnalogInput, {A0 - A7} \ {A5} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_4()
{
  channelAccSum[3] += ADC1_JDR1;                // Read channel 3 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A4
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(4);              // A4 = AnalogInput, {A0 - A7} \ {A4} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_3()
{
  channelAccSum[2] += ADC1_JDR1;                // Read channel 2 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A3
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(3);              // A3 = AnalogInput, {A0 - A7} \ {A3} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_2()
{
  channelAccSum[1] += ADC1_JDR1;                // Read channel 1 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A2
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(2);              // A2 = AnalogInput, {A0 - A7} \ {A2} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_1()
{
  channelAccSum[0] += ADC1_JDR1;                // Read channel 0 Value
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A1
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(1);              // A1 = AnalogInput, {A0 - A7} \ {A1} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr_channel_0()
{
  channelAccSum[7] += ADC1_JDR1;
  NOP_A();
  ADC1_CR2 = ADC_CR2_JSWSTART | ADC1_CR2_BASE;  // Start Injected conversion on A0
  NOP_B();
  GPIOA_CRL = GPIO_MEASURE_PIN(0);              // A0 = AnalogInput, {A0 - A7} \ {A0} = OutPP HIGH

  uint32_t TCNT_value = TIM2_CNT;
  if(TCNT_value < TCNT_value_min) TCNT_value_min = TCNT_value;
  if(TCNT_value > TCNT_value_max) TCNT_value_max = TCNT_value;
  sumTCNT_value += TCNT_value;
  count_TCNT += 1;
}

void __attribute__((aligned(16))) tim2_isr()
{
  (*tim2_isr_func)();
}

uint32_t DMA_Wait()
{
  uint32_t IdleCount = 0;
  while ((DMA1_ISR & DMA_ISR_TCIF4) == 0)  // Wait transfer complete
  {
    IdleCount++;
  }
  DMA1_IFCR = DMA_IFCR_CTCIF4;             // Xoa co transfer complete
  DMA1_CCR4 = DMA1_CHANNEL4_CCR_BASE;      // Disable DMA1 Channel 1 transfer
  DMA1_CNDTR4 = CHANNEL_N*2;
  return IdleCount;
}

//==========================================================
void Do_TB_PS_delta()
{
  static uint32_t lastMillis = 0;
  DMA_Wait();
  DMA_Wait();

  int32_t delta_sum = 0;
  int64_t   sum[CHANNEL_N] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t sum2[CHANNEL_N] = {0, 0, 0, 0, 0, 0, 0, 0};
  float SNR[CHANNEL_N];

  uint32_t IdleCount_sum = 0;
  uint32_t IdleCount_min, IdleCount_max;
  IdleCount_min = 2000000;
  IdleCount_max = 0;
  for (int i = 0; i < STAT_N; i++)
  {
    uint32_t temp = DMA_Wait();
    IdleCount_sum += temp;
    if(IdleCount_min > temp) IdleCount_min = temp;
    if(IdleCount_max < temp) IdleCount_max = temp;

    delta_sum = 0;
    _FOR_CHANNEL_( delta_sum += channelValue[channel]; )

//    if (sum_delta < (1.0*SCALE*CHANNEL_N))
//    {
//      for(int channel = 0; channel < CHANNEL_N; channel++)
//      {
//        baseline[channel] += channelValue[channel] / 512;
//      }
//    }

    for(int channel = 0; channel < CHANNEL_N; channel++)
    {
      int64_t d =  channelValue[channel];
      sum[channel] += d;
      sum2[channel] += d*d;
    }

//    for(int i = 0; i < 8; i++)
//    {
//      printf("%d\t", channelValue[i]);
//    }
//    printf("\n");

    uint8_t countOFF = 0;

    // So sanh nguong & cap nhat keyboard_event()
    for(int channel = 0; channel < 8; channel++)
    {
      if (channelValue[channel] >= THRESHOLD_HI)
      {
        keyboard_event_touch(&key[channel], true);
        GPIOB_ODR |= 0x04;
        delay_ms(1);
      }

      else if (channelValue[channel] <= THRESHOLD_LOW)
      {
        keyboard_event_touch(&key[channel], false);
        countOFF += 1;
      }
      else countOFF += 1;

    }
    if (countOFF == 8) // Khong co phim nao duoc nhan ?
    {
      GPIOB_ODR &= ~0x04;
      countOFF = 0;
    }

//    if ((usb_now() - lastMillis) >= 1)
//    {
      usb_update_keyboard(&keyboard_state);
      printf("usb_now = %d\n", usb_now());
//      lastMillis = usb_now();
//    }

  }
  printf("IdleCount = %6ld\t", IdleCount_sum / (STAT_N * AVERAGE));
  printf("min = %6ld\t", IdleCount_min / AVERAGE);
  printf("max = %6ld\n", IdleCount_max / AVERAGE);

  static uint32_t lastMicros = 0;
  uint32_t t = dwt_Micros();
  uint32_t ReportPeriod = t - lastMicros;
  float ReportRate = (1000000.0*STAT_N) / ReportPeriod;
  lastMicros = t;
  printf("%11.3f\t", 1.0*ReportPeriod/1e6);                    // Thoi gian đo STAT_N lan.
  printf("%10.1f\t", ReportRate);                              // measure rate
  printf("%10ld\t", (int32_t)delta_sum);
  printf("%10.3f\n", (delta_sum)*1.0/(SCALE*CHANNEL_N));       // Test
  printf("------------------\n");
  for(int channel = 0; channel < CHANNEL_N; channel++)
  {
    float TB_delta = (1.0 * sum[channel]) / STAT_N;
    float PS_delta = sqrt( fabs( ((double)sum2[channel]*STAT_N) - (double) sum[channel]*sum[channel]) ) / STAT_N;
    float SNR = fabs(TB_delta) / PS_delta;
    float baseline_normalized = (float) 1.0* (-channelBaseline[channel]);

    TB_delta /= SCALE;
    PS_delta /= SCALE;
    baseline_normalized /= SCALE;

    printf("%12.1f\t", baseline_normalized);
    printf("%8.2f\t", TB_delta);                      // Trung binh Delta.
    printf("%8.2f\t", PS_delta);                      // Phuong sai Delta.
    printf("%8.2f\t\n", SNR);                // Tin hieu / Nhieu (Signal To Noise Ratio).
  }

  printf("TCNT_TB  = %d\t", sumTCNT_value / count_TCNT);
  printf("TCNT_min = %d\t", TCNT_value_min);
  printf("TCNT_max = %d\n", TCNT_value_max);

  printf("------------------------------------------------------------------\n");
//  for(int i = 0; i < 8; i++)
//  {
//    printf(" channelValue[%d] = %d\t", i, channelValue[i]);
//  }
//  printf("\n");

}

//==========================================================

void BaselineCalibrate()
{
  _FOR_CHANNEL_( channelBaseline[channel] = 0; )

  // Bo qua cac gia tri khong mong muon
  for(int i = 0; i < 10; i++)
  {
    DMA_Wait();

  }
  //DMA_Wait();

  // Tinh muc gia tri khi chua cham (baseline)
  uint64_t sum_baseline[CHANNEL_N] = {0, 0, 0, 0, 0, 0, 0, 0};
  for(int i = 0; i < CALIBRATE_N; i++)
  {
    DMA_Wait();
    _FOR_CHANNEL_( sum_baseline[channel] += channelValue[channel]; )
  }

  _FOR_CHANNEL_( channelBaseline[channel] = 0 - (sum_baseline[channel] + CALIBRATE_N/2) / CALIBRATE_N; )

  // In ra muc gia tri khi chua cham
  _FOR_CHANNEL_( printf("%ld\t", -channelBaseline[channel] / SCALE); )
  printf("\n");

}

//==========================================================
uint32_t tim2_isr_channel_x_mem[CHANNEL_N] =
{
  &tim2_isr_channel_0,
  &tim2_isr_channel_1,
  &tim2_isr_channel_2,
  &tim2_isr_channel_3,
  &tim2_isr_channel_4,
  &tim2_isr_channel_5,
  &tim2_isr_channel_6,
  &tim2_isr_channel_7,
};

int main(void)
{
  dwt_enable_cycle_counter();
  clock_init();
  RCC_Config();
  GPIO_Config();
  serial_active = false;
  UART1_Init_A9A10(250000);
  usb_init();
  DMAchannel4_config();
  DMAchannel2_TIM2_UP_config();
  DMAchannel7_TIM2_CH2_config();
  DMAchannel5_TIM2_CH1_config();
  DMAchannel1_TIM2_CH3_config();
  ADC_Config();
  TIM2_Config();
  NVIC_Congfig();

  PrintSystemInfo();

  BaselineCalibrate();

  TCNT_value_min = 1000000; TCNT_value_max = 0; sumTCNT_value = 0; count_TCNT = 0;

  for(int channel = 0; channel < CHANNEL_N; channel++)
  {
    printf("tim2_isr_channel_x_mem[%d] = %X\n", channel, tim2_isr_channel_x_mem[channel] );
  }

  while (true)
  {
    Do_TB_PS_delta();
//    for(int i = 0; i < 8; i++)
//    {
//      printf("keyPress[%d] = %d\t", i, keyPress[i]);
//
//    }
//    printf("\n");

  }
}

//==========================================================

void PrintADC1Info()
{
  printf("ADC1 Init:\n");
  printf("RCC_CFGR = %08lX\n", RCC_CFGR);
  printf("CR1      = %08lX\n", ADC1_CR1);
  printf("CR2      = %08lX\n", ADC1_CR2);
  printf("SMPR1    = %08lX\n", ADC1_SMPR1);
  printf("SMPR2    = %08lX\n", ADC1_SMPR2);
  printf("SQR1     = %08lX\n", ADC1_SQR1);
  printf("SQR2     = %08lX\n", ADC1_SQR2);
  printf("SQR3     = %08lX\n", ADC1_SQR3);
  printf("JSQR     = %08lX\n", ADC1_JSQR);
  printf("\n");
}

void PrintSystemInfo()
{
  printf("SystemInfo:\n");
  printf("SCB_VTOR = %08X\n", SCB_VTOR);
  printf("&SCB_VTOR = %08X\n", &SCB_VTOR);
  printf("FLASH_BASE = %08X\n", FLASH_BASE);
  printf("VectorTableRAM size = %d\n", sizeof(VectorTableRAM));
  printf("&VectorTableRAM = %08X\n", &VectorTableRAM);


  vector_table_t *VectorTableFlash = (vector_table_t*)(FLASH_BASE + SCB_VTOR);

  // Copy VectorTableFlash to VectorTableRAM
  volatile uint32_t *src, *dest;
  src = VectorTableFlash;
  dest = &VectorTableRAM;
	for (int i = 0; i < sizeof(VectorTableRAM)/4; i++, src++, dest++) {
		*dest = *src;
	}

  printf("NVIC_TIM2_IRQ = %08X\n", VectorTableFlash->irq[NVIC_TIM2_IRQ]);
  printf("VectorTableRAM = %08X\n", VectorTableRAM.irq[NVIC_TIM2_IRQ]);
  printf("tim2_isr = %08X\n", &tim2_isr);
  printf("\n");

}




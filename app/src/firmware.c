#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
#include "usart.h"
#include "adc.h"

#define SYSTICK_FREQ                    (1000)            // the desired systick frequency, 1000Hz means 1ms per tick  
#define ADC_PORT                        (PORT_GPIOA)      // this is an external LED connected to PA0
#define ADC_PIN                         (PIN_GPIO1)

volatile uint32_t systick_ticks = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "ADC: Potentiometer Readings Terminal\r\n";
static const uint8_t msg_prompt[]  = "Type 'HELP' for commands:\r\n";
static const uint8_t msg_prompt2[] = "\r\n>";

void systick_handler(void){
    systick_ticks++;
}

void uart_setup(){
    rcc_periph_clock_enable(RCC_GPIOA);
    // USART Setup
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO9, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO10, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    // Configure USART1
    usart_init(USART1, 115200, &USART1_TX_RX_8BIT);    // the function will automatically compute the correct BRR for 44MHz

    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
    usart_printf(USART1, "APB2: %lu Hz\r\n", rcc_get_apb2_freq());
    usart_write(USART1, msg_prompt2, sizeof(msg_prompt2) - 1);
}

void adc_setup(){
    // Configure PA0 as analog input
    volatile uint32_t apb2_before = RCC_APB2_ENR;
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(ADC_PORT, ADC_PIN, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG);

    // Initialize ADC1: Channel 0, continuous mode
    adc_scan_init(ADC1, &ADC_SCAN_TEST);

    volatile uint32_t apb2_after = RCC_APB2_ENR;
    volatile uint32_t cr1 = ADC1->CR1;
    volatile uint32_t cr2 = ADC1->CR2;
    volatile uint32_t sr  = ADC1->SR;
    volatile uint32_t sqr3 = ADC1->SQR3;
    volatile uint32_t cfgr = RCC_CFGR;
    volatile uint32_t smpr2 = ADC1->SMPR2;
    volatile uint32_t smpr1 = ADC1->SMPR1;
    usart_printf(USART1, "CR1: 0x%08lX\r\n", cr1);
    usart_printf(USART1, "CR2: 0x%08lX\r\n", cr2);
    usart_printf(USART1, "SR:  0x%08lX\r\n", sr);
    usart_printf(USART1, "SQR3: 0x%08lX\r\n", sqr3);
    usart_printf(USART1, "RCC_CFGR: 0x%08lX\r\n", cfgr);
    usart_printf(USART1, "SMPR2: 0x%08lX\r\n", smpr2);
    usart_printf(USART1, "SMPR1: 0x%08lX\r\n", smpr1);
    usart_printf(USART1, "APB2 before: 0x%08lX, after: 0x%08lX\r\n", apb2_before, apb2_after);

    volatile uint32_t crl = PORT_GPIOA->CRL;
    usart_printf(USART1, "GPIOA_CRL: 0x%08lX\r\n", crl);

}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    systick_set_frequency(SYSTICK_FREQ, rcc_get_ahb_freq()); // 1ms tick, interrupt enabled by default
    uart_setup();
    adc_setup();

    while(1){
        uint16_t results[2];
        adc_scan_read(ADC1, results, 2);

        usart_printf(USART1, "CH1: %lu mV | Temp: %ld.%02ld C\r\n",
            (results[0] * 3300) / 4096,
            convert_internal_temp(results[1]) / 100,
            convert_internal_temp(results[1]) % 100);
    }
    return 0;
}
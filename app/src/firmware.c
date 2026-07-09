#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "timers.h"
#include "usart.h"
#include "dma.h"

#define LED_PORT                        (PORT_GPIOA)       // this is an external LED connected to PA0
#define LED_PIN                         (PIN_GPIO0)
#define LED2_PORT                       (PORT_GPIOA) 
#define LED2_PIN                        (PIN_GPIO1)       // TIM2_CH2

#define RX_BUFF_SIZE                    64

volatile uint32_t systick_ticks = 0;
static volatile uint8_t rx_buffer[RX_BUFF_SIZE];
static volatile uint8_t rx_index = 0;
static volatile uint32_t dma_full_complete = 0;
static volatile uint32_t dma_half_complete = 0;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "Blue Pill Echo Terminal\r\n";
static const uint8_t msg_prompt[]  = "Type something and press Enter:\r\n";
//static const uint8_t msg_prefix[]  = "\r\nYou typed: ";
//static const uint8_t msg_suffix[]  = "\r\n> ";
static const uint8_t msg_newline[] = "\r\n";
static uint8_t msg_prompt2[] = "> ";

// static void process_line(const volatile uint8_t *line, uint8_t length) {
//     usart_write(USART1, msg_prefix, sizeof(msg_prefix)-1);
//     usart_write(USART1, (const uint8_t*)line, length);
//     usart_write(USART1, msg_suffix, sizeof(msg_suffix)-1);  // New prompt
// }

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    gpio_set_mode(LED2_PORT, LED2_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

void dma1_channel5_isr(void) {
    // Clear the interrupt flag!
    if (DMA1_Controller->ISR & DMA_ISR_HTIF(5)) {
        DMA1_Controller->IFCR = DMA_IFCR_CHTIF(5); // clear HTIF5
        dma_half_complete = 1;
    }

    if (DMA1_Controller->ISR & DMA_ISR_TCIF(5)) {
        DMA1_Controller->IFCR = DMA_IFCR_CTCIF(5); // look at reference manual rm008,  CGIF5(16) CTCIF5(17) CHTIF5(18) CTEIF5(19) for channel 5
        dma_full_complete = 1;
    }
    
}

void uart_setup(){
    // USART Setup
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO9, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO10, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);

    // Configure USART1
    usart_init(USART1, 115200, &USART1_TX_RX_8BIT);    // the function will automatically compute the correct BRR for 44MHz
    //usart_rx_interrupt_enable(USART1);
    USART1->CR1 &= ~USART_CR1_RXNEIE;  // Ensure RXNE interrupt is OFF
    USART1->CR1 &= ~USART_CR1_TXEIE;  // Ensure TXE interrupt is OFF

    // startup messages
    for (volatile uint32_t i = 0; i < 1000000; i++);
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_write(USART1, msg_prompt2, sizeof(msg_prompt2)-1);
}

void dma_setup(){
    // 1. Enable DMA1 clock
    rcc_periph_clock_enable(RCC_DMA1);
    // 2. Initialize DMA1 Channel 5 for USART1 RX
    dma_init(DMA1_Channel5, (uint32_t)&USART1->DR, (uint32_t)rx_buffer, RX_BUFF_SIZE);
    // 3. Enable DMA interrupt & NVIC
    dma_enable(DMA1_Channel5);
    // 4. Enable USART DMA request 
    usart_rx_dma_enable(USART1);
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();
    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick
    uart_setup();
    dma_setup();

    // TIM2 clock = APB1 frequency × 2
    uint32_t tim_clock = rcc_get_apb1_freq() * 2;
    tim_pwm_init(TIM2, &PWM_CH1_1KHZ_50, tim_clock);            // channel 1
    tim_pwm_init(TIM2, &PWM_CH2_1KHZ_50, tim_clock);            // channel 2 PWM

    // Sweep duty cycle up and down
    uint8_t duty = 0;
    uint8_t direction = 1;
    uint32_t last_update = systick_ticks;

    while(1){

        // ==== DMA : Handle half complete 32 bytes ===
        if (dma_half_complete) {
            dma_half_complete = 0;
            // Echo first half of buffer
            usart_write(USART1, (const uint8_t *) rx_buffer, RX_BUFF_SIZE/2);
            usart_write(USART1, msg_newline, sizeof(msg_newline) - 1);
            usart_write(USART1, msg_prompt2, sizeof(msg_prompt2)-1);
        }

        // ==== DMA : Handle full complete 64 bytes ===
        if (dma_full_complete) {
            dma_full_complete = 0;
            usart_write(USART1, (const uint8_t *) (rx_buffer + RX_BUFF_SIZE/2), RX_BUFF_SIZE/2);
            usart_write(USART1, msg_newline, sizeof(msg_newline) - 1);
            usart_write(USART1, msg_prompt2, sizeof(msg_prompt2) - 1);
        }

        // PWM : Update duty cycle every 10ms
        if (systick_ticks - last_update >= 10) {
            last_update = systick_ticks;
            duty += direction;
            if (duty >= 100) direction = -1;
            if (duty <= 0) direction = 1;
            tim_pwm_set_duty(TIM2, TIM_CH1, duty);
            tim_pwm_set_duty(TIM2, TIM_CH2, 100 - duty);        // LED2: 100→0 (opposite!)
        }
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}
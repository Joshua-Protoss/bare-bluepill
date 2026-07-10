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

#define RX_BUFF_SIZE                    128

volatile uint32_t systick_ticks = 0;
static volatile uint8_t rx_buffer[RX_BUFF_SIZE];
static volatile bool message_ready = false;
static volatile uint16_t message_length = 0;
static volatile bool dma_full_complete = false;

// Pre-defined messages as uint8_t arrays
static const uint8_t msg_welcome[] = "Blue Pill Echo Terminal\r\n";
static const uint8_t msg_prompt[]  = "Type something and press Enter:\r\n";
//static const uint8_t msg_prefix[]  = "\r\nYou typed: ";
static const uint8_t msg_prompt2[] = ">";
static const uint8_t msg_new_prompt[] = "\r\n>";

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
    if (DMA1_Controller->ISR & DMA_ISR_TCIF(5)) {
        DMA1_Controller->IFCR = DMA_IFCR_CTCIF(5); // look at reference manual rm008,  CGIF5(16) CTCIF5(17) CHTIF5(18) CTEIF5(19) for channel 5
        dma_full_complete = true;
    }
}

void usart1_isr(void) {
    // === IDLE LINE: Message complete! ===
    if (USART1->SR & USART_SR_IDLE) {
        // Clear IDLE flag: read SR, then read DR
        // Clearing an Overrun Error (ORE), Flushing the Buffer/Data Discard 
        volatile uint32_t dummy = USART1->SR;
        dummy = USART1->DR;
        (void) dummy;

        // Calculate how many bytes were received
        message_length = RX_BUFF_SIZE - DMA1_Channel5->CNDTR;
        if (message_length > 0) {
            message_ready = true;
        }
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
    // Clear IDLE flag: read SR, then read DR
    // Clearing an Overrun Error (ORE), Flushing the Buffer/Data Discard 
    volatile uint32_t dummy = USART1->SR;
    dummy = USART1->DR;
    (void) dummy;
    // Enable IDLE LINE interrupt
    usart_idle_interrupt_enable(USART1);

    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
    usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
    usart_write(USART1, msg_prompt2, sizeof(msg_prompt2) - 1);
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
    uint8_t duty = 0, direction = 1;
    uint32_t last_update = systick_ticks;

    while(1){
        // === Handle received message ===
        if (message_ready) {
            message_ready = false;

            // Check if the last character is Enter (\r or \n)
            bool ends_with_enter = false;
            if (message_length > 0) {
                uint8_t last_char = rx_buffer[message_length - 1]; 
                if (last_char == '\r' || last_char == '\n') {
                    ends_with_enter = true;
                    message_length--; // Don't include Enter in display
                }
            }
            
            // Process backspaces in the message
            uint16_t clean_length = 0;
            for (uint16_t i = 0; i < message_length; i++) {
                if (rx_buffer[i] == 8 || rx_buffer[i] == 127) {
                    // Backspace: remove previous character
                    if (clean_length > 0) {
                        clean_length--;
                    }
                } else {
                    // Normal character: add to clean buffer
                    rx_buffer[clean_length++] = rx_buffer[i];
                }
            }

            // Echo with prefix without new lines
            usart_write(USART1, (const uint8_t*) rx_buffer, message_length);

            if (ends_with_enter) {
                usart_write(USART1, msg_new_prompt, sizeof(msg_new_prompt) - 1);
            } 

            // reset DMA for next message
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel5->CNDTR = RX_BUFF_SIZE;
            DMA1_Channel5->CMAR = (uint32_t) rx_buffer;
            DMA1_Channel5->CCR |= DMA_CCR_EN;
            
        }

        // ==== DMA : Handle full complete 128 bytes ===
        if (dma_full_complete) {
            // this is a safeguard, only happen if the user is a maniac
            dma_full_complete = 0;
            usart_write(USART1, (const uint8_t *) rx_buffer, RX_BUFF_SIZE);
            usart_write(USART1, msg_new_prompt, sizeof(msg_new_prompt) - 1);
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
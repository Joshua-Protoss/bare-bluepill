#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include <string.h>
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
static const uint8_t msg_welcome[] = "Blue Pill LED Command Terminal\r\n";
static const uint8_t msg_prompt[]  = "Type 'HELP' for commands:\r\n";
//static const uint8_t msg_prefix[]  = "\r\nYou typed: ";
static const uint8_t msg_prompt2[] = ">";
static const uint8_t msg_new_prompt[] = "\r\n>";

// LED states
static bool led1_state = false;
static bool led2_state = false;

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_set_mode(LED2_PORT, LED2_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);

    // Start with LEDs OFF
    gpio_write_pin(LED_PORT, LED_PIN, true);   // Active low? Adjust as needed
    gpio_write_pin(LED2_PORT, LED2_PIN, true);
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
    
    // Clear pending IDLE flag before enabling interrupt
    volatile uint32_t dummy = USART1->SR;
    dummy = USART1->DR;
    (void) dummy;

    // Enable IDLE LINE interrupt
    usart_idle_interrupt_enable(USART1);

    // startup messages
    usart_write(USART1, msg_welcome, sizeof(msg_welcome)-1);
    usart_write(USART1, msg_prompt, sizeof(msg_prompt)-1);
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

void process_command(const char *cmd){

    if (strcmp(cmd, "HELP") == 0) {
        usart_printf(USART1, "Commands:\r\n");
        usart_printf(USART1, "  LED1 ON   - Turn LED1 ON\r\n");
        usart_printf(USART1, "  LED1 OFF  - Turn LED1 OFF\r\n");
        usart_printf(USART1, "  LED2 ON   - Turn LED2 ON\r\n");
        usart_printf(USART1, "  LED2 OFF  - Turn LED2 OFF\r\n");
        usart_printf(USART1, "  STATUS    - Show LED states\r\n");
        usart_printf(USART1, "  CLOCK     - Show system clocks\r\n");
        usart_printf(USART1, "  HELP      - This message\r\n");
    } else if (strcmp(cmd, "LED1 ON") == 0) {
        gpio_write_pin(LED_PORT, LED_PIN, false);
        led1_state = true;
        usart_printf(USART1, "LED1: ON\r\n");
    } else if (strcmp(cmd, "LED1 OFF") == 0) {
        gpio_write_pin(LED_PORT, LED_PIN, true);
        led1_state = false;
        usart_printf(USART1, "LED1: OFF\r\n");
    } else if (strcmp(cmd, "LED2 ON") == 0) {
        gpio_write_pin(LED2_PORT, LED2_PIN, false);
        led2_state = true;
        usart_printf(USART1, "LED2: ON\r\n");
    } else if (strcmp(cmd, "LED2 OFF") == 0) {
        gpio_write_pin(LED2_PORT, LED2_PIN, true);
        led2_state = false;
        usart_printf(USART1, "LED2: OFF\r\n");
    } else if (strcmp(cmd, "STATUS") == 0) {
        usart_printf(USART1, "LED1: %s\r\n", led1_state ? "ON" : "OFF");
        usart_printf(USART1, "LED2: %s\r\n", led2_state ? "ON" : "OFF");
        usart_printf(USART1, "Uptime: %lu sec\r\n", systick_ticks / 1000);
    } else if (strcmp(cmd, "CLOCK") == 0) {
        usart_printf(USART1, "SysClk: %lu Hz\r\n", rcc_get_sysclk_freq());
        usart_printf(USART1, "AHB:    %lu Hz\r\n", rcc_get_ahb_freq());
        usart_printf(USART1, "APB1:   %lu Hz\r\n", rcc_get_apb1_freq());
        usart_printf(USART1, "APB2:   %lu Hz\r\n", rcc_get_apb2_freq());
    } else {
        usart_printf(USART1, "Unknown: '%s'\r\n", cmd);
        usart_printf(USART1, "Type 'HELP' for commands\r\n");
    }
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();
    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick
    uart_setup();
    dma_setup();

    gpio_write_pin(LED_PORT, LED_PIN, true);
    
    while(1){
        // === Handle received message ===
        if (message_ready) {
            message_ready = false;

            // Check if last character is Enter
            bool ends_with_enter = false;
            if (message_length > 0) {
                uint8_t last_char = rx_buffer[message_length-1];
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
                    if (clean_length > 0) clean_length--;
                } else {
                    // Normal character: add to clean buffer
                    rx_buffer[clean_length++] = rx_buffer[i];
                }
            }

            // Echo with prefix without new lines
            usart_write_DR(USART1, (uint16_t)rx_buffer[message_length-1]);

            if (ends_with_enter){
                usart_write(USART1, msg_new_prompt, sizeof(msg_new_prompt)-1);
                if (clean_length > 0) {
                    rx_buffer[clean_length] = '\0';
                    process_command((const char *)rx_buffer);
                    usart_write(USART1, msg_prompt2, sizeof(msg_prompt2) - 1);
                }

                // reset DMA for next message
                DMA1_Channel5->CCR &= ~DMA_CCR_EN;
                DMA1_Channel5->CNDTR = RX_BUFF_SIZE;
                DMA1_Channel5->CMAR = (uint32_t) rx_buffer;
                DMA1_Channel5->CCR |= DMA_CCR_EN;
            }
        }

        // ==== DMA : Handle full complete 128 bytes ===
        if (dma_full_complete) {
            // this is a safeguard, only happen if the user is a maniac
            dma_full_complete = 0;
            usart_write(USART1, (const uint8_t *) rx_buffer, RX_BUFF_SIZE);
            usart_write(USART1, msg_new_prompt, sizeof(msg_new_prompt) - 1);

            // reset DMA for next message
            DMA1_Channel5->CCR &= ~DMA_CCR_EN;
            DMA1_Channel5->CNDTR = RX_BUFF_SIZE;
            DMA1_Channel5->CMAR = (uint32_t) rx_buffer;
            DMA1_Channel5->CCR |= DMA_CCR_EN;
        }
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}
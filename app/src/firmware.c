#include "common.h"
#include "rcc.h"
#include "gpio.h"
#include "systick.h"
#include "timers.h"
#include "usart.h"

#define LED_PORT         (PORT_GPIOA)       // this is an external LED connected to PA0
#define LED_PIN          (PIN_GPIO0)
#define LED2_PORT        (PORT_GPIOA) 
#define LED2_PIN         (PIN_GPIO1)       // TIM2_CH2

volatile uint32_t systick_ticks = 0;

void gpio_setup(void){
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    gpio_set_mode(LED2_PORT, LED2_PIN, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

void systick_handler(void){
    systick_ticks++;
}

int main(void) {
    rcc_clock_configure(&RCC_CLOCK_HSE_44MHZ);
    gpio_setup();
    systick_set_frequency(1000, rcc_get_ahb_freq()); // 1ms tick

    // USART Setup
    rcc_periph_clock_enable(RCC_USART1);
    // PA9 = TX (AF push-pull)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO9, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
    // PA10 = RX (floating input)
    gpio_set_mode(PORT_GPIOA, PIN_GPIO10, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
    // Configure USART1
    //uint32_t apb2_clock = rcc_get_apb2_freq();  // 44MHz
    USART1->BRR = (44000000 / 115200); // Baud rate
    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    // Send a character
    USART1->DR = 'A';
    while(!(USART1->SR & USART_SR_TC));

    // PWM configuration: 1kHz, 50% duty cycle on CH1
    tim_pwm_config_t pwm_config = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH1,
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_PWM_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    // Default
        .cms_mode = TIM_CMS_EDGE,      // Default
        .direction = TIM_DIR_UP,       // Default
    };

    // TIM2 clock = APB1 frequency × 2
    uint32_t tim_clock = rcc_get_apb1_freq() * 2;
    tim_pwm_init(TIM2, &pwm_config, tim_clock);

    // CH2: LED2 fading out (opposite phase)
    tim_pwm_config_t ch2_config = {
        .frequency = 1000,
        .duty_cycle = 50,
        .channel = TIM_CH2,          // ← Channel 2!
        .oc_mode = TIM_OC_MODE_PWM1,
        .op_mode = TIM_MODE_PWM_CONTINUOUS,
        .clock_div = TIM_CKD_DIV1,    // Default
        .cms_mode = TIM_CMS_EDGE,      // Default
        .direction = TIM_DIR_UP,       // Default
    };

    tim_pwm_init(TIM2, &ch2_config, tim_clock);

    // Sweep duty cycle up and down
    int8_t duty = 0;
    int8_t direction = 1;
    uint32_t last_update = systick_ticks;

    while(1){

        // Update duty cycle every 10ms
        if (systick_ticks - last_update >= 10) {
            last_update = systick_ticks;
            
            duty += direction;
            if (duty >= 100) {
                direction = -1;
            } else if (duty <= 0) {
                direction = 1;
            }
            
            tim_pwm_set_duty(TIM2, TIM_CH1, duty);
            tim_pwm_set_duty(TIM2, TIM_CH2, 100 - duty);  // LED2: 100→0 (opposite!)
        }
        
       __asm__("wfi");  // Sleep, save power!
    }
    return 0;
}
#ifndef INC_NVIC_H
#define INC_NVIC_H

#define NVIC_WWDG_IRQ            0
#define NVIC_PVD_IRQ             1
#define NVIC_TAMPER_IRQ          2
#define NVIC_RTC_IRQ             3
#define NVIC_FLASH_IRQ           4
#define NVIC_RCC_IRQ             5
#define NVIC_EXTI0_IRQ           6
#define NVIC_EXTI1_IRQ           7
#define NVIC_EXTI2_IRQ           8
#define NVIC_EXTI3_IRQ           9
#define NVIC_EXTI4_IRQ          10
#define NVIC_DMA1_CHANNEL1_IRQ  11
#define NVIC_DMA1_CHANNEL2_IRQ  12
#define NVIC_DMA1_CHANNEL3_IRQ  13
#define NVIC_DMA1_CHANNEL4_IRQ  14
#define NVIC_DMA1_CHANNEL5_IRQ  15
#define NVIC_DMA1_CHANNEL6_IRQ  16
#define NVIC_DMA1_CHANNEL7_IRQ  17
#define NVIC_ADC1_2_IRQ         18
#define NVIC_CAN_TX_IRQ         19
#define NVIC_CAN_RX0_IRQ        20
#define NVIC_CAN_RX1_IRQ        21
#define NVIC_CAN_SCE_IRQ        22
#define NVIC_EXTI9_5_IRQ        23
#define NVIC_TIM1_BRK_IRQ       24
#define NVIC_TIM1_UP_IRQ        25
#define NVIC_TIM1_TRG_COM_IRQ   26
#define NVIC_TIM1_CC_IRQ        27
#define NVIC_TIM2_IRQ           28
#define NVIC_TIM3_IRQ           29
#define NVIC_TIM4_IRQ           30
#define NVIC_I2C1_EV_IRQ        31
#define NVIC_I2C1_ER_IRQ        32
#define NVIC_I2C2_EV_IRQ        33
#define NVIC_I2C2_ER_IRQ        34
#define NVIC_SPI1_IRQ           35
#define NVIC_SPI2_IRQ           36
#define NVIC_USART1_IRQ         37
#define NVIC_USART2_IRQ         38
#define NVIC_USART3_IRQ         39
#define NVIC_EXTI15_10_IRQ      40
#define NVIC_RTC_ALARM_IRQ      41
#define NVIC_USB_WAKEUP_IRQ     42

void nvic_wwdg_isr(void);
void nvic_pvd_isr(void);
void nvic_tamper_isr(void);
void nvic_rtc_isr(void);
void nvic_flash_isr(void);
void nvic_rcc_isr(void);
void nvic_exti0_isr(void);
void nvic_exti1_isr(void);
void nvic_exti2_isr(void);
void nvic_exti3_isr(void);
void nvic_exti4_isr(void);
void nvic_dma1_channel1_isr(void);
void nvic_dma1_channel2_isr(void);
void nvic_dma1_channel3_isr(void);
void nvic_dma1_channel4_isr(void);
void nvic_dma1_channel5_isr(void);
void nvic_dma1_channel6_isr(void);
void nvic_dma1_channel7_isr(void);
void nvic_adc1_2_isr(void);
void nvic_can_tx_isr(void);
void nvic_can_rx0_isr(void);
void nvic_can_rx1_isr(void);
void nvic_can_sce_isr(void);
#endif // INC_NVIC_H

///c/gcc-arm-none-eabi/bin/arm-none-eabi-objdump -d firmware.elf | less
//"cortex-debug.openocdPath": "c:/openocd/bin/openocd",
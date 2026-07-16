#ifndef INC_ADC_H
#define INC_ADC_H

#include "common.h"
#include "rcc.h"

// ADC Base Addresses
#define ADC1_BASE                           (PERIPHERAL_APB2_BASE + 0x2400U)
#define ADC2_BASE                           (PERIPHERAL_APB2_BASE + 0x2800U)

// ADC Register Map
typedef struct {
    volatile uint32_t SR;                   // 0x00: Status Register
    volatile uint32_t CR1;                  // 0x04: Control Register 1
    volatile uint32_t CR2;                  // 0x08: Control Register 2
    volatile uint32_t SMPR1;                // 0x0C: Sample Time Register 1
    volatile uint32_t SMPR2;                // 0x10: Sample Time Register 2
    volatile uint32_t JOFR1;                // 0x14: Injected Channel Offset 1
    volatile uint32_t JOFR2;                // 0x18: Injected Channel Offset 2
    volatile uint32_t JOFR3;                // 0x1C: Injected Channel Offset 3
    volatile uint32_t JOFR4;                // 0x20: Injected Channel Offset 4
    volatile uint32_t HTR;                  // 0x24: Watchdog High Threshold
    volatile uint32_t LTR;                  // 0x28: Watchdog Low Threshold
    volatile uint32_t SQR1;                 // 0x2C: Regular Sequence Register 1
    volatile uint32_t SQR2;                 // 0x30: Regular Sequence Register 2
    volatile uint32_t SQR3;                 // 0x34: Regular Sequence Register 3
    volatile uint32_t JSQR;                 // 0x38: Injected Sequence Register
    volatile uint32_t JDR1;                 // 0x3C: Injected Data Register 1
    volatile uint32_t JDR2;                 // 0x40: Injected Data Register 2
    volatile uint32_t JDR3;                 // 0x44: Injected Data Register 3
    volatile uint32_t JDR4;                 // 0x48: Injected Data Register 4
    volatile uint32_t DR;                   // 0x4C: Regular Data Register
}ADC_reg_t;

// ADC Instances                            bluepill doesn't have ADC3
#define ADC1                                ((volatile ADC_reg_t *) ADC1_BASE)
#define ADC2                                ((volatile ADC_reg_t *) ADC2_BASE)

// ===== SR Bits =====
#define ADC_SR_AWD                          BIT(0)      // Analog Watchdog
#define ADC_SR_EOC                          BIT(1)      // End of Conversion
#define ADC_SR_JEOC                         BIT(2)      // Injected End of Conversion

// ===== CR1 Bits =====
#define ADC_CR1_EOCIE                       BIT(5)      // EOC Interrupt Enable
#define ADC_CR1_SCAN                        BIT(8)      // Scan Mode Enable
#define ADC_CR1_AWDEN                       BIT(23)     // Analog watchdog enable on regular channels

// ===== CR2 Bits =====
#define ADC_CR2_ADON                        BIT(0)      // ADC On
#define ADC_CR2_CONT                        BIT(1)      // Continuous Conversion
#define ADC_CR2_CAL                         BIT(2)      // A/D Calibration
#define ADC_CR2_RSTCAL                      BIT(3)      // Reset calibration
#define ADC_CR2_DMA                         BIT(8)      // DMA Enable
#define ADC_CR2_ALIGN                       BIT(11)     // Data Alignment (0=right, 1=left)
#define ADC_CR2_EXTTRIG                     BIT(20)     // External trigger conversion mode for regular channels
#define ADC_CR2_SWSTART                     BIT(22)     // Start Conversion (software)
#define ADC_CR2_TSVREFE                     BIT(23)     // Temperature Sensor Enable

// External trigger sources for CR2_EXTSEL
#define ADC_CR2_EXTSEL_TIM1_CC1            (0x00 << 17)    // 000: Timer 1 CC1 event
#define ADC_CR2_EXTSEL_TIM1_CC2            (0x01 << 17)    // 001: Timer 1 CC2 event
#define ADC_CR2_EXTSEL_TIM1_CC3            (0x02 << 17)    // 010: Timer 1 CC3 event
#define ADC_CR2_EXTSEL_TIM2_CC2            (0x03 << 17)    // 011: Timer 2 CC2 event
#define ADC_CR2_EXTSEL_TIM3_TRGO           (0x04 << 17)    // 100: Timer 3 TRGO event
#define ADC_CR2_EXTSEL_TIM4_CC4            (0x05 << 17)    // 101: Timer 4 CC4 event
#define ADC_CR2_EXTSEL_TIM8_TRGO           (0x06 << 17)    // 110: EXTI line 11/TIM8_TRGO event not available in bluepill
#define ADC_CR2_EXTSEL_SWSTART             (0x07 << 17)    // 111: SWSTART

#define ADC_SQR1_CONV_NUM_SHIFT            (20U)            // L[3:0]: Regular channel sequence length

// ===== SMPR Sample Times =====
typedef enum {
    ADC_SMP_1_5_CYCLES = 0x00,          // 1.5 ADC cycles
    ADC_SMP_7_5_CYCLES = 0x01,          // 7.5 ADC cycles
    ADC_SMP_13_5_CYCLES = 0x02,         // 13.5 ADC cycles
    ADC_SMP_28_5_CYCLES = 0x03,         // 28.5 ADC cycles
    ADC_SMP_41_5_CYCLES = 0x04,         // 41.5 ADC cycles
    ADC_SMP_55_5_CYCLES = 0x05,         // 55.5 ADC cycles
    ADC_SMP_71_5_CYCLES = 0x06,         // 71.5 ADC cycles
    ADC_SMP_239_5_CYCLES = 0x07,        // 239.5 ADC cycles
} ADC_sample_time_t;

// ADC Channels
typedef enum {
    ADC_CH0 = 0,
    ADC_CH1 = 1,
    ADC_CH2 = 2,
    ADC_CH3 = 3,
    ADC_CH4 = 4,
    ADC_CH5 = 5,
    ADC_CH6 = 6,
    ADC_CH7 = 7,
    ADC_CH8 = 8,
    ADC_CH9 = 9,
    ADC_CH10 = 10,
    ADC_CH11 = 11,
    ADC_CH12 = 12,
    ADC_CH13 = 13,
    ADC_CH14 = 14,
    ADC_CH15 = 15,
    ADC_CH16 = 16,      // Temperature sensor
    ADC_CH17 = 17,      // VREFINT
} ADC_channel_t;

// ADC Single Mode Configuration
typedef struct {
    ADC_channel_t channel;              // Which channel to read
    ADC_sample_time_t sample_time;      // Sampling duration
    rcc_adc_div_t prescaler;            // ADC Prescaler
    bool continuous;                    // Continuous or single conversion
} ADC_config_t;

// ADC Scan Mode Configuration
typedef struct {
    uint8_t num_channels;               // How many channels to scan (1-16)
    ADC_channel_t channels[16];         // Channel list in scan order
    ADC_sample_time_t sample_time;      // Same sample time for all channels
    rcc_adc_div_t prescaler;            // ADC Prescaler
    bool continuous;                    // Continuous or single conversion
} ADC_scan_config_t;

// Function Prototypes
void adc_init(volatile ADC_reg_t *adc, const ADC_config_t *config);
uint16_t adc_read(volatile ADC_reg_t *adc);
void adc_scan_init(volatile ADC_reg_t *adc, const ADC_scan_config_t *config);
void adc_start(volatile ADC_reg_t *adc);
void adc_stop(volatile ADC_reg_t *adc);
int32_t convert_internal_temp(uint16_t adc_raw);

extern const ADC_config_t ADC_CH0_TEST;
extern const ADC_config_t ADC_CH16_VREFINT;
extern const ADC_config_t ADC_CH1_TEST;

#endif //INC_ADC_H

        // uint16_t raw = adc_read(ADC1);
        // uint32_t mv = (raw * 3300) / 4096;
        // usart_printf(USART1, "ADC: %4lu mV (%4u raw) \r\n", mv, raw);
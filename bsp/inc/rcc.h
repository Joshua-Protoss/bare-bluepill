#ifndef INC_RCC_H
#define INC_RCC_H

#include "common.h"

// Helper Macros
#define RCC_ENCODE(offset, bit_enable)          ((offset) << 5 | (bit_enable))
#define RCC_REG_ADDR(encoded)                   REG32(RCC_BASE + ((encoded) >> 5))
#define RCC_BIT_MASK(encoded)                   BIT((encoded) & 0x1F)

// registers offsets (STM32F103)
#define RCC_CR_OFFSET            0x00
#define RCC_CFGR_OFFSET          0x04
#define RCC_CIR_OFFSET           0x08
#define RCC_AHB_ENR_OFFSET       0x14
#define RCC_APB1_ENR_OFFSET      0x1C
#define RCC_APB2_ENR_OFFSET      0x18

// memory addresses
#define RCC_BASE                                (0x40021000U)
#define RCC_CR                                  REG32(RCC_BASE + RCC_CR_OFFSET)
#define RCC_CFGR                                REG32(RCC_BASE + RCC_CFGR_OFFSET)
#define RCC_CIR                                 REG32(RCC_BASE + RCC_CIR_OFFSET)
#define FLASH_ACR                               REG32(0x40022000)   // flash access control register

// RCC_CR BITS
#define RCC_CR_HSION             BIT(0)
#define RCC_CR_HSIRDY            BIT(1)
#define RCC_CR_HSEON             BIT(16)
#define RCC_CR_HSERDY            BIT(17)
#define RCC_CR_HSEBYP            BIT(18)
#define RCC_CR_CSSON             BIT(19)
#define RCC_CR_PLLON             BIT(24)
#define RCC_CR_PLLRDY            BIT(25)

// RCC_CFGR hex bits and bit shift
#define RCC_CFGR_SW_HSI         0x00
#define RCC_CFGR_SW_HSE         0x01
#define RCC_CFGR_SW_PLL         0x02
#define RCC_CFGR_SWS_HSI        0x00
#define RCC_CFGR_SWS_HSE        0x04
#define RCC_CFGR_SWS_PLL        0x08
#define RCC_CFGR_PLLSRC         BIT(16) 
#define RCC_CFGR_PLLXTPRE       BIT(17)

// RCC_CFGR bit positions
#define RCC_CFGR_SW_SHIFT        0
#define RCC_CFGR_HPRE_SHIFT      4       // AHB PRESCALER
#define RCC_CFGR_PPRE1_SHIFT     8       // APB1 PRESCALER
#define RCC_CFGR_PPRE2_SHIFT     11      // APB2 PRESCALER
#define RCC_CFGR_ADCPRE_SHIFT    14      // ADC PRESCALER
#define RCC_CFGR_PLLSRC_SHIFT    16 
#define RCC_CFGR_PLLMUL_SHIFT    18      // PLL MULTIPLIER
#define RCC_CFGR_USBPRE_SHIFT    22

// FLASH_ACR bit shift positions
#define FLASH_ACR_LATENCY_SHIFT  0
#define FLASH_ACR_HLFCYA         BIT(3)
#define FLASH_ACR_PRFTBE         BIT(4)
#define FLASH_ACR_PRFTBS         BIT(5) 

// FLASH_ACR_LATENCY wait states
#define FLASH_ACR_ZERO_WS       0x00   // if 0 < SYSCLK < 24MHz
#define FLASH_ACR_ONE_WS        0x01    // if 24MHz < SYSCLK < 48MHz
#define FLASH_ACR_TWO_WS        0x02     // if 48 MHz < SYSCLK < 72MHZ

// Bit positions for AHB peripherals
#define RCC_DMA1_BIT             0
#define RCC_DMA2_BIT             1

// Bit positions for APB2 peripherals
#define RCC_GPIOA_BIT            2      // GPIOA
#define RCC_GPIOB_BIT            3      // GPIOB
#define RCC_GPIOC_BIT            4      // GPIOC
#define RCC_GPIOD_BIT            5      // GPIOD
#define RCC_GPIOE_BIT            6      // GPIOE
#define RCC_GPIOF_BIT            7      // GPIOF
#define RCC_GPIOG_BIT            8      // GPIOG

// Bit positions for APB1 peripherals
#define RCC_TIM2_BIT             0
#define RCC_TIM3_BIT             1        
#define RCC_USART2_BIT           17

// encoded clock enable values
enum rcc_periph_clken {
    // AHB peripherals
    RCC_DMA1 = RCC_ENCODE(RCC_AHB_ENR_OFFSET, RCC_DMA1_BIT),
    RCC_DMA2 = RCC_ENCODE(RCC_AHB_ENR_OFFSET, RCC_DMA2_BIT),

    // APB1 peripherals
    RCC_TIM2 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_TIM2_BIT),
    RCC_TIM3 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_TIM3_BIT),
    RCC_USART2 = RCC_ENCODE(RCC_APB1_ENR_OFFSET, RCC_USART2_BIT),

    // APB2 peripherals
    RCC_GPIOA = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOA_BIT),
    RCC_GPIOB = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOB_BIT),
    RCC_GPIOC = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOC_BIT),
    RCC_GPIOD = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOD_BIT),
    RCC_GPIOE = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOE_BIT),
    RCC_GPIOF = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOF_BIT),
    RCC_GPIOG = RCC_ENCODE(RCC_APB2_ENR_OFFSET, RCC_GPIOG_BIT),
};

// System clock sources
typedef enum {
    RCC_SYSCLK_HSI = RCC_CFGR_SW_HSI,
    RCC_SYSCLK_HSE = RCC_CFGR_SW_HSE,
    RCC_SYSCLK_PLL = RCC_CFGR_SW_PLL,
} rcc_sysclk_t;

// PLL source
typedef enum {
    RCC_PLLSRC_HSI_DIV2PLL = 0,
    RCC_PLLSRC_HSE = 1
} rcc_pllsrc_t;

// PLL multiplier
typedef enum {
    RCC_PLLMULT_2 = 0x00,
    RCC_PLLMULT_3 = 0x01,
    RCC_PLLMULT_4 = 0x02,
    RCC_PLLMULT_5 = 0x03,
    RCC_PLLMULT_6 = 0x04,
    RCC_PLLMULT_7 = 0x05,
    RCC_PLLMULT_8 = 0x06,
    RCC_PLLMULT_9 = 0x07, // PLLMULT_9 (8MHz × 9 = 72MHz) is maximum for STM32F103, PLL output range: 16-72 MHz
    RCC_PLLMULT_10 = 0x08,
    RCC_PLLMULT_11 = 0x09,
    RCC_PLLMULT_12 = 0x0A,
    RCC_PLLMULT_13 = 0x0B,
    RCC_PLLMULT_14 = 0x0C,
    RCC_PLLMULT_15 = 0x0D,
    RCC_PLLMULT_16 = 0x0E
} rcc_pllmult_t;

// HSE divider
typedef enum {
    RCC_PLLXTPRE_DIV1 = 0,
    RCC_PLLXTPRE_DIV2 = 1,
} rcc_pllxtpre_t;

// AHB prescaler
typedef enum {
    RCC_AHB_DIV_1 = 0x00,
    RCC_AHB_DIV_2 = 0x08,
    RCC_AHB_DIV_4 = 0x09,
    RCC_AHB_DIV_8 = 0x0A,
    RCC_AHB_DIV_16 = 0x0B,
    RCC_AHB_DIV_64 = 0x0C,
    RCC_AHB_DIV_128 = 0x0D,
    RCC_AHB_DIV_256 = 0x0E,
    RCC_AHB_DIV_512 = 0x0F
} rcc_ahb_div_t;

// APB prescaler
typedef enum {
    RCC_APB_DIV_1 = 0x00,
    RCC_APB_DIV_2 = 0x04,
    RCC_APB_DIV_4 = 0x05,
    RCC_APB_DIV_8 = 0x06,
    RCC_APB_DIV_16 = 0x07
} rcc_apb_div_t;

// Clock configuration struct
typedef struct {
    uint32_t hse_freq_hz;
    uint32_t hsi_freq_hz;       // HSI frequency STM32F103 = 8MHZ
    rcc_sysclk_t sysclk_source;
    rcc_pllsrc_t pll_source;
    rcc_pllmult_t pll_mult;
    rcc_pllxtpre_t pll_hse_div;
    rcc_ahb_div_t ahb_div;
    rcc_apb_div_t apb1_div;
    rcc_apb_div_t apb2_div;
} rcc_clock_config_t;

// enable peripheral clock functions
void rcc_periph_clock_enable(enum rcc_periph_clken encoded_clken);
void rcc_periph_clock_disable(enum rcc_periph_clken encoded_clken);

// clock configuration functions
void rcc_clock_configure(const rcc_clock_config_t *config);
void rcc_clock_reset_to_default(void);
uint8_t apb_div_to_value(rcc_apb_div_t div);
uint16_t ahb_div_to_value(rcc_ahb_div_t div);
uint32_t rcc_get_sysclk_freq(void);
uint32_t rcc_get_ahb_freq(void);
uint32_t rcc_get_apb1_freq(void);
uint32_t rcc_get_apb2_freq(void);

extern const rcc_clock_config_t RCC_CLOCK_HSI_8MHZ;
extern const rcc_clock_config_t RCC_CLOCK_HSE_8MHZ;
extern const rcc_clock_config_t RCC_CLOCK_HSE_PLL_72MHZ;
extern const rcc_clock_config_t RCC_CLOCK_HSE_44MHZ;

#endif //INC_RCC_H
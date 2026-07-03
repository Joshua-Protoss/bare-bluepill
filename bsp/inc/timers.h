#ifndef INC_TIMERS_H
#define INC_TIMERS_H

#include "common.h"

// TIM base addresses
#define TIM1_BASE                   (PERIPHERAL_APB2_BASE + 0x2C00U)
#define TIM8_BASE                   (PERIPHERAL_APB2_BASE + 0x3400U)
#define TIM9_BASE                   (PERIPHERAL_APB2_BASE + 0x4C00U)
#define TIM10_BASE                  (PERIPHERAL_APB2_BASE + 0x5000U)
#define TIM11_BASE                  (PERIPHERAL_APB2_BASE + 0x5400U)
#define TIM2_BASE                   (PERIPHERAL_APB1_BASE + 0x0000U)
#define TIM3_BASE                   (PERIPHERAL_APB1_BASE + 0x0400U)
#define TIM4_BASE                   (PERIPHERAL_APB1_BASE + 0x0800U)
#define TIM5_BASE                   (PERIPHERAL_APB1_BASE + 0x0C00U)
#define TIM6_BASE                   (PERIPHERAL_APB1_BASE + 0x1000U)
#define TIM7_BASE                   (PERIPHERAL_APB1_BASE + 0x1400U)
#define TIM12_BASE                  (PERIPHERAL_APB1_BASE + 0x1800U)
#define TIM13_BASE                  (PERIPHERAL_APB1_BASE + 0x1C00U)
#define TIM14_BASE                  (PERIPHERAL_APB1_BASE + 0x2000U)

// TIM_reg_t: Works for both GP and Advanced timers
// NOTE: RCR and BDTR registers only exist on TIM1 and TIM8.
// Writing to these on TIM2-5, TIM9-14 is ignored by hardware.
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;    
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;      // not exist in general purpose timer
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;     // not exist in general purpose timer
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_reg_t;

// CCMR Output Compare mode
typedef enum {
    TIM_OC_MODE_FROZEN    = (0x00 << 4),     // Output frozen (no change)
    TIM_OC_MODE_ACTIVE    = (0x01 << 4),     // Set active on match
    TIM_OC_MODE_INACTIVE  = (0x02 << 4),     // Set inactive on match
    TIM_OC_MODE_TOGGLE    = (0x03 << 4),     // Toggle on match
    TIM_OC_MODE_FORCE_L   = (0x04 << 4),     // Force inactive (LOW)
    TIM_OC_MODE_FORCE_H   = (0x05 << 4),     // Force active (HIGH)
    TIM_OC_MODE_PWM1      = (0x06 << 4),     // PWM Mode 1
    TIM_OC_MODE_PWM2      = (0x07 << 4),     // PWM Mode 2
} tim_oc_mode_t;

// Timer instances
#define TIM1                                    ((TIM_reg_t *) TIM1_BASE)
#define TIM2                                    ((TIM_reg_t *) TIM2_BASE)
#define TIM3                                    ((TIM_reg_t *) TIM3_BASE)
#define TIM4                                    ((TIM_reg_t *) TIM4_BASE)
#define TIM5                                    ((TIM_reg_t *) TIM5_BASE)
#define TIM6                                    ((TIM_reg_t *) TIM6_BASE)
#define TIM7                                    ((TIM_reg_t *) TIM7_BASE)
#define TIM8                                    ((TIM_reg_t *) TIM8_BASE)
#define TIM9                                    ((TIM_reg_t *) TIM9_BASE)
#define TIM10                                   ((TIM_reg_t *) TIM10_BASE)
#define TIM11                                   ((TIM_reg_t *) TIM11_BASE)
#define TIM12                                   ((TIM_reg_t *) TIM12_BASE)
#define TIM13                                   ((TIM_reg_t *) TIM13_BASE)
#define TIM14                                   ((TIM_reg_t *) TIM14_BASE)

// ===== CR1 Bits =====         
#define TIM_CR1_CEN                             BIT(0)      // Counter enable
#define TIM_CR1_UDIS                            BIT(1)      // Update disable
#define TIM_CR1_URS                             BIT(2)      // Update request source
#define TIM_CR1_OPM                             BIT(3)      // One pulse mode
#define TIM_CR1_DIR                             BIT(4)      // Direction (0=up, 1=down)
#define TIM_CR1_CMS_MASK                        (0x03 << 5) 
#define TIM_CR1_CMS_EDGE                        (0x00 << 5) // Edge-aligned
#define TIM_CR1_CMS_CENTER1                     (0x01 << 5) // Center-aligned mode 1
#define TIM_CR1_CMS_CENTER2                     (0x02 << 5) // Center-aligned mode 2
#define TIM_CR1_CMS_CENTER3                     (0x03 << 5) // Center-aligned mode 3
#define TIM_CR1_ARPE                            BIT(7)         // Auto-reload preload enable
#define TIM_CR1_CKD_MASK                        (0x03 << 8) 
#define TIM_CR1_CKD_DIV1                        (0x00 << 8)
#define TIM_CR1_CKD_DIV2                        (0x01 << 8) 
#define TIM_CR1_CKD_DIV4                        (0x02 << 8) 

// ===== CCMR1 Bits =====
#define TIM_CCMR1_CC1S_MASK                     (0x03 << 0)
#define TIM_CCMR1_CC1S_OUTPUT                   (0x00 << 0)         // output
#define TIM_CCMR1_CC1S_INPUT_TI1                (0x01 << 0)         // input
#define TIM_CCMR1_CC1S_INPUT_TI2                (0x02 << 0)
#define TIM_CCMR1_CC1S_INPUT_TRC                (0x03 << 0)
#define TIM_CCMR1_OC1FE                         BIT(2)              // Output compare 1 fast enable
#define TIM_CCMR1_OC1PE                         BIT(3)              // Output compare 1 preload
#define TIM_CCMR1_OC1M_MASK                     (0x07 << 4)

// ===== CCER Bits =====
#define TIM_CCER_CC1E                           BIT(0)
#define TIM_CCER_CC1P                           BIT(1)          // determine wether active means HIGH or LOW
#define TIM_CCER_CC1NE                          BIT(2)
#define TIM_CCER_CC1NP                          BIT(3)
#define TIM_CCER_CC2E                           BIT(4)
#define TIM_CCER_CC2P                           BIT(5)
#define TIM_CCER_CC2NE                          BIT(6)
#define TIM_CCER_CC2NP                          BIT(7)
#define TIM_CCER_CC3E                           BIT(8)
#define TIM_CCER_CC3P                           BIT(9)
#define TIM_CCER_CC3NE                          BIT(10)
#define TIM_CCER_CC3NP                          BIT(11)
#define TIM_CCER_CC4E                           BIT(12)
#define TIM_CCER_CC4P                           BIT(13)
#define TIM_CCER_CC4NP                          BIT(15)

// ===== DIER Bits =====
#define TIM_DIER_UIE                            BIT(0)
#define TIM_DIER_CC1IE                          BIT(1)
#define TIM_DIER_CC2IE                          BIT(2)
#define TIM_DIER_CC3IE                          BIT(3)
#define TIM_DIER_CC4IE                          BIT(4)
#define TIM_DIER_COMIE                          BIT(5)
#define TIM_DIER_TIE                            BIT(6)
#define TIM_DIER_BIE                            BIT(7)
#define TIM_DIER_UDE                            BIT(8)
#define TIM_DIER_CC1DE                          BIT(9)
#define TIM_DIER_CC2DE                          BIT(10)
#define TIM_DIER_CC3DE                          BIT(11)
#define TIM_DIER_CC4DE                          BIT(12)
#define TIM_DIER_COMDE                          BIT(13)
#define TIM_DIER_TDE                            BIT(14)

// ===== SR Bits =====
#define TIM_SR_UIF                              BIT(0)
#define TIM_SR_CC1IF                            BIT(1)
#define TIM_SR_CC2IF                            BIT(2)
#define TIM_SR_CC3IF                            BIT(3)
#define TIM_SR_CC4IF                            BIT(4)
#define TIM_SR_COMIF                            BIT(5)
#define TIM_SR_TIF                              BIT(6)
#define TIM_SR_BIF                              BIT(7)
#define TIM_SR_CC1OF                            BIT(9)
#define TIM_SR_CC2OF                            BIT(10)
#define TIM_SR_CC3OF                            BIT(11)
#define TIM_SR_CC4OF                            BIT(12)

// ===== EGR Bits =====
#define TIM_EGR_UG                              BIT(0)
#define TIM_EGR_CC1G                            BIT(1)
#define TIM_EGR_CC2G                            BIT(2)
#define TIM_EGR_CC3G                            BIT(3)
#define TIM_EGR_CC4G                            BIT(4)
#define TIM_EGR_COMG                            BIT(5)
#define TIM_EGR_TG                              BIT(6)
#define TIM_EGR_BG                              BIT(7)

// Timer channels
typedef enum {
    TIM_CH1 = 0,
    TIM_CH2 = 1,
    TIM_CH3 = 2,
    TIM_CH4 = 3,
} tim_channel_t;

// PWM options
typedef enum {
    TIM_MODE_PWM_CONTINUOUS,   // Regular PWM (runs forever)
    TIM_MODE_PWM_ONE_SHOT,     // One-pulse mode
} tim_op_mode_t;

typedef enum {
    TIM_CKD_DIV1 = (0x00 << 8),   // tDTS = tCK_INT
    TIM_CKD_DIV2 = (0x01 << 8),   // tDTS = 2 × tCK_INT
    TIM_CKD_DIV4 = (0x02 << 8),   // tDTS = 4 × tCK_INT
} tim_ckd_t;

typedef enum {
    TIM_CMS_EDGE    = (0x00 << 5),  // Edge-aligned
    TIM_CMS_CENTER1 = (0x01 << 5),  // Center-aligned (up-down, interrupt at count=0)
    TIM_CMS_CENTER2 = (0x02 << 5),  // Center-aligned (interrupt at count=ARR)
    TIM_CMS_CENTER3 = (0x03 << 5),  // Center-aligned (interrupt at count=0 and ARR)
} tim_cms_t;

typedef enum {
    TIM_DIR_UP   = (0 << 4),    // Up-counting
    TIM_DIR_DOWN = (1 << 4),    // Down-counting
} tim_dir_t;

// PWM configuration 
typedef struct {
    uint32_t frequency;         // Desired PWM frequency
    uint8_t duty_cycle;         // Initial duty cycle (0-100)
    tim_channel_t channel;      // Which channel to use
    tim_oc_mode_t oc_mode;      // Output compare mode
    tim_op_mode_t op_mode;
    tim_ckd_t clock_div;      
    tim_cms_t cms_mode;       
    tim_dir_t direction;
} tim_pwm_config_t;

// Function prototypes
void tim_pwm_init(TIM_reg_t *tim, const tim_pwm_config_t * config, uint32_t tim_clock_hz);
void tim_pwm_set_duty(TIM_reg_t *tim, tim_channel_t channel, uint8_t duty_percent);
void tim_enable(TIM_reg_t *tim);
void tim_disable(TIM_reg_t *tim);

extern const tim_pwm_config_t PWM_CH1_1KHZ_50;
extern const tim_pwm_config_t PWM_CH2_1KHZ_50;

#endif // INC_TIMERS_H
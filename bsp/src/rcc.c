#include "rcc.h"

// ===== RCC PERIPHERAL FUNCTIONS IMPLEMENTATION ===
void rcc_periph_clock_enable(enum rcc_periph_clken encoded_clken){
    RCC_REG_ADDR(encoded_clken) |= RCC_BIT_MASK(encoded_clken);
}

void rcc_periph_clock_disable(enum rcc_periph_clken encoded_clken){
    RCC_REG_ADDR(encoded_clken) &= ~RCC_BIT_MASK(encoded_clken);
}

// ===== STANDARD RCC CONFIGURATIONS =====
static rcc_clock_config_t rcc_current_config = {
    .sysclk_source = RCC_SYSCLK_HSI,
    .hsi_freq_hz = 8000000,
};

// default HSI 8MHz clock configuration
const rcc_clock_config_t RCC_CLOCK_HSI_8MHZ = {
    .sysclk_source = RCC_SYSCLK_HSI,
};
// HSE 8MHz clock configuration
const rcc_clock_config_t RCC_CLOCK_HSE_8MHZ = {
    .sysclk_source = RCC_SYSCLK_HSE,
    .hse_freq_hz = 8000000
};
// HSE 8MHz PLL 72MHz Maximum performance
const rcc_clock_config_t RCC_CLOCK_HSE_PLL_72MHZ = {
    .hse_freq_hz = 8000000,
    .sysclk_source = RCC_SYSCLK_PLL,
    .pll_source = RCC_PLLSRC_HSE,
    .pll_mult = RCC_PLLMULT_9,
    .pll_hse_div = RCC_PLLXTPRE_DIV1,
    .ahb_div = RCC_AHB_DIV_1,
    .apb1_div = RCC_APB_DIV_2,
    .apb2_div = RCC_APB_DIV_1
};

// 44MHz: HSE (8MHz) / 2 = 4MHz × 11 = 44MHz
const rcc_clock_config_t RCC_CLOCK_HSE_44MHZ = {
    .hse_freq_hz = 8000000,
    .sysclk_source = RCC_SYSCLK_PLL,
    .pll_source = RCC_PLLSRC_HSE,
    .pll_mult = RCC_PLLMULT_11,
    .pll_hse_div = RCC_PLLXTPRE_DIV2,   // 8MHz / 2 = 4MHz input
    .ahb_div = RCC_AHB_DIV_1,           // 44MHz AHB
    .apb1_div = RCC_APB_DIV_2,          // 22MHz APB1 (max 36MHz)
    .apb2_div = RCC_APB_DIV_1,          // 44MHz APB2
};

// ===== RCC CLOCK CONFIGURATION FUNCTIONS =====
void rcc_clock_configure(const rcc_clock_config_t *config){
    // === Check if user Enable HSE, configure HSE, wait until RCC_CR_HSERDY turns 1 (READY) ===
    if (config->sysclk_source == RCC_SYSCLK_HSE || 
        (config->sysclk_source == RCC_SYSCLK_PLL && config->pll_source == RCC_PLLSRC_HSE)){
        
        RCC_CR &= ~RCC_CR_HSEBYP; // 0: external 4-16 MHz oscillator not bypassed, turn 1 if MCU receives an active clock signal and not a passive crystal
        RCC_CR |= RCC_CR_HSEON;   // turn on HSE
        while(!(RCC_CR & RCC_CR_HSERDY)); // wait until HSE ready
    }

    // === Configure PLL if used ===
    if (config->sysclk_source == RCC_SYSCLK_PLL) {
        RCC_CR &= ~RCC_CR_PLLON;        // Turn off PLL before configuring, the hardware will automatically set PLLRDY to 0 (unlock)
        while (RCC_CR & RCC_CR_PLLRDY);  // wait for PLL off (PLLRDY set to 0)

        // Clear PLL config bits, and set HSE divider to 1 (default: not divided)
        RCC_CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | (0x0F << RCC_CFGR_PLLMUL_SHIFT));

        // Set PLL source
        if (config->pll_source == RCC_PLLSRC_HSE) {
            RCC_CFGR |= RCC_CFGR_PLLSRC;

            // set HSE divider for PLL
            if (config->pll_hse_div == RCC_PLLXTPRE_DIV2){
                RCC_CFGR |= RCC_CFGR_PLLXTPRE;
            } 
            // else: RCC_CFGR_PLLXTPRE = 0 (HSE not divided, default)
        }
        
        // Set PLL multiplier
        RCC_CFGR |= (config->pll_mult << RCC_CFGR_PLLMUL_SHIFT);

        // Enable PLL
        RCC_CR |= RCC_CR_PLLON;
        while(!(RCC_CR & RCC_CR_PLLRDY)); // wait for PLL ready
    }

    // === Calculate new system clock frequency ===
    uint32_t new_sysclk_freq;
    switch (config->sysclk_source) {
        case RCC_SYSCLK_HSI:
            new_sysclk_freq = config->hsi_freq_hz;
            break;
        case RCC_SYSCLK_HSE:
            new_sysclk_freq = config->hse_freq_hz;
            break;
        case RCC_SYSCLK_PLL:
            if(config->pll_source == RCC_PLLSRC_HSE) {

                uint32_t pll_input = config->hse_freq_hz;
                if (config->pll_hse_div == RCC_PLLXTPRE_DIV2) {
                    pll_input /= 2;             // HSE frequency divided by 2 
                }
                new_sysclk_freq = pll_input * (config->pll_mult + 2);

            } else {
                new_sysclk_freq = (config->hsi_freq_hz / 2) * (config->pll_mult + 2);
            }
            break;
        default:
            new_sysclk_freq = 8000000;
    }
    
    // === Set flash latency to match the new clock speed ===
    FLASH_ACR |= FLASH_ACR_PRFTBE; // Enable Prefetch buffer
    FLASH_ACR &= ~(0x07 << FLASH_ACR_LATENCY_SHIFT); // Clear any existing latency bits

    if (new_sysclk_freq <= 24000000) {
        FLASH_ACR |= (FLASH_ACR_ZERO_WS << FLASH_ACR_LATENCY_SHIFT);
    } else if (new_sysclk_freq <= 48000000) {
        FLASH_ACR |= (FLASH_ACR_ONE_WS << FLASH_ACR_LATENCY_SHIFT);
    } else {
        FLASH_ACR |= (FLASH_ACR_TWO_WS << FLASH_ACR_LATENCY_SHIFT);
    }

    // === Configure bus prescalers ===
    // Clear bus prescalers bits
    RCC_CFGR &= ~((0x0F << RCC_CFGR_HPRE_SHIFT) | (0x07 << RCC_CFGR_PPRE1_SHIFT) | (0x07 << RCC_CFGR_PPRE2_SHIFT));
    // Set bus prescalers
    RCC_CFGR |= (config->ahb_div << RCC_CFGR_HPRE_SHIFT) | (config->apb1_div << RCC_CFGR_PPRE1_SHIFT) | (config->apb2_div << RCC_CFGR_PPRE2_SHIFT);
    
    // === Switch system clock ===
    RCC_CFGR &= ~(0x03 << RCC_CFGR_SW_SHIFT); // Clear system switch bits
    RCC_CFGR |= (config->sysclk_source << RCC_CFGR_SW_SHIFT); // Select system clock : HSI, HSE, or PLL

    // Wait for switch
    uint32_t sws_mask = (config->sysclk_source == RCC_SYSCLK_PLL) ? 0x08 : (config->sysclk_source << 2);
    volatile uint32_t timeout = 100000;
    while (((RCC_CFGR & 0x0C) != sws_mask) && --timeout); // wait for switch until timeout
    
    if (timeout == 0) { // handle fail switching, use HSI as fallback clock
        if (!(RCC_CR & RCC_CR_HSIRDY)){ // just double check if HSI is ready
            RCC_CR |= RCC_CR_HSION;
            while (!(RCC_CR & RCC_CR_HSIRDY));
        }
        // Switching back to HSI
        RCC_CFGR &= ~(0x03 << RCC_CFGR_SW_SHIFT);           // Clear CFGR_SW bits = Set CFGR_SW to HSI (0x00)
        while((RCC_CFGR & 0x0C) != RCC_CFGR_SW_HSI);        // wait for the switch status register to indicate HSI

        // Set current config to HSI (default)
        rcc_current_config = (rcc_clock_config_t){
            .sysclk_source = RCC_SYSCLK_HSI,
            .hsi_freq_hz = 8000000,
        };
        return;                    // The failed config is not cached
    }

    // === Turn off PLL if not used (power saving) ===
    if (config->sysclk_source != RCC_SYSCLK_PLL) {
        RCC_CR &= ~RCC_CR_PLLON;
    }

    // cache the configuration
    rcc_current_config = *config;
}

uint8_t apb_div_to_value(rcc_apb_div_t div){
    switch(div){
        case RCC_APB_DIV_1: return 1;
        case RCC_APB_DIV_2: return 2;
        case RCC_APB_DIV_4: return 4;
        case RCC_APB_DIV_8: return 8;
        case RCC_APB_DIV_16: return 16;
        default:            return 1;
    }
}

uint16_t ahb_div_to_value(rcc_ahb_div_t div){
    switch(div){
        case RCC_AHB_DIV_1: return 1;
        case RCC_AHB_DIV_2: return 2;
        case RCC_AHB_DIV_4: return 4;
        case RCC_AHB_DIV_8: return 8;
        case RCC_AHB_DIV_16: return 16;
        case RCC_AHB_DIV_64: return 64;
        case RCC_AHB_DIV_128: return 128;
        case RCC_AHB_DIV_256: return 256;
        case RCC_AHB_DIV_512: return 512;
        default:               return 1;
    }
}

uint32_t rcc_get_sysclk_freq(void){
    switch(rcc_current_config.sysclk_source){
        case RCC_SYSCLK_HSI:
            return rcc_current_config.hsi_freq_hz;
        case RCC_SYSCLK_HSE:
            return rcc_current_config.hse_freq_hz;
        case RCC_SYSCLK_PLL:
            if(rcc_current_config.pll_source == RCC_PLLSRC_HSE) {
                uint32_t pll_input = rcc_current_config.hse_freq_hz;

                // Apply HSE divider if configured
                if (rcc_current_config.pll_hse_div == RCC_PLLXTPRE_DIV2) {
                    pll_input /= 2;
                }
                return pll_input * (rcc_current_config.pll_mult + 2);
            } else {
                return (rcc_current_config.hsi_freq_hz / 2) * (rcc_current_config.pll_mult + 2);
            }
        default:
            return rcc_current_config.hsi_freq_hz;
    }
    return 0;
}

uint32_t rcc_get_ahb_freq(void){
    return rcc_get_sysclk_freq() / ahb_div_to_value(rcc_current_config.ahb_div);
}

uint32_t rcc_get_apb1_freq(void){
    return rcc_get_ahb_freq() / apb_div_to_value(rcc_current_config.apb1_div);
}

uint32_t rcc_get_apb2_freq(void){
    return rcc_get_ahb_freq() / apb_div_to_value(rcc_current_config.apb2_div);
}

void rcc_clock_reset_to_default(void){
    rcc_clock_configure(&RCC_CLOCK_HSI_8MHZ);
}
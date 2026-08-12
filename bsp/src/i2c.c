#include "i2c.h"
#include "systick.h"

// Current configuration
static I2C_config_t current_config;
static bool initialized = false;

// ===== LOW-LEVEL GPIO CONTROL =====
static inline void scl_low(void) {
    gpio_write_pin(current_config.port, current_config.scl_pin, 0);
}

static inline void scl_high(void) {
    gpio_write_pin(current_config.port, current_config.scl_pin, 0);
}

static inline void sda_low(void) {                                       // For open-drain, set pin LOW (output mode drives low)
    gpio_write_pin(current_config.port, current_config.sda_pin, 0);      // First, ensure pin is in output mode
}

static inline void sda_high (void) {                                     // For open-drain, "release" the pin (set to input/floating)
    gpio_write_pin(current_config.port, current_config.sda_pin, 1);      // The pull-up resistor will pull it HIGH
}

static inline uint8_t sda_read(void) {
    return gpio_read_pin(current_config.port, current_config.sda_pin);
}

// ===== DELAY FUNCTIONS =====                                            // For 100kHz: bit period = 10µs, half = 5µs
static void i2c_delay_half_bit(void) {                                    // For 400kHz: bit period = 2.5µs, half = 1.25µs
    
    if (current_config.speed_hz <= 100000) {                              // ~5µs delay for 100kHz
        for (volatile uint32_t i = 0; i < 50; i++) {
            __asm__("nop");
        }
    } else {                                                              // ~1.25µs delay for 400kHz
        for (volatile uint32_t i = 0; i < 12; i++) {
            __asm__("nop");
        }
    }
}

// ===== I2C SIGNAL GENERATION =====
static void i2c_start(void) {                                              // START: SDA goes LOW while SCL is HIGH
    sda_high();
    scl_high();
    i2c_delay_half_bit();
    sda_low();
    i2c_delay_half_bit();
    scl_low();
}

static void i2c_stop(void) {                                                // STOP: SDA goes HIGH while SCL is HIGH
    sda_low();
    scl_high();
    i2c_delay_half_bit();
    sda_high();
    i2c_delay_half_bit();
}

static uint8_t i2c_write_byte(uint8_t data) {                               // Send 8 bits, MSB first
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) {
            sda_high();
        } else {
            sda_low();
        }
        i2c_delay_half_bit();
        scl_high();
        i2c_delay_half_bit();
        scl_low();
        data <<= 1;
    }

    // Read ACK bit (9th clock)
    sda_high();
}

void i2c_bitbang_init(const I2C_config_t *config) {

}

bool i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);
bool i2c_probe(uint8_t addr);
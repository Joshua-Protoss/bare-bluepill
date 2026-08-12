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
    return gpio_read_pin(current_config.port, current_config. sda_pin);
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
    sda_high();                                                               // Release SDA for slave to drive
    i2c_delay_half_bit();
    scl_high();
    i2c_delay_half_bit();
    uint8_t ack = sda_read();                                                 // 0 = ACK, 1 = NACK
    scl_low();
    i2c_delay_half_bit();
    return ack;                                                               // 0 = success, 1 = failure
}

static uint8_t i2c_read_byte(bool send_ack) {
    uint8_t data = 0;
    sda_high();                                                               // Release SDA for slave to drive

    for (uint8_t i = 0; i < 8; i++) {                                         // Read 8 bits, MSB first
        scl_high();
        i2c_delay_half_bit();
        data <<= 1;
        if (sda_read()) {
            data |= 1;
        }
        scl_low();
        i2c_delay_half_bit();
    }

    if (send_ack) {                                                           // Send ACK or NACK
        sda_low();                                                            // ACK: pull low
    } else {
        sda_high();                                                           // NACK: leave high
    }
    i2c_delay_half_bit();
    scl_high();
    i2c_delay_half_bit();
    scl_low();
    sda_high();                                                                // Release SDA
    return data;
}

// ===== INITIALIZATION =====
void i2c_bitbang_init(const I2C_config_t *config) {                     // Configure SCL and SDA as open-drain outputs
    current_config = *config;                                           // Use push-pull but switch between output LOW and input FLOATING
                                                                        // This mimics open-drain behavior with the pull-up resistors
    gpio_set_mode(config->port, config->scl_pin,                        // SCL: Output push-pull (always drive it)
        GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);              // SDA: switch between output LOW and input FLOATING
    gpio_set_mode(config->port, config->sda_pin,                        // Start with output HIGH (which drives it high via push-pull temporarily)
        GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_PUSHPULL);              // Use gpio_write_pin to set LOW, or switch to input for HIGH
    
    scl_high();                                                         // Ensure both lines start HIGH (idle state)
    sda_high();
    initialized = true;
}

static void sda_set_output(void) {                                       // Better SDA handling: switch between output and input
    gpio_set_mode(current_config.port, current_config.sda_pin,           // Set SDA as output push-pull to drive LOW
        GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
}

static void sda_set_input(void) {                                        // Set SDA as input floating to "release" (pull-up pulls it HIGH)
    gpio_set_mode(current_config.port, current_config.sda_pin,
        GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_INPUT_FLOATING);
}

// ===== HIGH-LEVEL FUNCTIONS =====
bool i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (!initialized) return false;

    i2c_start();

    if (i2c_write_byte((addr << 1) | 0x00)) {                           // Send slave address (write mode)
        i2c_stop();
        return false;                                                   // No ACK from slave
    }

    if (i2c_write_byte(reg)) {                                          // Send register address
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i])) {
            i2c_stop();
            return false;
        }
    }

    i2c_stop();
    return true;
}

bool i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (!initialized || len == 0) return false;
    i2c_start();                                                        // First, write the register address

    if (i2c_write_byte((addr << 1) | 0x00)) {
        i2c_stop();
        return false;
    }

    if (i2c_write_byte(reg)) {
        i2c_stop();
        return false;
    }

    i2c_start();                                                        // Repeated START for read
    if (i2c_write_byte((addr << 1) | 0x01)) {
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {                                 // Read data bytes
        data[i] = i2c_read_byte(i < (len - 1));                         // Send ACK for all bytes except the last one
    }

    i2c_stop();
    return true;
}

bool i2c_probe(uint8_t addr) {
    if (!initialized) return false;

    i2c_start();
    uint8_t ack = i2c_write_byte((addr << 1) | 0x00);                   // Write mode
    i2c_stop();
    return (ack == 0);                                                  // true if device responded
}

const I2C_config_t MAX30102_I2C_CFG = {
    .scl_pin = PIN_GPIO6,                                               // PB6
    .sda_pin = PIN_GPIO7,                                               // PB7
    .port = PORT_GPIOB,
    .speed_hz = 100000,                                                 // 100kHz standard mode
};
#include "i2c.h"
#include "rcc.h"
#include "systick.h"

// Current configuration
static I2C_config_t current_config;
static bool initialized = false;

// ===== LOW-LEVEL GPIO CONTROL =====
static inline void scl_low(void) {
    gpio_set_mode(current_config.port, current_config.scl_pin,
        GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_write_pin(current_config.port, current_config.scl_pin, 0);
}

static inline void scl_high(void) {
    //  gpio_write_pin(current_config.port, current_config.scl_pin, 1);
    gpio_set_mode(current_config.port, current_config.scl_pin,
                GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
}

static inline void sda_low(void) {                                       // For open-drain, set pin LOW (output mode drives low)
    gpio_set_mode(current_config.port, current_config.sda_pin,
        GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_PUSHPULL);
    gpio_write_pin(current_config.port, current_config.sda_pin, 0);      // First, ensure pin is in output mode
}

static inline void sda_high (void) {                                     // For open-drain, "release" the pin (set to input/floating)
    //gpio_write_pin(current_config.port, current_config.sda_pin, 1);      // The pull-up resistor will pull it HIGH
    gpio_set_mode(current_config.port, current_config.sda_pin,
              GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOATING);
}

static inline uint8_t sda_read(void) {
    return gpio_read_pin(current_config.port, current_config. sda_pin);
}

// ===== DELAY FUNCTIONS =====                                            // For 100kHz: bit period = 10µs, half = 5µs
static void i2c_delay_half_bit(void) {                                    // For 400kHz: bit period = 2.5µs, half = 1.25µs
    if (current_config.speed_hz <= 100000) {                              // ~5µs delay for 100kHz
        for (volatile uint32_t i = 0; i < 500; i++) {
            __asm__("nop");
        }
    } else {                                                              // ~1.25µs delay for 400kHz
        for (volatile uint32_t i = 0; i < 12; i++) {
            __asm__("nop");
        }
    }
}

// ===== I2C SIGNAL GENERATION =====
static void i2c_start(void) {                                          // START: SDA goes LOW while SCL is HIGH
    sda_high();                                                        // sda_high() and scl_high() is the idle state
    scl_high();                                                        // The I2C protocol dictates that data on the SDA line 
    i2c_delay_half_bit();                                              // can only change while SCL is LOW.
    sda_low();                                                         // A split-second later, the Master pulls SCL LOW.
    i2c_delay_half_bit();                                              // Every slave on the bus sees this specific transition
    scl_low();                                                         // and resets its internal bit counters.
}

static void i2c_stop(void) {                                           // STOP: SDA goes HIGH while SCL is HIGH
    sda_low();                                                         // While SCL is LOW, the Master pulls SDA LOW
    scl_high();                                                        // The Master releases SCL HIGH
    i2c_delay_half_bit();                                              // While SCL remains HIGH, the Master releases SDA HIGH.
    sda_high();                                                        // The bus is now back in its Idle State (both lines HIGH)
    i2c_delay_half_bit();                                              // Waiting for the next START condition.
}

static uint8_t i2c_write_byte(uint8_t data) {                          // Send 8 bits, MSB first
    for (uint8_t i = 0; i < 8; i++) {                                  // While SCL is LOW, the Master changes SDA 
        if (data & 0x80) {                                             // to match the data bit (1 = float/high, 0 = pull low).
            sda_high();                                                // The I2C protocol dictates that data on the SDA line 
        } else {                                                       // can only change while SCL is LOW.
            sda_low();
        }
        i2c_delay_half_bit();                                          // The Master drives SCL HIGH. The lines freeze. 
        scl_high();                                                    // Every slave reads the SDA state right now.
        i2c_delay_half_bit();                                          // Prepare Next Bit
        scl_low();                                                     // The Master drives SCL LOW again.
        data <<= 1;                                                    // Send the next bit
    }

    // The 9th Clock Pulse: The ACK/NACK Check                         // See if a slave actually exists at that address.
    sda_high();                                                        // Release SDA for slave to drive, slave will set SDA low
    i2c_delay_half_bit();                                              // If a slave matches the address: 
    scl_high();                                                        // The slave actively pulls SDA LOW (ACK = 0) while SCL is still LOW
    i2c_delay_half_bit();                                              // If no slave exists: The line stays HIGH via the resistors (NACK = 1)
    uint8_t ack = sda_read();                                          // The Master set the SCL HIGH before reading this pin state,     
    scl_low();                                                         // then drives SCL LOW to conclude the address phase.
    i2c_delay_half_bit();                                              // 0 = ACK, 1 = NACK
    return ack;                                                        // 0 = success, 1 = failure
}

static uint8_t i2c_read_byte(bool send_ack) {                          // Release SDA for slave to drive with sda_high() 
    uint8_t data = 0;                                                  // right before sda_read() is mandatory because it                                         
    sda_high();                                                        // Physically steps the Master off the SDA line so the slave
    for (uint8_t i = 0; i < 8; i++) {                                  // Can safely pull it down to ground without a short circuit.      
        scl_high();                                                    
        i2c_delay_half_bit();
        data <<= 1;                                                    // Read 8 bits, MSB first
        if (sda_read()) {                                               
            data |= 1;
        }
        scl_low();
        i2c_delay_half_bit();
    }

    if (send_ack) {                                                     // Send ACK or NACK
        sda_low();                                                      // ACK: pull low
    } else {
        sda_high();                                                     // NACK: leave high
    }
    i2c_delay_half_bit();
    scl_high();
    i2c_delay_half_bit();
    scl_low();
    sda_high();                                                         // Release SDA
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

// ===== HIGH-LEVEL FUNCTIONS =====
bool i2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    if (!initialized) return false;
    i2c_start();
    if (i2c_write_byte((addr << 1) | 0x00)) {                           // Send slave address (write mode)
        i2c_stop();                                                     // 0 = ACK (success), 1 = NACK (failure)
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
    if (i2c_write_byte((addr << 1) | 0x00)) {                           // tell the slave in this address that we want to write
        i2c_stop();                                                     // 0 = ACK (success), 1 = NACK (failure)
        return false;   
    }

    if (i2c_write_byte(reg)) {                                          // Write the register address that we want to read from
        i2c_stop();                                                     // the slave will set its register pointer to this address
        return false;
    }

    i2c_start();                                                        // Repeated START for read, if we stop first, other master might interrupt
    if (i2c_write_byte((addr << 1) | 0x01)) {                           // tell the slave in this address that we want to read
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {                                 // Read data bytes, from the register address that we set earlier
        data[i] = i2c_read_byte(i < (len - 1));                         // Send ACK for all bytes except the last one
    }

    i2c_stop();                                                         // Only stop at the end
    return true;
}

bool i2c_probe(uint8_t addr) {                              // In I2C communication, standard device addresses are 7 bits long (ranging from 0x00 to 0x7F) 
    if (!initialized) return false;                         // the I2C protocol mandates that the very first byte transmitted after a START condition must be 8 bits long
    i2c_start();                                            // Bit:    7    6    5    4    3    2    1    0
    uint8_t ack = i2c_write_byte((addr << 1) | 0x00);       //         [---- 7-Bit Device Address ----]  [R/W]
    i2c_stop();                                             // initiating a Write sequence              | 0x00 
    return (ack == 0);                                      // true if device responded
}

const I2C_config_t MAX30102_I2C_CFG = {
    .scl_pin = PIN_GPIO6,                                               // PB6
    .sda_pin = PIN_GPIO7,                                               // PB7
    .port = PORT_GPIOB,
    .speed_hz = 100000,                                                 // 100kHz standard mode
};

// ===== HARDWARE I2C FUNCTIONS =====
void i2c_hardware_init(void) {
    // Enable GPIOB and I2C1 clocks
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);

    // PB6 = I2C1_SCL (Alternate Function Open-Drain)
    gpio_set_mode(PORT_GPIOB, PIN_GPIO6, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_OPENDRAIN);
    // PB7 = I2C1_SDA
    gpio_set_mode(PORT_GPIOB, PIN_GPIO7, GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_OPENDRAIN);

    // Reset I2C
    I2C1->CR1 |= I2C_CR1_SWRST;                                          
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // Configure clock for 100kHz
    // PCLK1 = 22MHz (your APB1)
    // CCR = PCLK1 / (2 * 100kHz) = 110
    I2C1->CR2 = 22;                                                 // FREQ = 22MHz
    I2C1->CR2 = 110;                                                // 100kHz
    I2C1->TRISE = 23;                                               // Max rise time

    // Enable I2C
    I2C1->CR1 |= I2C_CR1_PE;                                        // PE
}

bool i2c_hardware_read(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len) {
    // Wait for bus idle
    while(I2C1->SR2 & BIT(1));                                      // BUSY

    // Generate START
    
}

// static void sda_set_output(void) {                                       // Better SDA handling: switch between output and input
//     gpio_set_mode(current_config.port, current_config.sda_pin,           // Set SDA as output push-pull to drive LOW
//         GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_OUTPUT_AF_PUSHPULL);
// }

// static void sda_set_input(void) {                                        // Set SDA as input floating to "release" (pull-up pulls it HIGH)
//     gpio_set_mode(current_config.port, current_config.sda_pin,
//         GPIO_MODE_OUTPUT_50MHZ, GPIO_CNF_INPUT_FLOATING);
// }
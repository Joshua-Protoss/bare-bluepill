#ifndef INC_FAULT_LOG_H
#define INC_FAULT_LOG_H

#include "common.h"

// Backup register layout for FOC system
#define BKP_DR1_FAULT_CODE              1       // Fault type
#define BKP_DR2_FAULT_TIMESTAMP         2       // systick when fault occurred
#define BKP_DR3_LAST_STATE              3       // What was system doing?
#define BKP_DR4_PHASE_CURRENT           4       // Ia at fault (scaled)
#define BKP_DR5_BUS_VOLTAGE             5       // Vbus at fault (scaled)
#define BKP_DR6_STACK_POINTER           6       // SP at fault
#define BKP_DR7_LOOP_COUNTER            7       // Control loop iteration
#define BKP_DR8_MAGIC_NUMBER            8       // Validates data integrity
#define BKP_DR9_WWDG_COUNTER            9       // WWDG counter value at fault
#define BKP_DR10_RESERVED               10      // Future use

// Magic number to validate backup registers contain real fault data
#define BKP_MAGIC_VALID                 0xBEEF

// System states
typedef enum {
    STATE_INIT                      = 0x00,
    STATE_IDLE                      = 0x10,
    STATE_MOTOR_STARTING            = 0x20,
    STATE_FOC_RUNNING               = 0x30,
    STATE_READING_ADC               = 0x40,
    STATE_CALCULATING               = 0x50,
    STATE_UPDATING_PWM              = 0x60,
    STATE_EMERGENCY_STOP            = 0x70,
    STATE_FAULT_HANDLER             = 0x80,
    STATE_CRASHED                   = 0x99,
} system_state_t;

// Fault codes with priority
typedef enum {
    FAULT_NONE                      = 0x0000,
    FAULT_WWDG_TIMING               = 0x0001,
    FAULT_IWDG_TIMEOUT              = 0x0002,
    FAULT_OVERCURRENT_PHASE_A       = 0x0010,
    FAULT_OVERCURRENT_PHASE_B       = 0x0011,
    FAULT_OVERCURRENT_PHASE_C       = 0x0012,
    FAULT_OVERTEMP_MOSFET           = 0x0020,
    FAULT_OVERTEMP_MOTOR            = 0x0021,
    FAULT_UNDERVOLTAGE              = 0x0030,
    FAULT_OVERVOLTAGE               = 0x0031,
    FAULT_ENCODER_LOST              = 0x0040,
    FAULT_HARD_FAULT                = 0x00FF,
    FAULT_UNKWOWN                   = 0xFFFF,
} fault_code_t;

// Function prototypes
void fault_log_init(void);
void fault_log_record(fault_code_t fault, system_state_t state, uint16_t extra1, uint16_t extra2);
void fault_log_dump(void);
void fault_log_clear(void);
bool fault_log_is_valid(void);

#endif
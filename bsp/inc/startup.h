#ifndef INC_STARTUP_H
#define INC_STARTUP_H

#include "common.h"
#define NVIC_IRQ_COUNT 43

// function pointer type for the vector table struct
typedef void (*vector_table_entry_t)(void); 

typedef struct {
    uint32_t *initial_sp_value;
    vector_table_entry_t reset;
    vector_table_entry_t nmi;
    vector_table_entry_t hard_fault;
    vector_table_entry_t memory_management;
    vector_table_entry_t bus_fault;
    vector_table_entry_t usage_fault;
    vector_table_entry_t reserved_0x001c[4];
    vector_table_entry_t sv_call;
    vector_table_entry_t debug_monitor;
    vector_table_entry_t reserved_0x0034;
    vector_table_entry_t pend_sv;
    vector_table_entry_t systick;
    vector_table_entry_t irq_handler[NVIC_IRQ_COUNT];
} __attribute__((aligned(256))) vector_table_t;


#endif // INC_STARTUP_H
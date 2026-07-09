#ifndef INC_DMA_H
#define INC_DMA_H

#include "common.h"

// DMA Base Addresses
#define DMA1_BASE                   (PERIPHERAL_AHB_BASE + 0x8000U)
#define DMA2_BASE                   (PERIPHERAL_AHB_BASE + 0x8400U)

// DMA Channel struct
typedef struct {
   volatile uint32_t CCR;               // Channel configuration register  (Offset 0x00)
   volatile uint32_t CNDTR;             // Channel number of data register (Offset 0x04)
   volatile uint32_t CPAR;              // Channel peripheral address reg  (Offset 0x08)
   volatile uint32_t CMAR;              // Channel memory address register  (Offset 0x0C)
   uint32_t reserved;                   // 4-byte padding for every channel
} DMA_Channel_reg_t;

typedef struct {
    volatile uint32_t ISR;              // Interrupt status register      (Offset 0x00)
    volatile uint32_t IFCR;             // Interrupt flag clear register  (Offset 0x04)
    // Channels start immediately after the global status registers at offset 0x08
    //DMA_Channel_reg_t channel[7];       // DMA1 has 7 channels, DMA2 has 5
} DMA_Controller_reg_t;

// DMA instances                                    Read rm0008 for address offsets : 0x08 + 0d20 × (channel number – 1)
#define DMA1_Controller                            ((volatile DMA_Controller_reg_t*) DMA1_BASE)
#define DMA1_Channel1                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x08U))
#define DMA1_Channel2                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x1CU)) 
#define DMA1_Channel3                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x30U))
#define DMA1_Channel4                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x44U))
#define DMA1_Channel5                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x58U)) 
#define DMA1_Channel6                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x6CU))
#define DMA1_Channel7                              ((volatile DMA_Channel_reg_t*) (DMA1_BASE + 0x80U))
#define DMA2_Channel1                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x08U))
#define DMA2_Channel2                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x1CU)) 
#define DMA2_Channel3                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x30U))
#define DMA2_Channel4                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x44U))
#define DMA2_Channel5                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x58U)) 
#define DMA2_Channel6                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x6CU))
#define DMA2_Channel7                              ((volatile DMA_Channel_reg_t*) (DMA2_BASE + 0x80U))

// DMA Channel Mapping
#define DMA_CH1_ADC1                                0
#define DMA_CH2_SPI1_RX                             1
#define DMA_CH3_SPI1_TX                             2
#define DMA_CH4_USART1_TX                           3
#define DMA_CH5_USART1_RX                           4
#define DMA_CH6_USART2_RX                           5
#define DMA_CH7_USART2_TX                           6

// CCR Bits
#define DMA_CCR_EN                                  BIT(0)        // Channel enable
#define DMA_CCR_TCIE                                BIT(1)        // Transfer complete interrupt
#define DMA_CCR_HTIE                                BIT(2)        // Half transfer interrupt
#define DMA_CCR_TEIE                                BIT(3)        // Transfer error interrupt
#define DMA_CCR_DIR                                 BIT(4)        // Data transfer direction (0=periph→mem, 1=mem→periph)
#define DMA_CCR_CIRC                                BIT(5)        // Circular mode
#define DMA_CCR_PINC                                BIT(6)        // Peripheral increment mode
#define DMA_CCR_MINC                                BIT(7)        // Memory increment mode
#define DMA_CCR_PSIZE_8BIT                          (0x00 << 8)
#define DMA_CCR_PSIZE_16BIT                         (0x01 << 8)
#define DMA_CCR_PSIZE_32BIT                         (0x02 << 8)
#define DMA_CCR_MSIZE_8BIT                          (0x00 << 10)
#define DMA_CCR_MSIZE_16BIT                         (0x01 << 10)
#define DMA_CCR_MSIZE_32BIT                         (0x02 << 10)
#define DMA_CCR_PL_LOW                              (0x00 << 12)
#define DMA_CCR_PL_MEDIUM                           (0x01 << 12)    
#define DMA_CCR_PL_HIGH                             (0x02 << 12)                   
#define DMA_CCR_PL_VHIGH                            (0x03 << 12)    
#define DMA_CCR_MEM2MEM                             BIT(14)       // Memory to memory mode

// Function prototypes
void dma_init(volatile DMA_Channel_reg_t *dma_channel, uint32_t peripheral_addr, uint32_t memory_addr, uint16_t transfer_size);
void dma_enable(volatile DMA_Channel_reg_t *dma_channel);
void dma_disable(volatile DMA_Channel_reg_t *dma_channel);

#endif // INC_DMA_H
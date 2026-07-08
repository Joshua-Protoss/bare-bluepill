#include "dma.h"
#include "nvic.h"

void dma_init(volatile DMA_Channel_reg_t *dma_channel, uint32_t peripheral_addr, uint32_t memory_addr, uint16_t transfer_size){
    // 1. Safely disable the channel before modifying control registers
    //dma_channel->CCR = 0;
    dma_channel->CCR &= ~DMA_CCR_EN;    // clear EN bit

    // 2. Set the target memory and peripheral addresses
    dma_channel->CPAR = peripheral_addr;
    dma_channel->CMAR = memory_addr;

    // 3. Define how many items to move
    dma_channel->CNDTR = transfer_size;

    // 4. Apply clean configurations (e.g., Memory Increment, Peripheral-to-Memory)
    dma_channel->CCR = DMA_CCR_MINC            // Increment memory pointer
                     | DMA_CCR_PSIZE_8BIT      // 8-bit peripheral
                     | DMA_CCR_MSIZE_8BIT      // 8-bit memory
                     | DMA_CCR_CIRC            // Circular mode (keep receiving)
                     //| DMA_CCR_HTIE            // Interrupt on half transfer complete
                     | DMA_CCR_TCIE;           // Interrupt on transfer complete
                                               // DIR=0: Read from peripheral → write to memory (default)
}

void dma_enable(volatile DMA_Channel_reg_t *dma_channel){
    if (dma_channel == DMA1_Channel1) nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    if (dma_channel == DMA1_Channel2) nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);
    if (dma_channel == DMA1_Channel3) nvic_enable_irq(NVIC_DMA1_CHANNEL3_IRQ);
    if (dma_channel == DMA1_Channel4) nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);
    if (dma_channel == DMA1_Channel5) nvic_enable_irq(NVIC_DMA1_CHANNEL5_IRQ);
    if (dma_channel == DMA1_Channel6) nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);
    if (dma_channel == DMA1_Channel7) nvic_enable_irq(NVIC_DMA1_CHANNEL7_IRQ);

    // DMA2 channels (only compiled if DMA2 exists, doesn't exist in bluepill)
    #ifdef DMA2_BASE
    if (dma_channel == DMA2_Channel1) nvic_enable_irq(NVIC_DMA2_CHANNEL1_IRQ);
    if (dma_channel == DMA2_Channel2) nvic_enable_irq(NVIC_DMA2_CHANNEL2_IRQ);
    if (dma_channel == DMA2_Channel3) nvic_enable_irq(NVIC_DMA2_CHANNEL3_IRQ);
    if (dma_channel == DMA2_Channel4) nvic_enable_irq(NVIC_DMA2_CHANNEL4_5_IRQ);
    if (dma_channel == DMA2_Channel5) nvic_enable_irq(NVIC_DMA2_CHANNEL4_5_IRQ);
    #endif

    dma_channel->CCR |= DMA_CCR_EN;
}

void dma_disable(volatile DMA_Channel_reg_t *dma_channel){
    if (dma_channel == DMA1_Channel1) nvic_disable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    if (dma_channel == DMA1_Channel2) nvic_disable_irq(NVIC_DMA1_CHANNEL2_IRQ);
    if (dma_channel == DMA1_Channel3) nvic_disable_irq(NVIC_DMA1_CHANNEL3_IRQ);
    if (dma_channel == DMA1_Channel4) nvic_disable_irq(NVIC_DMA1_CHANNEL4_IRQ);
    if (dma_channel == DMA1_Channel5) nvic_disable_irq(NVIC_DMA1_CHANNEL5_IRQ);
    if (dma_channel == DMA1_Channel6) nvic_disable_irq(NVIC_DMA1_CHANNEL6_IRQ);
    if (dma_channel == DMA1_Channel7) nvic_disable_irq(NVIC_DMA1_CHANNEL7_IRQ);

        // DMA2 channels (only compiled if DMA2 exists, doesn't exist in bluepill)
    #ifdef DMA2_BASE
    if (dma_channel == DMA2_Channel1) nvic_disable_irq(NVIC_DMA2_CHANNEL1_IRQ);
    if (dma_channel == DMA2_Channel2) nvic_disable_irq(NVIC_DMA2_CHANNEL2_IRQ);
    if (dma_channel == DMA2_Channel3) nvic_disable_irq(NVIC_DMA2_CHANNEL3_IRQ);
    if (dma_channel == DMA2_Channel4) nvic_disable_irq(NVIC_DMA2_CHANNEL4_5_IRQ);
    if (dma_channel == DMA2_Channel5) nvic_disable_irq(NVIC_DMA2_CHANNEL4_5_IRQ);
    #endif

    dma_channel->CCR &= ~DMA_CCR_EN;
}
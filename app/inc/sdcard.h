#include "common.h"
#include "spi.h"

// SD Card Commands
#define CMD0     0x40  // GO_IDLE_STATE (reset)
#define CMD1     0x41  // SEND_OP_COND (initialize)
#define CMD8     0x48  // SEND_IF_COND (check voltage)
#define CMD9     0x49  // SEND_CSD (get card info)
#define CMD10    0x4A  // SEND_CID (get card ID)
#define CMD16    0x50  // SET_BLOCKLEN
#define CMD17    0x51  // READ_SINGLE_BLOCK
#define CMD24    0x58  // WRITE_BLOCK
#define CMD55    0x77  // APP_CMD (prefix for ACMD)
#define CMD58    0x7A  // READ_OCR (operating conditions)
#define ACMD41   0x69  // SD_SEND_OP_COND (SD init)

// Function prototypes
void sd_card_spi_test(void);
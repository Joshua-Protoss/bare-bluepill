#include <sys/stat.h>

// ===== Minimal syscall stubs for bare-metal printf =====

int _close(int file) {
    return -1;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _write(int file, char *ptr, int len) {
    // If you want printf to go to UART, implement here later
    return len;
}

void _exit(int status) {
    while (1);
}

void *_sbrk(int incr) {
    extern char _ebss;
    static char *heap_end = 0;
    char *prev_heap_end;
    
    if (heap_end == 0) {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;
    heap_end += incr;
    return (void *)prev_heap_end;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    return -1;
}
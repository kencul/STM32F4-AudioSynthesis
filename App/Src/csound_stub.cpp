#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <stddef.h>
#include "stm32f4xx_hal.h"

extern "C" {
    // Linker symbols from your .ld file
    extern uint8_t _end;       
    extern uint8_t _estack;    
    extern uint32_t _Min_Stack_Size; 

    static uint8_t *heap_end = NULL;

    // --- MEMORY MANAGEMENT ---
    __attribute__((used))
    void * _sbrk(ptrdiff_t incr) {
        uint8_t *prev_heap_end;
        if (heap_end == NULL) {
            heap_end = &_end;
        }
        prev_heap_end = heap_end;

        if (heap_end + incr > (uint8_t*)(&_estack - _Min_Stack_Size)) {
            errno = ENOMEM;
            return (void *)-1;
        }

        heap_end += incr;
        return (void *)prev_heap_end;
    }

    // --- TIME KEEPING ---
    int _gettimeofday(struct timeval *tv, void *tzvp) {
        if (tv) {
            uint32_t ticks = HAL_GetTick(); 
            tv->tv_sec = ticks / 1000;
            tv->tv_usec = (ticks % 1000) * 1000;
        }
        return 0;
    }

    clock_t _times(struct tms *buf) {
        return (clock_t)-1;
    }

    // --- FILE SYSTEM STUBS (Fail gracefully) ---
    int _open(const char *path, int flags, ...) { return -1; }
    int _close(int file) { return -1; }
    int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
    int _stat(const char *file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
    int _isatty(int file) { return 1; }
    int _lseek(int file, int ptr, int dir) { return 0; }
    int _read(int file, char *ptr, int len) { return 0; }
    int _write(int file, char *ptr, int len) { return len; }
    int _unlink(const char *name) { errno = ENOENT; return -1; }

    // --- SYSTEM PROCESS STUBS ---
    void _exit(int status) { while(1); }
    int _kill(int id, int sig) { errno = EINVAL; return -1; }
    int _getpid(void) { return 1; }
}

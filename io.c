#include "io.h"

void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void io_wait(void) {
    /* Port 0x80 is used for 'checkpoints' during POST. */
    /* Linux kernel seems to think it is free for use :-/ */
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <device.h>
#include <ringbuf.h>

typedef struct  acia_data               acia_data_t;
typedef union   acia_status             acia_status_t;
typedef union   acia_command            acia_command_t;

typedef struct  display_data            display_data_t;
typedef struct  display_video_mode      display_video_mode_t;

typedef struct  timer_data              timer_data_t;

union acia_status {
    uint8_t     raw;
    struct {
        uint8_t     pe      : 1;    // Parity Error
        uint8_t     fe      : 1;    // Frame Error
        uint8_t     ovrn    : 1;    // Overrun
        uint8_t     rdrf    : 1;    // Receive Data Register Full
        uint8_t     tdre    : 1;    // Transmit Data Register Empty
        uint8_t     dcdb    : 1;    // Data Carrier Detect
        uint8_t     dsrb    : 1;    // Date Set Ready
        uint8_t     irq     : 1;    // Interrupt Occurred
    };
};

union acia_command {
    uint8_t     raw;
    struct {
        uint8_t     dtr     : 1;    // Data Terminal Ready
        uint8_t     ird     : 1;    // Interrupt Request Disabled
        uint8_t     tic     : 2;    // Transmit Interrupt Control
        uint8_t     rem     : 1;    // Receive Echo Mode
        uint8_t     pme     : 1;    // Parity Mode Enable
        uint8_t     pmc     : 2;    // Parity Mode Control
    };
};

struct acia_data {
    uint32_t        pty_master;
    uint32_t        pty_slave;
    char            pty_name[64];

    ringbuf_t*      tx_ringbuf;
    ringbuf_t*      rx_ringbuf;

    acia_status_t   status;
    acia_command_t  command;
};

struct display_video_mode {
    uint32_t    width;
    uint32_t    height;
};

struct display_data {
    Display*    display;
    int32_t     screen;
    Window      window;
    GC          gc;

    display_video_mode_t* mode;

    struct {
        uint8_t     redraw : 1;
        uint8_t     reserved : 1;
    };

    uint32_t    scale_factor;

    uint8_t     x;
    uint8_t     y;

    uint8_t*    framebuffer;
};

struct timer_data {
    double      last_tick;

    uint8_t     counter;
    uint8_t     divider;
};

extern void mem_init(device_t* dev, uint32_t size, bool is_rom);
extern void acia_init(device_t* dev);
extern void display_init(device_t* dev);
extern void timer_init(device_t* dev);
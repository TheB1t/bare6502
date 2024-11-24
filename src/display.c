#include <peripheral.h>
#include <utils.h>

display_video_mode_t video_modes[] = {
    {
        .width  = 256,
        .height = 256,
    }
};

static uint32_t did = 0;


void set_non_resizable(Display *display, Window window, int width, int height) {
    XSizeHints* hints = XAllocSizeHints();

    if (!hints) {
        printf("Unable to allocate size hints\n");
        exit(1);
    }

    hints->min_width = width;
    hints->max_width = width;
    hints->min_height = height;
    hints->max_height = height;

    hints->flags = PMinSize | PMaxSize;

    XSetWMNormalHints(display, window, hints);
    XFree(hints);
}

void __destroy_window(display_data_t* locals) {
    free(locals->framebuffer);
    XFreeGC(locals->display, locals->gc);
    XDestroyWindow(locals->display, locals->window);
}

void __create_window(display_data_t* locals, display_video_mode_t* mode) {
    if (locals->window)
        __destroy_window(locals);

    locals->window = XCreateSimpleWindow(
        locals->display,
        RootWindow(locals->display, locals->screen),
        10, 10, mode->width * locals->scale_factor, mode->height * locals->scale_factor, 1,
        BlackPixel(locals->display, locals->screen),
        BlackPixel(locals->display, locals->screen)
    );

    set_non_resizable(locals->display, locals->window, mode->width * locals->scale_factor, mode->height * locals->scale_factor);

    char buf[128];
    sprintf(buf, "Display %d", did++);
    XStoreName(locals->display, locals->window, buf);
    XSelectInput(locals->display, locals->window, ExposureMask | KeyPressMask);
    XMapWindow(locals->display, locals->window);

    locals->gc = XCreateGC(locals->display, locals->window, 0, NULL);
    locals->framebuffer = (uint8_t*)malloc(mode->width * mode->height);
    locals->mode = mode;
}

void display_free(device_t* dev) {
    display_data_t* locals = (display_data_t*)dev->data;

    __destroy_window(locals);
    XCloseDisplay(locals->display);
    free(dev->data);
}

unsigned long rgb332_to_x11(unsigned char rgb332) {
    unsigned char red = (rgb332 >> 5) & 0x07; // Extract 3-bit Red
    unsigned char green = (rgb332 >> 2) & 0x07; // Extract 3-bit Green
    unsigned char blue = rgb332 & 0x03; // Extract 2-bit Blue

    unsigned long red_x11 = (red * 255) / 7;
    unsigned long green_x11 = (green * 255) / 7;
    unsigned long blue_x11 = (blue * 255) / 3;

    return (red_x11 << 16) | (green_x11 << 8) | blue_x11;
}

uint32_t display_io_bus_read(bus_link_t* link, uint32_t address) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    display_data_t* locals = (display_data_t*)dev->data;
    uint32_t data       = 0;

    switch (address) {
        case 0x00:
            data = locals->x;
            break;

        case 0x01:
            data = locals->y;
            break;

        case 0x02:
            break;

        case 0x03:
            locals->redraw = 1;
            break;
    }

    return data;
}

void display_io_bus_write(bus_link_t* link, uint32_t address, uint32_t data) {
    device_t* dev       = bus_container_of(link->dst_bus, device_t);
    display_data_t* locals = (display_data_t*)dev->data;

    switch (address) {
        case 0x00:
            locals->x = data;
            break;

        case 0x01:
            locals->y = data;
            break;

        case 0x02: {
            uint32_t pixel = (locals->y * locals->mode->width) + locals->x;
            locals->framebuffer[pixel] = data;
        } break;

        case 0x03:
            break;
    }
}

void display_step(device_t* dev) {
    display_data_t* locals = (display_data_t*)dev->data;

    XEvent event;
    while (XPending(locals->display)) {
        XNextEvent(locals->display, &event);
        switch (event.type) {
            default:
                break;
        }
    }

    if (locals->redraw) {
        locals->redraw = 0;

        for (uint32_t y = 0; y < locals->mode->height; y++) {
            for (uint32_t x = 0; x < locals->mode->width; x++) {
                uint32_t pixel = (y * locals->mode->width) + x;
                uint32_t rgb = rgb332_to_x11(locals->framebuffer[pixel]);
                XSetForeground(locals->display, locals->gc, rgb);
                // XDrawPoint(locals->display, locals->window, locals->gc, x, y);
                XFillRectangle(locals->display, locals->window, locals->gc, x * locals->scale_factor, y * locals->scale_factor, locals->scale_factor, locals->scale_factor);
            }
        }
    }
}

bus_entry_t display_bus_entries[] = {
    {
        .name   = "bus",
        .type   = BUS_TYPE_BUS,

        .address_width  = 16,
        .data_width     = 8,

        .io_bus_read    = display_io_bus_read,
        .io_bus_write   = display_io_bus_write,
    }
};

void display_init(device_t* dev) {
    display_data_t* locals = (display_data_t*)malloc(sizeof(display_data_t));
    dev->data = (void*)locals;

    locals->scale_factor = 2;
    locals->display = XOpenDisplay(NULL);
    if (!locals->display) {
        printf("Can't open display\n");
        exit(1);
    }

    locals->screen = DefaultScreen(locals->display);
    __create_window(locals, &video_modes[0]);

    dev->bus.entries        = display_bus_entries;
    dev->bus.entry_count    = sizeof(display_bus_entries) / sizeof(bus_entry_t);
    dev->bus.size           = 4;

    dev->step       = display_step;
    dev->free       = display_free;
}
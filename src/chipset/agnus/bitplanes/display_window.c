#include "display_window.h"

void display_window_reset(display_window_t *w)
{
    w->diwstrt = 0;
    w->diwstop = 0;
    w->ddfstrt = 0;
    w->ddfstop = 0;
}

void display_window_set_diwstrt(display_window_t *w, rigel_u16 val) { w->diwstrt = val; }
void display_window_set_diwstop(display_window_t *w, rigel_u16 val) { w->diwstop = val; }
void display_window_set_ddfstrt(display_window_t *w, rigel_u16 val) { w->ddfstrt = val & 0x00FCu; }
void display_window_set_ddfstop(display_window_t *w, rigel_u16 val) { w->ddfstop = val & 0x00FCu; }

bool display_window_fetch_active(const display_window_t *w, rigel_u16 hpos)
{
    /* TODO(bitplanes): verify low-bit masking matches hardware */
    rigel_u16 h = hpos & 0x00FEu;
    return h >= (w->ddfstrt & 0x00FEu) && h <= (w->ddfstop & 0x00FEu);
}

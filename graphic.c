#include <stdlib.h>
#include </usr/local/Cellar/cairo/1.18.2/include/cairo/cairo.h>

int main (int argc, char *argv[])
{
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 100, 100);
    cairo_t *cr = cairo_create (surface);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 100, 100);
    cairo_set_line_width (cr, 2);
    cairo_stroke (cr);
    cairo_destroy (cr);
    cairo_surface_write_to_png (surface, "hello.png");
    cairo_surface_destroy (surface);
    return 0;
}

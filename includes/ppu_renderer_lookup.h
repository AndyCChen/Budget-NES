#ifndef PPU_RENDERER_LOOKUP
#define PPU_RENDERER_LOOKUP

typedef void (*render_events_t) (void);

extern const render_events_t scanline_lookup[341];

#endif
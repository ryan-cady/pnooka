#include <pebble.h>

extern uint32_t MESSAGE_KEY_BG_COLOR;
extern uint32_t MESSAGE_KEY_FILLED_COLOR;
extern uint32_t MESSAGE_KEY_EMPTY_COLOR;
extern uint32_t MESSAGE_KEY_SHOW_DATE;

#define GRID_COLS 4
#define GRID_ROWS 3

#define PERSIST_KEY_BG        1
#define PERSIST_KEY_FILLED    2
#define PERSIST_KEY_EMPTY     3
#define PERSIST_KEY_SHOW_DATE 4

#ifdef PBL_BW
#define DEFAULT_BG_ARGB     0xFF  // white
#define DEFAULT_FILLED_ARGB 0xC0  // black
#define DEFAULT_EMPTY_ARGB  0xEA  // light gray (dithered on B&W display)
#else
#define DEFAULT_BG_ARGB     0xFF
#define DEFAULT_FILLED_ARGB 0xD5
#define DEFAULT_EMPTY_ARGB  0xEA
#endif

static Window *s_window;
static Layer  *s_canvas_layer;

static int     s_hours, s_minutes;
static uint8_t s_is_pm;
static uint8_t s_bg_argb, s_filled_argb, s_empty_argb;
static uint8_t s_show_date;

static uint8_t rgb24_to_argb8(int32_t val) {
    uint8_t r = (val >> 16) & 0xFF;
    uint8_t g = (val >> 8)  & 0xFF;
    uint8_t b =  val        & 0xFF;
    return (uint8_t)((3 << 6) | ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6));
}

static GColor contrasting_color(uint8_t bg_argb) {
    int r = (bg_argb >> 4) & 0x3;
    int g = (bg_argb >> 2) & 0x3;
    int b =  bg_argb       & 0x3;
    return (r * 30 + g * 59 + b * 11 >= 150) ? GColorBlack : GColorWhite;
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;

    GColor bg     = (GColor){.argb = s_bg_argb};
    GColor filled = (GColor){.argb = s_filled_argb};
    GColor empty  = (GColor){.argb = s_empty_argb};

    int radius  = (w >= 180) ? 13 : 8;
    int spacing = (w >= 180) ? 48 : 32;
    int bar_h   = (w >= 180) ? 18 : 14;
    int bar_gap = spacing - 2 * radius;  // same visual gap as between dots
    int font_h  = (w >= 180) ? 22 : 18;

    int grid_w = (GRID_COLS - 1) * spacing + radius * 2;
    int margin = (w - grid_w) / 2 - 1;
    int grid_x = margin + radius;

    // Segmented bar geometry: 1px segment + 1px gap on Basalt, 2px + 1px on Emery
    int seg_w  = (w >= 180) ? 2 : 1;
    int seg_sp = seg_w + 1;        // stride per segment (visible + 1px gap)
    int bar_w  = 60 * seg_sp;      // total bar width
    int bm     = (w - bar_w) / 2 - 1;  // bar margin, same centering offset as dots

    // Center all content vertically: dots + bar + half-gap + text row
    int total_h = (GRID_ROWS - 1) * spacing + 2 * radius + bar_gap + bar_h + bar_gap / 2 + font_h;
    int grid_y  = (h - total_h) / 2 + radius + 2;

    // ── background ────────────────────────────────────────────────────────────
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // ── hours grid ────────────────────────────────────────────────────────────
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int hour = col * GRID_ROWS + row + 1;
            GPoint center = GPoint(grid_x + col * spacing, grid_y + row * spacing);
            graphics_context_set_fill_color(ctx, (hour <= s_hours) ? filled : empty);
            graphics_fill_circle(ctx, center, radius);
        }
    }

    // ── minutes bar: 60 segments, each separated by a 1px background gap ────
    int bar_y = grid_y + (GRID_ROWS - 1) * spacing + radius + bar_gap;

    for (int i = 0; i < 60; i++) {
        graphics_context_set_fill_color(ctx, (i < s_minutes) ? filled : empty);
        graphics_fill_rect(ctx, GRect(bm + i * seg_sp, bar_y, seg_w, bar_h), 0, GCornerNone);
    }

    // 10-minute tick dashes below the bar (including 0 and 60)
    graphics_context_set_fill_color(ctx, filled);
    for (int i = 0; i <= 60; i += 10) {
        int pos = (i == 60) ? 59 : i;
        graphics_fill_rect(ctx, GRect(bm + pos * seg_sp, bar_y + bar_h + 1, seg_w, 2), 0, GCornerNone);
    }

    // ── bottom row: date left, AM/PM right — spaced below bar by bar_gap ─────
    int bar_bottom = bar_y + bar_h;
    int text_y     = bar_bottom + bar_gap / 2 + 2;

    GFont info_font = (w >= 180) ? fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
                                 : fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

    graphics_context_set_text_color(ctx, contrasting_color(s_bg_argb));

    char buf[16];

    // Date — left-aligned with bar left edge
    if (s_show_date) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(buf, sizeof(buf), "%m/%d/%y", t);
        graphics_draw_text(ctx, buf, info_font,
                           GRect(bm, text_y, bar_w, font_h),
                           GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }

    // AM/PM indicator — right-aligned with bar right edge
    // Layout (right to left): [AM][●] [gap] [PM][●]  | bm+bar_w
    {
        int dot_r   = (w >= 180) ? 4 : 3;
        int lbl_gap = 3;  // px between label text and its dot
        int grp_gap = 5;  // px between AM group and PM group
        int x_right = bm + bar_w;
        int dot_cy  = text_y + font_h / 2;

        GSize am_sz = graphics_text_layout_get_content_size(
            "AM", info_font, GRect(0, 0, bar_w / 2, font_h),
            GTextOverflowModeWordWrap, GTextAlignmentLeft);
        GSize pm_sz = graphics_text_layout_get_content_size(
            "PM", info_font, GRect(0, 0, bar_w / 2, font_h),
            GTextOverflowModeWordWrap, GTextAlignmentLeft);

        int pm_dot_x = x_right - dot_r;
        int pm_lx    = pm_dot_x - dot_r - lbl_gap - pm_sz.w;
        int am_dot_x = pm_lx - grp_gap - dot_r;
        int am_lx    = am_dot_x - dot_r - lbl_gap - am_sz.w;

        graphics_draw_text(ctx, "AM", info_font,
            GRect(am_lx, text_y, am_sz.w + 2, font_h),
            GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
        graphics_context_set_fill_color(ctx, s_is_pm ? empty : filled);
        graphics_fill_circle(ctx, GPoint(am_dot_x, dot_cy), dot_r);

        graphics_draw_text(ctx, "PM", info_font,
            GRect(pm_lx, text_y, pm_sz.w + 2, font_h),
            GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
        graphics_context_set_fill_color(ctx, s_is_pm ? filled : empty);
        graphics_fill_circle(ctx, GPoint(pm_dot_x, dot_cy), dot_r);
    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *t;

    t = dict_find(iter, MESSAGE_KEY_BG_COLOR);
    if (t) { s_bg_argb = rgb24_to_argb8(t->value->int32); persist_write_int(PERSIST_KEY_BG, s_bg_argb); }

    t = dict_find(iter, MESSAGE_KEY_FILLED_COLOR);
    if (t) { s_filled_argb = rgb24_to_argb8(t->value->int32); persist_write_int(PERSIST_KEY_FILLED, s_filled_argb); }

    t = dict_find(iter, MESSAGE_KEY_EMPTY_COLOR);
    if (t) { s_empty_argb = rgb24_to_argb8(t->value->int32); persist_write_int(PERSIST_KEY_EMPTY, s_empty_argb); }

    t = dict_find(iter, MESSAGE_KEY_SHOW_DATE);
    if (t) { s_show_date = (uint8_t)t->value->int32; persist_write_int(PERSIST_KEY_SHOW_DATE, s_show_date); }

    layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    s_hours   = tick_time->tm_hour % 12;
    if (s_hours == 0) s_hours = 12;
    s_minutes = tick_time->tm_min;
    s_is_pm   = (tick_time->tm_hour >= 12) ? 1 : 0;
    layer_mark_dirty(s_canvas_layer);
}

static void window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    s_canvas_layer = layer_create(layer_get_bounds(root));
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(root, s_canvas_layer);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    s_hours   = t->tm_hour % 12;
    if (s_hours == 0) s_hours = 12;
    s_minutes = t->tm_min;
    s_is_pm   = (t->tm_hour >= 12) ? 1 : 0;
}

static void window_unload(Window *window) {
    layer_destroy(s_canvas_layer);
}

static void init(void) {
    s_bg_argb     = persist_exists(PERSIST_KEY_BG)     ? (uint8_t)persist_read_int(PERSIST_KEY_BG)     : DEFAULT_BG_ARGB;
    s_filled_argb = persist_exists(PERSIST_KEY_FILLED) ? (uint8_t)persist_read_int(PERSIST_KEY_FILLED) : DEFAULT_FILLED_ARGB;
    s_empty_argb  = persist_exists(PERSIST_KEY_EMPTY)  ? (uint8_t)persist_read_int(PERSIST_KEY_EMPTY)  : DEFAULT_EMPTY_ARGB;

    s_show_date = persist_exists(PERSIST_KEY_SHOW_DATE) ? (uint8_t)persist_read_int(PERSIST_KEY_SHOW_DATE) : 1;

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
        .load   = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(128, 0);
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    window_destroy(s_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}

#include <pebble.h>

#define DOLLAR_OFFSET -39
#define CENT1_OFFSET 6
#define CENT10_OFFSET 16
#define CENT100_OFFSET 26
#define SUBTOTAL_HEADER_OFFSET -55
#define SUBTOTAL_AMOUNT_OFFSET -5
#define UP_ARROW_OFFSET -17
#define DOWN_ARROW_OFFSET 35
#define TIP_OFFSET -65
#define TIP_AMOUNT_OFFSET -45
#define TOTAL_HEADER_OFFSET -10
#define TOTAL_AMOUNT_OFFSET 15

static Window *subtotal_window, *tip_window;
static TextLayer *subtotal_header_layer, *subtotal_amount_layer, *tip_layer, *tip_amount_layer, *total_header_layer, *total_amount_layer;
static Layer *tip_circle_layer;
static GFont subtotal_header_font, subtotal_amount_font, tip_font, total_header_font, total_amount_font;
static BitmapLayer *up_arrow_layer, *down_arrow_layer;
static GBitmap *up_arrow, *down_arrow;

static char subtotal_buffer[10];
static char total_buffer[11];
static char tip_buffer[11];
static char tip_amount_buffer[10];

static int16_t arrow_x, window_width, window_height;
static int16_t dollar = 0;
static int8_t cent = 0;
static int16_t total_dollar = 0;
static int16_t total_cent = 0;
static int8_t tip = 15;
static int16_t tip_dollar = 0;
static int8_t tip_cent = 0;
static int8_t subtotal_field = 0;

static void update_tip_and_total();
static void update_subtotal();
static void update_arrows();

static void tip_back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_remove(tip_window, true);
}

static void tip_up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (tip == 99) {
    tip = 0;
  }
  else {
    tip += 1;
  }
  update_tip_and_total();
}

static void tip_down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (tip == 0) {
    tip = 99;
  }
  else {
    tip -= 1;
  }
  update_tip_and_total();
}

static void subtotal_back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (subtotal_field == 0) {
    window_stack_pop_all(true);
  }
  else if (subtotal_field == 1) {
    subtotal_field = 0;
    
    arrow_x = (window_width / 2) + DOLLAR_OFFSET;
    bitmap_layer_destroy(up_arrow_layer);
    bitmap_layer_destroy(down_arrow_layer);
    update_arrows();
  }
}

static void subtotal_select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (subtotal_field == 0) {
    subtotal_field = 1;
    if (dollar < 10) {
      arrow_x = (window_width / 2) + CENT1_OFFSET;
    }
    else if (dollar < 100) {
      arrow_x = (window_width / 2) + CENT10_OFFSET;
    }
    else {
      arrow_x = (window_width / 2) + CENT100_OFFSET;
    }
    
    // Destroy existing GBitmaps and Bitmap Layers
    bitmap_layer_destroy(up_arrow_layer);
    bitmap_layer_destroy(down_arrow_layer);
    update_arrows();
  }
  else if (subtotal_field == 1) {
    // Show the tip Window on the watch
    window_stack_push(tip_window, true);
  }
}

static void subtotal_up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (subtotal_field == 0) {
    if (dollar == 999) {
      dollar = 0;
    }
    else {
      dollar += 1;
    }
  }
  else if (subtotal_field == 1) {
    if (cent == 99) {
      cent = 0;
    }
    else {
      cent += 1;
    }
  }
  
  update_subtotal();
}

static void subtotal_down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (subtotal_field == 0) {
    if (dollar == 0) {
      dollar = 999;
    }
    else {
      dollar -= 1;
    }
  }
  else if (subtotal_field == 1) {
    if (cent == 0) {
      cent = 99;
    }
    else {
      cent -= 1;
    }
  }
  
  update_subtotal();
}

static void tip_config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_BACK, tip_back_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, tip_up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, tip_down_single_click_handler);
}

static void subtotal_config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_BACK, subtotal_back_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, subtotal_select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, subtotal_up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, subtotal_down_single_click_handler);
}

static void draw_tip_circle_layer(Layer *layer, GContext *ctx) {
  int16_t tip_radius_outer = 80;
  int16_t tip_radius_inner = 70;
  int8_t tip_angle_start = 0;
  int32_t tip_angle_end = (TRIG_MAX_ANGLE * tip) / 100;

  GPoint tip_center = {
    .x = window_width / 2,
    .y = window_height / 2
  };

  graphics_context_set_fill_color(ctx, GColorDarkGreen);
  graphics_fill_radial(ctx, tip_center, tip_radius_inner, tip_radius_outer, tip_angle_start, tip_angle_end);
}

static void update_tip_and_total() {
  snprintf(tip_buffer, sizeof(tip_buffer), "TIP  %d%%", tip);
  text_layer_set_text(tip_layer, tip_buffer);
  layer_mark_dirty(tip_circle_layer);

  tip_dollar = (dollar * tip) / 100;

  tip_cent = (dollar * tip) % 100;
  if ((cent * tip) % 100 >= 50) {
    tip_cent += 1; // Third digit in cents is 5 or over, round up
  }
  if (tip_cent >= 100) {
    tip_dollar += tip_cent / 100; // Find how many dollars to add depending on how many hundred cents
    tip_cent = tip_cent % 100; // Remove the hundreds to leave the cents
  }
  total_cent = ((cent * tip) / 100) + tip_cent + cent;

  total_dollar = tip_dollar + dollar;
  if (total_cent >= 100) {
    total_dollar += total_cent / 100; // Find how many dollars to add depending on how many hundred cents
    total_cent = total_cent % 100; // Remove the hundreds to leave the cents
  }
  snprintf(tip_amount_buffer, sizeof(tip_amount_buffer), "%d.%02d", tip_dollar, tip_cent);
  text_layer_set_text(tip_amount_layer, tip_amount_buffer);

  snprintf(total_buffer, sizeof(total_buffer), "%d.%02d", total_dollar, total_cent);
  text_layer_set_text(total_amount_layer, total_buffer);
}

static void update_subtotal() {
  snprintf(subtotal_buffer, sizeof(subtotal_buffer), "%d.%02d", dollar, cent);
  text_layer_set_text(subtotal_amount_layer, subtotal_buffer);
}

static void update_arrows() {
  // Get the root layer
  Layer *subtotal_window_layer = window_get_root_layer(subtotal_window);

  uint16_t up_arrow_y = (window_height / 2) + UP_ARROW_OFFSET;
  uint16_t down_arrow_y = (window_height / 2) + DOWN_ARROW_OFFSET;

  up_arrow_layer = bitmap_layer_create(GRect(arrow_x, up_arrow_y, 20, 20));
  down_arrow_layer = bitmap_layer_create(GRect(arrow_x, down_arrow_y, 20, 20));

  bitmap_layer_set_background_color(up_arrow_layer, GColorClear);
  bitmap_layer_set_background_color(down_arrow_layer, GColorClear);
  bitmap_layer_set_bitmap(up_arrow_layer, up_arrow);
  bitmap_layer_set_bitmap(down_arrow_layer, down_arrow);
  layer_add_child(subtotal_window_layer, bitmap_layer_get_layer(up_arrow_layer));
  layer_add_child(subtotal_window_layer, bitmap_layer_get_layer(down_arrow_layer));
}

static void tip_window_load(Window *window) {
  // Get the root layer
  Layer *tip_window_layer = window_get_root_layer(window);
  
  int16_t tip_y = (window_height / 2) + TIP_OFFSET;
  int16_t tip_amount_y = (window_height / 2) + TIP_AMOUNT_OFFSET;
  int16_t tip_height = 40;
  int16_t total_header_y = (window_height / 2) + TOTAL_HEADER_OFFSET;
  int16_t total_header_height = 40;
  int16_t total_amount_y = (window_height / 2) + TOTAL_AMOUNT_OFFSET;
  int16_t total_amount_height = 40;
  
  // Create tip TextLayer
  tip_layer = text_layer_create(GRect(0, tip_y, window_width, tip_height));
  text_layer_set_background_color(tip_layer, GColorClear);
  text_layer_set_text_color(tip_layer, GColorBlack);
  
  // Apply tip to TextLayer
  tip_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  text_layer_set_font(tip_layer, tip_font);
  text_layer_set_text_alignment(tip_layer, GTextAlignmentCenter);
  layer_add_child(tip_window_layer, text_layer_get_layer(tip_layer));
  
  // Create tip amount TextLayer
  tip_amount_layer = text_layer_create(GRect(0, tip_amount_y, window_width, tip_height));
  text_layer_set_background_color(tip_amount_layer, GColorClear);
  text_layer_set_text_color(tip_amount_layer, GColorBlack);
  
  // Apply tip amount to TextLayer
  text_layer_set_font(tip_amount_layer, tip_font);
  text_layer_set_text_alignment(tip_amount_layer, GTextAlignmentCenter);
  layer_add_child(tip_window_layer, text_layer_get_layer(tip_amount_layer));
  
  // Create total header TextLayer
  total_header_layer = text_layer_create(GRect(0, total_header_y, window_width, total_header_height));
  text_layer_set_background_color(total_header_layer, GColorClear);
  text_layer_set_text_color(total_header_layer, GColorBlack);
  
  // Apply total header to TextLayer
  total_header_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  text_layer_set_font(total_header_layer, total_header_font);
  text_layer_set_text(total_header_layer, "TOTAL");
  text_layer_set_text_alignment(total_header_layer, GTextAlignmentCenter);
  layer_add_child(tip_window_layer, text_layer_get_layer(total_header_layer));
  
  // Create total amount TextLayer
  total_amount_layer = text_layer_create(GRect(0, total_amount_y, window_width, total_amount_height));
  text_layer_set_background_color(total_amount_layer, GColorClear);
  text_layer_set_text_color(total_amount_layer, GColorBlack);
  
  // Apply total amount to TextLayer
  total_amount_font = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  text_layer_set_font(total_amount_layer, total_amount_font);
  text_layer_set_text_alignment(total_amount_layer, GTextAlignmentCenter);
  layer_add_child(tip_window_layer, text_layer_get_layer(total_amount_layer));
  
  // Create tip circle TextLayer
  tip_circle_layer = layer_create(GRect(0, 0, window_width, window_height));
  layer_set_update_proc(tip_circle_layer, draw_tip_circle_layer);
  layer_add_child(tip_window_layer, tip_circle_layer);
  
  // Sets the tip and total
  update_tip_and_total();
}

static void tip_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(tip_layer);
  text_layer_destroy(total_header_layer);
  text_layer_destroy(total_amount_layer);

  // Destroy Layer
  layer_destroy(tip_circle_layer);
}

static void subtotal_window_load(Window *window) {
  // Get the root layer
  Layer *subtotal_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(subtotal_window_layer);
  window_width = bounds.size.w;
  window_height = bounds.size.h;

  int16_t subtotal_header_y = (window_height / 2) + SUBTOTAL_HEADER_OFFSET;
  int16_t subtotal_header_height = 40;
  int16_t subtotal_amount_y = (window_height / 2) + SUBTOTAL_AMOUNT_OFFSET;
  int16_t subtotal_amount_height = 50;
  
  // Create subtotal header TextLayer
  subtotal_header_layer = text_layer_create(GRect(0, subtotal_header_y, window_width, subtotal_header_height));
  text_layer_set_background_color(subtotal_header_layer, GColorClear);
  text_layer_set_text_color(subtotal_header_layer, GColorBlack);

  // Apply subtotal header to TextLayer
  subtotal_header_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  text_layer_set_font(subtotal_header_layer, subtotal_header_font);
  text_layer_set_text(subtotal_header_layer, "SUBTOTAL");
  text_layer_set_text_alignment(subtotal_header_layer, GTextAlignmentCenter);
  layer_add_child(subtotal_window_layer, text_layer_get_layer(subtotal_header_layer));

  // Create subtotal amount TextLayer
  subtotal_amount_layer = text_layer_create(GRect(0, subtotal_amount_y, window_width, subtotal_amount_height));
  text_layer_set_background_color(subtotal_amount_layer, GColorClear);
  text_layer_set_text_color(subtotal_amount_layer, GColorBlack);

  // Apply subtotal amount to TextLayer
  subtotal_amount_font = fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS);
  text_layer_set_font(subtotal_amount_layer, subtotal_amount_font);
  update_subtotal();
  text_layer_set_text_alignment(subtotal_amount_layer, GTextAlignmentCenter);
  layer_add_child(subtotal_window_layer, text_layer_get_layer(subtotal_amount_layer));

  // Create up and down arrow bitmaps
  up_arrow = gbitmap_create_with_resource(RESOURCE_ID_UP_IMAGE);
  down_arrow = gbitmap_create_with_resource(RESOURCE_ID_DOWN_IMAGE);

  arrow_x = (window_width / 2) + DOLLAR_OFFSET;
  update_arrows();
}

static void subtotal_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(subtotal_header_layer);
  text_layer_destroy(subtotal_amount_layer);

  // Destroy GBitmaps
  gbitmap_destroy(up_arrow);
  gbitmap_destroy(down_arrow);

  // Destroy BitmapLayers
  bitmap_layer_destroy(up_arrow_layer);
  bitmap_layer_destroy(down_arrow_layer);
}

static void init() {
  // Create Window elements and assign to pointers
  subtotal_window = window_create();
  tip_window = window_create();
  window_set_background_color(subtotal_window, GColorCadetBlue);
  window_set_background_color(tip_window, GColorIslamicGreen);

  // Set handlers to manage the elements inside the Windows
  window_set_window_handlers(subtotal_window, (WindowHandlers) {
    .load = subtotal_window_load,
    .unload = subtotal_window_unload
  });
  window_set_window_handlers(tip_window, (WindowHandlers) {
    .load = tip_window_load,
    .unload = tip_window_unload
  });

  // Set click config provider callback
  window_set_click_config_provider(subtotal_window, (ClickConfigProvider) subtotal_config_provider);
  window_set_click_config_provider(tip_window, (ClickConfigProvider) tip_config_provider);

  // Show the subtotal Window on the watch
  window_stack_push(subtotal_window, true);
}

static void deinit() {
  // Destroy Windows
  window_destroy(subtotal_window);
  window_destroy(tip_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

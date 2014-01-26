#include <pebble.h>

static Window *window;
static TextLayer *text_layer, *title_layer_1, *title_layer_2, *title_layer_3;
static ActionBarLayer *action_bar;
static AppTimer *app_timer;
static GBitmap *bmp_plus;
static GBitmap *bmp_select;
static GBitmap *bmp_minus;
static GFont custom_font;
static int engaged;
static uint32_t index;
static char interval_char[3]; // To hold a 2 digit number at max
static const uint32_t INC = 0;
static const uint32_t DEC = 1;
static const uint32_t MAX_INDEX = 6;
static const uint32_t interval_arr[7] = {1,2,5,10,15,30,60};

static void update_interval_char() {
  uint32_t interval = interval_arr[index];
  if (interval > 99) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Interval out of range: %u", (unsigned int)interval);
  } else if (interval < 10) { // 1 digit
    interval_char[0]=(char)(((int)'0')+interval);
    interval_char[1]='\0';
  } else { // 2 digits
    interval_char[0]=(char)(((int)'0')+(interval/10));
    interval_char[1]=(char)(((int)'0')+(interval%10));
    interval_char[2]='\0';
  }
}

static void vibe_callback() {
  vibes_long_pulse();
  app_timer = app_timer_register(interval_arr[index]*60000,vibe_callback,interval_char);
}

static void update_interval(uint32_t direction) {
  if (direction == INC) {
    if (index < MAX_INDEX) {
      ++index;
    } else {
      index = 0;
    }
  } else {
    if (index > 0) {
      --index;
    } else {
      index = MAX_INDEX;
    }
  }
  update_interval_char();
  text_layer_set_text(text_layer, interval_char);
  text_layer_set_text(title_layer_3, "Minutes");
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (engaged) {
    engaged = 0;
    app_timer_cancel(app_timer);
    psleep(10); // Wait and repeat in case cancel occurs exactly at register time
    app_timer_cancel(app_timer);
    text_layer_set_text(text_layer, "Vibe Off");
    text_layer_set_text(title_layer_3, "");
  } else {
    engaged = 1;
    app_timer = app_timer_register(interval_arr[index]*60000,vibe_callback,interval_char);
    text_layer_set_text(text_layer, "Vibe On");
    text_layer_set_text(title_layer_3, "");
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_interval(INC);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  update_interval(DEC);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void create_action_bar(Window *window) {
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar,
                                             click_config_provider);
  // Set the icons:
  bmp_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLUS);
  bmp_select = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SELECT);
  bmp_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MINUS);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, bmp_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, bmp_select);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, bmp_minus);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Title text layer
  title_layer_1 = text_layer_create((GRect) { .origin = { 0, 24 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(title_layer_1, "Set Desired");
  text_layer_set_text_alignment(title_layer_1, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer_1));
  title_layer_2 = text_layer_create((GRect) { .origin = { 0, 40 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(title_layer_2, "Interval");
  text_layer_set_text_alignment(title_layer_2, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer_2));

  // Interval text layer
  text_layer = text_layer_create((GRect) { .origin = { 0, 60 }, .size = { bounds.size.w, 60 } });
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, custom_font);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  //"Minutes"
  title_layer_3 = text_layer_create((GRect) { .origin = { 0, 108 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(title_layer_3, "Minutes");
  text_layer_set_text_alignment(title_layer_3, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer_3));
  create_action_bar(window);

  update_interval(INC); // This sets the text to index+1 as the default value
}

static void window_unload(Window *window) {
  text_layer_destroy(title_layer_1);
  text_layer_destroy(title_layer_2);
  text_layer_destroy(title_layer_3);
  text_layer_destroy(text_layer);
  gbitmap_destroy(bmp_plus);
  gbitmap_destroy(bmp_select);
  gbitmap_destroy(bmp_minus);
}

static void init(void) {
  window = window_create();
  index = 1;
  engaged = 0;
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OSP_DIN_44));
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

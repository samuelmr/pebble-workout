#include <pebble.h>
static Window *window;
static TextLayer *text_layer;
static AppTimer *timer;
static const uint16_t timer_interval_ms = 1000;
static const uint32_t WORK_KEY = 1 << 1;
static const uint32_t REST_KEY = 1 << 2;
static const uint32_t REPEAT_KEY = 1 << 3;
int seconds;
int default_work;
int default_rest;
int default_repeat;
char *time_key;
int working;
int resting;
int paused;
int repeats;
GColor work_bg;
GColor work_text;
GColor rest_bg;
GColor rest_text;

static void show_time(void) {
  static char body_text[9];
  snprintf(body_text, sizeof(body_text), "%02d", seconds);
  text_layer_set_text(text_layer, body_text);
  if (working || resting) {
    if (seconds == 0) {
      vibes_long_pulse();
    }
    else if ((seconds % 30) == 0) {
      vibes_double_pulse();
    }
    if ((seconds > 0) && (seconds <= 5)) {
      vibes_short_pulse();
    }
  }
}

static void timer_callback(void *data) {

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %02d", seconds);

  seconds--;
  if (seconds <= 0) {
    // if rounds > repeat
    // app_timer_cancel(timer);
    if (working) {
      text_layer_set_background_color(text_layer, rest_bg);
      text_layer_set_text_color(text_layer, rest_text);
      working = 0;
      resting = 1;
      seconds = default_rest;
    }
    else {
      text_layer_set_background_color(text_layer, work_bg);
      text_layer_set_text_color(text_layer, work_text);
      working = 1;
      resting = 0;
      seconds = default_work;
    }
  }
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  show_time();
}

static void reset(void) {
  if (timer) {
    app_timer_cancel(timer);
  }
  working = 0;
  resting = 0;
  paused = 0;
  seconds = default_work;
  show_time();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (paused || (seconds < 0) || ((working == 0) && (resting == 0))) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Start: %02d", seconds);
    working = 1;
    resting = 0;
    paused = 0;
    seconds--;
    show_time();
    timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pause: %02d", seconds);
    paused = 1;
    app_timer_cancel(timer);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Restart: %02d", seconds);
  vibes_short_pulse();
  app_timer_cancel(timer);
  seconds = default_work;
  working = 1;
  resting = 0;
  paused = 0;
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  show_time();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Stop: %02d", seconds);
  vibes_short_pulse();
  reset();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

/*
void in_received_handler(DictionaryIterator *received, void *context) {

  Tuple *msg_type = dict_read_first(received);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message from phone: %s", msg_type->value->cstring);
  if (strcmp(msg_type->value->cstring, time_key) == 0) {
    Tuple *hrs = dict_find(received, 1);
    default_hours = hrs->value->int8;
    Tuple *mins = dict_find(received, 2);
    default_minutes = mins->value->int8;
    Tuple *secs = dict_find(received, 3);
    default_seconds = secs->value->int8;
	reset();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "New config: %d:%02d:%02d", hours, minutes, seconds);
  }
  else {
    Tuple *val = dict_read_next(received);
    if (strcmp(msg_type->value->cstring, long_key) == 0) {
      strcpy(default_long_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set long vibes to: %s", default_long_vibes);
    }
    else if (strcmp(msg_type->value->cstring, single_key) == 0) {
      strcpy(default_single_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set single vibes to: %s", default_single_vibes);
    }
    else if (strcmp(msg_type->value->cstring, double_key) == 0) {
      strcpy(default_double_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set double vibes to: %s", default_double_vibes);
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message from phone dropped: %d", reason);
}
*/

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  text_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(text_layer, work_bg);
  text_layer_set_text_color(text_layer, work_text);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  reset();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
/*
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 128;
  const uint32_t outbound_size = 128;
  app_message_open(inbound_size, outbound_size);
*/

#ifdef PBL_COLOR
  rest_bg = GColorDukeBlue;
  rest_text = GColorDukeBlue;
  work_bg = GColorDukeBlue;
  work_text = GColorDukeBlue;
#else
  rest_bg = GColorWhite;
  rest_text = GColorBlack;
  work_bg = GColorBlack;
  work_text = GColorWhite;
#endif

  working = 0;
  resting = 0;
  repeats = 0;
  // default_work = persist_exists(WORK_KEY) ? persist_read_int(WORK_KEY) : 90;
  default_work = 90;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised work to: %d", default_work);
  // default_rest = persist_exists(REST_KEY) ? persist_read_int(REST_KEY) : 30;
  default_rest = 30;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised rest to: %d", default_rest);
  default_repeat = persist_exists(REPEAT_KEY) ? persist_read_int(REPEAT_KEY) : 4;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised repeat to %d times", default_repeat);

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  persist_write_int(WORK_KEY, default_work);
  persist_write_int(REST_KEY, default_rest);
  persist_write_int(REPEAT_KEY, default_repeat);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

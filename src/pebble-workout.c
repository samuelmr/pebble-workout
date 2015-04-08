#include <pebble.h>
static Window *window;
static TextLayer *routine_layer;
static TextLayer *time_layer;
static TextLayer *lap_layer;
static TextLayer *next_layer;
static AppTimer *timer;
static const uint16_t timer_interval_ms = 1000;
static const uint32_t WORK_KEY = 1 << 1;
static const uint32_t REST_KEY = 1 << 2;
static const uint32_t REPEAT_KEY = 1 << 3;
static const uint32_t ROUTINES_KEY = 1 << 4;
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

char routine[20][20];
int routines;
int current_routine;
int lap;
static const char empty[2];

static void show_time(void) {
  static char time_text[4];
  snprintf(time_text, sizeof(time_text), "%02d", seconds);
  text_layer_set_text(time_layer, time_text);
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

static void update_lap_text(void) {
  static char lap_text[20];
  // break into two text fields?
  if (lap) {
    snprintf(lap_text, sizeof(lap_text), "%d/%d     Lap %d", current_routine+1, routines, lap);
    text_layer_set_text(lap_layer, lap_text);
  }
  else {
    text_layer_set_text(lap_layer, empty);
  }
}

static void update_next_text(int index) {
  static char next_text[20];
  snprintf(next_text, sizeof(next_text), "Next: %s", routine[index]);
  text_layer_set_text(next_layer, next_text);
}

static void timer_callback(void *data) {

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %02d", seconds);

  seconds--;
  if (seconds <= 0) {
    if (working) {
      text_layer_set_background_color(time_layer, rest_bg);
      text_layer_set_text_color(time_layer, rest_text);
      text_layer_set_background_color(routine_layer, rest_bg);
      text_layer_set_text_color(routine_layer, rest_text);
      text_layer_set_background_color(lap_layer, rest_bg);
      text_layer_set_text_color(lap_layer, rest_text);
      text_layer_set_background_color(next_layer, rest_bg);
      text_layer_set_text_color(next_layer, rest_text);
      working = 0;
      resting = 1;
      seconds = default_rest;
      text_layer_set_text(routine_layer, "Rest");
      int next_routine = current_routine + 1;
      if (next_routine >= routines) {
        next_routine = 0;
      }
      update_next_text(next_routine);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Changed mode to Rest, next %s", routine[next_routine]);
    }
    else {
      text_layer_set_background_color(time_layer, work_bg);
      text_layer_set_text_color(time_layer, work_text);
      text_layer_set_background_color(routine_layer, work_bg);
      text_layer_set_text_color(routine_layer, work_text);
      text_layer_set_background_color(lap_layer, work_bg);
      text_layer_set_text_color(lap_layer, work_text);
      text_layer_set_background_color(next_layer, work_bg);
      text_layer_set_text_color(next_layer, work_text);
      working = 1;
      resting = 0;
      seconds = default_work;
      current_routine++;
      if (current_routine >= routines) {
        lap++;
        current_routine = 0;
      }
      update_lap_text();
      text_layer_set_text(routine_layer, routine[current_routine]);
      text_layer_set_text(next_layer, empty);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Started %s", routine[current_routine]);
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
  current_routine = 0;
  lap = 0;
  seconds = default_work;
  text_layer_set_background_color(time_layer, work_bg);
  text_layer_set_text_color(time_layer, work_text);
  text_layer_set_background_color(routine_layer, work_bg);
  text_layer_set_text_color(routine_layer, work_text);
  text_layer_set_background_color(lap_layer, work_bg);
  text_layer_set_text_color(lap_layer, work_text);
  text_layer_set_background_color(next_layer, work_bg);
  text_layer_set_text_color(next_layer, work_text);
  text_layer_set_text(routine_layer, empty);
  show_time();
  update_next_text(current_routine);
  update_lap_text();
}

static void start_or_pause(ClickRecognizerRef recognizer, void *context) {
  if (paused || (seconds < 0) || ((working == 0) && (resting == 0))) {
    if (lap == 0) {
      lap = 1;
      update_lap_text();
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Start: %02d", seconds);
    working = 1;
    resting = 0;
    paused = 0;
    seconds--;
    show_time();
    text_layer_set_text(next_layer, empty);
    text_layer_set_text(routine_layer, routine[current_routine]);
    if (timer) {
      app_timer_cancel(timer);
    }
    timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Pause: %02d", seconds);
    paused = 1;
    if (timer) {
      app_timer_cancel(timer);
    }
    text_layer_set_text(next_layer, "SEL to continue");
  }
}
static void stop(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Stop: %02d", seconds);
  vibes_short_pulse();
  reset();
}
static void stop_message(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Reset... %02d", seconds);
  text_layer_set_text(next_layer, "Reset");
}

static void skip_back(ClickRecognizerRef recognizer, void *context) {
  vibes_short_pulse();
  if (timer) {
    app_timer_cancel(timer);
  }
  if ((seconds == default_work) && ((lap > 1) || (current_routine > 1))) {
    current_routine--;
    if (current_routine < 0) {
      current_routine = routines - 1;
      lap--;
    }
  }
  seconds = default_work;
  working = 0;
  resting = 0;
  paused = 0;
  text_layer_set_text(routine_layer, empty);
  update_next_text(current_routine);
  show_time();
  update_lap_text();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skip back to lap %d, routine %d/%d", lap, current_routine, routines);
}
static void skip_back_message(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skipping back... %02d", seconds);
  text_layer_set_text(next_layer, "Skip back");
}

static void skip(ClickRecognizerRef recognizer, void *context) {
  vibes_short_pulse();
  if (timer) {
    app_timer_cancel(timer);
  }
  current_routine++;
  if (current_routine >= routines) {
    current_routine = 0;
    lap++;
  }
  seconds = default_work;
  working = 0;
  resting = 0;
  paused = 0;
  text_layer_set_text(routine_layer, empty);
  update_next_text(current_routine);
  show_time();
  update_lap_text();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skip to lap %d, routine %d/%d", lap, current_routine, routines);
}
static void skip_message(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skipping...");
  text_layer_set_text(next_layer, "Skip routine");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, start_or_pause);
  window_long_click_subscribe(BUTTON_ID_UP, 0, skip_back_message, skip_back);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, skip_message, skip);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, stop_message, stop);
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

  lap_layer = text_layer_create(GRect(0, 0, bounds.size.w, 30));
  text_layer_set_font(lap_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(lap_layer, GTextAlignmentCenter);
  text_layer_set_background_color(lap_layer, rest_bg);
  text_layer_set_text_color(lap_layer, rest_text);
  layer_add_child(window_layer, text_layer_get_layer(lap_layer));

  time_layer = text_layer_create(GRect(0, 30, bounds.size.w, 80));
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(time_layer, rest_bg);
  text_layer_set_text_color(time_layer, rest_text);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  routine_layer = text_layer_create(GRect(0, bounds.size.h-60, bounds.size.w, 30));
  text_layer_set_font(routine_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(routine_layer, GTextAlignmentCenter);
  text_layer_set_background_color(routine_layer, rest_bg);
  text_layer_set_text_color(routine_layer, rest_text);
  layer_add_child(window_layer, text_layer_get_layer(routine_layer));

  next_layer = text_layer_create(GRect(0, bounds.size.h-30, bounds.size.w, 30));
  text_layer_set_font(next_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(next_layer, GTextAlignmentCenter);
  text_layer_set_background_color(next_layer, rest_bg);
  text_layer_set_text_color(next_layer, rest_text);
  layer_add_child(window_layer, text_layer_get_layer(next_layer));

  reset();
}

static void window_unload(Window *window) {
  text_layer_destroy(time_layer);
  text_layer_destroy(routine_layer);
  text_layer_destroy(next_layer);
  text_layer_destroy(lap_layer);
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
  rest_text = GColorWhite;
  work_bg = GColorOrange;
  work_text = GColorWhite;
#else
  rest_bg = GColorWhite;
  rest_text = GColorBlack;
  work_bg = GColorBlack;
  work_text = GColorWhite;
#endif

  working = 0;
  resting = 0;
  repeats = 0;
  current_routine = 0;
  lap = 0;
  default_work = persist_exists(WORK_KEY) ? persist_read_int(WORK_KEY) : 90;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised work to: %d", default_work);
  default_rest = persist_exists(REST_KEY) ? persist_read_int(REST_KEY) : 30;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised rest to: %d", default_rest);
  default_repeat = persist_exists(REPEAT_KEY) ? persist_read_int(REPEAT_KEY) : 4;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised repeat to %d times", default_repeat);
  routines = persist_exists(ROUTINES_KEY) ? persist_read_int(ROUTINES_KEY) : 4;
  strcpy(routine[0], "Push-ups");
  strcpy(routine[1], "Sit-ups");
  strcpy(routine[2], "Lunges");
  strcpy(routine[3], "Pull-ups");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised %d routines", routines);

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

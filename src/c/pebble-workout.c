#include <pebble.h>
#define MAX_EXERCISES 50
#define EXERCISE_LENGTH 20
static Window *window;
static TextLayer *exercise_layer;
static TextLayer *time_layer;
static TextLayer *lap_layer;
static TextLayer *next_layer;
static AppTimer *timer;
static const uint16_t timer_interval_ms = 1000;
#ifdef PBL_HEALTH
static TextLayer *hr_layer;
#endif
  
enum MessageKey {
  WORK = 0,      // TUPLE_INT
  REST = 1,      // TUPLE_INT
  REPEAT = 2,    // TUPLE_INT
  EXERCISES = 3,  // TUPLE_INT
};

static int seconds;
static int default_work;
static int default_rest;
static int default_repeat;
static char *time_key;
static const char work[5] = "work";
static const char rest[5] = "rest";
static int working;
static int resting;
static int paused;
static int repeats;
static GColor work_bg;
static GColor work_text;
static GColor rest_bg;
static GColor rest_text;

static char exercise[MAX_EXERCISES][EXERCISE_LENGTH];
static int exercises;
static int current_exercise;
static int lap;
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
    snprintf(lap_text, sizeof(lap_text), "%d/%d     Lap %d", current_exercise+1, exercises, lap);
    text_layer_set_text(lap_layer, lap_text);
  }
  else {
    text_layer_set_text(lap_layer, empty);
  }
}

static void update_next_text(int index) {
  static char next_text[20];
  snprintf(next_text, sizeof(next_text), "Next: %s", exercise[index]);
  text_layer_set_text(next_layer, next_text);
}

static void set_colors(const char *mode) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting color scheme to %s", mode);
  if (strcmp(mode, work) == 0) {
    window_set_background_color(window, work_bg);
    text_layer_set_background_color(time_layer, work_bg);
    text_layer_set_text_color(time_layer, work_text);
    text_layer_set_background_color(exercise_layer, work_bg);
    text_layer_set_text_color(exercise_layer, work_text);
    text_layer_set_background_color(lap_layer, work_bg);
    text_layer_set_text_color(lap_layer, work_text);
    text_layer_set_background_color(next_layer, work_bg);
    text_layer_set_text_color(next_layer, work_text);
#ifdef PBL_HEALTH
    text_layer_set_background_color(hr_layer, work_bg);
    text_layer_set_text_color(hr_layer, work_text);
#endif    
  }
  else {
    window_set_background_color(window, rest_bg);
    text_layer_set_background_color(time_layer, rest_bg);
    text_layer_set_text_color(time_layer, rest_text);
    text_layer_set_background_color(exercise_layer, rest_bg);
    text_layer_set_text_color(exercise_layer, rest_text);
    text_layer_set_background_color(lap_layer, rest_bg);
    text_layer_set_text_color(lap_layer, rest_text);
    text_layer_set_background_color(next_layer, rest_bg);
    text_layer_set_text_color(next_layer, rest_text);
#ifdef PBL_HEALTH
    text_layer_set_background_color(hr_layer, rest_bg);
    text_layer_set_text_color(hr_layer, rest_text);
#endif    
  }
}

static void timer_callback(void *data) {

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %02d", seconds);

  seconds--;
  if (seconds <= 0) {
    if (working) {
      set_colors(rest);
      working = 0;
      resting = 1;
      seconds = default_rest;
      text_layer_set_text(exercise_layer, "Rest");
      int next_exercise = current_exercise + 1;
      if (next_exercise >= exercises) {
        next_exercise = 0;
      }
      update_next_text(next_exercise);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Changed mode to Rest, next %s", exercise[next_exercise]);
    }
    else {
      set_colors(work);
      working = 1;
      resting = 0;
      seconds = default_work;
      current_exercise++;
      if (current_exercise >= exercises) {
        lap++;
        current_exercise = 0;
      }
      update_lap_text();
      text_layer_set_text(exercise_layer, exercise[current_exercise]);
      text_layer_set_text(next_layer, empty);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Started %s", exercise[current_exercise]);
    }
  }
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  show_time();
}

static void reset(void) {
  if (timer) {
    app_timer_cancel(timer);
    timer = NULL;
  }
  working = 0;
  resting = 0;
  paused = 0;
  current_exercise = 0;
  lap = 0;
  seconds = default_work;
  set_colors(rest);
  text_layer_set_text(exercise_layer, empty);
  show_time();
  update_next_text(current_exercise);
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
    set_colors(work);
    text_layer_set_text(next_layer, empty);
    text_layer_set_text(exercise_layer, exercise[current_exercise]);
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
      timer = NULL;
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
    timer = NULL;
  }
  if ((seconds == default_work) && ((lap > 1) || (current_exercise > 1))) {
    current_exercise--;
    if (current_exercise < 0) {
      current_exercise = exercises - 1;
      lap--;
    }
  }
  seconds = default_work;
  working = 0;
  resting = 0;
  paused = 0;
  set_colors(rest);
  text_layer_set_text(exercise_layer, empty);
  update_next_text(current_exercise);
  show_time();
  update_lap_text();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skip back to lap %d, exercise %d/%d", lap, current_exercise, exercises);
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
  current_exercise++;
  if (current_exercise >= exercises) {
    current_exercise = 0;
    lap++;
  }
  seconds = default_work;
  working = 0;
  resting = 0;
  paused = 0;
  set_colors(rest);
  text_layer_set_text(exercise_layer, empty);
  update_next_text(current_exercise);
  show_time();
  update_lap_text();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skip to lap %d, exercise %d/%d", lap, current_exercise, exercises);
}
static void skip_message(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Skipping...");
  text_layer_set_text(next_layer, "Skip exercise");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, start_or_pause);
  window_long_click_subscribe(BUTTON_ID_UP, 0, skip_back_message, skip_back);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, skip_message, skip);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, stop_message, stop);
}

void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *wt = dict_find(received, WORK);
  default_work = wt->value->int16;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured work to: %d", default_work);

  Tuple *rt = dict_find(received, REST);
  default_rest = rt->value->int16;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured rest to: %d", default_rest);

  Tuple *et = dict_find(received, REPEAT);
  default_repeat = et->value->int16;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured repeat to %d times", default_repeat);

  Tuple *ct = dict_find(received, EXERCISES);
  exercises = (ct->value->int8 < MAX_EXERCISES) ? ct->value->int8 : MAX_EXERCISES;

  for (int i=0; i<exercises; i++) {
    if (i > MAX_EXERCISES) {
      break;
    }
    Tuple *ro = dict_find(received, 1 + EXERCISES + i);
    strcpy(exercise[i], ro->value->cstring);
  };
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Configured %d exercises", exercises);
  reset();
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message from phone dropped: %d", reason);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  int top_margin = (bounds.size.h - 160)/2;

#ifdef PBL_HEALTH
  hr_layer = text_layer_create(GRect(0, top_margin, bounds.size.w, 30));
  text_layer_set_font(hr_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(hr_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(hr_layer));
#endif

  lap_layer = text_layer_create(GRect(0, top_margin+30, bounds.size.w, 20));
  text_layer_set_font(lap_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(lap_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(lap_layer));

  time_layer = text_layer_create(GRect(0, top_margin+50, bounds.size.w, 50));
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  exercise_layer = text_layer_create(GRect(0, top_margin+100, bounds.size.w, 30));
  text_layer_set_font(exercise_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(exercise_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(exercise_layer));

  next_layer = text_layer_create(GRect(0, top_margin+130, bounds.size.w, bounds.size.h-(top_margin+130)));
  text_layer_set_font(next_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(next_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(next_layer));

  reset();
}

static void window_unload(Window *window) {
  text_layer_destroy(time_layer);
  text_layer_destroy(exercise_layer);
  text_layer_destroy(next_layer);
  text_layer_destroy(lap_layer);
}

#ifdef PBL_HEALTH
static void prv_on_health_data(HealthEventType type, void *context) {
  if (type == HealthEventHeartRateUpdate) {
    HealthValue heart_rate = health_service_peek_current_value(HealthMetricHeartRateBPM);
    static char s_hrm_buffer[8];
    snprintf(s_hrm_buffer, sizeof(s_hrm_buffer), "%lu BPM", (uint32_t) heart_rate);
    text_layer_set_text(hr_layer, s_hrm_buffer);
  }
}
#endif

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  // app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(1024, 32);

#ifdef PBL_COLOR
  rest_bg = GColorDukeBlue;
  rest_text = GColorWhite;
  work_bg = GColorOrange;
  work_text = GColorBlack;
#else
  rest_bg = GColorWhite;
  rest_text = GColorBlack;
  work_bg = GColorBlack;
  work_text = GColorWhite;
#endif

#ifdef PBL_HEALTH
  health_service_events_subscribe(prv_on_health_data, NULL);
#endif
  
  working = 0;
  resting = 0;
  repeats = 0;
  current_exercise = 0;
  lap = 0;
  default_work = 90;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised work to: %d", default_work);
  default_rest = 30;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised rest to: %d", default_rest);
  default_repeat = 4;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised repeat to %d times", default_repeat);
  exercises = 4;
  strcpy(exercise[0], "Push-ups");
  strcpy(exercise[1], "Sit-ups");
  strcpy(exercise[2], "Lunges");
  strcpy(exercise[3], "Pull-ups");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised %d exercises", exercises);

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void prv_update_app_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
  static char message[80];
  time_t t = time(NULL);
  struct tm *tms;
  tms = localtime(&t);
  strftime(message, sizeof(message), "Last used %b %d %H:%M", tms);
  const AppGlanceSlice slice = (AppGlanceSlice) {
    .layout = {
      .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string = message
    },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION
  };
  AppGlanceResult result = app_glance_add_slice(session, slice);
  if (result != APP_GLANCE_RESULT_SUCCESS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error adding AppGlanceSlice: %d", result);
  }
}

static void deinit(void) {
  app_glance_reload(prv_update_app_glance, NULL);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

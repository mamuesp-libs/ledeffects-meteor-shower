#include "mgos.h"
#include "led_master.h"

typedef struct {
    uint8_t meteor_size;
    double meteor_trail_decay;
    bool meteor_random_decay;
    tools_rgb_data color;
    tools_rgb_data out_pix;
} meteor_data;

static meteor_data* md_pool = NULL;
static tools_rgb_data color;
static tools_rgb_data out_pix;
static int loop = 0;

static meteor_data *mgos_internal_meteor_shower_init(mgos_rgbleds* leds)
{
    leds->timeout =  mgos_sys_config_get_ledeffects_meteor_shower_timeout();
    leds->dim_all =  mgos_sys_config_get_ledeffects_meteor_shower_dim_all();

    meteor_data *new_md_pool = calloc(leds->panel_width, sizeof(meteor_data));
    for (int col = 0; col < leds->panel_width; col++) {
        meteor_data* curr_md = &new_md_pool[col];
        //memset(md, 0, sizeof(meteor_data));
        //tools_config_get_color("ledmaster.anim.%s.color", "meteor_shower", &(curr_md->color));
        //md->color = tools_get_random_color(curr_md->color, &curr_md->color, 1, 0.1);
        curr_md->out_pix = curr_md->color;
        curr_md->meteor_size = mgos_sys_config_get_ledeffects_meteor_shower_size();
        curr_md->meteor_trail_decay = mgos_sys_config_get_ledeffects_meteor_shower_trail_decay();
        curr_md->meteor_random_decay = mgos_sys_config_get_ledeffects_meteor_shower_random_decay();
    }
    mgos_universal_led_clear(leds);

    return new_md_pool;
}

static void *mgos_internal_meteor_shower_exit(mgos_rgbleds* leds, meteor_data* curr_md)
{
    if (curr_md != NULL) {
        free(curr_md);
    }

    return NULL;
}

static void mgos_internal_fade_to_black(mgos_rgbleds* leds, int led_x, int led_y, double fade_value)
{
    tools_rgb_data old_color = mgos_universal_led_get_from_pos(leds, led_x, led_y, NULL, 0);
    tools_rgb_data new_color = old_color;

    double h, s, v;

    tools_rgb_to_hsv(old_color, &h, &s, &v);
    v = v * fade_value;
    new_color = tools_hsv_to_rgb(h, s, v);

    mgos_universal_led_plot_pixel(leds, led_x, leds->panel_height - 1 - led_y, new_color, false);
}

static void mgos_internal_meteor_shower_loop(mgos_rgbleds* leds)
{
    uint32_t num_rows = leds->panel_height;
    loop = (loop + 1) % ((2 * num_rows) + 1);

    for (int x = 0; x < leds->panel_width; x++) {

        meteor_data* curr_md = &md_pool[x];

        if (loop == 0) {
            color = tools_get_random_color_fade(curr_md->color, &(curr_md->color), 1, 0.1, 1.0, 0.6);
            out_pix = color;
        }
        
        curr_md->color = color;
        curr_md->out_pix = out_pix;

        // fade brightness all LEDs one step
        for (int y = 0; y < num_rows; y++) {
            int rand = tools_get_random(0, 10);
            if ((!curr_md->meteor_random_decay) || rand > 5) {
                mgos_internal_fade_to_black(leds, x, y, curr_md->meteor_trail_decay);
            }
        }

        // draw meteor
        for (int j = 0; j < curr_md->meteor_size; j++) {
            int pos = num_rows - (loop - j);
            if (pos >= 0 && pos < num_rows) {
                mgos_universal_led_plot_pixel(leds, x, leds->panel_height - 1 - pos, curr_md->out_pix, false);
            }
        }
    }

    mgos_universal_led_show(leds);
     
}

void mgos_ledeffects_meteor_shower(void* param, mgos_rgbleds_action action)
{
    static bool do_time = false;
    static uint32_t max_time = 0;
    uint32_t time = (mgos_uptime_micros() / 1000);
    mgos_rgbleds* leds = (mgos_rgbleds*)param;

    switch (action) {
    case MGOS_RGBLEDS_ACT_INIT:
        LOG(LL_INFO, ("mgos_ledeffects_meteor_shower: called (init)"));
        md_pool = mgos_internal_meteor_shower_init(leds);
        break;
    case MGOS_RGBLEDS_ACT_EXIT:
        LOG(LL_INFO, ("mgos_ledeffects_meteor_shower: called (exit)"));
        md_pool = mgos_internal_meteor_shower_exit(leds, md_pool);
        break;
    case MGOS_RGBLEDS_ACT_LOOP:
        LOG(LL_VERBOSE_DEBUG, ("mgos_ledeffects_meteor_shower: called (exit)"));
        mgos_internal_meteor_shower_loop(leds);
        if (do_time) {
            time = (mgos_uptime_micros() /1000) - time;
            max_time = (time > max_time) ? time : max_time;
            LOG(LL_VERBOSE_DEBUG, ("Meteor shower loop duration: %d milliseconds, max: %d ...", time / 1000, max_time / 1000));
        }
        break;
    }
}

bool mgos_meteor_shower_init(void) {
  LOG(LL_INFO, ("mgos_meteor_shower_init ..."));
  ledmaster_add_effect("ANIM_METEOR_SHOWER", mgos_ledeffects_meteor_shower);
  return true;
}

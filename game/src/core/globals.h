#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

/*
 * Global constants.
 */

extern const struct str_app_type str_app[36];
extern const struct int_app_type int_app[36];
extern const struct wis_app_type wis_app[36];
extern const struct dex_app_type dex_app[36];
extern const struct con_app_type con_app[36];

extern const struct jope_type jope_table[];

extern const struct cmd_type cmd_table[];
extern struct liq_type liq_table[LIQ_MAX];
extern const struct skill_type skill_table[MAX_SKILL];
extern const char *discipline[MAX_DISCIPLINES];
extern const char *wwgift[MAX_GIFTS];
extern struct social_type social_table[];
extern int social_count;
/* Removed: xsocial_table extern - xsocial system removed */

/*
 * Global variables.
 */
extern list_head_t g_helps;
extern list_head_t ban_list;
extern list_head_t g_characters;
extern list_head_t g_descriptors;
extern list_head_t g_objects;

extern ROOM_INDEX_DATA *room_list;
extern list_head_t g_dns_lookups;
extern char bug_buf[];
extern time_t current_time;
extern time_t boot_time;
extern time_t last_copyover_time;
extern bool fLogAll;
extern FILE *fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern int arena;
extern bool global_exp;
extern bool arena_open;
extern bool arena_base;
extern bool global_qp;
extern bool extra_log;
extern int players_logged;
extern int players_decap;
extern int players_gstolen;
extern int thread_count;
extern int iDelete;
extern bool arenaopen;
extern bool ragnarok;
extern int ragnarok_on_timer;
extern int ragnarok_safe_timer;
extern int ragnarok_cost;
extern int pulse_arena;
extern int pulse_doubleexp;
extern int pulse_doubleqp;

extern int thread_status;
extern char *last_command;

/*
 * Global Constants
 */
extern char *const dir_name[];
extern const int rev_dir[];

/*
 * Global variables
 */

extern list_head_t g_areas;
extern AREA_DATA *area_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern int top_vnum_mob;
extern int top_vnum_obj;
extern int top_vnum_room;

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

#endif /* GLOBALS_H */

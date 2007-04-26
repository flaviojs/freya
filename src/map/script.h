// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _SCRIPT_H_
#define _SCRIPT_H_

extern struct Script_Config {
	int warn_func_no_comma;
	int warn_cmd_no_comma;
	int warn_func_mismatch_paramnum;
	int warn_cmd_mismatch_paramnum;
	int check_cmdcount;
	int check_gotocount;

	int event_script_type;
	char* die_event_name;
	char* kill_event_name;
	char* login_event_name;
	char* logout_event_name;
	char* loadmap_event_name;
	int event_requires_trigger;
} script_config;

struct script_data {
	int type;
	union {
		int num;
		char *str;
	} u;
};

struct script_stack {
	int sp, sp_max;
	struct script_data *stack_data;
};
struct script_state {
	struct script_stack *stack;
	int start, end;
	int pos, state;
	int rid, oid;
	char *script,*new_script;
	int defsp, new_pos, new_defsp;
};

unsigned char * parse_script(unsigned char *,int);
int run_script(unsigned char *, int, int, int);

struct dbt* script_get_label_db();
struct dbt* script_get_userfunc_db();

int script_config_read(char *cfgName);
int do_init_script();
int do_final_script();

extern char mapreg_txt[1024];
extern int mapreg_txt_timer;

extern short log_script_level;

#endif // _SCRIPT_H_


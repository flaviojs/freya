/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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


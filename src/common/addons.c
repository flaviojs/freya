// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#ifdef __WIN32
#include <windows.h>
#include <io.h>
#define RTLD_NOW 0
const char *our_dlerror(void);
#else
#include <dlfcn.h>
#endif

#include <fcntl.h>

#include "../common/malloc.h"
#include "../common/timer.h"

#include "addons.h"
#include "addons_exports.h"

// Required includes only for exported functions
#include "core.h"

struct _Module *loaded_modules;
// SPECIAL DECLARATION
// This function is present in every server (login, char, map) and
// need to be called. It will be declared here since there's no
// specific "common" header file for this...
#ifdef DYNAMIC_LINKING
void init_localcalltable(void);
#endif

/** \brief Prepare call-table for addons.
 * This function prepares the calltable, in order to make functions
 * available to addons.
 */
void init_calltable() {
#ifdef DYNAMIC_LINKING
	init_localcalltable();
	call_table = malloc(MFNC_COUNT * sizeof(void *));
	// put here list of exported functions...
	MFNC_LOCAL_TABLE(void);
	MFNC_DISPLAY_TITLE(void);
	MFNC_ADD_TIMER(void);
	MFNC_ADD_TIMER_INTERVAL(void);
	MFNC_DELETE_TIMER(void);
	MFNC_ADDTICK_TIMER(void);
	MFNC_GET_VERSION(void);
#endif
	set_termfunc(addons_unload_all);
}

/** \brief Enable all loaded addons.
 * This function will enable (call Mod_Load) each addon loaded in the
 * configuration file.
 * \return Number of successfully enabled modules.
 */
int addons_enable_all(void) {
	// enable all addons
	struct _Module *mod;
	int (*Mod_Load)();
	int i = 0;

	mod = loaded_modules;
	if (!mod) return 0; // no module loaded
	// go to first module
	while (mod->prev) mod = mod->prev;

	// try to enable all modules...
	while (mod) {
		if (mod->state == MOD_STATE_DISABLED) {
			mod_dlsym(mod->dll, "Mod_Load", Mod_Load);
			if (!Mod_Load) {
				printf("Failed to start module %s\n", mod->header->name);
				continue;
			}
			if ((*Mod_Load)() != MOD_SUCCESS) {
				printf("Error while starting module %s\n", mod->header->name);
				continue;
			}
			mod->state = MOD_STATE_ENABLED;
			i++;
		}
		mod=mod->next;
	}

	return i;
}

int addons_load(char *addon_file, char addon_target) {

	char *modulename;
	struct _Module *mod;
	ModuleHeader *mod_header;
	int (*Mod_Init)(void *);
	int (*Mod_Test)();
	if (!call_table) init_calltable();
	CALLOC(modulename, char, strlen(addon_file) + 7 + 5); // 7 = "addons/", 5 = ".so\0" or ".dll\0"
	sprintf(modulename, "addons/%s" ADDONS_EXT, addon_file);
	CALLOC(mod, struct _Module, 1);

	mod->dll = mod_dlopen(modulename, RTLD_NOW);
	if (!mod->dll) {
		printf("Could not load module %s : %s\n", modulename, mod_dlerror());
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	mod_dlsym(mod->dll, "Mod_Header", mod_header);
	if (!mod_header) {
		printf("Module %s is not a valid Freya module.\n", modulename);
		mod_dlclose(mod->dll);
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	if (strcmp(mod_header->modversion,MOD_VERSION)!=0) {
		printf("Module version %s does not match our version (" MOD_VERSION ")\n", mod_header->modversion);
		mod_dlclose(mod->dll);
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	if ((mod_header->addon_target != ADDONS_ALL) && (mod_header->addon_target != addon_target)) {
		printf("Module %s is not designed for this server.\n", modulename);
		mod_dlclose(mod->dll);
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	mod_dlsym(mod->dll, "Mod_Init", Mod_Init);
	mod_dlsym(mod->dll, "Mod_Test", Mod_Test);
	if ((!Mod_Init) || (!Mod_Test)) {
		printf("Module %s is not a valid Freya module !\n", modulename);
		mod_dlclose(mod->dll);
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	if (((*Mod_Init)(call_table) != MOD_SUCCESS) ||
	    ((*Mod_Test)() != MOD_SUCCESS)) {
		printf("Initialisation of module %s failed !\n", modulename);
		mod_dlclose(mod->dll);
		FREE(mod);
		FREE(modulename);
		return MOD_FAILED;
	}
	mod->header = mod_header;
	mod->filename = modulename;
	mod->state = MOD_STATE_DISABLED;
	if (loaded_modules) {
		loaded_modules->next = mod;
		mod->prev = loaded_modules;
	}
	loaded_modules = mod;

	return 0;
}

int addons_unload(struct _Module *mod, int force_unload) {
	int (*Mod_Unload)();
	int result;

	printf("Unloading module %s...",mod->header->name);
	mod_dlsym(mod->dll, "Mod_Unload", Mod_Unload);
	if (Mod_Unload) {
		result = (*Mod_Unload)();
		if (!force_unload) if (result != MOD_SUCCESS) return MOD_FAILED;
	}
	mod_dlclose(mod->dll);
	// remove from chain
	if (mod->next) mod->next->prev = mod->prev;
	if (mod->prev) mod->prev->next = mod->next;
	if (loaded_modules == mod) {
		if (mod->prev) {
			// should not happen
			loaded_modules = mod->prev;
		} else if (mod->next) {
			loaded_modules = mod->next;
		} else {
			loaded_modules = NULL;
		}
	}
	FREE(mod->filename);
	FREE(mod);

	return MOD_SUCCESS;
}

void addons_unload_all(void) {
	while(loaded_modules) addons_unload(loaded_modules, 1);
}

#ifdef __WIN32
const char *our_dlerror(void) {
	static char errbuf[513];
	DWORD err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
			0, errbuf, 512, NULL);

	return errbuf;
}
#endif


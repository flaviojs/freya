// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef MODULES_H
#define MODULES_H

#ifdef __WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define MOD_VERSION	"1.0"

#ifdef __WIN32
 #define DLLFUNC	_declspec(dllexport)
 #define mod_dlopen(x,y) LoadLibrary(x)
 #define mod_dlclose FreeLibrary
 #define mod_dlsym(x,y,z) z = (void *)GetProcAddress(x,y)
 #define mod_dlerror our_dlerror
#else
 #define mod_dlopen dlopen
 #define mod_dlclose dlclose
 #define mod_dlsym(x,y,z) z = dlsym(x,y)
 #define mod_dlerror dlerror
 #define DLLFUNC
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

/* used for various temporary usages */
void *addon_tmp_pointer;
/* used to contain the common calls table */
void *call_table;
/* used to contain the local calls table */
void *local_table;

typedef struct _Module Module;

/*
 * Module header that every module must include, with the name of
 * mod_header
*/

typedef struct _ModuleHeader {
	char *name;
	char *version;
	char *description;
	char *modversion;
	char addon_target;
} ModuleHeader;

typedef struct {
	int size;
	int module_load;
	Module *handle;
} ModuleInfo;


struct _Module {
	struct _Module *prev, *next;
	ModuleHeader *header; /* The module's header */
	char *filename; /* filename of the module file */
#ifdef __WIN32
	HMODULE dll;	/* Return value of LoadLibrary */
#else
	void *dll;	/* Return value of dlopen */
#endif
	int state; /* state of the addon (loaded or not, etc...) */
};

/* Module function return values */
#define MOD_SUCCESS 0
#define MOD_FAILED -1

/* Addons states */
#define MOD_STATE_EMPTY 0 /* No addon here, empty space */
#define MOD_STATE_DISABLED 1 /* addon currently disabled */
#define MOD_STATE_ENABLED 2 /* addon currently enabled */

#define ADDONS_ALL 0
#define ADDONS_LOGIN 1
#define ADDONS_CHAR 2
#define ADDONS_MAP 3

#ifdef DYNAMIC_LINKING
 #define MOD_HEADER(name) Mod_Header
 #define MOD_TEST(name) Mod_Test
 #define MOD_INIT(name) Mod_Init
 #define MOD_LOAD(name) Mod_Load
 #define MOD_UNLOAD(name) Mod_Unload
#else
 #define MOD_HEADER(name) name##_Header
 #define MOD_TEST(name) name##_Test
 #define MOD_INIT(name) name##_Init
 #define MOD_LOAD(name) name##_Load
 #define MOD_UNLOAD(name) name##_Unload
#endif

int addons_enable_all(void);
int addons_load(char *, char);
int addons_unload(struct _Module *, int);
void addons_unload_all(void);

#endif // MODULES_H


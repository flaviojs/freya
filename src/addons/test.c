// $Id: test.c 397 2005-10-08 18:27:36Z Yor $
// Test addon !

#define __ADDON
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "../common/addons.h"
#include "../common/addons_exports.h"

DLLFUNC int test(int);

int value;

DLLFUNC ModuleHeader MOD_HEADER(test) = {
	"test",
	"$Id: test.c 397 2005-10-08 18:27:36Z Yor $",
	"Test addon",
	MOD_VERSION,
	ADDONS_ALL
};

DLLFUNC int MOD_TEST(test)() {
	// add commands, return MOD_FAILED on error
	printf(__FILE__ ":%d: MOD_TEST called!\n", __LINE__);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_INIT(test)(void *ct) {
	call_table = ct;
	/* load local symbols table */
	printf(__FILE__ ":%d: MOD_INIT called!\n", __LINE__);
	MFNC_LOCAL_TABLE(local_table);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(test)() {
	printf(__FILE__ ":%d: MOD_LOAD called!\n", __LINE__);
	value = value + 1;
	printf("This is displayed from the test addon %d !! Hahahaha\n", value);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(test)() {
	printf(__FILE__ ":%d: MOD_UNLOAD called!\n", __LINE__);

	return MOD_SUCCESS;
}

DLLFUNC int test(int blah) {
	// test function...
	static void (*display_title)(void);
	MFNC_DISPLAY_TITLE(display_title);

	(*display_title)();

	return 0;
}


/* PHP NPCs support
 * $Id: npc_php.c 183 2006-02-03 14:09:22Z MagicalTux $
 * Sourcecode by MagicalTux
 */

#include <config.h>

#ifdef __PHP_ENGINE

#include <stdio.h>
#include <string.h>
#include <php/php/main/php.h>
#include <php/sapi/nezumi.h>

#include "../common/malloc.h"
#include "npc_php.h"

struct phpnpc_src_list {
	struct phpnpc_src_list *next;
	char *name;
};

static struct phpnpc_src_list *first_phpnpc_file = NULL;

void do_init_phpnpc(void) {
	PHP_NEZUMI_SAPI_START_BLOCK();
	zend_eval_string_ex("echo \"\\n\\n\\nTEST: \".phpversion().\"\\n\\n\\n\";", NULL, "Test Code", 1);
	PHP_NEZUMI_SAPI_END_BLOCK();
}

void npc_php_addsrcfile(char *name) {
	struct phpnpc_src_list *new;
	size_t len=strlen(name);
	CALLOC(new, struct phpnpc_src_list, 1);
	CALLOC(new->name, char, len+1);
	memcpy(new->name, name, len);
	*(new->name+len)=0;
	new->next = first_phpnpc_file;
	first_phpnpc_file = new;
}

#endif /* defined(__PHP_ENGINE) */


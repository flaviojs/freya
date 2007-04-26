// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h> // atoi
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h> // tolower, isprint

#include "utils.h"
#include "../common/timer.h" /* gettimeofday for win32 */

//
// --- convert boolean/hexadecimal into integer
//
int config_switch(const char *str)
{
	if(strcmp(str, "no") == 0 || strcmp(str, "off") == 0)
		return 0;
	if(strcmp(str, "yes") == 0 || strcmp(str, "on") == 0)
		return 1;
	if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X') && ((str[3] >= '0' && str[3] <= '9') || (str[3] >= 'a' && str[3] <= 'f') || (str[3] >= 'A' && str[3] <= 'F')))
	{
		int i;
		if(sscanf(str, "%x", &i) == 1)
			return i;
	}
	return atoi(str);
}

//---------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid).
//---------------------------------------------------
int e_mail_check(const char *email) {
	int ch;
	size_t email_len;
	char* last_arobas;

	email_len = strlen(email);

	// freya limits
	if (email_len < 3 || email_len > 40)
		return 0;

	last_arobas = strrchr(email, '@');

	// part of RFC limits (official reference of e-mail description)
	if (last_arobas == NULL || email[email_len - 1] == '@')
		return 0;

	if (email[email_len - 1] == '.')
		return 0;

	if (last_arobas[1] == '.' || // "@."
	    strstr(last_arobas, "..") != NULL)
		return 0;

	ch = 0;
	while(last_arobas[ch]) {
		if (iscntrl((int)last_arobas[ch])) // if (last_arobas[ch] > 0 && last_arobas[ch] < 32)
			return 0;
		ch++;
	}

	if (strchr(last_arobas, (int)' ') != NULL ||
	    strchr(last_arobas, (int)';') != NULL)
		return 0;

	// all correct
	return 1;
}

//-----------------------------------------------------
// Function to suppress control characters in a string.
//-----------------------------------------------------
int remove_control_chars(char *str) {
	int change = 0;

	while(*str) {
		if (iscntrl((int)(*str))) { // if (*str > 0 && *str < 32) {
			*str = '_';
			change++;
		}
		str++;
	}

	return change;
}

/*-----------------------------
  Convert a 'int' to a string
-----------------------------*/
inline int int2string(char *buffer, int val) {
	char *pointer;
	char tmp;
	int len;
	register int i;

	if (val == 0) {
		*buffer++ = '0';
		*buffer = '\0';
		return 1;
	}

	pointer = buffer;
	tmp = 0;
	if (val < 0) {
		tmp = -1; /* true */
		val = -val;
	}

	i = 0;
	do {
		pointer[i++] = val % 10 + '0';
		val /= 10;
	} while(val);
	if (tmp)
		pointer[i++] = '-';
	pointer[i] = '\0';
	len = i;

	while(buffer < (pointer + (--i))) {
		tmp = *buffer;
		*buffer = pointer[i];
		pointer[i] = tmp;
		buffer++;
	}

	return len;
}

/*----------------------------------
  Convert a 'long int' to a string
----------------------------------*/
inline int lint2string(char *buffer, long int val) {
	char *pointer;
	char tmp;
	int len;
	register int i;

	if (val == 0) {
		*buffer++ = '0';
		*buffer = '\0';
		return 1;
	}

	pointer = buffer;
	tmp = 0;
	if (val < 0) {
		tmp = -1; /* true */
		val = -val;
	}

	i = 0;
	do {
		pointer[i++] = val % 10 + '0';
		val /= 10;
	} while(val);
	if (tmp)
		pointer[i++] = '-';
	pointer[i] = '\0';
	len = i;

	while(buffer < (pointer + (--i))) {
		tmp = *buffer;
		*buffer = pointer[i];
		pointer[i] = tmp;
		buffer++;
	}

	return len;
}

/*----------------------------------
  Code to replace strcasecmp and strncasecmp on any system
----------------------------------*/
static unsigned char char_map[256];
static char char_map_init = 0;

void init_char_map(void) {
	int i;

	for (i = 0; i < 256; i++)
		char_map[i] = tolower((unsigned char)i); // tolower needs unsigned char

	char_map_init = 1;

	return;
}

int stringcasecmp(const char *s1, const char *s2) {
	const unsigned char *cm = (const unsigned char *)char_map;
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;

	// if char_map is not init, init it
	if (!char_map_init)
		init_char_map();

	while (cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return 0;

	return (cm[*us1] - cm[*(us2 - 1)]);
}

int stringncasecmp(const char *s1, const char *s2, size_t n) {
	const unsigned char *cm = (const unsigned char *)char_map;
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;

	// if char_map is not init, init it
	if (!char_map_init)
		init_char_map();

	while (n != 0 && cm[*us1] == cm[*us2++]) {
		if (*us1++ == '\0')
			return 0;
		n--;
	}

	return (n == 0 ? 0 : cm[*us1] - cm[*(us2 - 1)]);
}


#ifndef USE_MYSQL
/*-----------------------------------------
  Replace mysql_escape_string standard function
-----------------------------------------*/
unsigned long mysql_escape_string(char * to, const char * from, unsigned long length) {
	const char *to_start = to;
	const char *end, *to_end = to_start + 2 * length;
	char overflow = 0; /* FALSE */
	for (end = from + length; from < end; from++) {
		char escape= 0;
		switch (*from) {
			case 0: /* Must be escaped for 'mysql' */
				escape = '0';
				break;
			case '\n': /* Must be escaped for logs */
				escape = 'n';
				break;
			case '\r':
				escape = 'r';
				break;
			case '\\':
				escape= '\\';
				break;
			case '\'':
				escape= '\'';
				break;
			case '"': /* Better safe than sorry */
				escape= '"';
				break;
			case '\032': /* This gives problems on Win32 */
				escape= 'Z';
				break;
		}
		if (escape) {
			if (to + 2 > to_end) {
				overflow = 1; /* TRUE */
				break;
			}
			*to++= '\\';
			*to++= escape;
		} else {
			if (to + 1 > to_end) {
				overflow = 1; /* TRUE */
				break;
			}
			*to++ = *from;
		}
	}
	*to = 0;

	return (overflow) ? (unsigned long)~0 : (unsigned long)(to - to_start);
}
#endif /* not USE_MYSQL */

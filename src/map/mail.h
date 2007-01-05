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

#ifndef _MAIL_H_
#define _MAIL_H_

void mail_check(struct map_session_data *sd, unsigned char type);
void mail_read(struct map_session_data *sd, int message_id);
void mail_delete(struct map_session_data *sd, int message_id);
void mail_send(struct map_session_data *sd, char *name, char *message, short flag);

void do_init_mail(void);

#endif // _MAIL_H_

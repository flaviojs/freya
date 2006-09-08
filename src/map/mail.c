// Mail System for eAthena SQL
// Created by Valaris
#include <config.h>

#ifdef USE_SQL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/socket.h"
#include "../common/timer.h"

#include "map.h"
#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "pc.h"
#include "mail.h"
#include "atcommand.h"

char mail_db[32] = "mail";

int MAIL_CHECK_TIME = 120000; // every 2 min

#ifdef MEMWATCH
#include "memwatch.h"
#endif

void mail_check(struct map_session_data *sd, unsigned char type) { // type: 1:checkmail, 2:listmail, 3:listnewmail
	int i, new, priority;
	char message[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)

	if (sd == NULL)
		return;

	sql_request("SELECT `message_id`,`to_account_id`,`from_char_name`,`read_flag`,`priority`,`check_flag` FROM `%s` WHERE `to_account_id` = \"%d\" ORDER by `message_id`", mail_db, sd->status.account_id);

	i = 0;
	new = 0;
	priority = 0;
	while (sql_get_row()) {
		i++;
		// if not checked
		if (sql_get_integer(5) == 0)
			sql_request("UPDATE `%s` SET `check_flag`='1' WHERE `message_id`= \"%d\"", mail_db, sql_get_integer(0));
		// if not read
		if (sql_get_integer(3) == 0) {
			new++;
			if (sql_get_integer(4))
				priority++;
			if (type == 2 || type == 3) { // type: 1:checkmail, 2:listmail, 3:listnewmail
				if (sql_get_string(4)) {
					sprintf(message, msg_txt(575), i, sql_get_string(2)); // %d - From : %s (New - Priority)
					clif_displaymessage(sd->fd, message);
				} else {
					sprintf(message, msg_txt(576), i, sql_get_string(2)); // %d - From : %s (New)
					clif_displaymessage(sd->fd, message);
				}
			}
		} else if (type == 2) { // type: 1:checkmail, 2:listmail, 3:listnewmail
			sprintf(message, msg_txt(577), i, sql_get_string(2)); // %d - From : %s
			clif_displaymessage(sd->fd, message);
		}
	}

	if (i == 0) {
		clif_displaymessage(sd->fd, msg_txt(574)); // You have no message.
		return;
	}

	if (new == 0) {
		clif_displaymessage(sd->fd, msg_txt(580)); // You have no new message.
	} else {
		if (type == 1) { // type: 1:checkmail, 2:listmail, 3:listnewmail
			sprintf(message, msg_txt(578), new); // You have %d new messages.
			clif_displaymessage(sd->fd, message);
			if (priority > 0) {
				sprintf(message, msg_txt(579), priority); // You have %d unread priority messages.
				clif_displaymessage(sd->fd, message);
			}
		}
	}

	return;
}

void mail_read(struct map_session_data *sd, int message_id) {
	char message[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)

	if (sd == NULL)
		return;

	sql_request("SELECT `message_id`,`to_account_id`,`from_char_name`,`message`,`read_flag`,`priority`,`check_flag` from `%s` WHERE `to_account_id` = \"%d\" ORDER by `message_id` LIMIT %d, 1", mail_db, sd->status.account_id, message_id - 1);
	if (!sql_get_row()) {
		clif_displaymessage(sd->fd, msg_txt(581)); // Message not found.
		return;
	}

	sprintf(message, msg_txt(582), sql_get_string(2)); // Reading message from %s
	clif_displaymessage(sd->fd, message);
	clif_displaymessage(sd->fd, sql_get_string(3));

	sql_request("UPDATE `%s` SET `read_flag`='1', `check_flag`='1' WHERE `message_id`= \"%d\"", mail_db, sql_get_integer(0));

	return;
}

void mail_delete(struct map_session_data *sd, int message_id) {
	if (sd == NULL)
		return;

	sql_request("SELECT `message_id`,`to_account_id`,`read_flag`,`priority`,`check_flag` from `%s` WHERE `to_account_id` = \"%d\" ORDER by `message_id` LIMIT %d, 1", mail_db, sd->status.account_id, message_id - 1);
	if (!sql_get_row()) {
		clif_displaymessage(sd->fd, msg_txt(581)); // Message not found.
		return;
	}

	if (!sql_get_integer(2) && sql_get_integer(3)) {
		clif_displaymessage(sd->fd, msg_txt(583)); // Cannot delete unread priority message.
		return;
	}
	if (!sql_get_integer(4)) {
		clif_displaymessage(sd->fd, msg_txt(584)); // You have recieved new messages, use @listmail before deleting.
		return;
	}

	if (sql_request("DELETE FROM `%s` WHERE `message_id` = \"%d\"", mail_db, sql_get_integer(0)))
		clif_displaymessage(sd->fd, msg_txt(585)); // Message deleted.

	return;
}

void mail_send(struct map_session_data *sd, char *name, char *message, short flag) {
	char t_name[49]; // (24 * 2) + NULL
	char t_name2[49]; // (24 * 2) + NULL
	char t_buf[200];
	int i;

	if (sd == NULL)
		return;

	if (sd->GM_level < 20 && sd->mail_counter > 0) {
		clif_displaymessage(sd->fd, msg_txt(586)); // You must wait 10 minutes before sending another message.
		return;
	}

	if (strcmp(name, "*") == 0) {
		if (sd->GM_level < 20) {
			clif_displaymessage(sd->fd, msg_txt(587)); // Access Denied.
			return;
		} else
			sql_request("SELECT DISTINCT `account_id` FROM `%s` WHERE `account_id` <> '%d' ORDER BY `account_id`", char_db, sd->status.account_id);
	} else {
		mysql_escape_string(t_buf, name, strlen(name));
		sql_request("SELECT `account_id`,`name` FROM `%s` WHERE `name` = \"%s\"", char_db, t_buf);
	}

	i = 0;
	mysql_escape_string(t_name2, sd->status.name, strlen(sd->status.name));
	mysql_escape_string(t_buf, message, strlen(message));
	while (sql_get_row()) {
		i++;
		if (strcmp(name, "*") == 0) {
			sql_request("INSERT INTO `%s` (`to_account_id`,`from_account_id`,`from_char_name`,`message`,`priority`)"
			            " VALUES ('%d', '%d', '%s', '%s', '%d')", mail_db, sql_get_integer(0), sd->status.account_id, t_name2, t_buf, flag);
		} else {
			mysql_escape_string(t_name, sql_get_string(1), strlen(sql_get_string(1)));
			sql_request("INSERT INTO `%s` (`to_account_id`,`to_char_name`,`from_account_id`,`from_char_name`,`message`,`priority`)"
			            " VALUES ('%d', '%s', '%d', '%s', '%s', '%d')", mail_db, sql_get_integer(0), t_name, sd->status.account_id, t_name2, t_buf, flag);
		}
	}

	if (i == 0)
		clif_displaymessage(sd->fd, msg_txt(588)); // Character does not exist.
	else {
		clif_displaymessage(sd->fd, msg_txt(589)); // Message has been sent.
		if (sd->GM_level < 20)
			sd->mail_counter = 5;
	}

	return;
}

int mail_check_timer(int tid, unsigned int tick, int id, int data) {
	struct map_session_data *sd = NULL;
	int i, account_id;

	sql_request("SELECT DISTINCT `to_account_id` FROM `%s` WHERE `read_flag` = '0' AND `check_flag` = '0'", mail_db);
	while (sql_get_row()) {
		account_id = sql_get_integer(0);
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (sd = session[i]->session_data) && !sd->new_message_flag && sd->status.account_id == account_id && sd->state.auth) {
				clif_displaymessage(sd->fd, msg_txt(590)); // You have new message.
				sd->new_message_flag = 1; // to avoid to send twice: you have new messages [Yor]
			}
		}
	}

	// decrease counter, remove flag for "you have new messages"
	for (i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth) {
			if (sd->mail_counter > 0)
				sd->mail_counter--;
			sd->new_message_flag = 0;
		}

	sql_request("UPDATE `%s` SET `check_flag`='1' WHERE `check_flag`= '0'", mail_db);

	return 0;
}

void do_init_mail(void) {
	add_timer_func_list(mail_check_timer, "mail_check_timer");
	add_timer_interval(gettick_cache + MAIL_CHECK_TIME, mail_check_timer, 0, 0, MAIL_CHECK_TIME);

	return;
}
#endif /* USE_SQL */


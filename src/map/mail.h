// Mail System for eAthena
// Created by Valaris

#ifndef _MAIL_H_
#define _MAIL_H_

void mail_check(struct map_session_data *sd, unsigned char type);
void mail_read(struct map_session_data *sd, int message_id);
void mail_delete(struct map_session_data *sd, int message_id);
void mail_send(struct map_session_data *sd, char *name, char *message, short flag);

void do_init_mail(void);

#endif // _MAIL_H_

// $Id: int_party.h 1 2006-01-13 22:47:50Z MagicalTux $
#ifndef _INT_PARTY_H_
#define _INT_PARTY_H_

void inter_party_init();

int inter_party_parse_frommap(int fd);
void inter_party_leave(int party_id, int account_id);

#ifdef TXT_ONLY
extern char party_txt[1024];
#endif /* TXT_ONLY */

#endif // _INT_PARTY_H_

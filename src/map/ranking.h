// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _RANKING_H_
#define _RANKING_H_

#include "map.h"

extern struct Ranking_Data ranking_data[RK_MAX][MAX_RANKER];

int ranking_get_pos(struct map_session_data * sd, unsigned int ranking_id);
int ranking_id2rank(unsigned int char_id, unsigned int ranking_id);

void ranking_gain_point(struct map_session_data * sd, const int ranking_id, unsigned int point);

int ranking_sort(unsigned int ranking_id);

void do_init_ranking(void);

#endif

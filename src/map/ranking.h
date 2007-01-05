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

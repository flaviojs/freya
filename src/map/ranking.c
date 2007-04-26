// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ranking.h"
#include "clif.h"
#include "chrif.h"

struct Ranking_Data ranking_data[RK_MAX][MAX_RANKER];

//PCのランキングを返す
// 0 : ランク外
int ranking_get_pos(struct map_session_data * sd, unsigned int ranking_id) {
	int i;

	for(i = 0; i < MAX_RANKER; i++) {
		if(sd->status.char_id == ranking_data[ranking_id][i].char_id)
			return i + 1;
	}

	return 0;
}

//idからランキングを求める
// 0 : ランク外
int ranking_id2rank(unsigned int char_id, unsigned int ranking_id) {
	unsigned int i;

	if(char_id <= 0)
		return 0;

	for(i = 0; i < MAX_RANKER; i++) {
		if(ranking_data[ranking_id][i].char_id == char_id)
			return i + 1;
	}

	return 0;
}

void ranking_gain_point(struct map_session_data *sd, const int ranking_id, unsigned int point) {
	unsigned int i, update_flag = 0;

	sd->status.fame_point[ranking_id] += point;

	switch(ranking_id) {
		case RK_BLACKSMITH:
		case RK_ALCHEMIST:
		case RK_TAEKWON:
			/*if(sd->status.fame_point[ranking_id]%100 == 0)
				clif_taekwon_point(sd->fd, sd->status.fame_point[ranking_id] / 100, 1);
			//	clif_taekwon_point(sd->fd,sd->status.fame_point[ranking_id]/100,point);*/
		case RK_PK:
			clif_fame_point(sd, ranking_id, point);
			break;
	}

	if(!map_is_alone) { //Multimap: Fame list is updated/sorted in char-server
		chrif_updatefame(sd->status.char_id, ranking_id, sd->status.fame_point[ranking_id]);
		return;
	}

	//探す
	for(i = 0; i < MAX_RANKER; i++) {
		//既にランカーならばpoint更新
		if(sd->status.char_id == ranking_data[ranking_id][i].char_id || !ranking_data[ranking_id][i].char_id) {
			ranking_data[ranking_id][i].point = sd->status.fame_point[ranking_id];
			if(ranking_data[ranking_id][i].char_id)
				update_flag = 1; //Sort
			else { //Free room?, insert in it the name/char_id
				strncpy(ranking_data[ranking_id][i].name, sd->status.name, 24);
				ranking_data[ranking_id][i].char_id = sd->status.char_id;
			}
			break;
		}
	}

	//順位にはなかった
	if(MAX_RANKER == i) {
		//最下位より高得点なら最下位にランクイン
		if(ranking_data[ranking_id][MAX_RANKER - 1].point < sd->status.fame_point[ranking_id]) {
			strncpy(ranking_data[ranking_id][MAX_RANKER - 1].name, sd->status.name, 24);
			ranking_data[ranking_id][MAX_RANKER - 1].point = sd->status.fame_point[ranking_id];
			ranking_data[ranking_id][MAX_RANKER - 1].char_id = sd->status.char_id;
			update_flag = 1; //Sort
		}
	}

	if(update_flag)
		ranking_sort(ranking_id);

	return;
}

int compare_ranking_data(const struct Ranking_Data *a, const struct Ranking_Data *b) {
	if((a->point - b->point) > 0)
		return -1;

	if(a->point == b->point)
		return 0;

	return 1;
}

int ranking_sort(unsigned int ranking_id) {
	//ランキング対象がない
	if(ranking_id >= RK_MAX)
		return 0;
	
	qsort(ranking_data[ranking_id], MAX_RANKER, sizeof(struct Ranking_Data),(int (*)(const void*,const void*))compare_ranking_data);

	return 1;
}

void ranking_init_data(void) {

	memset(ranking_data, 0, sizeof(ranking_data));

	return;
}

//初期化
void do_init_ranking(void) {
	ranking_init_data();
	return;
}

// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/timer.h"

#include "map.h"
#include "status.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "battle.h"

#include "nullpo.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

/*==========================================
 *
 *------------------------------------------
 */
int unit_can_move(struct block_list *bl) {

	// Create monster / player structures
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;

	// Verify pointer is valid
	nullpo_retr(0, bl);

	// Get monster / player data
	if (bl->type == BL_PC) {
		sd = (struct map_session_data*)bl;
	} else if (bl->type == BL_MOB) {
		md = (struct mob_data*)bl;
	}

	// Get status data
	struct status_change *sc_data = status_get_sc_data(bl);
	short *sc_count = status_get_sc_count(bl);

	// Player can move checks
	if (sd) {
		// Check time between last move
		if (DIFF_TICK(sd->canmove_tick, gettick_cache) > 0)
			return 0;

		// Cannot move during Freeze / Stun / etc. statuses (With the exception of Stone Curse in its first phase)
		if (sd->opt1 > 0 && sd->opt1 != 6)
			return 0;

		// Cannot move during Hiding (With the exception of Rogues with RG_TUNNELDRIVE)
		if ((sd->status.option&2) && pc_checkskill(sd, RG_TUNNELDRIVE) == 0)
			return 0;

		// Cannot move while casting a skill (With the exception of Sages with SA_FREECAST)
		if (sd->skilltimer != -1 && pc_checkskill(sd, SA_FREECAST) == 0 && (sc_data && sc_data[SC_SELFDESTRUCTION].timer == -1))
			return 0;
	}

	// Monster can move checks
	if (md) {
		// Retrieve monster mode if missing
		int mode;
		if (!md->mode)
			mode = mob_db[md->class].mode;
		else
			mode = md->mode;

		// Make sure monster can move based on its mode (Example: Geographers can't move)
		if (!(mode&1))
			return 0;

		// Check time between last move
		if (md->canmove_tick > gettick_cache)
			return 0;

		// Cannot move during Freeze / Stun / etc. statuses (With the exception of Stone Curse in its first phase)
		if (md->opt1 > 0 && md->opt1 != 6)
			return 0;

		// Cannot move during Hiding
		if (md->option&2)
			return 0;

		// Cannot move while casting skills
		if (md->skilltimer != -1 && (sc_data && sc_data[SC_SELFDESTRUCTION].timer == -1))
			return 0;
	}

	// Check for statuses that immobilize the source
	if (sc_data && sc_count) {
		if (sc_data[SC_STOP].timer != -1)
			return 0;
		if (sc_data[SC_ANKLE].timer != -1)
			return 0;
		if (sc_data[SC_AUTOCOUNTER].timer != -1)
			return 0;
		if (sc_data[SC_TRICKDEAD].timer != -1)
			return 0;
		if (sc_data[SC_BLADESTOP].timer != -1)
			return 0;
		if (sc_data[SC_SPIDERWEB].timer != -1)
			return 0;
		if (sc_data[SC_MADNESSCANCEL].timer != -1)
			return 0;
		if (sc_data[SC_CLOSECONFINE].timer != -1 || sc_data[SC_CLOSECONFINE2].timer != -1)
			return 0;
		if (sc_data[SC_DANCING].timer != -1 && sc_data[SC_DANCING].val4 && sc_data[SC_LONGING].timer == -1)
			return 0;
		if (sc_data[SC_GOSPEL].timer != -1 && sc_data[SC_GOSPEL].val4 == BCT_SELF)
			return 0;
		if (sc_data[SC_GRAVITATION].timer != -1 && sc_data[SC_GRAVITATION].val3 == BCT_SELF)
			return 0;
		if (sc_data[SC_BASILICA].timer != -1 && sc_data[SC_BASILICA].val3 == BCT_SELF)
			return 0;
	}

	// Monster / Player can move
	return 1;
}

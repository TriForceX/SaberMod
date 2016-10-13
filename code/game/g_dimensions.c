#include "g_local.h"

/*
=================
G_BlameForEntity

Set blameEntityNum and snapshotIgnore fields for all clients.
=================
*/
void G_BlameForEntity( int blame, gentity_t *ent )
{
	assert(blame >= 0 && blame < MAX_GENTITIES);

	if (blame >= MAX_CLIENTS && blame != ENTITYNUM_NONE && blame != ENTITYNUM_WORLD) {
		blame = g_entities[blame].blameEntityNum;
		assert(blame < MAX_CLIENTS || blame == ENTITYNUM_NONE || blame == ENTITYNUM_WORLD);
	}

	ent->blameEntityNum = blame;
	// For now cgame can ignore only dueling players
	if (ent - g_entities < MAX_CLIENTS) {
		ent->dimension = DEFAULT_DIMENSION;
	} else {
		ent->dimension = ALL_DIMENSIONS;
	}

	if (mvapi) {
		mvsharedEntity_t	*mvEnt = mv_entities + ent->s.number;
		int					i;

		if (blame == ENTITYNUM_WORLD) {
			for (i = 0; i < level.maxclients; i++) {
				mvEnt->snapshotIgnore[i] = 0;
			}
		} else {
			for (i = 0; i < level.maxclients; i++) {
				gclient_t	*client;

				if (!g_entities[i].inuse) {
					continue;
				}

				client = level.clients + i;

				if (client->pers.privateDuel && client->ps.duelInProgress &&
					blame != i && blame != client->ps.duelIndex) {
					mvEnt->snapshotIgnore[i] = 1;
				} else {
					mvEnt->snapshotIgnore[i] = 0;
				}
			}
		}
	}
}

unsigned G_GetFreeDuelDimension(void)
{
	unsigned dimension;
	qboolean free;
	int i;

	for (dimension = 1 << 15; dimension != 0; dimension <<= 1) {
		free = qtrue;

		for (i = 0; i < level.maxclients; i++) {
			if (!g_entities[i].inuse) {
				continue;
			}
			if ((g_entities[i].dimension & dimension) != 0) {
				free = qfalse;
				break;
			}
		}

		if (free) {
			return dimension;
		}
	}

	assert(0); // didn't find a free dimension
	return DEFAULT_DIMENSION;
}

unsigned G_EntitiesCollide(gentity_t *ent1, gentity_t *ent2)
{
	unsigned common = ent1->dimension & ent2->dimension;

#ifndef NDEBUG
	qboolean collision;

	// cgame collision test has to follow the same logic
	if (ent1->client && ent2->client) {
		if (ent1->client->ps.duelInProgress) {
			if (ent1->client->ps.duelIndex == ent2->s.number) {
				collision = qtrue;
			} else {
				collision = qfalse;
			}
		} else {
			if (ent2->client->ps.duelInProgress) {
				collision = qfalse;
			} else {
				collision = qtrue;
			}
		}

		assert(!!common == collision);
	}
#endif

	return common;
}

void G_StartPrivateDuel(gentity_t *ent)
{
	if (mvapi) {
		int	clientNum = ent->s.number;
		int opponentNum;
		int	i;

		if (!ent->client->pers.privateDuel) {
			return;
		}
		if (!ent->client->ps.duelInProgress) {
			return;
		}

		opponentNum = ent->client->ps.duelIndex;

		for (i = 0; i < level.num_entities; i++) {
			if (g_entities[i].inuse) {
				int blame = g_entities[i].blameEntityNum;

				if (blame == ENTITYNUM_WORLD || blame == clientNum || blame == opponentNum) {
					mv_entities[i].snapshotIgnore[clientNum] = 0;
				} else {
					mv_entities[i].snapshotIgnore[clientNum] = 1;
				}
			}
		}
	}
}

void G_StopPrivateDuel(gentity_t *ent)
{
	if (mvapi) {
		int clientNum = ent->s.number;
		int i;

		for (i = 0; i < level.num_entities; i++) {
			mv_entities[i].snapshotIgnore[clientNum] = 0;
		}
	}
}

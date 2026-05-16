/*
 * =======================================================================
 *
 * Jump in into the game.so and support functions.
 *
 * =======================================================================
 */

#include "header/local.h"

game_locals_t game;
level_locals_t level;
game_import_t gi;
game_export_t globals;
spawn_temp_t st;

int sm_meat_index;
int snd_fry;
int meansOfDeath;

edict_t *g_edicts;

cvar_t *deathmatch;
cvar_t *coop;
cvar_t *dmflags;
cvar_t *zdmflags;
cvar_t *skill;
cvar_t *fraglimit;
cvar_t *timelimit;
cvar_t *password;
cvar_t *maxclients;
cvar_t *maxentities;
cvar_t *g_select_empty;
cvar_t *dedicated;

cvar_t *filterban;

cvar_t *sv_maxvelocity;
cvar_t *sv_gravity;

cvar_t *sv_rollspeed;
cvar_t *sv_rollangle;
cvar_t *gun_x;
cvar_t *gun_y;
cvar_t *gun_z;

cvar_t *run_pitch;
cvar_t *run_roll;
cvar_t *bob_up;
cvar_t *bob_pitch;
cvar_t *bob_roll;

cvar_t  *gamedir;

cvar_t	*sv_cheats;

cvar_t *aimfix;
cvar_t *g_machinegun_norecoil;
cvar_t *g_quick_weap;
cvar_t *g_swap_speed;

//===================================================================


void
ShutdownGame(void)
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2_DLL_EXPORTED game_export_t *
GetGameAPI(const game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	/* Initalize the PRNG */
	randk_seed();

	return &globals;
}

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	gi.error(ERR_FATAL, "%s", text);
}

void
Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

//======================================================================


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int i;

	// calc the player views now that all pushing
	// and damage has been added
	for (i = 0 ; i < maxclients->value ; i++)
	{
		edict_t *ent;

		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
		{
			continue;
		}

		ClientEndServerFrame (ent);
	}
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void
EndDMLevel(void)
{
	edict_t		*ent;

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.mapname;
    ent->spawnflags2 = 0;
	}
	else if (level.nextmap[0])
	{	// go to a specific map
		ent = G_Spawn ();
		ent->classname = "target_changelevel";
		ent->map = level.nextmap;
    ent->spawnflags2 = 0;
	}
	else
	{	// search for a changeleve
		ent = G_Find(NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			ent = G_Spawn ();
			ent->classname = "target_changelevel";
			ent->map = level.mapname;
			ent->spawnflags2 = 0;
		}
	}

	BeginIntermission (ent);
}

/*
=================
CheckDMRules
=================
*/
void
CheckDMRules(void)
{
	if (level.intermissiontime)
	{
		return;
	}

	if (!deathmatch->value)
	{
		return;
	}

	if (timelimit->value)
	{
		if (level.time >= timelimit->value*60)
		{
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->value)
	{
		int i;

		for (i = 0 ; i < maxclients->value; i++)
		{
			const gclient_t *cl;

			cl = game.clients + i;
			if (!g_edicts[i+1].inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel ();
				return;
			}
		}
	}
}


/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	ClientEndServerFrames ();

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;
	}
	level.fadeFrames = 0;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	gibsthisframe = 0;
	lastgibframe = 0;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i;
	edict_t	*ent;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.fadeFrames > 1)
	{
		level.fadeFrames--;
	}
	else if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

    if(!(ent->flags & FL_DONTSETOLDORIGIN))
    {
		  VectorCopy(ent->s.origin, ent->s.old_origin);
    }

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
			continue;
		}

		G_RunEntity (ent);
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();
}


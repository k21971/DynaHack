/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#ifndef HACK_H
#define HACK_H

#ifndef CONFIG_H
#include "config.h"
#endif
#include "nitrohack.h"

/*	For debugging beta code.	*/
#ifdef BETA
#define Dpline	pline
#endif

#define TELL		1
#define NOTELL		0
#define BOLT_LIM	8 /* from this distance ranged attacks will be made */
#define MAX_CARR_CAP	1000	/* so that boulders can be heavier */

/*
 * Style of rooms and corridors on standard dungeon levels.
 * Also the values that RANDOM_CORRIDORS accepts in special level files.
 */
#define LEVSTYLE_STANDARD	0
#define LEVSTYLE_ANYTOANY	1
#define LEVSTYLE_RING		2
#define LEVSTYLE_HUB		3
#define LEVSTYLE_GRID		4
#define LEVSTYLE_MAX_TYPES	5

/* symbolic names for capacity levels */
#define UNENCUMBERED	0
#define SLT_ENCUMBER	1	/* Burdened */
#define MOD_ENCUMBER	2	/* Stressed */
#define HVY_ENCUMBER	3	/* Strained */
#define EXT_ENCUMBER	4	/* Overtaxed */
#define OVERLOADED	5	/* Overloaded */

/* Possible hunger states for u.uhs; update hu_stat[] in botl.c if changed! */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE	0
#define BY_COOKIE	1
#define BY_PAPER	2
#define BY_OTHER	9

/* Macros for why you are no longer riding */
#define DISMOUNT_GENERIC	0
#define DISMOUNT_FELL		1
#define DISMOUNT_THROWN		2
#define DISMOUNT_POLY		3
#define DISMOUNT_ENGULFED	4
#define DISMOUNT_BONES		5
#define DISMOUNT_BYCHOICE	6

/* sellobj_state() states */
#define SELL_NORMAL	(0)
#define SELL_DELIBERATE	(1)
#define SELL_DONTSELL	(2)

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANIC separates the deaths from the non-deaths.
 */
#define DIED		 0
#define CHOKING		 1
#define POISONING	 2
#define STARVING	 3
#define DROWNING	 4
#define BURNING		 5
#define DISSOLVED	 6
#define CRUSHING	 7
#define STONING		 8
#define TURNED_SLIME	 9
#define GENOCIDED	10
#define DISINTEGRATED	11
#define MAX_SURVIVABLE_DEATH	DISINTEGRATED
#define PANICKED	12
#define TRICKED		13
#define QUIT		14
#define ESCAPED		15
#define DEFIED		16
#define ASCENDED	17

#include "align.h"
#include "dungeon.h"
#include "monsym.h"
#include "mkroom.h"
#include "objclass.h"
#include "youprop.h"
#include "decl.h"
#include "timeout.h"

extern coord bhitpos;	/* place where throw or zap hits or stops */

/* types of calls to bhit() */
#define ZAPPED_WAND	0
#define THROWN_WEAPON	1
#define KICKED_WEAPON	2
#define FLASHED_LIGHT	3
#define INVIS_BEAM	4

#include "trap.h"
#include "flag.h"
#include "rm.h"
#include "vision.h"
#include "display.h"
#include "engrave.h"
#include "rect.h"
#include "region.h"
#include "monuse.h"
#include "extern.h"
#include "magic.h"
#include "winprocs.h"
#include "rnd.h"

#define NO_SPELL	0

/* flags to control makemon() */
#define NO_MM_FLAGS	  0x00	/* use this rather than plain 0 */
#define NO_MINVENT	  0x01	/* suppress minvent when creating mon */
#define MM_NOWAIT	  0x02	/* don't set STRAT_WAITMASK flags */
#define MM_EDOG		  0x04	/* add edog structure */
#define MM_EMIN		  0x08	/* add emin structure */
#define MM_ANGRY	  0x10  /* monster is created angry */
#define MM_NONAME	  0x20  /* monster is not christened */
#define MM_NOCOUNTBIRTH	  0x40  /* don't increment born counter (for revival) */
#define MM_IGNOREWATER	  0x80	/* ignore water when positioning */
#define MM_ADJACENTOK	  0x100 /* it is acceptable to use adjacent coordinates */

/* special mhpmax value when loading bones monster to flag as extinct or genocided */
#define DEFUNCT_MONSTER	(-100)

/* flags for special ggetobj status returns */
#define ALL_FINISHED	  0x01  /* called routine already finished the job */

/* flags to control query_objlist() */
#define BY_NEXTHERE	  0x1	/* follow objlist by nexthere field */
#define AUTOSELECT_SINGLE 0x2	/* if only 1 object, don't ask */
#define USE_INVLET	  0x4	/* use object's invlet */
#define INVORDER_SORT	  0x8	/* sort objects by packorder */
#define SIGNAL_NOMENU	  0x10	/* return -1 rather than 0 if none allowed */
#define SIGNAL_ESCAPE	  0x20	/* return -2 rather than 0 if menu escaped */
#define FEEL_COCKATRICE	  0x40	/* engage cockatrice checks and react */
#define SHOW_MAGIC_CHEST  0x80	/* show 'magic chest' as the first entry */

/* Flags to control query_category() */
/* BY_NEXTHERE used by query_category() too, so skip 0x01 */
#define UNPAID_TYPES 0x02
#define GOLD_TYPES   0x04
#define WORN_TYPES   0x08
#define ALL_TYPES    0x10
#define BILLED_TYPES 0x20
#define CHOOSE_ALL   0x40
#define BUC_BLESSED  0x80
#define BUC_CURSED   0x100
#define BUC_UNCURSED 0x200
#define BUC_UNKNOWN  0x400
#define UNIDENTIFIED 0x800
#define BUC_ALLBKNOWN (BUC_BLESSED|BUC_CURSED|BUC_UNCURSED)
#define ALL_TYPES_SELECTED -2

/* Flags to control find_mid() */
#define FM_FMON	       0x01	/* search the level->monlist chain */
#define FM_MIGRATE     0x02	/* search the migrating monster chain */
#define FM_MYDOGS      0x04	/* search mydogs */
#define FM_EVERYWHERE  (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control dotrap() in trap.c */
#define NOWEBMSG	0x01	/* suppress stumble into web message */
#define FORCEBUNGLE	0x02	/* adjustments appropriate for bungling */
#define RECURSIVETRAP	0x04	/* trap changed into another type this same turn */

/* Flags to control test_move in hack.c */
#define DO_MOVE		0	/* really doing the move */
#define TEST_MOVE	1	/* test a normal move (move there next) */
#define TEST_TRAV	2	/* test a future travel location */
#define TEST_TRAP	3	/* check if a future travel location is a trap */

/*** some utility macros ***/
#define yn(query) yn_function(query,ynchars, 'n')
#define ynq(query) yn_function(query,ynqchars, 'q')
#define ynaq(query) yn_function(query,ynaqchars, 'y')
#define nyaq(query) yn_function(query,ynaqchars, 'n')

/* Macros for scatter */
#define VIS_EFFECTS	0x01	/* display visual effects */
#define MAY_HITMON	0x02	/* objects may hit monsters */
#define MAY_HITYOU	0x04	/* objects may hit you */
#define MAY_HIT		(MAY_HITMON|MAY_HITYOU)
#define MAY_DESTROY	0x08	/* objects may be destroyed at random */
#define MAY_FRACTURE	0x10	/* boulders & statues may fracture */

/* Macros for launching objects */
#define ROLL		0x01	/* the object is rolling */
#define FLING		0x02	/* the object is flying thru the air */
#define LAUNCH_UNSEEN	0x40	/* hero neither caused nor saw it */
#define LAUNCH_KNOWN	0x80	/* the hero caused this by explicit action */

/* Macros for explosion types */
#define EXPL_DARK	0
#define EXPL_NOXIOUS	1
#define EXPL_MUDDY	2
#define EXPL_WET	3
#define EXPL_MAGICAL	4
#define EXPL_FIERY	5
#define EXPL_FROSTY	6
#define EXPL_MAX	7

/* Zap types and macros, mostly for zap.c. */
/* Ensure that flash_types[] in zap.c matches these! */
#define ZT_MAGIC_MISSILE	(AD_MAGM-1)
#define ZT_FIRE			(AD_FIRE-1)
#define ZT_COLD			(AD_COLD-1)
#define ZT_SLEEP		(AD_SLEE-1)
#define ZT_DEATH		(AD_DISN-1)	/* or disintegration */
#define ZT_LIGHTNING		(AD_ELEC-1)
#define ZT_POISON_GAS		(AD_DRST-1)
#define ZT_ACID			(AD_ACID-1)
/* 8 and 9 are currently unassigned */

#define ZT_WAND(x)		(x)
#define ZT_SPELL(x)		(10+(x))
#define ZT_BREATH(x)		(20+(x))

#define is_hero_spell(type)	((type) >= 10 && (type) < 20)

/* Macros for messages referring to hands, eyes, feet, etc... */
#define ARM 0
#define EYE 1
#define FACE 2
#define FINGER 3
#define FINGERTIP 4
#define FOOT 5
#define HAND 6
#define HANDED 7
#define HEAD 8
#define LEG 9
#define LIGHT_HEADED 10
#define NECK 11
#define SPINE 12
#define TOE 13
#define HAIR 14
#define BLOOD 15
#define LUNG 16
#define NOSE 17
#define STOMACH 18

/* Constants to control matching in filtered object menus */
#define OBJMATCH_LOOSE	 0
#define OBJMATCH_TIGHT	 1

/* Flags to control menus */
#define MENU_PARTIAL	 2
#define MENU_FULL	 3

/* sizes for base64 string buffers, with extra for potential compression overhead */
#define ENCBUFSZ	512	/* > ceil( BUFSZ/3) * 4 == 344 */
#define EQBUFSZ		256	/* > ceil(QBUFSZ/3) * 4 == 172 */

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#define plur(x) (((x) == 1) ? "" : "s")

#define makeknown(x)	discover_object((x),TRUE,TRUE)
#define distu(xx,yy)	dist2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)
#define onlineu(xx,yy)	online2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)

#define rn1(x,y)	(rn2(x)+(y))

/* negative armor class is randomly weakened to prevent invulnerability */
#define AC_VALUE(AC)	((AC) >= 0 ? (AC) : -rnd(-(AC)))

/* Byte swapping macros for the savefile code to make endian-independent saves.
 * Many systems have such macros, but using different names every time.
 * Defining out own is easier than figuring that out.
 */
#define _byteswap16(x) ((((x) & 0x00ffU) << 8) | \
                        (((x) & 0xff00U) >> 8))

#define _byteswap32(x) ((((x) & 0x000000ffU) << 24) | \
                        (((x) & 0x0000ff00U) <<  8) | \
                        (((x) & 0x00ff0000U) >>  8) | \
                        (((x) & 0xff000000U) >> 24))

#define _byteswap64(x) ((((x) & 0x00000000000000ffULL) << 56) | \
			(((x) & 0x000000000000ff00ULL) << 40) | \
			(((x) & 0x0000000000ff0000ULL) << 24) | \
			(((x) & 0x00000000ff000000ULL) <<  8) | \
			(((x) & 0x000000ff00000000ULL) >>  8) | \
			(((x) & 0x0000ff0000000000ULL) >> 24) | \
			(((x) & 0x00ff000000000000ULL) >> 40) | \
			(((x) & 0xff00000000000000ULL) >> 56))

/* If endian.h exists (on Linux for example and perhaps on other UNIX) and is
 * indirectly included via the system headers, we may be able to find out what
 * the endianness is.  Otherwise define IS_BIG_ENDIAN in config.h */
#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN
# define IS_BIG_ENDIAN
#endif

/* 
 * every api function that might ultimately call panic() must use
 * api_entry_checkpoint / api_exit
 * this MUST be a macro: stack values get clobbered; this includes the return address
 */
#define api_entry_checkpoint() \
    (exit_jmp_buf_valid++ ? 1 : nh_setjmp(exit_jmp_buf) ? 0 : 1)
    
#define api_exit() do {--exit_jmp_buf_valid; } while(0)

#endif /* HACK_H */

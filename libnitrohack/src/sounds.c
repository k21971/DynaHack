/*  Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"

static int domonnoise(struct monst *);
static int dochat(void);
static int mon_in_room(struct monst *,int);

/* this easily could be a macro, but it might overtax dumb compilers */
static int mon_in_room(struct monst *mon, int rmtyp)
{
    int rno = level->locations[mon->mx][mon->my].roomno;

    return level->rooms[rno - ROOMOFFSET].rtype == rmtyp;
}

void dosounds(void)
{
    struct mkroom *sroom;
    int hallu, vx, vy;
    struct monst *mtmp;

    if (!flags.soundok || u.uswallow || Underwater) return;

    if (level->sounds && !rn2(level->sounds->freq)) {
        int idx = rn2(level->sounds->n_sounds);
        char *buf;
        struct lvl_sound_bite *snd = &level->sounds->sounds[idx];
        buf = string_subst(snd->msg);
        switch (snd->flags) {
        default:
        case LVLSND_HEARD:  You_hear(buf);          break;
        case LVLSND_PLINED: pline(buf);         break;
        case LVLSND_VERBAL: verbalize(buf);         break;
        case LVLSND_FELT:   pline("You feel %s", buf);  break;
        }
    }

    hallu = Hallucination ? 1 : 0;

    if (level->flags.nfountains && !rn2(400)) {
        static const char * const fountain_msg[4] = {
                                                     "bubbling water.",
                                                     "water falling on coins.",
                                                     "the splashing of a naiad.",
                                                     "a soda fountain!",
        };
        You_hear(fountain_msg[rn2(3)+hallu]);
    }

    if (level->flags.nsinks && !rn2(300)) {
        static const char * const sink_msg[3] = {
                                                 "a slow drip.",
                                                 "a gurgling noise.",
                                                 "dishes being washed!",
        };
        You_hear(sink_msg[rn2(2)+hallu]);
    }

    if (level->flags.has_court && !rn2(200)) {
        static const char * const throne_msg[4] = {
                                                   "the tones of courtly conversation.",
                                                   "a sceptre pounded in judgment.",
                                                   "Someone shouts \"Off with %s head!\"",
                                                   "Queen Beruthiel's cats!",
        };
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if ((mtmp->msleeping ||
                 is_lord(mtmp->data) || is_prince(mtmp->data)) &&
                !is_animal(mtmp->data) &&
                mon_in_room(mtmp, COURT)) {
                /* finding one is enough, at least for now */
                int which = rn2(3)+hallu;

                if (which != 2) You_hear(throne_msg[which]);
                else        pline(throne_msg[2], uhis());
                return;
            }
        }
    }
    if (level->flags.has_garden && !rn2(200)) {
        static const char * const garden_msg[4] = {
                                                   "crickets chirping.",
                                                   "birds singing.",
                                                   "grass growing!",
                                                   "wind in the willows!",
        };
        You_hear(garden_msg[rn2(2) + 2 * hallu]);
        return;
    }
    if (level->flags.has_swamp && !rn2(200)) {
        static const char * const swamp_msg[3] = {
                                                  "You hear mosquitoes!",
                                                  "You smell marsh gas!", /* so it's a smell...*/
                                                  "You hear Donald Duck!",
        };
        pline(swamp_msg[rn2(2)+hallu]);
        return;
    }
    if (level->flags.has_vault && !rn2(200)) {
        if (!(sroom = search_special(level, VAULT))) {
            /* strange ... */
            level->flags.has_vault = 0;
            return;
        }
        if (gd_sound())
            switch (rn2(2)+hallu) {
            case 1: {
                boolean gold_in_vault = FALSE;

                for (vx = sroom->lx;vx <= sroom->hx; vx++)
                    for (vy = sroom->ly; vy <= sroom->hy; vy++)
                        if (gold_at(level, vx, vy))
                            gold_in_vault = TRUE;
                if (vault_occupied(u.urooms) !=
                    (ROOM_INDEX(sroom) + ROOMOFFSET))
                    {
                        if (gold_in_vault)
                            You_hear(!hallu ? "someone counting money." :
                                     "the quarterback calling the play.");
                        else
                            You_hear("someone searching.");
                        break;
                    }
                /* fall into... (yes, even for hallucination) */
            }
            case 0:
                You_hear("the footsteps of a guard on patrol.");
                break;
            case 2:
                You_hear("Ebenezer Scrooge!");
                break;
            }
        return;
    }
    if (level->flags.has_beehive && !rn2(200)) {
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data)) &&
                mon_in_room(mtmp, BEEHIVE)) {
                switch (rn2(2)+hallu) {
                case 0:
                    You_hear("a low buzzing.");
                    break;
                case 1:
                    You_hear("an angry drone.");
                    break;
                case 2:
                    You_hear("bees in your %sbonnet!",
                             uarmh ? "" : "(nonexistent) ");
                    break;
                }
                return;
            }
        }
    }
    if (level->flags.has_lemurepit && !rn2(20)) {
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if (mtmp->data == &mons[PM_LEMURE] &&
                mon_in_room(mtmp, LEMUREPIT)) {
                if (hallu) {
                    switch (rn2(3)) {
                    case 0:
                        You_hear("screams of lust!");
                        break;
                    case 1:
                        You_hear("the crack of your mistress's whip!");
                        break;
                    case 2:
                        You_hear("a weeping willow!");
                        break;
                    }
                } else {
                    switch (rn2(6)) {
                    case 0:
                        You_hear("the crack of a barbed whip!");
                        break;
                    case 1:
                        You_hear("the screams of tortured souls!");
                        break;
                    case 2:
                        You_hear("a wail of eternal anguish!");
                        break;
                    case 3:
                        You_hear("diabolical laughter!");
                        break;
                    case 4:
                        You_hear("cries of repentance!");
                        break;
                    case 5:
                        You_hear("futile pleas for mercy!");
                        break;
                    }
                }
                return;
            }
        }
    }
    if (level->flags.has_morgue && !rn2(200)) {
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if (is_undead(mtmp->data) &&
                mon_in_room(mtmp, MORGUE)) {
                switch (rn2(2)+hallu) {
                case 1:
                    if (!strcmp(body_part(HAIR), "hair")) {
                        pline("The %s on the back of your %s stands up.",
                              body_part(HAIR), body_part(NECK));
                        break;
                    }
                    /* fall through */
                case 2:
                    if (!strcmp(body_part(HAIR), "hair")) {
                        pline("The %s on your %s seems to stand up.",
                              body_part(HAIR), body_part(HEAD));
                        break;
                    }
                    /* fall through */
                case 0:
                    pline("You suddenly realize it is unnaturally quiet.");
                    break;
                }
                return;
            }
        }
    }
    if (level->flags.has_barracks && !rn2(200)) {
        static const char * const barracks_msg[4] = {
                                                     "blades being honed.",
                                                     "loud snoring.",
                                                     "dice being thrown.",
                                                     "General MacArthur!",
        };
        int count = 0;

        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if (is_mercenary(mtmp->data) &&
                mon_in_room(mtmp, BARRACKS) &&
                /* sleeping implies not-yet-disturbed (usually) */
                (mtmp->msleeping || ++count > 5)) {
                You_hear(barracks_msg[rn2(3)+hallu]);
                return;
            }
        }
    }
    if (level->flags.has_zoo && !rn2(200)) {
        static const char * const zoo_msg[3] = {
                                                "a sound reminiscent of an elephant stepping on a peanut.",
                                                "a sound reminiscent of a seal barking.",
                                                "Doctor Dolittle!",
        };
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp)) continue;
            if ((mtmp->msleeping || is_animal(mtmp->data)) &&
                mon_in_room(mtmp, ZOO)) {
                You_hear(zoo_msg[rn2(2)+hallu]);
                return;
            }
        }
    }
    if (level->flags.has_shop && !rn2(200)) {
        if (!(sroom = search_special(level, ANY_SHOP))) {
            /* strange... */
            level->flags.has_shop = 0;
            return;
        }
        if (tended_shop(sroom) &&
            !strchr(u.ushops, ROOM_INDEX(sroom) + ROOMOFFSET)) {
            static const char * const shop_msg[3] = {
                                                     "someone cursing shoplifters.",
                                                     "the chime of a cash register.",
                                                     "Neiman and Marcus arguing!",
            };
            You_hear(shop_msg[rn2(2)+hallu]);
        }
        return;
    }
    if (Is_oracle_level(&u.uz) && !rn2(400)) {
        /* make sure the Oracle is still here */
        for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon)
            if (!DEADMONSTER(mtmp) && mtmp->data == &mons[PM_ORACLE])
                break;
        /* and don't produce silly effects when she's clearly visible */
        if (mtmp && (hallu || !canseemon(level, mtmp))) {
            static const char * const ora_msg[5] = {
                                                    "a strange wind.",      /* Jupiter at Dodona */
                                                    "convulsive ravings.",  /* Apollo at Delphi */
                                                    "snoring snakes.",      /* AEsculapius at Epidaurus */
                                                    "someone say \"No more woodchucks!\"",
                                                    "a loud ZOT!"       /* both rec.humor.oracle */
            };
            You_hear(ora_msg[rn2(3)+hallu*2]);
        }
        return;
    }
    if (!Is_blackmarket(&u.uz) && at_dgn_entrance(&u.uz, "One-eyed Sam's Market") &&
        !rn2(200)) {
        static const char *blkmar_msg[3] = {
                                            "You hear someone complaining about the prices.",
                                            "Somebody whispers: \"Food rations? Only 900 zorkmids.\"",
                                            "You feel like searching for more gold.",
        };
        pline(blkmar_msg[rn2(2)+hallu]);
    }
}


static const char * const h_sounds[] = {
                                        "beep", "boing", "sing", "belche", "creak", "cough", "rattle",
                                        "ululate", "pop", "jingle", "sniffle", "tinkle", "eep",
                                        "clatter", "hum", "sizzle", "twitter", "wheeze", "rustle",
                                        "honk", "lisp", "yodel", "coo", "burp", "moo", "boom",
                                        "murmur", "oink", "quack", "rumble", "twang", "bellow",
                                        "toot", "gargle", "hoot", "warble",
};

const char *growl_sound(struct monst *mtmp)
{
    const char *ret;

    switch (mtmp->data->msound) {
    case MS_MEW:
    case MS_HISS:
        ret = "hiss";
        break;
    case MS_BARK:
    case MS_GROWL:
        ret = "growl";
        break;
    case MS_ROAR:
        ret = "roar";
        break;
    case MS_BUZZ:
        ret = "buzz";
        break;
    case MS_SQEEK:
        ret = "squeal";
        break;
    case MS_SQAWK:
        ret = "screech";
        break;
    case MS_NEIGH:
        ret = "neigh";
        break;
    case MS_WAIL:
        ret = "wail";
        break;
    case MS_SILENT:
        ret = "commotion";
        break;
    default:
        ret = "scream";
    }
    return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void growl(struct monst *mtmp)
{
    const char *growl_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        growl_verb = growl_sound(mtmp);
    if (growl_verb) {
        pline("%s %s!", Monnam(mtmp), vtense(NULL, growl_verb));
        if (flags.run) nomul(0, NULL);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void yelp(struct monst *mtmp)
{
    const char *yelp_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
        case MS_MEW:
            yelp_verb = "yowl";
            break;
        case MS_BARK:
        case MS_GROWL:
            yelp_verb = "yelp";
            break;
        case MS_ROAR:
            yelp_verb = "snarl";
            break;
        case MS_SQEEK:
            yelp_verb = "squeal";
            break;
        case MS_SQAWK:
            yelp_verb = "screak";
            break;
        case MS_WAIL:
            yelp_verb = "wail";
            break;
        }
    if (yelp_verb) {
        pline("%s %s!", Monnam(mtmp), vtense(NULL, yelp_verb));
        if (flags.run) nomul(0, NULL);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
}

/* the sounds of distressed pets */
void whimper(struct monst *mtmp)
{
    const char *whimper_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
        case MS_MEW:
        case MS_GROWL:
            whimper_verb = "whimper";
            break;
        case MS_BARK:
            whimper_verb = "whine";
            break;
        case MS_SQEEK:
            whimper_verb = "squeal";
            break;
        }
    if (whimper_verb) {
        pline("%s %s.", Monnam(mtmp), vtense(NULL, whimper_verb));
        if (flags.run) nomul(0, NULL);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
}

/* pet makes "I'm hungry" noises */
void beg(struct monst *mtmp)
{
    if (mtmp->msleeping || !mtmp->mcanmove ||
        !(carnivorous(mtmp->data) || herbivorous(mtmp->data)))
        return;

    /* presumably nearness and soundok checks have already been made */
    if (!is_silent(mtmp->data) && mtmp->data->msound <= MS_ANIMAL)
        domonnoise(mtmp);
    else if (mtmp->data->msound >= MS_HUMANOID) {
        if (!canspotmon(level, mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        verbalize("I'm hungry.");
    }
}

static int domonnoise(struct monst *mtmp)
{
    const char *pline_msg = 0,  /* Monnam(mtmp) will be prepended */
        *verbl_msg = 0; /* verbalize() */
    const struct permonst *ptr = mtmp->data;
    char verbuf[BUFSZ];

    /* presumably nearness and sleep checks have already been made */
    if (!flags.soundok) return 0;
    if (is_silent(ptr)) return 0;

    /* Make sure its your role's quest quardian; adjust if not */
    if (ptr->msound == MS_GUARDIAN && ptr != &pm_guardian) {
        int mndx = monsndx(ptr);
        ptr = &mons[genus(mndx,1)];
    }

    /* be sure to do this before talking; the monster might teleport away, in
     * which case we want to check its pre-teleport position
     */
    if (!canspotmon(level, mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    switch (ptr->msound) {
    case MS_ORACLE:
        return doconsult(mtmp);
    case MS_PRIEST:
        priest_talk(mtmp);
        break;
    case MS_LEADER:
    case MS_NEMESIS:
    case MS_GUARDIAN:
        quest_chat(mtmp);
        break;
    case MS_SELL: /* pitch, pay, total */
        shk_chat(mtmp);
        break;
    case MS_VAMPIRE:
        {
            /* vampire messages are varied by tameness, peacefulness, and time of night */
            boolean isnight = night();
            boolean kindred = maybe_polyd(u.umonnum == PM_VAMPIRE ||
                                          u.umonnum == PM_VAMPIRE_LORD,
                                          Race_if(PM_VAMPIRE));
            boolean nightchild = (Upolyd && (u.umonnum == PM_WOLF ||
                                             u.umonnum == PM_WINTER_WOLF ||
                                             u.umonnum == PM_WINTER_WOLF_CUB));
            const char *racenoun = (flags.female && urace.individual.f) ?
                urace.individual.f : (urace.individual.m) ?
                urace.individual.m : urace.noun;

            if (mtmp->mtame) {
                if (kindred) {
                    sprintf(verbuf, "Good %s to you Master%s",
                            isnight ? "evening" : "day",
                            isnight ? "!" : ".  Why do we not rest?");
                    verbl_msg = verbuf;
                } else {
                    sprintf(verbuf,"%s%s",
                            nightchild ? "Child of the night, " : "",
                            midnight() ?
                            "I can stand this craving no longer!" :
                            isnight ?
                            "I beg you, help me satisfy this growing craving!" :
                            "I find myself growing a little weary.");
                    verbl_msg = verbuf;
                }
            } else if (mtmp->mpeaceful) {
                if (kindred && isnight) {
                    sprintf(verbuf, "Good feeding %s!",
                            flags.female ? "sister" : "brother");
                    verbl_msg = verbuf;
                } else if (nightchild && isnight) {
                    sprintf(verbuf,
                            "How nice to hear you, child of the night!");
                    verbl_msg = verbuf;
                } else
                    verbl_msg = "I only drink... potions.";
            } else {
                int vampindex;
                static const char * const vampmsg[] = {
                                                       /* These first two (0 and 1) are specially handled below */
                                                       "I vant to suck your %s!",
                                                       "I vill come after %s without regret!",
                                                       /* other famous vampire quotes can follow here if desired */
                };
                if (kindred)
                    verbl_msg = "This is my hunting ground that you dare to prowl!";
                else if (youmonst.data == &mons[PM_SILVER_DRAGON] ||
                         youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
                    /* Silver dragons are silver in color, not made of silver */
                    sprintf(verbuf, "%s! Your silver sheen does not frighten me!",
                            youmonst.data == &mons[PM_SILVER_DRAGON] ?
                            "Fool" : "Young Fool");
                    verbl_msg = verbuf;
                } else {
                    vampindex = rn2(SIZE(vampmsg));
                    if (vampindex == 0) {
                        sprintf(verbuf, vampmsg[vampindex], body_part(BLOOD));
                        verbl_msg = verbuf;
                    } else if (vampindex == 1) {
                        sprintf(verbuf, vampmsg[vampindex],
                                Upolyd ? an(mons_mname(&mons[u.umonnum])) : an(racenoun));
                        verbl_msg = verbuf;
                    } else
                        verbl_msg = vampmsg[vampindex];
                }
            }
        }
        break;
    case MS_WERE:
        if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
            pline("%s throws back %s head and lets out a blood curdling %s!",
                  Monnam(mtmp), mhis(level, mtmp),
                  ptr == &mons[PM_HUMAN_WERERAT] ? "shriek" : "howl");
            wake_nearto(mtmp->mx, mtmp->my, 11*11);
        } else
            pline_msg =
                "whispers inaudibly.  All you can make out is \"moon\".";
        break;
    case MS_BARK:
        if (flags.moonphase == FULL_MOON && night()) {
            pline_msg = "howls.";
        } else if (mtmp->mpeaceful) {
            if (mtmp->mtame &&
                (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
                 moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
                pline_msg = "whines.";
            else if (mtmp->mtame && EDOG(mtmp)->hungrytime > moves + 1000)
                pline_msg = "yips.";
            else {
                if (mtmp->data != &mons[PM_DINGO])  /* dingos do not actually bark */
                    pline_msg = "barks.";
            }
        } else {
            pline_msg = "growls.";
        }
        break;
    case MS_MEW:
        if (mtmp->mtame) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
                mtmp->mtame < 5)
                pline_msg = "yowls.";
            else if (moves > EDOG(mtmp)->hungrytime)
                pline_msg = "meows.";
            else if (EDOG(mtmp)->hungrytime > moves + 1000)
                pline_msg = "purrs.";
            else
                pline_msg = "mews.";
            break;
        } /* else FALLTHRU */
    case MS_GROWL:
        if (mtmp->mtame &&
            (mtmp->data == &mons[PM_MONKEY] ||
             mtmp->data == &mons[PM_APE] ||
             mtmp->data == &mons[PM_CARNIVOROUS_APE])) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
                moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5) {
                pline_msg = "shrieks.";
                wake_nearto(mtmp->mx, mtmp->my, 8*8);
            } else if (EDOG(mtmp)->hungrytime > moves + 1000) {
                pline_msg = "chatters.";
            } else {
                pline_msg = "hoots.";
            }
        } else {
            pline_msg = mtmp->mpeaceful ? "snarls." : "growls!";
        }
        break;
    case MS_ROAR:
        pline_msg = mtmp->mpeaceful ? "snarls." : "roars!";
        break;
    case MS_SQEEK:
        pline_msg = "squeaks.";
        break;
    case MS_SQAWK:
        if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful)
            verbl_msg = "Nevermore!";
        else
            pline_msg = "squawks.";
        break;
    case MS_HISS:
        if (!mtmp->mpeaceful)
            pline_msg = "hisses!";
        else return 0;  /* no sound */
        break;
    case MS_BUZZ:
        pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily.";
        break;
    case MS_GRUNT:
        pline_msg = "grunts.";
        break;
    case MS_NEIGH:
        if (mtmp->mtame < 5)
            pline_msg = "neighs.";
        else if (moves > EDOG(mtmp)->hungrytime)
            pline_msg = "whinnies.";
        else
            pline_msg = "whickers.";
        break;
    case MS_WAIL:
        pline_msg = "wails mournfully.";
        break;
    case MS_GURGLE:
        pline_msg = "gurgles.";
        break;
    case MS_BURBLE:
        pline_msg = "burbles.";
        break;
    case MS_SHRIEK:
        pline_msg = "shrieks.";
        aggravate();
        break;
    case MS_IMITATE:
        pline_msg = "imitates you.";
        break;
    case MS_BONES:
        pline("%s rattles noisily.", Monnam(mtmp));
        pline("You freeze for a moment.");
        nomul(-2, "scared by rattling");
        break;
    case MS_LAUGH:
        {
            static const char * const laugh_msg[4] = {
                                                      "giggles.", "chuckles.", "snickers.", "laughs.",
            };
            pline_msg = laugh_msg[rn2(4)];
        }
        break;
    case MS_MUMBLE:
        pline_msg = "mumbles incomprehensibly.";
        break;
    case MS_DJINNI:
        if (mtmp->mtame) {
            verbl_msg = "Sorry, I'm all out of wishes.";
        } else if (mtmp->mpeaceful) {
            if (ptr == &mons[PM_WATER_DEMON])
                pline_msg = "gurgles.";
            else
                verbl_msg = "I'm free!";
        } else verbl_msg = "This will teach you not to disturb me!";
        break;
    case MS_BOAST:  /* giants */
        if (!mtmp->mpeaceful) {
            switch (rn2(4)) {
            case 0: pline("%s boasts about %s gem collection.",
                          Monnam(mtmp), mhis(level, mtmp));
                break;
            case 1: pline_msg = "complains about a diet of mutton.";
                break;
            default: pline_msg = "shouts \"Fee Fie Foe Foo!\" and guffaws.";
                wake_nearto(mtmp->mx, mtmp->my, 7*7);
                break;
            }
            break;
        }
        /* else FALLTHRU */
    case MS_HUMANOID:
        if (!mtmp->mpeaceful) {
            if (In_endgame(&u.uz) && is_mplayer(ptr)) {
                mplayer_talk(mtmp);
                break;
            } else return 0;    /* no sound */
        }
        /* Generic peaceful humanoid behaviour. */
        if (mtmp->mflee)
            pline_msg = "wants nothing to do with you.";
        else if (mtmp->mhp < mtmp->mhpmax/4)
            pline_msg = "moans.";
        else if (mtmp->mconf || mtmp->mstun)
            verbl_msg = !rn2(3) ? "Huh?" : rn2(2) ? "What?" : "Eh?";
        else if (!mtmp->mcansee)
            verbl_msg = "I can't see!";
        else if (mtmp->mtrapped) {
            struct trap *t = t_at(level, mtmp->mx, mtmp->my);

            if (t) t->tseen = 1;
            verbl_msg = "I'm trapped!";
        } else if (mtmp->mhp < mtmp->mhpmax/2)
            pline_msg = "asks for a potion of healing.";
        else if (mtmp->mtame && !mtmp->isminion &&
                 moves > EDOG(mtmp)->hungrytime)
            verbl_msg = "I'm hungry.";
        /* Specific monsters' interests */
        else if (is_elf(ptr))
            pline_msg = "curses orcs.";
        else if (is_dwarf(ptr))
            pline_msg = "talks about mining.";
        else if (likes_magic(ptr))
            pline_msg = "talks about spellcraft.";
        else if (ptr->mlet == S_CENTAUR)
            pline_msg = "discusses hunting.";
        else switch (monsndx(ptr)) {
            case PM_HOBBIT:
                pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10) ?
                    "complains about unpleasant dungeon conditions."
                    : "asks you about the One Ring.";
                break;
            case PM_ARCHEOLOGIST:
                pline_msg = "describes a recent article in \"Spelunker Today\" magazine.";
                break;
            case PM_TOURIST:
                verbl_msg = "Aloha.";
                break;
            default:
                pline_msg = "discusses dungeon exploration.";
                break;
            }
        break;
    case MS_SEDUCE:
        if (ptr->mlet != S_NYMPH && flags.seduce_enabled &&
            could_seduce(mtmp, &youmonst, NULL) == 1) {
            doseduce(mtmp);
            break;
        }

        switch ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0)
            {
            case 2:
                verbl_msg = "Hello, sailor.";
                break;
            case 1:
                pline_msg = "comes on to you.";
                break;
            default:
                pline_msg = "cajoles you.";
            }
        break;
    case MS_ARREST:
        if (mtmp->mpeaceful)
            verbalize("Just the facts, %s.",
                      flags.female ? "Ma'am" : "Sir");
        else {
            static const char * const arrest_msg[3] = {
                                                       "Anything you say can be used against you.",
                                                       "You're under arrest!",
                                                       "Stop in the name of the Law!",
            };
            verbl_msg = arrest_msg[rn2(3)];
        }
        break;
    case MS_BRIBE:
        if (monsndx(ptr) == PM_PRISON_GUARD) {
            long gdemand = 500 * u.ulevel;
            long goffer = 0;

            if (!mtmp->mpeaceful && !mtmp->mtame) {
                pline("%s demands %ld %s to avoid re-arrest.",
                      Amonnam(mtmp), gdemand, currency(gdemand));
                if ((goffer = bribe(mtmp)) >= gdemand) {
                    verbl_msg = "Good.  Now beat it, scum!";
                    mtmp->mpeaceful = 1;
                    set_malign(mtmp);
                } else {
                    verbalize("I said %ld!", gdemand);
                    mtmp->mspec_used = 1000;
                }
            } else {
                verbl_msg = "Out of my way, scum!"; /* still a jerk */
            }
            break;
        } else if (mtmp->mpeaceful && !mtmp->mtame) {
            demon_talk(mtmp);
            break;
        }
        /* fall through */
    case MS_CUSS:
        if (!mtmp->mpeaceful)
            cuss(mtmp);
        break;
    case MS_SPELL:
        /* deliberately vague, since it's not actually casting any spell */
        pline_msg = "seems to mutter a cantrip.";
        break;
    case MS_NURSE:
        if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
            verbl_msg = "Put that weapon away before you hurt someone!";
        else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
            verbl_msg = Role_if (PM_HEALER) ?
                "Doc, I can't help you unless you cooperate." :
                "Please undress so I can examine you.";
        else if (uarmu)
            verbl_msg = "Take off your shirt, please.";
        else
            verbl_msg = "Relax, this won't hurt a bit.";
        break;
    case MS_GUARD:
        if (money_cnt(invent))
            verbl_msg = "Please drop that gold and follow me.";
        else
            verbl_msg = "Please follow me.";
        break;
    case MS_SOLDIER:
        {
            static const char * const soldier_foe_msg[3] = {
                                                            "Resistance is useless!",
                                                            "You're dog meat!",
                                                            "Surrender!",
            },        * const soldier_pax_msg[3] = {
                                                    "What lousy pay we're getting here!",
                                                    "The food's not fit for Orcs!",
                                                    "My feet hurt, I've been on them all day!",
            };
            verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
                : soldier_foe_msg[rn2(3)];
        }
        break;
    case MS_RIDER:
        if (ptr == &mons[PM_DEATH] && !rn2(10))
            pline_msg = "is busy reading a copy of Sandman #8.";
        else
            verbl_msg = (ptr == &mons[PM_DEATH]) ?
                "WHO DO YOU THINK YOU ARE, WAR?" :
                "Who do you think you are, War?";
        break;
    }

    if (pline_msg) pline("%s %s", Monnam(mtmp), pline_msg);
    else if (verbl_msg) verbalize(verbl_msg);
    return 1;
}


int dotalk(void)
{
    int result;
    boolean save_soundok = flags.soundok;
    flags.soundok = 1;  /* always allow sounds while chatting */
    result = dochat();
    flags.soundok = save_soundok;
    return result;
}

static int dochat(void)
{
    struct monst *mtmp;
    int tx, ty;
    struct obj *otmp;
    schar dx, dy, dz;
    int mdx, mdy, mon_count;

    if (is_silent(youmonst.data)) {
        pline("As %s, you cannot speak.", an(mons_mname(youmonst.data)));
        return 0;
    }
    if (Strangled) {
        pline("You can't speak.  You're choking!");
        return 0;
    }
    if (u.uswallow) {
        pline("They won't hear you out there.");
        return 0;
    }
    if (Underwater) {
        pline("Your speech is unintelligible underwater.");
        return 0;
    }

    if (!Blind && (otmp = shop_object(u.ux, u.uy)) != NULL) {
        /* standing on something in a shop and chatting causes the shopkeeper
           to describe the price(s).  This can inhibit other chatting inside
           a shop, but that shouldn't matter much.  shop_object() returns an
           object iff inside a shop and the shopkeeper is present and willing
           (not angry) and able (not asleep) to speak and the position contains
           any objects other than just gold.
        */
        price_quote(otmp);
        return 1;
    }

    /* count the monsters around the player */
    mon_count = 0;
    for (mdx = -1; mdx <= 1; mdx++) {
        for (mdy = -1; mdy <= 1; mdy++) {
            if (mdx == 0 && mdy == 0) {
                /* account for steed */
                if (u.usteed) {
                    mon_count++;
                    dx = 0;
                    dy = 0;
                    dz = 1;
                }
                continue;
            }

            mtmp = m_at(level, u.ux + mdx, u.uy + mdy);
            if (mtmp && canspotmon(level, mtmp)) {
                mon_count++;
                dx = mdx;
                dy = mdy;
                dz = 0;
            }
        }
    }

    /* don't ask for a direction if there's only one monster around */
    if ((mon_count != 1 || iflags.paranoid_chat) &&
        !getdir("Talk to whom? (in what direction)", &dx, &dy, &dz)) {
        /* decided not to chat */
        return 0;
    }

    if (u.usteed && dz > 0) {
        if (!u.usteed->mcanmove || u.usteed->msleeping) {
            pline("Your steed remains silent...");
            return 0;
        }
        return domonnoise(u.usteed);
    }
    if (dz) {
        pline("They won't hear you %s there.", dz < 0 ? "up" : "down");
        return 0;
    }

    if (dx == 0 && dy == 0) {
        /*
         * Let's not include this.  It raises all sorts of questions: can you wear
         * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
         * etc...  --KAA
         if (u.umonnum == PM_ETTIN) {
         pline("You discover that your other head makes boring conversation.");
         return 1;
         }
        */
        pline("Talking to yourself is a bad habit for a dungeoneer.");
        return 0;
    }

    tx = u.ux + dx;
    ty = u.uy + dy;
    mtmp = m_at(level, tx, ty);

    if (!mtmp || mtmp->mundetected ||
        mtmp->m_ap_type == M_AP_FURNITURE ||
        mtmp->m_ap_type == M_AP_OBJECT)
        return 0;

    /* sleeping monsters won't talk, except priests (who wake up) */
    if ((!mtmp->mcanmove || mtmp->msleeping) && !mtmp->ispriest) {
        /* If it is unseen, the player can't tell the difference between
           not noticing him and just not existing, so skip the message. */
        if (canspotmon(level, mtmp))
            pline("%s seems not to notice you.", Monnam(mtmp));
        return 0;
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (mtmp->mtame && mtmp->meating) {
        if (!canspotmon(level, mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        pline("%s is eating noisily.", Monnam(mtmp));
        return 0;
    }

    if (Role_if(PM_CONVICT) && is_rat(mtmp->data) &&
        !mtmp->mpeaceful && !mtmp->mtame) {
        int soothe_roll = rnl(10);
        pline("You attempt to soothe the %s with chittering sounds.",
              l_monnam(mtmp));
        if (soothe_roll < 2) {
            tamedog(mtmp, NULL);
        } else if (soothe_roll > 8) {
            pline("%s unfortunately ignores your overtures.", Monnam(mtmp));
        } else {
            mtmp->mpeaceful = 1;
            set_malign(mtmp);
        }
        return 1;
    }

    return domonnoise(mtmp);
}

/*sounds.c*/

/*  Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* DynaHack may be freely redistributed.  See license for details. */

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 */

#include "hack.h"

static void polyman(const char *,const char *);
static void break_armor(void);
static void drop_weapon(int);
static void uunstick(void);
static int armor_to_dragon(int);
static void newman(void);

/* Assumes u.umonster is set up already. */
/* Use u.umonster since we might be restoring and you may be polymorphed. */
void init_uasmon(void)
{
    int i;

    upermonst = mons[u.umonster];

    /* Fix up the flags */
    /* Default flags assume human, so replace with your race's flags. */

    upermonst.mflags1 &= ~(mons[PM_HUMAN].mflags1);
    upermonst.mflags1 |= (mons[urace.malenum].mflags1);

    upermonst.mflags2 &= ~(mons[PM_HUMAN].mflags2);
    upermonst.mflags2 |= (mons[urace.malenum].mflags2);

    upermonst.mflags3 &= ~(mons[PM_HUMAN].mflags3);
    upermonst.mflags3 |= (mons[urace.malenum].mflags3);

    /* Fix up the attacks for vampires */
    /* This replaces the role's attacks with the vampire's race attacks. */
    if (Race_if(PM_VAMPIRE)) {
        for (i = 0; i < NATTK; i++) {
            upermonst.mattk[i] = mons[urace.malenum].mattk[i];
        }
    }

    set_uasmon();
}

/* update the youmonst.data structure pointer */
void set_uasmon(void)
{
    const struct permonst *pm = &mons[u.umonnum];
    if (!Upolyd)
        pm = &upermonst;
    set_mon_data(&youmonst, pm, 0);
}

/* Returns true if the player monster is genocided. */
boolean is_playermon_genocided(void)
{
    return ((mvitals[urole.malenum].mvflags & G_GENOD) ||
            (urole.femalenum != NON_PM &&
             (mvitals[urole.femalenum].mvflags & G_GENOD)) ||
            (mvitals[urace.malenum].mvflags & G_GENOD) ||
            (urace.femalenum != NON_PM &&
             (mvitals[urace.femalenum].mvflags & G_GENOD)));
}

/* make a (new) human out of the player */
static void polyman(const char *fmt, const char *arg)
{
    boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
        was_mimicking = (youmonst.m_ap_type == M_AP_OBJECT);
    boolean could_pass_walls = Passes_walls;
    boolean was_blind = !!Blind;

    if (Upolyd) {
        u.acurr = u.macurr; /* restore old attribs */
        u.amax = u.mamax;
        u.umonnum = u.umonster;
        flags.female = u.mfemale;
    }
    set_uasmon();

    u.mh = u.mhmax = 0;
    u.mtimedone = 0;
    skinback(FALSE);
    u.uundetected = 0;

    if (sticky) uunstick();
    find_ac();
    if (was_mimicking) {
        if (multi < 0) unmul("");
        youmonst.m_ap_type = M_AP_NOTHING;
    }

    newsym(u.ux,u.uy);

    pline(fmt, arg);
    /* check whether player foolishly genocided self while poly'd */
    if (is_playermon_genocided()) {
        /* intervening activity might have clobbered genocide info */
        killer = delayed_killer;
        if (!killer || !strstri(killer, "genocid")) {
            killer_format = KILLED_BY;
            killer = "self-genocide";
        }
        done(GENOCIDED);
    }

    if (u.twoweap && !could_twoweap(youmonst.data))
        untwoweapon();

    if (u.utraptype == TT_PIT) {
        if (could_pass_walls) { /* player forms cannot pass walls */
            u.utrap = rn1(6,2);
        }
    }
    if (was_blind && !Blind) {  /* reverting from eyeless */
        Blinded = 1L;
        make_blinded(0L, TRUE); /* remove blindness */
    }

    if (!Levitation && !u.ustuck &&
        (is_pool(level, u.ux,u.uy) || is_lava(level, u.ux,u.uy)))
        spoteffects(TRUE);

    see_monsters();
}

void change_sex(void)
{
    /* setting u.umonster for caveman/cavewoman or priest/priestess
       swap unintentionally makes `Upolyd' appear to be true */
    boolean already_polyd = (boolean) Upolyd;

    /* Some monsters are always of one sex and their sex can't be changed */
    /* succubi/incubi can change, but are handled below */
    /* !already_polyd check necessary because is_male() and is_female()
       are true if the player is a priest/priestess */
    if (!already_polyd || (!is_male(youmonst.data) && !is_female(youmonst.data) && !is_neuter(youmonst.data)))
        flags.female = !flags.female;
    if (already_polyd)  /* poly'd: also change saved sex */
        u.mfemale = !u.mfemale;
    max_rank_sz();      /* [this appears to be superfluous] */
    if ((already_polyd ? u.mfemale : flags.female) && urole.name.f)
        strcpy(pl_character, urole.name.f);
    else
        strcpy(pl_character, urole.name.m);
    u.umonster = ((already_polyd ? u.mfemale : flags.female) && urole.femalenum != NON_PM) ?
        urole.femalenum : urole.malenum;
    if (!already_polyd) {
        u.umonnum = u.umonster;
    } else if (u.umonnum == PM_SUCCUBUS || u.umonnum == PM_INCUBUS) {
        flags.female = !flags.female;
        /* change monster type to match new sex */
        u.umonnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
        set_uasmon();
    }
}

static void newman(void)
{
    int tmp, oldlvl;

    tmp = u.uhpmax;
    oldlvl = u.ulevel;
    u.ulevel = u.ulevel + rn1(5, -2);
    if (u.ulevel > 127 || u.ulevel < 1) { /* level went below 0? */
        u.ulevel = oldlvl; /* restore old level in case they lifesave */
        goto dead;
    }
    if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;
    /* If your level goes down, your peak level goes down by
       the same amount so that you can't simply use blessed
       full healing to undo the decrease.  But if your level
       goes up, your peak level does *not* undergo the same
       adjustment; you might end up losing out on the chance
       to regain some levels previously lost to other causes. */
    if (u.ulevel < oldlvl) u.ulevelmax -= (oldlvl - u.ulevel);
    if (u.ulevelmax < u.ulevel) u.ulevelmax = u.ulevel;

    if (!rn2(10)) change_sex();

    adjabil(oldlvl, (int)u.ulevel);
    reset_rndmonst(NON_PM); /* new monster generation criteria */

    /* random experience points for the new experience level */
    u.uexp = rndexp(FALSE);

    /* u.uhpmax * u.ulevel / oldlvl: proportionate hit points to new level
     * -10 and +10: don't apply proportionate HP to 10 of a starting
     *   character's hit points (since a starting character's hit points
     *   are not on the same scale with hit points obtained through level
     *   gain)
     * 9 - rn2(19): random change of -9 to +9 hit points
     */
    u.uhpmax = ((u.uhpmax - 10) * (long)u.ulevel / oldlvl + 10) +
        (9 - rn2(19));

    u.uhp = u.uhp * (long)u.uhpmax/tmp;

    tmp = u.uenmax;
    u.uenmax = u.uenmax * (long)u.ulevel / oldlvl + 9 - rn2(19);
    if (u.uenmax < 0)
        u.uenmax = 0;
    u.uen = (tmp ? u.uen * (long)u.uenmax / tmp : u.uenmax);

    redist_attr();
    u.uhunger = rn1(500,500);
    if (Sick)
        make_sick(0L, NULL, FALSE, SICK_ALL);
    Stoned = 0;
    delayed_killer = 0;
    if (u.uhp <= 0 || u.uhpmax <= 0) {
        if (Polymorph_control) {
            if (u.uhp <= 0) u.uhp = 1;
            if (u.uhpmax <= 0) u.uhpmax = 1;
        } else {
        dead: /* we come directly here if their experience level went to 0 or less */
            pline("Your new form doesn't seem healthy enough to survive.");
            killer_format = KILLED_BY_AN;
            killer="unsuccessful polymorph";
            done(DIED);
            newuhs(FALSE);
            return; /* lifesaved */
        }
    }
    newuhs(FALSE);
    polyman("You feel like a new %s!",
            (flags.female && urace.individual.f) ? urace.individual.f :
            (urace.individual.m) ? urace.individual.m : urace.noun);
    if (Slimed) {
        pline("Your body transforms, but there is still slime on you.");
        Slimed = 10L;
    }
    iflags.botl = 1;
    see_monsters();
    encumber_msg();
}

void polyself(boolean forcecontrol)
{
    char buf[BUFSZ];
    int old_light, new_light;
    int mntmp = NON_PM;
    int tries=0;
    boolean draconian = (uarm &&
                         uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
                         uarm->otyp <= CHROMATIC_DRAGON_SCALES);
    boolean iswere = (u.ulycn >= LOW_PM || is_were(youmonst.data));
    boolean isvamp = is_vampire(youmonst.data);
    boolean was_floating = (Levitation || Flying);

    if (!Polymorph_control && !forcecontrol && !draconian && !iswere && !isvamp) {
        if (rn2(20) > ACURR(A_CON)) {
            pline("You shudder for a moment.");
            losehp(rnd(30), "system shock", KILLED_BY_AN);
            exercise(A_CON, FALSE);
            return;
        }
    }
    old_light = Upolyd ? emits_light(youmonst.data) : 0;

    if (Polymorph_control || forcecontrol) {
        do {
            getlin("Become what kind of monster? [type the name]",
                   buf);
            mntmp = name_to_mon(buf);
            if (mntmp < LOW_PM)
                pline("I've never heard of such monsters.");
            /* Note:  humans are illegal as monsters, but an
             * illegal monster forces newman(), which is what we
             * want if they specified a human.... */
            else if (!polyok(&mons[mntmp]) && !your_race(&mons[mntmp]))
                pline("You cannot polymorph into that.");
            else break;
        } while (++tries < 5);
        if (tries==5) pline("That's enough tries!");
        /* allow skin merging, even when polymorph is controlled */
        if (draconian &&
            (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
            goto do_merge;
    } else if (draconian || iswere || isvamp) {
        /* special changes that don't require polyok() */
        if (draconian) {
        do_merge:
            mntmp = armor_to_dragon(uarm->otyp);
            if (!(mvitals[mntmp].mvflags & G_GENOD)) {
                /* allow G_EXTINCT */
                pline("You merge with your scaly armor.");
                uskin = uarm;
                uarm = NULL;
                /* save/restore hack */
                uskin->owornmask |= I_SPECIAL;
            }
        } else if (iswere) {
            if (is_were(youmonst.data))
                mntmp = PM_HUMAN; /* Illegal; force newman() */
            else
                mntmp = u.ulycn;
        } else if (isvamp) {
            if (u.umonnum != PM_VAMPIRE_BAT)
                mntmp = PM_VAMPIRE_BAT;
            else
                mntmp = PM_HUMAN; /* newman() */
        }
        /* if polymon fails, "you feel" message has been given
           so don't follow up with another polymon or newman */
        if (mntmp == PM_HUMAN) newman();    /* werecritter */
        else polymon(mntmp);
        goto made_change;    /* maybe not, but this is right anyway */
    }

    if (mntmp < LOW_PM) {
        tries = 0;
        do {
            /* randomly pick an "ordinary" monster */
            mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
        } while ((!polyok(&mons[mntmp]) || is_placeholder(&mons[mntmp]))
                 && tries++ < 200);
    }

    /* The below polyok() fails either if everything is genocided, or if
     * we deliberately chose something illegal to force newman().
     */
    if (!polyok(&mons[mntmp]) || (!rn2(5) && !forcecontrol) || your_race(&mons[mntmp]))
        newman();
    else if (!polymon(mntmp)) return;

    if (!uarmg) selftouch("No longer petrify-resistant, you");

 made_change:
    new_light = Upolyd ? emits_light(youmonst.data) : 0;
    if (old_light != new_light) {
        if (old_light)
            del_light_source(level, LS_MONSTER, &youmonst);
        if (new_light == 1) ++new_light;  /* otherwise it's undetectable */
        if (new_light)
            new_light_source(level, u.ux, u.uy, new_light,
                             LS_MONSTER, &youmonst);
    }
    if (is_pool(level, u.ux,u.uy) && was_floating && !(Levitation || Flying) &&
        !breathless(youmonst.data) && !amphibious(youmonst.data) &&
        !Swimming) drown();
}

/* (try to) make a mntmp monster out of the player */
/* returns 1 if polymorph successful */
int polymon(int mntmp)
{
    boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
        was_blind = !!Blind, dochange = FALSE;
    boolean could_pass_walls = Passes_walls;
    int mlvl;

    if (mvitals[mntmp].mvflags & G_GENOD) { /* allow G_EXTINCT */
        pline("You feel rather %s-ish.", mons_mname(&mons[mntmp]));
        exercise(A_WIS, TRUE);
        return 0;
    }

    /* KMH, conduct */
    u.uconduct.polyselfs++;

    if (!Upolyd) {
        /* Human to monster; save human stats */
        u.macurr = u.acurr;
        u.mamax = u.amax;
        u.mfemale = flags.female;
    } else {
        /* Monster to monster; restore human stats, to be
         * immediately changed to provide stats for the new monster
         */
        u.acurr = u.macurr;
        u.amax = u.mamax;
        flags.female = u.mfemale;
    }

    if (youmonst.m_ap_type) {
        /* stop mimicking immediately */
        if (multi < 0) unmul("");
    } else if (mons[mntmp].mlet != S_MIMIC) {
        /* as in polyman() */
        youmonst.m_ap_type = M_AP_NOTHING;
    }
    if (is_male(&mons[mntmp])) {
        if (flags.female) dochange = TRUE;
    } else if (is_female(&mons[mntmp])) {
        if (!flags.female) dochange = TRUE;
    } else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
        if (!rn2(10)) dochange = TRUE;
    }
    if (dochange) {
        flags.female = !flags.female;
        pline("You %s %s%s!",
              (u.umonnum != mntmp) ? "turn into a" : "feel like a new",
              (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? "" :
              flags.female ? "female " : "male ",
              mons_mname(&mons[mntmp]));
    } else {
        if (u.umonnum != mntmp)
            pline("You turn into %s!", an(mons_mname(&mons[mntmp])));
        else
            pline("You feel like a new %s!", mons_mname(&mons[mntmp]));
    }
    if (Stoned && poly_when_stoned(&mons[mntmp])) {
        /* poly_when_stoned already checked stone golem genocide */
        pline("You turn to stone!");
        mntmp = PM_STONE_GOLEM;
        Stoned = 0;
        delayed_killer = 0;
    }

    u.mtimedone = rn1(500, 500);
    u.umonnum = mntmp;
    set_uasmon();

    /* New stats for monster, to last only as long as polymorphed.
     * Currently only strength gets changed.
     */
    if (strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = STR18(100);

    if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
        Stoned = 0;
        delayed_killer = 0;
        pline("You no longer seem to be petrifying.");
    }
    if (Sick_resistance && Sick) {
        make_sick(0L, NULL, FALSE, SICK_ALL);
        pline("You no longer feel sick.");
    }
    if (Slimed) {
        if (flaming(youmonst.data)) {
            pline("The slime burns away!");
            Slimed = 0L;
            iflags.botl = 1;
        } else if (mntmp == PM_GREEN_SLIME) {
            /* do it silently */
            Slimed = 0L;
            iflags.botl = 1;
        }
    }
    if (nohands(youmonst.data)) Glib = 0;

    /*
      mlvl = adj_lev(&mons[mntmp]);
      * We can't do the above, since there's no such thing as an
      * "experience level of you as a monster" for a polymorphed character.
      */
    mlvl = (int)mons[mntmp].mlevel;
    if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
        u.mhmax = In_endgame(&u.uz) ? (8*mlvl) : (4*mlvl + dice(mlvl,4));
    } else if (is_golem(youmonst.data)) {
        u.mhmax = golemhp(mntmp);
    } else {
        if (!mlvl) u.mhmax = rnd(4);
        else u.mhmax = dice(mlvl, 8);
        if (is_home_elemental(&u.uz, &mons[mntmp])) u.mhmax *= 3;
    }
    u.mh = u.mhmax;

    if (u.ulevel < mlvl) {
        /* Low level characters can't become high level monsters for long */
        u.mtimedone = u.mtimedone * u.ulevel / mlvl;
    }

    if (uskin && mntmp != armor_to_dragon(uskin->otyp))
        skinback(FALSE);
    break_armor();
    drop_weapon(1);
    if (hides_under(youmonst.data))
        u.uundetected = OBJ_AT(u.ux, u.uy);
    else if (youmonst.data->mlet == S_EEL)
        u.uundetected = is_pool(level, u.ux, u.uy);
    else
        u.uundetected = 0;

    if (u.utraptype == TT_PIT) {
        if (could_pass_walls && !Passes_walls) {
            u.utrap = rn1(6,2);
        } else if (!could_pass_walls && Passes_walls) {
            u.utrap = 0;
        }
    }
    if (was_blind && !Blind) {  /* previous form was eyeless */
        Blinded = 1L;
        make_blinded(0L, TRUE); /* remove blindness */
    }
    newsym(u.ux,u.uy);      /* Change symbol */

    if (!sticky && !u.uswallow && u.ustuck && sticks(youmonst.data)) {
        u.ustuck = 0;
        u.uwilldrown = 0;
    } else if (sticky && !sticks(youmonst.data)) {
        uunstick();
    }

    if (u.usteed) {
        if (touch_petrifies(u.usteed->data) && !Stone_resistance && rnl(3)) {
            char kbuf[BUFSZ];

            pline("No longer petrifying-resistant, you touch %s.",
                  mon_nam(u.usteed));
            sprintf(kbuf, "riding %s", an(mons_mname(u.usteed->data)));
            if (delayed_petrify(NULL, kbuf))
                dismount_steed(DISMOUNT_FELL);
        }
        if (u.usteed && !can_ride(u.usteed))
            dismount_steed(DISMOUNT_POLY);
    }

    if (flags.verbose) {
        static const char use_thec[] = "Use the command #%s to %s.";
        static const char monsterc[] = "monster";
        if (can_breathe(youmonst.data))
            pline(use_thec,monsterc,"use your breath weapon");
        if (attacktype(youmonst.data, AT_SPIT))
            pline(use_thec,monsterc,"spit venom");
        if (youmonst.data->mlet == S_NYMPH)
            pline(use_thec,monsterc,"remove an iron ball");
        if (attacktype(youmonst.data, AT_GAZE))
            pline(use_thec,monsterc,"gaze at monsters");
        if (is_hider(youmonst.data))
            pline(use_thec,monsterc,"hide");
        if (is_were(youmonst.data))
            pline(use_thec,monsterc,"summon help");
        if (webmaker(youmonst.data))
            pline(use_thec,monsterc,"spin a web");
        if (u.umonnum == PM_GREMLIN)
            pline(use_thec,monsterc,"multiply in a fountain");
        if (is_unicorn(youmonst.data))
            pline(use_thec,monsterc,"use your horn");
        if (is_mind_flayer(youmonst.data))
            pline(use_thec,monsterc,"emit a mental blast");
        if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
            pline(use_thec,monsterc,"shriek");
        if (lays_eggs(youmonst.data) && flags.female)
            pline(use_thec,"sit","lay an egg");
    }
    /* you now know what an egg of your type looks like */
    if (lays_eggs(youmonst.data)) {
        learn_egg_type(u.umonnum);
        /* make queen bees recognize killer bee eggs */
        learn_egg_type(egg_type_from_parent(u.umonnum, TRUE));
    }
    find_ac();
    if ((!Levitation && !u.ustuck && !Flying &&
         (is_pool(level, u.ux,u.uy) || is_lava(level, u.ux,u.uy))) ||
        (Underwater && !Swimming))
        spoteffects(TRUE);
    if (Passes_walls && u.utrap && u.utraptype == TT_INFLOOR) {
        u.utrap = 0;
        pline("The rock seems to no longer trap you.");
    } else if (likes_lava(youmonst.data) && u.utrap && u.utraptype == TT_LAVA) {
        u.utrap = 0;
        pline("The lava now feels soothing.");
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
        if (Punished) {
            pline("You slip out of the iron chain.");
            unpunish();
        }
    }
    if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_BEARTRAP) &&
        (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data) ||
         (youmonst.data->msize <= MZ_SMALL && u.utraptype == TT_BEARTRAP))) {
        pline("You are no longer stuck in the %s.",
              u.utraptype == TT_WEB ? "web" : "bear trap");
        /* probably should burn webs too if PM_FIRE_ELEMENTAL */
        u.utrap = 0;
    }
    if (webmaker(youmonst.data) && u.utrap && u.utraptype == TT_WEB) {
        pline("You orient yourself on the web.");
        u.utrap = 0;
    }
    iflags.botl = 1;
    vision_full_recalc = 1;
    see_monsters();
    exercise(A_CON, FALSE);
    exercise(A_WIS, TRUE);
    encumber_msg();
    return 1;
}

static void break_armor(void)
{
    struct obj *otmp;

    if (breakarm(youmonst.data)) {
        if ((otmp = uarm) != 0) {
            if (donning(otmp)) cancel_don();
            pline("You break out of your armor!");
            exercise(A_STR, FALSE);
            Armor_gone(FALSE);
            useup(otmp);
        }
        if ((otmp = uarmc) != 0) {
            if (otmp->oartifact) {
                pline("Your %s falls off!", cloak_simple_name(otmp));
                Cloak_off();
                dropx(otmp);
            } else {
                pline("Your %s tears apart!", cloak_simple_name(otmp));
                Cloak_off();
                useup(otmp);
            }
        }
        if (uarmu) {
            pline("Your shirt rips to shreds!");
            useup(uarmu);
        }
    } else if (sliparm(youmonst.data)) {
        if (((otmp = uarm) != 0) && (racial_exception(&youmonst, otmp) < 1)) {
            if (donning(otmp)) cancel_don();
            pline("Your armor falls around you!");
            Armor_gone(FALSE);
            dropx(otmp);
        }
        if ((otmp = uarmc) != 0) {
            if (is_whirly(youmonst.data))
                pline("Your %s falls, unsupported!", cloak_simple_name(otmp));
            else pline("You shrink out of your %s!", cloak_simple_name(otmp));
            Cloak_off();
            dropx(otmp);
        }
        if ((otmp = uarmu) != 0) {
            if (is_whirly(youmonst.data))
                pline("You seep right through your shirt!");
            else pline("You become much too small for your shirt!");
            setworn(NULL, otmp->owornmask & W_ARMU);
            dropx(otmp);
        }
    }
    if (has_horns(youmonst.data)) {
        if ((otmp = uarmh) != 0) {
            if (is_flimsy(otmp) && !donning(otmp)) {
                char hornbuf[BUFSZ], yourbuf[BUFSZ];

                /* Future possiblities: This could damage/destroy helmet */
                sprintf(hornbuf, "horn%s", plur(num_horns(youmonst.data)));
                pline("Your %s %s through %s %s.", hornbuf, vtense(hornbuf, "pierce"),
                      shk_your(yourbuf, otmp), xname(otmp));
            } else {
                if (donning(otmp)) cancel_don();
                pline("Your helmet falls to the %s!", surface(u.ux, u.uy));
                Helmet_off();
                dropx(otmp);
            }
        }
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data)) {
        if ((otmp = uarmg) != 0) {
            if (donning(otmp)) cancel_don();
            /* Drop weapon along with gloves */
            pline("You drop your gloves%s!", uwep ? " and weapon" : "");
            drop_weapon(0);
            Gloves_off();
            dropx(otmp);
        }
        if ((otmp = uarms) != 0) {
            pline("You can no longer hold your shield!");
            Shield_off();
            dropx(otmp);
        }
        if ((otmp = uarmh) != 0) {
            if (donning(otmp)) cancel_don();
            pline("Your helmet falls to the %s!", surface(u.ux, u.uy));
            Helmet_off();
            dropx(otmp);
        }
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data) ||
        slithy(youmonst.data) || youmonst.data->mlet == S_CENTAUR) {
        if ((otmp = uarmf) != 0) {
            if (donning(otmp)) cancel_don();
            if (is_whirly(youmonst.data))
                pline("Your boots fall away!");
            else pline("Your boots %s off your feet!",
                       verysmall(youmonst.data) ? "slide" : "are pushed");
            Boots_off();
            dropx(otmp);
        }
    }
}

static void drop_weapon(int alone)
{
    struct obj *otmp;
    struct obj *otmp2;

    if ((otmp = uwep) != 0) {
        /* !alone check below is currently superfluous but in the
         * future it might not be so if there are monsters which cannot
         * wear gloves but can wield weapons
         */
        if (!alone || cantwield(youmonst.data)) {
            struct obj *wep = uwep;

            if (alone) pline("You find you must drop your weapon%s!",
                             u.twoweap ? "s" : "");
            otmp2 = u.twoweap ? uswapwep : 0;
            uwepgone();
            if (!wep->cursed || wep->otyp != LOADSTONE)
                dropx(otmp);
            if (otmp2 != 0) {
                uswapwepgone();
                if (!otmp2->cursed || otmp2->otyp != LOADSTONE)
                    dropx(otmp2);
            }
            untwoweapon();
        } else if (!could_twoweap(youmonst.data)) {
            untwoweapon();
        }
    }
}

void rehumanize(void)
{
    /* You can't revert back while unchanging */
    if (Unchanging && (u.mh < 1)) {
        killer_format = NO_KILLER_PREFIX;
        killer = "killed while stuck in creature form";
        done(DIED);
    }

    if (emits_light(youmonst.data))
        del_light_source(level, LS_MONSTER, &youmonst);
    polyman("You return to %s form!", urace.adj);

    if (u.uhp < 1) {
        char kbuf[256];

        sprintf(kbuf, "reverting to unhealthy %s form", urace.adj);
        killer_format = KILLED_BY;
        killer = kbuf;
        done(DIED);
    }
    if (!uarmg) selftouch("No longer petrify-resistant, you");
    nomul(0, NULL);

    iflags.botl = 1;
    vision_full_recalc = 1;
    encumber_msg();
}

int dobreathe(void)
{
    const struct attack *mattk;
    int typ;
    schar dx, dy, dz;

    if (Strangled) {
        pline("You can't breathe.  Sorry.");
        return 0;
    }
    if (u.uen < 15) {
        pline("You don't have enough energy to breathe!");
        return 0;
    }
    u.uen -= 15;
    iflags.botl = 1;

    if (!getdir(NULL, &dx, &dy, &dz))
        return 0;

    mattk = attacktype_fordmg(youmonst.data, AT_BREA, AD_ANY);
    /* if new breath types are added, change AD_ACID to max type */
    /* see also breamu() in mthrowu.c */
    /* see also http://qdb.rawrnix.com/?1120 */
    typ = (mattk->adtyp == AD_RBRE) ? rnd(AD_ACID) : mattk->adtyp;
    if (!mattk)
        warning("bad breath attack?");  /* mouthwash needed... */
    else if (typ >= AD_MAGM && typ <= AD_ACID)
        buzz(20 + typ - 1, (int)mattk->damn, u.ux, u.uy, dx, dy);
    else
        warning("Breath weapon %d used", typ - 1);
    return 1;
}

int dospit(void)
{
    struct obj *otmp;
    schar dx, dy, dz;

    if (!getdir(NULL, &dx, &dy, &dz))
        return 0;
    otmp = mksobj(level, u.umonnum==PM_COBRA ? BLINDING_VENOM : ACID_VENOM,
                  TRUE, FALSE);
    otmp->spe = 1; /* to indicate it's yours */
    throwit(otmp, NULL, 0L, FALSE, dx, dy, dz);
    return 1;
}

int doremove(void)
{
    if (!Punished) {
        pline("You are not chained to anything!");
        return 0;
    }
    unpunish();
    return 1;
}

int dospinweb(void)
{
    struct trap *ttmp = t_at(level, u.ux, u.uy);

    if (Levitation || Is_airlevel(&u.uz)
        || Underwater || Is_waterlevel(&u.uz)) {
        pline("You must be on the ground to spin a web.");
        return 0;
    }
    if (u.uswallow) {
        pline("You release web fluid inside %s.", mon_nam(u.ustuck));
        if (is_animal(u.ustuck->data)) {
            expels(u.ustuck, u.ustuck->data, TRUE);
            return 0;
        }
        if (is_whirly(u.ustuck->data)) {
            int i;

            for (i = 0; i < NATTK; i++)
                if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
                    break;
            if (i == NATTK)
                warning("Swallower has no engulfing attack?");
            else {
                char sweep[30];

                sweep[0] = '\0';
                switch(u.ustuck->data->mattk[i].adtyp) {
                case AD_FIRE:
                    strcpy(sweep, "ignites and ");
                    break;
                case AD_ELEC:
                    strcpy(sweep, "fries and ");
                    break;
                case AD_COLD:
                    strcpy(sweep,
                           "freezes, shatters and ");
                    break;
                }
                pline("The web %sis swept away!", sweep);
            }
            return 0;
        }            /* default: a nasty jelly-like creature */
        pline("The web dissolves into %s.", mon_nam(u.ustuck));
        return 0;
    }
    if (u.utrap) {
        pline("You cannot spin webs while stuck in a trap.");
        return 0;
    }
    exercise(A_DEX, TRUE);
    if (ttmp) switch (ttmp->ttyp) {
        case PIT:
        case SPIKED_PIT: pline("You spin a web, covering up the pit.");
            deltrap(level, ttmp);
            bury_objs(u.ux, u.uy);
            newsym(u.ux, u.uy);
            return 1;
        case SQKY_BOARD: pline("The squeaky board is muffled.");
            deltrap(level, ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case TELEP_TRAP:
        case LEVEL_TELEP:
        case MAGIC_PORTAL:
        case VIBRATING_SQUARE:
            pline("Your webbing vanishes!");
            return 0;
        case WEB: pline("You make the web thicker.");
            return 1;
        case HOLE:
        case TRAPDOOR:
            pline("You web over the %s.",
                  (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");
            deltrap(level, ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case ROLLING_BOULDER_TRAP:
            pline("You spin a web, jamming the trigger.");
            deltrap(level, ttmp);
            newsym(u.ux, u.uy);
            return 1;
        case ARROW_TRAP:
        case DART_TRAP:
        case BEAR_TRAP:
        case ROCKTRAP:
        case FIRE_TRAP:
        case LANDMINE:
        case SLP_GAS_TRAP:
        case RUST_TRAP:
        case MAGIC_TRAP:
        case ANTI_MAGIC:
        case POLY_TRAP:
        case SHURIKEN_TRAP:
            pline("You have triggered a trap!");
            dotrap(ttmp, 0);
            return 1;
        default:
            warning("Webbing over trap type %d?", ttmp->ttyp);
            return 0;
        }
    else if (On_stairs(u.ux, u.uy)) {
        /* cop out: don't let them hide the stairs */
        pline("Your web fails to impede access to the %s.",
              (level->locations[u.ux][u.uy].typ == STAIRS) ? "stairs" : "ladder");
        return 1;

    }
    ttmp = maketrap(level, u.ux, u.uy, WEB);
    if (ttmp) {
        ttmp->tseen = 1;
        ttmp->madeby_u = 1;
    }
    newsym(u.ux, u.uy);
    return 1;
}

int dosummon(void)
{
    int placeholder;
    if (u.uen < 10) {
        pline("You lack the energy to send forth a call for help!");
        return 0;
    }
    u.uen -= 10;
    iflags.botl = 1;

    pline("You call upon your brethren for help!");
    exercise(A_WIS, TRUE);
    if (!were_summon(youmonst.data, TRUE, &placeholder, NULL))
        pline("But none arrive.");
    return 1;
}

int dogaze(void)
{
    struct monst *mtmp;
    int looked = 0;
    int i;
    uchar adtyp = 0;

    for (i = 0; i < NATTK; i++) {
        if (youmonst.data->mattk[i].aatyp == AT_GAZE) {
            adtyp = youmonst.data->mattk[i].adtyp;
            break;
        }
    }
    if (adtyp != AD_CONF && adtyp != AD_FIRE) {
        warning("gaze attack %d?", adtyp);
        return 0;
    }


    if (Blind) {
        pline("You can't see anything to gaze at.");
        return 0;
    }
    if (u.uen < 15) {
        pline("You lack the energy to use your special gaze!");
        return 0;
    }
    u.uen -= 15;
    iflags.botl = 1;

    for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp)) continue;
        if (canseemon(level, mtmp) && couldsee(mtmp->mx, mtmp->my)) {
            looked++;
            if (Invis && !perceives(mtmp->data))
                pline("%s seems not to notice your gaze.", Monnam(mtmp));
            else if (mtmp->minvis && !See_invisible)
                pline("You can't see where to gaze at %s.", Monnam(mtmp));
            else if (mtmp->m_ap_type == M_AP_FURNITURE
                     || mtmp->m_ap_type == M_AP_OBJECT) {
                looked--;
                continue;
            } else if (flags.safe_dog && !Confusion && !Hallucination
                       && mtmp->mtame) {
                pline("You avoid gazing at %s.", y_monnam(mtmp));
            } else {
                if (mtmp->mpeaceful && !Confusion && !Hallucination) {
                    if (flags.safe_peaceful != 'n') {
                        boolean call_off_harm = FALSE;
                        if (flags.safe_peaceful == 'y') {
                            pline("You avoid gazing at %s.", mon_nam(mtmp));
                            call_off_harm = TRUE;
                        } else if (flags.safe_peaceful == 'a') {
                            char qbuf[QBUFSZ];
                            sprintf(qbuf, "Really %s %s?",
                                    (adtyp == AD_CONF) ? "confuse" : "attack",
                                    mon_nam(mtmp));
                            if (paranoid_yn(qbuf, iflags.paranoid_hit) != 'y')
                                call_off_harm = TRUE;
                        }
                        if (call_off_harm)
                            continue;
                    }
                    setmangry(mtmp);
                }
                if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleeping ||
                    !mtmp->mcansee || !haseyes(mtmp->data)) {
                    looked--;
                    continue;
                }
                /* No reflection check for consistency with when a monster
                 * gazes at *you*--only medusa gaze gets reflected then.
                 */
                if (adtyp == AD_CONF) {
                    if (!mtmp->mconf)
                        pline("Your gaze confuses %s!", mon_nam(mtmp));
                    else
                        pline("%s is getting more and more confused.",
                              Monnam(mtmp));
                    mtmp->mconf = 1;
                } else if (adtyp == AD_FIRE) {
                    int dmg = dice(2,6);
                    pline("You attack %s with a fiery gaze!", mon_nam(mtmp));
                    if (resists_fire(mtmp)) {
                        pline("The fire doesn't burn %s!", mon_nam(mtmp));
                        dmg = 0;
                    }
                    if (u.ulevel > rn2(20))
                        destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE, NULL);
                    if (u.ulevel > rn2(20))
                        destroy_mitem(mtmp, POTION_CLASS, AD_FIRE, NULL);
                    if (u.ulevel > rn2(25))
                        destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE, NULL);
                    if (dmg && !DEADMONSTER(mtmp)) mtmp->mhp -= dmg;
                    if (mtmp->mhp <= 0) killed(mtmp);
                }
                /* For consistency with passive() in uhitm.c, this only
                 * affects you if the monster is still alive.
                 */
                if (!DEADMONSTER(mtmp) &&
                    (mtmp->data==&mons[PM_FLOATING_EYE]) && !mtmp->mcan) {
                    if (!Free_action) {
                        pline("You are frozen by %s gaze!",
                              s_suffix(mon_nam(mtmp)));
                        nomul((u.ulevel > 6 || rn2(4)) ?
                              -dice((int)mtmp->m_lev+1,
                                    (int)mtmp->data->mattk[0].damd)
                              : -200, "frozen by a monster's gaze");
                        return 1;
                    } else
                        pline("You stiffen momentarily under %s gaze.",
                              s_suffix(mon_nam(mtmp)));
                }
                /* Technically this one shouldn't affect you at all because
                 * the Medusa gaze is an active monster attack that only
                 * works on the monster's turn, but for it to *not* have an
                 * effect would be too weird.
                 */
                if (!DEADMONSTER(mtmp) &&
                    (mtmp->data == &mons[PM_MEDUSA]) &&
                    !mtmp->mcan && !Hallucination) {
                    if (!(poly_when_stoned(youmonst.data) &&
                          polymon(PM_STONE_GOLEM))) {
                        pline(
                              "Gazing at the awake %s is not a very good idea.",
                              l_monnam(mtmp));
                        /* as if gazing at a sleeping anything is fruitful... */
                        if (!Stoned) pline("You begin to turn into stone!");
                        delayed_petrify(NULL, "deliberately meeting Medusa's gaze");
                    }
                }
            }
        }
    }
    if (!looked) pline("You gaze at no place in particular.");
    return 1;
}

int dohide(void)
{
    boolean ismimic = youmonst.data->mlet == S_MIMIC;

    if (u.uundetected || (ismimic && youmonst.m_ap_type != M_AP_NOTHING)) {
        pline("You are already hiding.");
        return 0;
    }
    if (ismimic) {
        /* should bring up a dialog "what would you like to imitate?" */
        youmonst.m_ap_type = M_AP_OBJECT;
        youmonst.mappearance = STRANGE_OBJECT;
    } else
        u.uundetected = 1;
    newsym(u.ux,u.uy);
    return 1;
}

int domindblast(void)
{
    struct monst *mtmp, *nmon;

    if (u.uen < 10) {
        pline("You concentrate but lack the energy to maintain doing so.");
        return 0;
    }
    u.uen -= 10;
    iflags.botl = 1;

    pline("You concentrate.");
    if (uarmh && uarmh->otyp == TINFOIL_HAT) {
        pline("A wave of psychic energy pours out, but it is blocked by %s.",
              yname(uarmh));
        return 0;
    }
    pline("A wave of psychic energy pours out.");
    for (mtmp=level->monlist; mtmp; mtmp = nmon) {
        int u_sen;

        nmon = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
            continue;
        if (mtmp->mpeaceful)
            continue;
        u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
        if (u_sen || (telepathic(mtmp->data) && rn2(2)) ||
            (!rn2(10) && which_armor(mtmp, W_ARMH) &&
             which_armor(mtmp, W_ARMH)->otyp != TINFOIL_HAT)) {
            pline("You lock in on %s %s.", s_suffix(mon_nam(mtmp)),
                  u_sen ? "telepathy" :
                  telepathic(mtmp->data) ? "latent telepathy" :
                  "mind");
            mtmp->mhp -= rnd(15);
            if (mtmp->mhp <= 0)
                killed(mtmp);
        }
    }
    return 1;
}

static void uunstick(void)
{
    pline("%s is no longer in your clutches.", Monnam(u.ustuck));
    u.ustuck = 0;
    u.uwilldrown = 0;
}

void skinback(boolean silently)
{
    if (uskin) {
        if (!silently) pline("Your skin returns to its original form.");
        uarm = uskin;
        uskin = NULL;
        /* undo save/restore hack */
        uarm->owornmask &= ~I_SPECIAL;
    }
}


const char *mbodypart(struct monst *mon, int part)
{
    static const char
        *const humanoid_parts[] = { "arm", "eye", "face", "finger",
                                    "fingertip", "foot", "hand", "handed", "head", "leg",
                                    "light headed", "neck", "spine", "toe", "hair",
                                    "blood", "lung", "nose", "stomach"},
        *const jelly_parts[] = { "pseudopod", "dark spot", "front",
                                 "pseudopod extension", "pseudopod extremity",
                                 "pseudopod root", "grasp", "grasped", "cerebral area",
                                 "lower pseudopod", "viscous", "middle", "surface",
                                 "pseudopod extremity", "ripples", "juices",
                                 "surface", "sensor", "stomach" },
        *const animal_parts[] = { "forelimb", "eye", "face", "foreclaw", "claw tip",
                                  "rear claw", "foreclaw", "clawed", "head", "rear limb",
                                  "light headed", "neck", "spine", "rear claw tip",
                                  "fur", "blood", "lung", "nose", "stomach" },
        *const bird_parts[] = { "wing", "eye", "face", "wing", "wing tip",
                                "foot", "wing", "winged", "head", "leg",
                                "light headed", "neck", "spine", "toe",
                                "feathers", "blood", "lung", "bill", "stomach" },
        *const horse_parts[] = { "foreleg", "eye", "face", "forehoof", "hoof tip",
                                 "rear hoof", "foreclaw", "hooved", "head", "rear leg",
                                 "light headed", "neck", "backbone", "rear hoof tip",
                                 "mane", "blood", "lung", "nose", "stomach"},
        *const sphere_parts[] = { "appendage", "optic nerve", "body", "tentacle",
                                  "tentacle tip", "lower appendage", "tentacle", "tentacled",
                                  "body", "lower tentacle", "rotational", "equator", "body",
                                  "lower tentacle tip", "cilia", "life force", "retina",
                                  "olfactory nerve", "interior" },
        *const fungus_parts[] = { "mycelium", "visual area", "front", "hypha",
                                  "hypha", "root", "strand", "stranded", "cap area",
                                  "rhizome", "sporulated", "stalk", "root", "rhizome tip",
                                  "spores", "juices", "gill", "gill", "interior" },
        *const vortex_parts[] = { "region", "eye", "front", "minor current",
                                  "minor current", "lower current", "swirl", "swirled",
                                  "central core", "lower current", "addled", "center",
                                  "currents", "edge", "currents", "life force",
                                  "center", "leading edge", "interior" },
        *const snake_parts[] = { "vestigial limb", "eye", "face", "large scale",
                                 "large scale tip", "rear region", "scale gap", "scale gapped",
                                 "head", "rear region", "light headed", "neck", "length",
                                 "rear scale", "scales", "blood", "lung", "forked tongue", "stomach" },
        *const fish_parts[] = { "fin", "eye", "premaxillary", "pelvic axillary",
                                "pelvic fin", "anal fin", "pectoral fin", "finned", "head", "peduncle",
                                "played out", "gills", "dorsal fin", "caudal fin",
                                "scales", "blood", "gill", "nostril", "stomach" };
    /* claw attacks are overloaded in mons[]; most humanoids with
       such attacks should still reference hands rather than claws */
    static const char not_claws[] = {
                                     S_HUMAN, S_MUMMY, S_ZOMBIE, S_ANGEL,
                                     S_NYMPH, S_LEPRECHAUN, S_QUANTMECH, S_VAMPIRE,
                                     S_ORC, S_GIANT,     /* quest nemeses */
                                     '\0'        /* string terminator; assert( S_xxx != 0 ); */
    };
    const struct permonst *mptr = mon->data;

    if (part == HAND || part == HANDED) {   /* some special cases */
        if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE ||
            mptr->mlet == S_YETI)
            return part == HAND ? "paw" : "pawed";
        if (humanoid(mptr) && attacktype(mptr, AT_CLAW) &&
            !strchr(not_claws, mptr->mlet) &&
            mptr != &mons[PM_STONE_GOLEM] &&
            mptr != &mons[PM_INCUBUS] && mptr != &mons[PM_SUCCUBUS])
            return part == HAND ? "claw" : "clawed";
    }
    if ((mptr == &mons[PM_MUMAK] || mptr == &mons[PM_MASTODON]) &&
        part == NOSE)
        return "trunk";
    if (mptr == &mons[PM_SHARK] && part == HAIR)
        return "skin";  /* sharks don't have scales */
    if (mptr == &mons[PM_JELLYFISH] && (part == ARM || part == FINGER ||
                                        part == HAND || part == FOOT || part == TOE))
        return "tentacle";
    if (mptr == &mons[PM_FLOATING_EYE] && part == EYE)
        return "cornea";
    if (humanoid(mptr) &&
        (part == ARM || part == FINGER || part == FINGERTIP ||
         part == HAND || part == HANDED))
        return humanoid_parts[part];
    if (mptr == &mons[PM_RAVEN])
        return bird_parts[part];
    if (mptr->mlet == S_CENTAUR || mptr->mlet == S_UNICORN ||
        (mptr == &mons[PM_ROTHE] && part != HAIR))
        return horse_parts[part];
    if (mptr->mlet == S_LIGHT) {
        if (part == HANDED) return "rayed";
        else if (part == ARM || part == FINGER ||
                 part == FINGERTIP || part == HAND) return "ray";
        else return "beam";
    }
    if (mptr->mlet == S_EEL && mptr != &mons[PM_JELLYFISH])
        return fish_parts[part];
    if (slithy(mptr) || (mptr->mlet == S_DRAGON && part == HAIR))
        return snake_parts[part];
    if (mptr->mlet == S_EYE)
        return sphere_parts[part];
    if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING ||
        mptr->mlet == S_BLOB || mptr == &mons[PM_JELLYFISH])
        return jelly_parts[part];
    if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
        return vortex_parts[part];
    if (mptr->mlet == S_FUNGUS)
        return fungus_parts[part];
    if (humanoid(mptr))
        return humanoid_parts[part];
    return animal_parts[part];
}

const char *body_part(int part)
{
    return mbodypart(&youmonst, part);
}


int poly_gender(void)
{
    /* Returns gender of polymorphed player; 0/1=same meaning as flags.female,
     * 2=none.
     */
    if (is_neuter(youmonst.data) || !humanoid(youmonst.data)) return 2;
    return flags.female;
}


void ugolemeffects(int damtype, int dam)
{
    int heal = 0;
    /* We won't bother with "slow"/"haste" since players do not
     * have a monster-specific slow/haste so there is no way to
     * restore the old velocity once they are back to human.
     */
    if (u.umonnum != PM_FLESH_GOLEM && u.umonnum != PM_IRON_GOLEM)
        return;
    switch (damtype) {
    case AD_ELEC: if (u.umonnum == PM_FLESH_GOLEM)
            heal = dam / 6; /* Approx 1 per die */
        break;
    case AD_FIRE: if (u.umonnum == PM_IRON_GOLEM)
            heal = dam;
        break;
    }
    if (heal && (u.mh < u.mhmax)) {
        u.mh += heal;
        if (u.mh > u.mhmax) u.mh = u.mhmax;
        iflags.botl = 1;
        pline("Strangely, you feel better than before.");
        exercise(A_STR, TRUE);
    }
}

static int armor_to_dragon(int atyp)
{
    switch(atyp) {
    case GRAY_DRAGON_SCALE_MAIL:
    case GRAY_DRAGON_SCALES:
        return PM_GRAY_DRAGON;
    case SILVER_DRAGON_SCALE_MAIL:
    case SILVER_DRAGON_SCALES:
        return PM_SILVER_DRAGON;
    case RED_DRAGON_SCALE_MAIL:
    case RED_DRAGON_SCALES:
        return PM_RED_DRAGON;
    case ORANGE_DRAGON_SCALE_MAIL:
    case ORANGE_DRAGON_SCALES:
        return PM_ORANGE_DRAGON;
    case WHITE_DRAGON_SCALE_MAIL:
    case WHITE_DRAGON_SCALES:
        return PM_WHITE_DRAGON;
    case BLACK_DRAGON_SCALE_MAIL:
    case BLACK_DRAGON_SCALES:
        return PM_BLACK_DRAGON;
    case BLUE_DRAGON_SCALE_MAIL:
    case BLUE_DRAGON_SCALES:
        return PM_BLUE_DRAGON;
    case GREEN_DRAGON_SCALE_MAIL:
    case GREEN_DRAGON_SCALES:
        return PM_GREEN_DRAGON;
    case GOLD_DRAGON_SCALE_MAIL:
    case GOLD_DRAGON_SCALES:
        return PM_GOLD_DRAGON;
    case YELLOW_DRAGON_SCALE_MAIL:
    case YELLOW_DRAGON_SCALES:
        return PM_YELLOW_DRAGON;
    case CHROMATIC_DRAGON_SCALE_MAIL:
    case CHROMATIC_DRAGON_SCALES:
        return PM_CHROMATIC_DRAGON;
    default:
        return -1;
    }
}

/*polyself.c*/

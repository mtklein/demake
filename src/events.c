/* Story content: rooms, interactions, cutscenes, the helm finale. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "events.h"
#include "screens.h"
#include "encounter.h"
#include "party5.h"

/* Speaker lines upgrade to portraits automatically as each portrait lands
 * in tools/art/portraits.py (missing art falls back to plain say). */
#ifdef POR_LAEZEL
#define SAY_LZ(t) say_p(POR_LAEZEL, t)
#else
#define SAY_LZ(t) say(t)
#endif
#ifdef POR_SHADOW
#define SAY_SH(t) say_p(POR_SHADOW, t)
#else
#define SAY_SH(t) say(t)
#endif
#ifdef POR_US
#define SAY_US(t) say_p(POR_US, t)
#else
#define SAY_US(t) say(t)
#endif
#ifdef POR_ZHALK
#define SAY_ZH(t) say_p(POR_ZHALK, t)
#else
#define SAY_ZH(t) say(t)
#endif

static int cur_room;
static int n_us = -1, n_lz = -1, n_sh = -1, n_zh = -1, n_fl = -1;
static int n_imp[3] = { -1, -1, -1 };
static int n_wander[2] = { -1, -1 };
static u16 seen;                     /* room-intro-seen bits */
static int sh_room_open;             /* Shadowheart pod opened this game */
static int sh_waiting;               /* freed but not yet recruited */

static int isclass(int c) { return G.pm[0].cls == c; }
static int us_with_us(void) {
    return (G.flags & GF_US_FREED) && !(G.flags & GF_US_MUTILATED);
}

static void heal_pod(void) {
    sfx_play(SFX_HEAL);
    party_heal_full();
    say("You step into the restoration pod. Warm ichor closes every wound.");
    dlg_close();
}

/* ------------------------------------------------------------ rooms */

void room_enter(int id, int sx, int sy, int face) {
    mgba_logf("room_enter %d at %d,%d flags=%x", id, sx, sy, G.flags);
    music(id == RM_HELM ? SONG_BOSS : SONG_EXPLORE);
    cur_room = id;
    crumb(CR_ROOM, id);
    n_us = n_lz = n_sh = n_zh = n_fl = -1;
    n_imp[0] = n_imp[1] = n_imp[2] = -1;
    n_wander[0] = n_wander[1] = -1;

    switch (id) {
        case RM_NURSERY: field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H); break;
        case RM_SURGERY: field_load(map_surgery, MAP_SURGERY_W, MAP_SURGERY_H); break;
        case RM_DECK:    field_load(map_deck, MAP_DECK_W, MAP_DECK_H); break;
        case RM_PODS:    field_load(map_pods, MAP_PODS_W, MAP_PODS_H); break;
        case RM_HELM:    field_load(map_helm, MAP_HELM_W, MAP_HELM_H); break;
    }
    field_spawn(sx, sy, face);

    switch (id) {
        case RM_SURGERY:
            if (G.flags & GF_US_FREED) field_set_meta(7, 2, MT_MYRNATH_EMPTY);
            if (us_with_us()) n_us = field_add_npc(9, 3, OBJT_US, 3, 0, NPC_2FRAME);
            break;
        case RM_DECK:
            if (!(G.flags & GF_DECK_FOUGHT)) {
                n_lz = field_add_npc(6, 3, OBJT_LAEZEL, 1, 0, 0);
                n_imp[0] = field_add_npc(4, 1, OBJT_IMPF, 4, 0, NPC_2FRAME);
                n_imp[1] = field_add_npc(8, 1, OBJT_IMPF, 4, 0, NPC_2FRAME);
                n_imp[2] = field_add_npc(6, 2, OBJT_IMPF, 4, 0, NPC_2FRAME);
            }
            if (us_with_us()) n_us = field_add_npc(12, 5, OBJT_US, 3, 0, NPC_2FRAME);
            if ((G.flags & (GF_DECK_FOUGHT | GF_W_DECK)) == GF_DECK_FOUGHT) {
                /* a straggler imp prowls the far rail -- avoidable */
                n_wander[0] = field_add_npc(16, 7, OBJT_IMPF, 4, 0, NPC_2FRAME);
                field_npc_patrol(n_wander[0], 28);
            }
            break;
        case RM_PODS:
            if (sh_room_open) field_set_meta(6, 2, MT_POD_O);
            if (G.flags & GF_RUNE) field_set_meta(5, 4, MT_CONSOLE_LIT);
            if (sh_waiting) n_sh = field_add_npc(6, 3, OBJT_SHADOW, 2, 0, 0);
            if (us_with_us()) n_us = field_add_npc(14, 8, OBJT_US, 3, 0, NPC_2FRAME);
            if (!(G.flags & GF_W_PODS)) {
                n_wander[1] = field_add_npc(15, 2, OBJT_IMPF, 4, 0, NPC_2FRAME);
                field_npc_patrol(n_wander[1], 28);
            }
            break;
        case RM_HELM:
            n_zh = field_add_npc(4, 2, OBJT_ZHALKF, 6, 3, NPC_2FRAME);
            n_fl = field_add_npc(7, 2, OBJT_FLAYERF, 5, 2, NPC_2FRAME);
            if (us_with_us()) n_us = field_add_npc(12, 7, OBJT_US, 3, 0, NPC_2FRAME);
            break;
    }
}

void intro_wake(void) {
    fade_in(24);
    field_wait(30);
    say("Wet dark. A heartbeat that is not yours.");
    say("Your pod has torn open, spilling you onto the deck of a ship made of meat.");
    field_shake(24);
    sfx_noise(20);
    field_wait(30);
    say("Behind your eye: a pressure. A wriggling. A PASSENGER.");
    if (isclass(CLS_WIZARD))
        say("[WIZARD] You know the word for this: ceremorphosis. In days, you become one of THEM.");
    if (isclass(CLS_BARD))
        say("[BARD] Wonderful. You finally have a story no one will believe.");
    dlg_close();
}

/* ------------------------------------------------------------ helpers */

static void door_to(int next, int sx, int sy) {
    G_FIELD_IDLE = 0;
    sfx_play(SFX_CONFIRM);
    fade_out(14);
    dlg_close();
    room_enter(next, sx, sy, 0);
    field_draw();
    fade_in(14);
    say("The sphincter door clenches shut behind you.");
    dlg_close();
    if (!(seen & (1 << next))) {
        seen |= (u16)(1 << next);
        switch (next) {
            case RM_SURGERY:
                say("A dissection chamber. Trays of instruments. Rune-carved slates.");
                say("A voice, INSIDE your skull: \"Remove us from this body. From this case, free us. PLEASE.\"");
                dlg_close();
                break;
            case RM_DECK:
                say("The hull is torn open. Wind. Brimstone. An orange sky screaming past.");
                field_shake(20);
                sfx_noise(24);
                say("A red dragon thunders by, close enough to touch. Githyanki riders rake the deck with fire.");
                if (isclass(CLS_RANGER))
                    say("[RANGER] You read their flight line. They circle for another pass -- move BETWEEN them.");
                dlg_close();
                break;
            case RM_PODS:
                say("Rows of pods, ribbed like closed eyes. From inside one of them: pounding.");
                say("A muffled voice: \"Hello!? Get me OUT of this thing!\"");
                dlg_close();
                break;
            case RM_HELM:
                say("The helm. Through the great viewport, all of Avernus burns.");
                say("A cambion commander duels a mind flayer across the deck, blade against tentacle.");
                SAY_ZH("ZHALK: \"Take this ship, or Zariel will have your head!\"");
                say("A cold voice floods your mind: \"Thrall. The transponder. CONNECT THE NERVES.\"");
                if (G.flags & GF_LAEZEL)
                    SAY_LZ("LAE'ZEL: \"The ghaik wants the ship grounded. For once, we agree. To the transponder!\"");
                dlg_close();
                break;
        }
    }
}

/* ------------------------------------------------------------ nursery */

static void nursery_interact(int mx, int my, int m) {
    if (m == MT_POOL) {
        say("A membrane pool, wriggling with pale tadpoles. A nursery.");
        say("One of these was lowered into your eye while you watched.");
        if (isclass(CLS_WIZARD))
            say("[WIZARD] The casing is fragile -- volatile, even. Best not to touch it.");
        dlg_close();
    } else if (m == MT_POD_C) {
        say("A sealed pod. Something inside shifts, slowly. You leave it be.");
        dlg_close();
    } else if (m == MT_POD_O && mx == 3 && my == 5) {
        say("Your pod. The lining still holds the shape of your body.");
        dlg_close();
    } else if (m == MT_RESTO) {
        heal_pod();
    } else if (m == MT_CORPSE) {
        if (!(G.flags & GF_SLATE_READ)) {
            G.flags |= GF_SLATE_READ;
            say("A dead thrall, days gone. A rune slate lies by his hand.");
            say("Touching it floods you with vision: ships between stars, a thousand worlds harvested.");
            dlg_close();
        } else {
            say("The thrall is past helping.");
            dlg_close();
        }
    } else if (m == MT_CHEST) {
        if (!(G.flags & GF_CHEST_NURSERY)) {
            G.flags |= GF_CHEST_NURSERY;
            sfx_play(SFX_CONFIRM);
            say("A cartilaginous chest. Inside, floating in brine: a Potion.");
            G.potions++;
        } else say("Empty, and somehow still damp.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ surgery */

static void us_extraction(void) {
    if (G.flags & GF_US_FREED) {
        say("The apparatus stands empty. The elf's body is at peace, such as it is.");
        dlg_close();
        return;
    }
    say("An elf sits strapped upright, skullcap hinged open. He is dying. The BRAIN inside is not.");
    say("\"Free us,\" it pleads. \"We are needed to navigate. Needed to LEAVE this realm.\"");
    if (isclass(CLS_WIZARD))
        say("[WIZARD] Arcana tells you plainly: that is a newborn intellect devourer.");

    const char* opts[3];
    int n = 0, styled = -1;
    if (isclass(CLS_ROGUE))  { opts[n] = "[ROGUE] Deft extraction"; styled = n; n++; }
    if (isclass(CLS_RANGER)) { opts[n] = "[RANGER] Steady hands"; styled = n; n++; }
    if (isclass(CLS_BARD))   { opts[n] = "[BARD] Soothe and cut"; styled = n; n++; }
    opts[n++] = "Wrench it out";
    opts[n++] = "Leave it";
    say_keep("Reach into the skull?");
    int c = choose(n, opts);

    if (c == n - 1) {
        say("The brain sags, pulsing. \"Please...\"");
        dlg_close();
        return;
    }
    if (c == styled) {
        say("Your hands are sure. The brain comes free whole, trailing silver threads.");
    } else {
        say("You grip and PULL. Something tears -- but the thing pops loose, quivering.");
    }
    sfx_play(SFX_HEAL);
    G.flags |= GF_US_FREED;
    field_set_meta(7, 2, MT_MYRNATH_EMPTY);

    say("It stands on tiny legs. \"We are us,\" it announces. \"We will fight beside you.\"");
    say_keep("The brain looks up at you, trusting.");
    static const char* const opts2[] = { "Welcome, Us.", "Mutilate it" };
    int c2 = choose(2, opts2);
    if (c2 == 1) {
        say("...You carve a shallow wound across its crown. The thing whimpers and cowers.");
        say("Us no longer trusts you. Us follows no one.");
        G.flags |= GF_US_MUTILATED;
    } else {
        SAY_US("US: \"Us. Yes. US.\"");
        n_us = field_add_npc(9, 3, OBJT_US, 3, 0, NPC_2FRAME);
    }
    dlg_close();
}

static void surgery_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_MYRNATH || m == MT_MYRNATH_EMPTY) { us_extraction(); return; }
    if (m == MT_RESTO) { heal_pod(); return; }
    if (m == MT_MESH) {
        say("Arterial mesh, woven into a ramp. The ship grew its own rigging.");
        dlg_close();
        return;
    }
    if (m == MT_CORPSE) {
        say("A goblin, half-dissected. Its expression suggests it did not volunteer.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.flags & GF_RELIQUARY)) {
            G.flags |= GF_RELIQUARY;
            sfx_play(SFX_CONFIRM);
            say("Surgical stores: a Scroll of Revivify, sealed in wax.");
            G.revivify++;
        } else say("Nothing left but instruments you'd rather not name.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ deck */

static void deck_fight(void) {
    field_face_npc(n_lz, 0);
    say("The githyanki warrior snaps around, blade up -- then stops, eyes narrowing.");
    SAY_LZ("LAE'ZEL: \"Tsk'va! You are no thrall. Vlaakith blesses me this day.\"");

    const char* opts[3];
    int n = 0;
    opts[n++] = "We fight together.";
    if (isclass(CLS_BARD)) opts[n++] = "[BARD] Love the armor.";
    opts[n++] = "Stay out of my way.";
    say_keep("She sizes you up.");
    int c = choose(n, opts);
    if (isclass(CLS_BARD) && c == 1)
        SAY_LZ("LAE'ZEL: \"...Chk. Flatter me when we are not on a burning ship.\"");
    else if (c == n - 1)
        SAY_LZ("LAE'ZEL: \"Bold. Stupid, but bold. I shall permit it.\"");
    else
        SAY_LZ("LAE'ZEL: \"Together we might survive.\"");

    SAY_LZ("LAE'ZEL: \"First -- ghaik vermin come. Draw steel!\"");
    dlg_close();
    party_add_laezel();
    party5_refresh_all();
    field_remove_npc(n_lz);

    EncSpawn deck5[4];
    int nd = 0;
    for (int i = 0; i < 3; i++) {
        deck5[nd].mon = R5M_LESSER_IMP; deck5[nd].npc = (s8)n_imp[i];
        deck5[nd].xp = 40; deck5[nd].side = 1; nd++;
    }
    if (us_with_us() && n_us >= 0) {
        deck5[nd].mon = R5M_DEVOURER; deck5[nd].npc = (s8)n_us;
        deck5[nd].xp = 0; deck5[nd].side = 2; nd++;
    }
    encounter_run(deck5, nd, 0, 0);
    G.flags |= GF_DECK_FOUGHT;
    music(SONG_EXPLORE);
    if (!(G.flags & GF_STINGER)) {
        G.flags |= GF_STINGER;
        loot_weapon(R5W_STINGER);
        sfx_play(SFX_CONFIRM);
        say("You cut the largest imp's stinger free. Still dripping. (A weapon, for the brave.)");
    }
    SAY_LZ("LAE'ZEL: \"You prove surprisingly adequate in battle. Now -- to the helm.\"");
    if (us_with_us()) SAY_US("US: \"Us helped. Us HELPED!\"");
    dlg_close();

}

static void deck_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_RESTO) { heal_pod(); return; }
    if (m == MT_GITH_CORPSE) {
        if (mx == 12 && my == 6 && !(G.flags & GF_DUELIST)) {
            G.flags |= GF_DUELIST;
            static const u8 find[4] = { R5W_RAPIER, R5W_SHORTSWORD,
                                        R5W_TRIDENT, R5W_DAGGER };
            int w = find[G.pm[0].cls];
            say("A githyanki duelist, dead at her post. Her kit survived the fire.");
            if (isclass(CLS_BARD))
                say("[BARD] Beneath the ash: a dueling rapier, silvered and balanced. It suits your hand like it was made for it.");
            if (isclass(CLS_ROGUE))
                say("[ROGUE] A silvered shortsword, weighted for quick, quiet work.");
            if (isclass(CLS_RANGER))
                say("[RANGER] A boarding trident -- made for ship-to-ship slaughter. It throws true.");
            if (isclass(CLS_WIZARD))
                say("[WIZARD] A balanced throwing dagger. Considerably better than a stick.");
            loot_weapon(w);
            sfx_play(SFX_CONFIRM);
            say_keep("Take it up now?");
            static const char* const o[] = { "Equip it", "Pocket it" };
            if (choose(2, o) == 0) {
                u8 old = G.weapon[0];
                G.weapon[0] = G.winv[--G.nwinv];
                loot_weapon(old);
                say("It settles into your grip.");
            }
            dlg_close();
            return;
        }
        say("A githyanki warrior, burnt where she stood. Her greatsword has melted into her hands.");
        dlg_close();
        return;
    }
    if (m == MT_WINDOW) {
        say("Below the ship: the Styx coils through fields of fire. Avernus, first of the Nine Hells.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ pods */

static void sh_pod(void) {
    if (sh_room_open) {
        say("The pod hangs open, dripping. \"A coffin would be more inviting,\" its former occupant observed.");
        dlg_close();
        return;
    }
    say("A woman pounds on the glass from inside, half-drowned in green fluid.");
    SAY_SH("SHADOWHEART: \"Get me OUT of this thing!\"");
    if (G.flags & GF_RUNE)
        say("The rune in your pack pulses toward the console nearby.");
    else
        say("The pod has no latch, no seam. Only the console beside it responds to anything at all.");
    dlg_close();
}

static void open_sh_pod(void) {
    sfx_play(SFX_HEAL);
    field_set_meta(5, 4, MT_CONSOLE_LIT);
    say("The console pulses like a beating heart. Will the pod to open, it seems to say.");
    say("You press your palm flat and THINK it open.");
    field_shake(20);
    sfx_noise(16);
    field_set_meta(6, 2, MT_POD_O);
    sh_room_open = 1;
    n_sh = field_add_npc(6, 3, OBJT_SHADOW, 2, 0, 0);
    field_wait(20);
    say("The pod bursts in a gush of fluid. A dark-haired woman spills out, gasping, clutching a wound that isn't there.");
    SAY_SH("SHADOWHEART: \"At last. I thought I was going to die in that blasted tube.\"");
    say_keep("She straightens, wary.");
    static const char* const opts[] = { "Join me. Safer together.", "Try to keep up.", "We go separate ways." };
    int c = choose(3, opts);
    if (c == 2) {
        SAY_SH("SHADOWHEART: \"Fine. I'll take my chances. ...Probably.\"");
        sh_waiting = 1;
        dlg_close();
        return;
    }
    if (c == 1) SAY_SH("SHADOWHEART: \"Ha. Just watch me.\"");
    else SAY_SH("SHADOWHEART: \"Agreed. Whatever those things intend for us -- we'll fare better together.\"");
    say("Shadowheart joins the party! (She carries a Scroll of Revivify.)");
    party_add_shadowheart();
    field_remove_npc(n_sh);
    n_sh = -1;
    sh_waiting = 0;
    dlg_close();
}

static void pod_console(void) {
    if (sh_room_open) {
        say("The console's pulse has gone still. Its work is done.");
        dlg_close();
        return;
    }
    say("A veined console. At its center: an empty socket, shaped for a rune.");
    if (G.flags & GF_RUNE) {
        say_keep("The eldritch rune hums in your hand.");
        static const char* const o[] = { "Slot the rune", "Not yet" };
        if (choose(2, o) == 0) { open_sh_pod(); return; }
        dlg_close();
        return;
    }
    if (isclass(CLS_WIZARD) || isclass(CLS_BARD)) {
        say_keep("[ARCANA] The warding glyphs are sloppy. You could inscribe counter-glyphs by hand.");
        static const char* const o[] = { "Inscribe counter-glyphs", "Search for the rune instead" };
        if (choose(2, o) == 0) { open_sh_pod(); return; }
        dlg_close();
        return;
    }
    say("Its socket sits empty, hungry. Somewhere on this deck there must be a rune to feed it.");
    dlg_close();
}

static void victims_console(void) {
    if (G.flags & GF_THRALLS_DONE) {
        say("The apparatus is quiet now.");
        dlg_close();
        return;
    }
    say("Two victims hang wired into the machine, eyes open, seeing nothing. The console offers three glyphs.");
    say_keep("Your tadpole translates, helpfully: UNLEASH. AGGRESSION. NOTHING.");
    static const char* const o[] = { "Unleash", "Aggression", "Leave it" };
    int c = choose(3, o);
    if (c == 2) { dlg_close(); return; }
    G.flags |= GF_THRALLS_DONE;
    if (c == 0) {
        field_shake(16);
        say("Psionic light floods the wires. The victims sigh once, and are gone.");
        say("Whatever mercy that was, it wasn't yours to give.");
        dlg_close();
        return;
    }
    say("The victims' eyes snap open -- white, furious, EMPTY. They tear free of the wires!");
    dlg_close();
    {
        int t0 = field_add_npc(9, 4, OBJT_SHADOW, 5, 0, 0);
        int t1 = field_add_npc(11, 4, OBJT_SHADOW, 5, 0, 0);
        EncSpawn th[2] = {
            { R5M_THRALL, (s8)t0, 60, 1 },
            { R5M_THRALL, (s8)t1, 60, 1 },
        };
        encounter_run(th, 2, 0, 1);   /* they wake mid-lunge: surprised */
    }
    music(SONG_EXPLORE);
    say("The thralls lie still. Freedom of a kind, you suppose.");
    dlg_close();
}

static void pods_interact(int mx, int my, int m) {
    if (mx == 6 && my == 2) { sh_pod(); return; }
    if (mx == 5 && my == 4) { pod_console(); return; }
    if (mx == 10 && my == 4) { victims_console(); return; }
    if (m == MT_POD_C) { say("Sealed. Whoever is inside is beyond waking."); dlg_close(); return; }
    if (m == MT_POD_O && sh_room_open && mx == 6) { sh_pod(); return; }
    if (m == MT_RESTO) { heal_pod(); return; }
    if (m == MT_CORPSE && mx == 12 && my == 5) {
        if (!(G.flags & GF_RUNE)) {
            G.flags |= GF_RUNE;
            sfx_play(SFX_CONFIRM);
            say("Clutched in a dead thrall's fist: an ELDRITCH RUNE, warm as a coal.");
        } else say("The thrall's hand is empty now. You saw to that.");
        dlg_close();
        return;
    }
    if (m == MT_CORPSE) {
        say("Another thrall. The wires did not keep any of them alive.");
        dlg_close();
        return;
    }
    if (m == MT_FLAYER_CORPSE) {
        say("A dead mind flayer, tentacles splayed. Even its own kind left it where it fell.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.flags & GF_CONSOLE_SEEN)) {
            G.flags |= GF_CONSOLE_SEEN;
            if (isclass(CLS_ROGUE)) {
                say("[ROGUE] An elaborate reliquary. Its lock surrenders in seconds. Two Potions -- and one for style.");
                G.potions = (u8)(G.potions + 3);
            } else {
                say("An elaborate reliquary, locked. You smash it open. Two Potions survive your technique.");
                G.potions = (u8)(G.potions + 2);
            }
            sfx_play(SFX_CONFIRM);
        } else say("The reliquary is a ruin. A pretty one.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ helm */

static void crash_sequence(int flayer_did_it);

static void helm_battle(void) {
    seen |= 0x8000;
    say("The mind flayer's voice cuts through the din: \"NOW, thrall. The nerves!\"");
    if (G.nparty == 1)
        say("You are alone, and the helm is very large. This may sting.");
    dlg_close();
    /* Zhalk and the flayer ARE the combatants; vermin join them */
    int i0 = field_add_npc(5, 4, OBJT_IMPF, 4, 0, NPC_2FRAME);
    int i1 = field_add_npc(9, 4, OBJT_IMPF, 4, 0, NPC_2FRAME);
    EncSpawn hs[6];
    int nh = 0;
    hs[nh].mon = R5M_ZHALK; hs[nh].npc = (s8)n_zh; hs[nh].xp = 300; hs[nh].side = 1; nh++;
    hs[nh].mon = R5M_LESSER_IMP; hs[nh].npc = (s8)i0; hs[nh].xp = 40; hs[nh].side = 1; nh++;
    hs[nh].mon = R5M_LESSER_IMP; hs[nh].npc = (s8)i1; hs[nh].xp = 40; hs[nh].side = 1; nh++;
    hs[nh].mon = R5M_FLAYER; hs[nh].npc = (s8)n_fl; hs[nh].xp = 0; hs[nh].side = 2; nh++;
    if (us_with_us() && n_us >= 0) {
        hs[nh].mon = R5M_DEVOURER; hs[nh].npc = (s8)n_us; hs[nh].xp = 0; hs[nh].side = 2; nh++;
    }
    int r = encounter_run(hs, nh, 15, 0);
    if (r == ENC_CONNECTED) crash_sequence(0);
    else crash_sequence(1);
}

static void helm_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_TRANSP_L || m == MT_TRANSP_R) {
        say("The transponder: a lobe of pilot-brain trailing nerve cables as thick as your wrist.");
        dlg_close();
        if (!(seen & 0x8000)) helm_battle();
        return;
    }
    if (m == MT_RESTO) { heal_pod(); return; }
    if (m == MT_TANK) {
        say("A bulbous tank of raw ichor, glowing from within. Volatile does not begin to cover it.");
        if (isclass(CLS_WIZARD))
            say("[WIZARD] You take four careful steps away from it.");
        dlg_close();
        return;
    }
    if (m == MT_VIEWPORT || m == MT_WINDOW) {
        say("Dragonfire washes across the viewport. It is not a view. It is a countdown.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ dispatch */

/* subclass selection when a member reaches its subclass level (Char 2.0) */
void level_up_choices(void) {
    for (int i = 0; i < G.nparty; i++) {
        PMember* p = &G.pm[i];
        if (p->subclass != 255) continue;                 /* already chosen */
        if (p->level < r5_subclass_level[p->cls]) continue;
        const u8* ids = r5_subclass_of_class[p->cls];
        const char* opts[3]; int map[3], n = 0;
        for (int k = 0; k < 3; k++)
            if (ids[k] != 255) { opts[n] = r5_subclasses[ids[k]].name; map[n] = ids[k]; n++; }
        if (!n) continue;
        {
            char m[40]; char* d = m;
            const char* s = p->name; while (*s) *d++ = *s++;
            s = " walks a new path:"; while (*s) *d++ = *s++; *d = 0;
            say(m);
        }
        int c = choose(n, opts);
        p->subclass = (u8)map[c];
        party5_refresh(i);
        mgba_logf("subclass pick %s -> %d", p->name, p->subclass);
        dlg_close();
    }
}

void ev_interact(int mx, int my) {
    int m = field_meta_at(mx, my);
    switch (cur_room) {
        case RM_NURSERY: nursery_interact(mx, my, m); break;
        case RM_SURGERY: surgery_interact(mx, my, m); break;
        case RM_DECK:    deck_interact(mx, my, m); break;
        case RM_PODS:    pods_interact(mx, my, m); break;
        case RM_HELM:    helm_interact(mx, my, m); break;
    }
}

void ev_step(int mx, int my) {
    int m = field_meta_at(mx, my);
    if (m == MT_DOOR_C) {
        switch (cur_room) {                /* top-wall doors lead backward */
            case RM_NURSERY: door_to(RM_SURGERY, 7, 1); return;
            case RM_SURGERY: door_to(RM_DECK, 10, 1); return;
            case RM_DECK:    door_to(my == 0 ? RM_SURGERY : RM_PODS,
                                     my == 0 ? 7 : 8, my == 0 ? 8 : 1); return;
            case RM_PODS:    if (my == 0) { door_to(RM_DECK, 10, 8); return; }
                             door_to(RM_HELM, 9, 10); return;
        }
    }
    if (cur_room == RM_PODS && mx == 12 && my == 5 && !(G.flags & GF_RUNE)) {
        G.flags |= GF_RUNE;
        sfx_play(SFX_CONFIRM);
        say("Clutched in a dead thrall's fist: an ELDRITCH RUNE, warm as a coal.");
        dlg_close();
        return;
    }
    if (cur_room == RM_DECK && !(G.flags & GF_DECK_FOUGHT) && my <= 3) {
        deck_fight();
        return;
    }
    if (cur_room == RM_HELM && !(seen & 0x8000) && my <= 5) {
        helm_battle();
        return;
    }
}

static int is_wanderer(int idx) {
    return idx >= 0 && (idx == n_wander[0] || idx == n_wander[1]);
}

static void wanderer_fight(int idx, int surprise) {
    u16 dead = idx == n_wander[0] ? GF_W_DECK : GF_W_PODS;
    EncSpawn sp = { R5M_LESSER_IMP, (s8)idx, 40, 1 };
    if (encounter_run(&sp, 1, 0, surprise) == ENC_WIN) G.flags |= dead;
    music(SONG_EXPLORE);
}

void ev_aggro(int idx) {
    if (!is_wanderer(idx)) return;
    say("The imp shrieks -- it has your scent!");
    dlg_close();
    wanderer_fight(idx, 2);          /* it found you: party surprised */
}

void ev_npc(int idx) {
    if (is_wanderer(idx)) {
        if (npcs[idx].chasing) { wanderer_fight(idx, 0); return; }
        say("It hasn't seen you. You strike first!");
        dlg_close();
        wanderer_fight(idx, 1);      /* unaware: enemies surprised */
        return;
    }
    if (idx == n_us) {
        static int said;
        if (!said) { said = 1; SAY_US("US: \"We are us. We are needed to navigate -- needed to LEAVE this realm.\""); }
        else SAY_US("US: \"Us follows. Us fights. Us is very brave.\"");
        dlg_close();
        return;
    }
    if (idx == n_sh && sh_waiting) {
        SAY_SH("SHADOWHEART: \"...Alright. I've reconsidered. Together?\"");
        say_keep("She waits.");
        static const char* const o[] = { "Welcome aboard.", "Still no." };
        if (choose(2, o) == 0) {
            say("Shadowheart joins the party! (She carries a Scroll of Revivify.)");
            party_add_shadowheart();
            field_remove_npc(n_sh);
            n_sh = -1;
            sh_waiting = 0;
        } else SAY_SH("SHADOWHEART: \"Charming to the last.\"");
        dlg_close();
        return;
    }
    if (idx == n_lz) {
        SAY_LZ("LAE'ZEL: \"Speak quickly, or draw steel.\"");
        dlg_close();
        return;
    }
    if (idx == n_zh) {
        say("The cambion has no attention to spare for vermin. Yet.");
        dlg_close();
        return;
    }
    if (idx == n_fl) {
        say("The mind flayer's eyes flick to you, and your tadpole THRUMS in answer. \"The nerves. Go.\"");
        dlg_close();
        return;
    }
}

/* ------------------------------------------------------------ ending */

static void crash_sequence(int flayer_did_it) {
    G_DONE = 2;                 /* harness: finale cutscene reached */
    /* battle already faded out; show narration over black */
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BLDCNT = 0;
    REG_BLDY = 0;

    if (flayer_did_it) {
        say("The last fiend falls. The mind flayer regards you across the silent helm.");
        say("\"You are no longer required.\"");
        say("It wraps the transponder nerves itself, almost gently.");
    } else {
        say("You seize the nerve-cables and JAM them together.");
    }
    say("Claws the size of doors seize the viewport. Dragonfire pours across the console --");
    dlg_close();

    /* white-out */
    music(SONG_CRASH);
    field_shake(40);
    REG_BLDCNT = 0x00BC;    /* brighten the world; text stays readable */
    for (int i = 0; i <= 24; i++) {
        REG_BLDY = (u16)(i * 16 / 24);
        vsync(); vsync();
    }

    say("The nautiloid folds through planes like paper. Sky. Water. Stone. SKY --");
    say("...");
    dlg_close();

    /* the ravaged beach */
    REG_BLDCNT = 0;
    REG_BLDY = 0;
    field_load(map_beach, MAP_BEACH_W, MAP_BEACH_H);
    field_spawn(8, 3, 0);
    field_draw();
    music(SONG_PRELUDE);
    fade_in(40);
    field_wait(70);
    say("Gulls. Surf. The smell of your own scorched hair.");
    say("You wake on a ravaged beach, alone in the wreckage. Alive. And not alone for long.");
    dlg_close();
    field_wait(30);

    /* tally */
    scr_tally();
    {
        static const char* const clsn[CLS_COUNT] = { "Bard", "Rogue", "Ranger",
            "Wizard", "Fighter", "Cleric", "Barbarian", "Druid", "Monk",
            "Paladin", "Sorcerer", "Warlock" };
        txt_put_n(SCR_TALLY_WHO_X, SCR_TALLY_WHO_Y, G.pm[0].name, 0, SCR_TALLY_WHO_W);
        txt_put_n(SCR_TALLY_CLS_X, SCR_TALLY_CLS_Y, clsn[G.pm[0].cls], 0, SCR_TALLY_CLS_W);
    }
#define TVAL(slot, cond, yes, no) \
    txt_put_n(SCR_TALLY_##slot##_X, SCR_TALLY_##slot##_Y, (cond) ? (yes) : (no), \
              (cond) ? 1 : 2, SCR_TALLY_##slot##_W)
    TVAL(V_US, us_with_us(), "YES", (G.flags & GF_US_MUTILATED) ? "HURT" : "no");
    TVAL(V_LZ, G.flags & GF_LAEZEL, "ALLY", "no");
    TVAL(V_SH, G.flags & GF_SH_FREED, "SAVED", "no");
    TVAL(V_ZH, G.flags & GF_ZHALK_DEAD, "SLAIN", "fled");
    TVAL(V_EB, G.everburn, "TAKEN", "-");
#undef TVAL

    G_DONE = 1;
    for (;;) {
        frame();
        if (key_hit() & KEY_START) break;
        if (G_DEMO) { field_wait(120); break; }
    }
    /* soft reset back to the title */
    *(vu8*)0x03007FFA = 0;
    __asm volatile("swi 0x00" ::: "r0", "r1", "r2", "r3", "memory");
    for (;;) frame();
}

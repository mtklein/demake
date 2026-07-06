/* Story content: rooms, interactions, cutscenes, the helm finale. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "events.h"
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
static u16 seen;                     /* room-intro-seen bits */
static int sh_room_open;             /* Shadowheart pod opened this game */
static int sh_waiting;               /* freed but not yet recruited */

/* beach npcs -- room_enter resets these before any dispatch can read them
 * (initializers here would land in .data and shift G, whose address the
 * scenario pokes depend on) */
static int n_shb;                    /* Shadowheart ashore (met or unconscious) */
static int n_scav[2];                /* the tiefling scavengers */

/* wandering on-map encounters: per-room patrol slots. Each remembers its
 * stat block, bounty, and the story bit that keeps it dead (ship kills live
 * in G.flags, beach kills in G.bflags). */
static int n_wander[2] = { -1, -1 };
static u8  wander_mon[2];
static u16 wander_xp[2];
static u16* wander_word[2];
static u16 wander_bit[2];

static void add_wanderer(int slot, int mx, int my, int objt, int pal, int face,
                         int mon, u16 xp, u16* word, u16 bit, int radius) {
    if (*word & bit) return;             /* already slain: stays slain */
    int i = field_add_npc(mx, my, objt, pal, face, NPC_2FRAME);
    if (i < 0) return;
    field_npc_patrol(i, radius);
    n_wander[slot] = i;
    wander_mon[slot] = (u8)mon;
    wander_xp[slot] = xp;
    wander_word[slot] = word;
    wander_bit[slot] = bit;
}

static int isclass(int c) { return G.pm[0].cls == c; }
static R5RNG fchk_rng;
static int fchk_seeded;
static char* put_num(char* d, int v) {
    if (v < 0) { *d++ = '-'; v = -v; }
    char t[6]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = t[--n];
    return d;
}
/* a skill check on the field: roll a d20 through the dialog, show it, vs DC */
static int field_check(int skill, int dc) {
    if (!fchk_seeded) { r5_seed(&fchk_rng, rnd() ^ 0x5EED); fchk_seeded = 1; }
    int d20;
    int total = r5_skill_check(&fchk_rng, &party5[0], skill,
                               G.pm[0].skills, G.pm[0].expert, &d20);
    int ok = total >= dc;
    char m[48]; char* d = m;
    const char* s = r5_skill_name[skill]; while (*s) *d++ = *s++;
    *d++ = ' ';
    d = put_num(d, d20); *d++ = '+';
    d = put_num(d, total - d20); *d++ = '=';
    d = put_num(d, total);
    s = ok ? " vs " : " vs "; while (*s) *d++ = *s++;
    d = put_num(d, dc); *d++ = ' ';
    s = ok ? "PASS" : "FAIL"; while (*s) *d++ = *s++;
    *d = 0;
    mgba_logf("field-check %s d20=%d tot=%d dc=%d %s",
              r5_skill_name[skill], d20, total, dc, ok ? "pass" : "fail");
    sfx_play(d20 == 20 ? SFX_HEAL : ok ? SFX_CONFIRM : SFX_CANCEL);
    say(m);
    return ok;
}

static int is_durge(void) { return G.origin == ORIG_DURGE; }
#define URGE(t) do { if (is_durge()) { mgba_log("urge-line"); say("[URGE] " t); } } while (0)
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

static int room_song(int id) {
    if (id == RM_HELM) return SONG_BOSS;
    if (id == RM_BEACH || id == RM_DUNES) return SONG_GAIA;
    return SONG_EXPLORE;
}

void room_enter(int id, int sx, int sy, int face) {
    mgba_logf("room_enter %d at %d,%d flags=%x", id, sx, sy, G.flags);
    music(room_song(id));
    cur_room = id;
    crumb(CR_ROOM, id);
    n_us = n_lz = n_sh = n_zh = n_fl = -1;
    n_imp[0] = n_imp[1] = n_imp[2] = -1;
    n_wander[0] = n_wander[1] = -1;
    n_shb = n_scav[0] = n_scav[1] = -1;

    switch (id) {
        case RM_NURSERY: field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H); break;
        case RM_SURGERY: field_load(map_surgery, MAP_SURGERY_W, MAP_SURGERY_H); break;
        case RM_DECK:    field_load(map_deck, MAP_DECK_W, MAP_DECK_H); break;
        case RM_PODS:    field_load(map_pods, MAP_PODS_W, MAP_PODS_H); break;
        case RM_HELM:    field_load(map_helm, MAP_HELM_W, MAP_HELM_H); break;
        case RM_BEACH:   field_load(map_beach, MAP_BEACH_W, MAP_BEACH_H); break;
        case RM_DUNES:   field_load(map_dunes, MAP_DUNES_W, MAP_DUNES_H); break;
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
            if (G.flags & GF_DECK_FOUGHT)
                /* a straggler imp prowls the far rail -- avoidable */
                add_wanderer(0, 16, 7, OBJT_IMPF, 4, 0,
                             R5M_LESSER_IMP, 40, &G.flags, GF_W_DECK, 28);
            break;
        case RM_PODS:
            if (sh_room_open) field_set_meta(6, 2, MT_POD_O);
            if (G.flags & GF_RUNE) field_set_meta(5, 4, MT_CONSOLE_LIT);
            if (sh_waiting) n_sh = field_add_npc(6, 3, OBJT_SHADOW, 2, 0, 0);
            if (us_with_us()) n_us = field_add_npc(14, 8, OBJT_US, 3, 0, NPC_2FRAME);
            add_wanderer(1, 15, 2, OBJT_IMPF, 4, 0,
                         R5M_LESSER_IMP, 40, &G.flags, GF_W_PODS, 28);
            break;
        case RM_HELM:
            n_zh = field_add_npc(4, 2, OBJT_ZHALKF, 6, 3, NPC_2FRAME);
            n_fl = field_add_npc(7, 2, OBJT_FLAYERF, 5, 2, NPC_2FRAME);
            if (us_with_us()) n_us = field_add_npc(12, 7, OBJT_US, 3, 0, NPC_2FRAME);
            break;
        case RM_BEACH:
            if (G.origin == ORIG_SHADOW) {
                /* story surgery: the player IS Shadowheart -- no one to
                 * recover, only her own pod beached where a body would be */
                field_set_meta(4, 7, MT_POD_O);
                mgba_log("beach reroute shadowheart");
            } else if (!(G.bflags & BF_SH_RECOVERED)) {
                if (G.flags & GF_SH_FREED)
                    n_shb = field_add_npc(11, 6, OBJT_SHADOW, 2, 2, 0);
                else
                    n_shb = field_add_npc(4, 7, OBJT_SHADOW_KO, 2, 0, 0);
            }
            add_wanderer(0, 15, 2, OBJT_DEVF, 5, 0,
                         R5M_DEVOURER, 50, &G.bflags, BF_DEV_CRASH, 28);
            break;
        case RM_DUNES:
            if (G.origin == ORIG_LAEZEL) {
                /* story surgery: the player IS the githyanki -- the cage
                 * stands sprung and empty, the scavengers twice as scared */
                field_set_meta(2, 2, MT_CAGE_OPEN);
                mgba_log("beach reroute laezel");
            } else if (G.bflags & BF_LZ_RECOVERED) {
                field_set_meta(2, 2, MT_CAGE_OPEN);
            }
            if (!(G.bflags & BF_SCAVS_GONE)) {
                n_scav[0] = field_add_npc(1, 3, OBJT_SCAV, 4, 0, NPC_2FRAME);
                n_scav[1] = field_add_npc(3, 3, OBJT_SCAV, 4, 0, NPC_2FRAME);
            }
            add_wanderer(0, 12, 7, OBJT_DEVF, 5, 0,
                         R5M_DEVOURER, 50, &G.bflags, BF_DEV_DUNE, 28);
            add_wanderer(1, 5, 9, OBJT_DEVF, 5, 0,
                         R5M_DEVOURER, 50, &G.bflags, BF_DEV_DUNE2, 28);
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
    URGE("Behind the passenger, something older stirs, and it is glad. It missed the killing.");
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
            say("A dead thrall, days gone. A rune slate lies by his hand, dense with glyphs.");
            if (field_check(SK_ARCANA, 12)) {
                say("The script yields: ships between stars, a thousand worlds harvested.");
                say("And a word your captors fear -- you file it away.");
            } else {
                say("The glyphs swim and refuse you. Just malice, and a headache.");
            }
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
    URGE("So soft. So exposed. It would take one squeeze. You could just... watch.");
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
    URGE("Trusting. They always trust you, right up until. Mutilate is such a warm word.");
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

/* ------------------------------------------------------------ the beach */

void level_up_choices(void);

/* xp for a resolved story beat (the encounter engine announces its own) */
static void beat_xp(u16 xp) {
    char names[32];
    sfx_play(SFX_CONFIRM);
    if (party_give_xp(xp, names)) {
        party5_refresh_all();
        say("A new level!");
        level_up_choices();
    }
}

/* open-air room change: no sphincter doors out here */
static void beach_go(int next, int sx, int sy) {
    G_FIELD_IDLE = 0;
    sfx_play(SFX_CONFIRM);
    fade_out(14);
    dlg_close();
    room_enter(next, sx, sy, 0);
    field_draw();
    fade_in(14);
    if (!(seen & (1 << next))) {
        seen |= (u16)(1 << next);
        if (next == RM_DUNES) {
            say("The dunes swallow the surf-sound. Small tracks stitch every slope: clawed, busy, WRONG.");
            dlg_close();
        }
    }
}

/* Tav wakes alone on the sand -- the moment the prologue used to spend on
 * a tally screen. Reached from the crash sequence, or directly by a
 * G_DEMO_BEACH boot (test/scenario.py beach_setup). */
void beach_wake(void) {
    party_scatter();                 /* companions scatter; the fall is the
                                      * arc's narrative long rest */
    room_enter(RM_BEACH, 10, 6, 0);
    field_draw();
    fade_in(40);
    field_wait(70);
    say("Gulls. Surf. The smell of your own scorched hair.");
    say("You wake on a ravaged beach, alone in the wreckage. Alive.");
    say("Behind your eye, the passenger stirs, and settles. Patient.");
    if (G.origin != ORIG_SHADOW && (G.flags & GF_SH_FREED))
        say("Down the sand, dark hair and darker mail: a familiar shape, upright and scowling at the sea.");
    if (us_with_us())
        say("Tiny claw-prints circle your landing and strike out inland. Us walked away from this. Us is very brave.");
    URGE("You wake smiling. Something in the wreck died screaming, and part of you kept the sound.");
    dlg_close();
    seen |= 1 << RM_BEACH;
    mgba_logf("beach wake flags=%x origin=%d nparty=%d",
              G.flags, G.origin, G.nparty);
    G_DONE = 1;   /* harness: the prologue's story moment -- scenarios sync
                   * here exactly as they did on the retired tally */
}

/* --- the dying mind flayer, half-crushed under its own ship (5,4) --- */

static void flayer_beat(void) {
    if (G.bflags & BF_FLAYER_DONE) {
        say(G.bflags & BF_FLAYER_SLAIN
            ? "The mind flayer lies still. The tide has already started burying it."
            : "The mind flayer is dead. The sea got there before mercy did.");
        dlg_close();
        return;
    }
    say("Pinned under a rib of the hull: a mind flayer. Dying. Its eyes find yours --");
    say("-- and COLD FINGERS close around your mind. THRALL. KNEEL. SERVE.");
    if (field_check(SK_ARCANA, 12)) {
        say("You know this grip now. You picture the ship burning around it, and SHOVE.");
        say("The hold shatters. The creature sags, spent.");
    } else {
        say("Your knee hits the sand before you know it's yours. Then the grip fails -- not you. It is too weak to keep you.");
    }
    URGE("It is helpless. No one would see. No one would EVER know.");
    say_keep("It watches you, tentacles curling feebly against the sand.");
    static const char* const o[] = { "Finish it", "Leave it to die" };
    if (choose(2, o) == 0) {
        say("You work a shard of hull plating loose. It is quick. It is not gentle.");
        say("The pressure behind your eye flinches -- a door slamming somewhere far away.");
        G.bflags |= BF_FLAYER_SLAIN;
        mgba_log("flayer beat finished");
    } else {
        say("You step back. The sea is patient, and so is dying.");
        say("Something follows you down the beach: gratitude, or contempt. With their kind, the difference is thin.");
        mgba_log("flayer beat spared");
    }
    G.bflags |= BF_FLAYER_DONE;
    say("Gained 25 XP.");
    beat_xp(25);
    dlg_close();
}

/* --- Shadowheart ashore: (11,6) awake if freed on ship, else (4,7) --- */

static void sh_recovered(void) {
    party_add_shadowheart();
    if (n_shb >= 0) field_remove_npc(n_shb);
    n_shb = -1;
    G.bflags |= BF_SH_RECOVERED;
    mgba_log("beach recover shadowheart");
    dlg_close();
}

static void sh_shore_meet(void) {        /* GF_SH_FREED: she swam too */
    field_face_npc(n_shb, field_player_mx() < 11 ? 2 : 3);
    SAY_SH("SHADOWHEART: \"So the pod-openers float. Good. I'd hate to owe a corpse.\"");
    SAY_SH("SHADOWHEART: \"Whatever dragged us out of the sky, we walked away from it. Same terms as the ship, then -- together.\"");
    say("Shadowheart rejoins the party!");
    sh_recovered();
}

static void sh_shore_wake(void) {        /* left behind: the surf coughed her up */
    say("Face-down at the tide line: a woman in dark mail. The one from the pod... breathing. Barely.");
    if (field_check(SK_MEDICINE, 10)) {
        say("You turn her head, clear her throat, and press the sea out of her in steady pushes. She convulses -- and coughs.");
        SAY_SH("SHADOWHEART: \"*hkk* -- get -- OFF -- ...you. From the ship. So you do stop for strangers. Eventually.\"");
    } else {
        say("You pound her back like a drum: less physician than percussionist. The sea fixes what you can't -- she retches awake on her own.");
        SAY_SH("SHADOWHEART: \"...Was that your idea of HEALING? Remind me to drown somewhere you aren't.\"");
    }
    SAY_SH("SHADOWHEART: \"Still. A graceless rescue beats a grave. I'll come along -- someone here clearly needs watching.\"");
    say("Shadowheart joins the party! (She carries a Scroll of Revivify.)");
    sh_recovered();
}

/* --- the scavenger cage on the dune path (2,2) --- */

static void scavs_flee(void) {
    if (n_scav[0] >= 0) field_remove_npc(n_scav[0]);
    if (n_scav[1] >= 0) field_remove_npc(n_scav[1]);
    n_scav[0] = n_scav[1] = -1;
    G.bflags |= BF_SCAVS_GONE;
}

static void cage_beat(void) {
    if (G.origin == ORIG_LAEZEL) {
        say("A cage of lashed bone and driftwood, torn open from the INSIDE. Whoever they caught didn't stay caught.");
        say("You approve.");
        dlg_close();
        return;
    }
    if (G.bflags & BF_LZ_RECOVERED) {
        say("The sprung cage creaks in the wind. It was never going to be enough.");
        dlg_close();
        return;
    }
    say("A cage of lashed bone and driftwood. Folded double inside, and furious about it: a githyanki warrior.");
    if (G.flags & GF_LAEZEL)
        SAY_LZ("LAE'ZEL: \"Chk. Of course it is you. The fates enjoy their little symmetries. CUT ME LOOSE.\"");
    else
        SAY_LZ("LAE'ZEL: \"You. Creature. Open this cage and I shall let you live. This is generosity.\"");
    if (n_scav[0] >= 0)
        say("The two tieflings scramble between you and the bars. \"Don't! It's a MONSTER -- it said it would eat our HEARTS!\"");
    say_keep("The gith's glare could strip paint.");
    static const char* const o[] = { "Open the cage", "Leave her caged" };
    if (choose(2, o) == 1) {
        SAY_LZ("LAE'ZEL: \"TSK'VA! Crawl back to the surf, coward!\"");
        dlg_close();
        return;
    }
    say("You unpick the lashings. The tieflings bolt before the second knot gives, wailing about hearts.");
    scavs_flee();
    sfx_play(SFX_CONFIRM);
    field_set_meta(2, 2, MT_CAGE_OPEN);
    if (G.flags & GF_LAEZEL)
        SAY_LZ("LAE'ZEL: \"You took your time. The ship is gone; Vlaakith's purpose is not. And you still carry the ghaik worm -- so our roads stay one road.\"");
    else
        SAY_LZ("LAE'ZEL: \"Hm. You keep your word even to strangers. Foolish. Useful. I am Lae'zel of Creche K'liir. We survive together, or not at all.\"");
    say("Lae'zel joins the party!");
    party_add_laezel();
    G.bflags |= BF_LZ_RECOVERED;
    mgba_log("beach recover laezel");
    dlg_close();
}

static void scav_talk(void) {
    if (G.origin == ORIG_LAEZEL) {
        say("\"AAAH! Another one! The cage didn't hold the FIRST one!\" They look at your face, then at each other, and RUN.");
        scavs_flee();
        dlg_close();
        return;
    }
    if (G.bflags & BF_LZ_RECOVERED) { scavs_flee(); return; }
    say("Two soot-streaked tieflings, all horns and nerves. \"We caught it FAIR! It fell right out of the sky!\"");
    say("\"...Do you want it? Please want it.\"");
    dlg_close();
}

/* --- interactions --- */

static void beach_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_FLAYER_DYING) { flayer_beat(); return; }
    if (m == MT_WRECK_L || m == MT_WRECK_R) {
        say("A rib of the nautiloid, tall as a house, buried nose-down in the sand. Still warm.");
        if (G.flags & GF_ZHALK_DEAD)
            say("Wedged in the plating: a scorched cambion pauldron. Zhalk's. The commander fell with his prize.");
        else if (G.flags & GF_DECK_FOUGHT)
            say("Scorch-lines rake the hull where the dragons found their mark.");
        dlg_close();
        return;
    }
    if (m == MT_WRECK) {
        say("A shard of hull. The flesh of it is already drying to leather in the sun.");
        dlg_close();
        return;
    }
    if (m == MT_POD_O) {   /* origin Shadowheart: her own pod, beached */
        say("A pod, burst open on the rocks. YOUR pod -- it followed you all the way down.");
        say("Lady Shar gives nothing back, but the sea apparently does.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.bflags & BF_CHEST_BEACH)) {
            G.bflags |= BF_CHEST_BEACH;
            sfx_play(SFX_CONFIRM);
            say("A ship's chest, burst on the rocks. One Potion survived the landing, still floating in its brine.");
            G.potions++;
        } else say("Empty, and full of sand besides.");
        dlg_close();
        return;
    }
    if (m == MT_ROCK) { say("Sea-worn stone, warm in the sun."); dlg_close(); return; }
    if (m == MT_SURF || m == MT_SEA) {
        say("The sea rolls in, patient as ever. Somewhere under it: most of a nautiloid.");
        dlg_close();
        return;
    }
    if (m == MT_DUNE) {
        say("Dune grass whispers on the ridge. The gap in the bank leads inland.");
        dlg_close();
    }
}

static void dunes_interact(int mx, int my, int m) {
    (void)mx;
    if (m == MT_CAGE || m == MT_CAGE_OPEN) { cage_beat(); return; }
    if (m == MT_ROCK) {
        if (my == 0) {
            /* stone 4's door: the chapel on the bluff, teased not opened */
            say("The path climbs north between the rocks. On the bluff above: a ruined chapel, crows wheeling over it.");
            say("Rockfall chokes the pass. You'd need to clear it -- another day.");
        } else say("Sea-worn stone, warm in the sun.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.bflags & BF_CHEST_DUNE)) {
            G.bflags |= BF_CHEST_DUNE;
            sfx_play(SFX_CONFIRM);
            say("A scavenger cache, half-buried: two Potions and a fistful of shells. The shells are worthless. Probably.");
            G.potions = (u8)(G.potions + 2);
        } else say("Just the shells now. Still worthless.");
        dlg_close();
        return;
    }
    if (m == MT_DUNE) {
        say("Wind-carved sand, tufted with grass. The dunes go on and on.");
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
        int c = G_DEMO ? 0 : choose(n, opts);   /* demo: auto-pick, don't eat the choice buffer */
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
        case RM_BEACH:   beach_interact(mx, my, m); break;
        case RM_DUNES:   dunes_interact(mx, my, m); break;
    }
}

void ev_skill_test(int skill, int dc) { field_check(skill, dc); dlg_close(); }

void ev_step(int mx, int my) {
    int m = field_meta_at(mx, my);
    if (cur_room == RM_BEACH) {
        if (my == 0) { beach_go(RM_DUNES, mx, 10); return; }   /* the dune gap */
        if (!(G.bflags & BF_FLAYER_DONE)) {
            /* step within reach of the dying flayer (5,4) and it grasps */
            int dx = mx - 5, dy = my - 4;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx + dy <= 1) { flayer_beat(); return; }
        }
        return;
    }
    if (cur_room == RM_DUNES) {
        if (my == 11) beach_go(RM_BEACH, mx, 1);   /* back to the crash site */
        return;
    }
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
    int s = idx == n_wander[0] ? 0 : 1;
    EncSpawn sp = { wander_mon[s], (s8)idx, wander_xp[s], 1 };
    if (encounter_run(&sp, 1, 0, surprise) == ENC_WIN)
        *wander_word[s] |= wander_bit[s];
    music(room_song(cur_room));
}

void ev_aggro(int idx) {
    if (!is_wanderer(idx)) return;
    int s = idx == n_wander[0] ? 0 : 1;
    if (wander_mon[s] == R5M_DEVOURER)
        say("The brain-thing SKITTERS around, claws up. It has tasted your thoughts!");
    else
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
    if (idx >= 0 && idx == n_shb) {
        if (G.flags & GF_SH_FREED) sh_shore_meet();
        else sh_shore_wake();
        return;
    }
    if (idx >= 0 && (idx == n_scav[0] || idx == n_scav[1])) {
        scav_talk();
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

/* ------------------------------------------------------------ the crash */

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

    REG_BLDCNT = 0;
    REG_BLDY = 0;
    /* The prologue tally that used to live here is retired: the game no
     * longer ends at the crash. Its accounting returns at the grove-gates
     * finale (beach arc, stone 6). The story lands on the sand instead. */
    beach_wake();
    /* ...and control returns to field_run: the beach is playable. */
}

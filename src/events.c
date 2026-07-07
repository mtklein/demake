/* Story content: rooms, interactions, cutscenes, the helm finale. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "events.h"
#include "encounter.h"
#include "party5.h"
#include "screens.h"     /* the tally layout (tools/ui_screens.py) */
#include "dice_ui.h"     /* the shared tumbling die, over the dialog */

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
#ifdef POR_ASTARION
#define SAY_AST(t) say_p(POR_ASTARION, t)
#else
#define SAY_AST(t) say(t)
#endif
#ifdef POR_GALE
#define SAY_GA(t) say_p(POR_GALE, t)
#else
#define SAY_GA(t) say(t)
#endif
#ifdef POR_ZEVLOR
#define SAY_ZV(t) say_p(POR_ZEVLOR, t)
#else
#define SAY_ZV(t) say(t)
#endif
#ifdef POR_WYLL
#define SAY_WY(t) say_p(POR_WYLL, t)
#else
#define SAY_WY(t) say(t)
#endif
#ifdef POR_WITHERS
#define SAY_WI(t) say_p(POR_WITHERS, t)
#else
#define SAY_WI(t) say(t)
#endif
#ifdef POR_FLAYER
#define SAY_FL(t) say_p(POR_FLAYER, t)
#else
#define SAY_FL(t) say(t)
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
static int n_ast;                    /* the pale elf lurking by the grass */
static int n_boar;                   /* origin Astarion's staked kill */
static int n_loot[3];                /* the looter band at the tomb door */
static int n_warryn;                 /* the small looter in the mask. That is
                                      * all the game will ever say about him. */
static int n_withers;                /* the keeper of the crypt's ledgers */
static int n_zev;                    /* Zevlor, holding the grove door */
static int n_wyllg;                  /* the Blade of Frontiers, out front */
static int n_gob[6];                 /* the goblin assault (last = warchief) */
static int n_camp[5];                /* companions around the campfire */
static const PMember* camp_member[5];

/* the fire at (7,4); whoever the beach has given back stands in its light
 * (five spots: the full six-soul roster, less Tav) */
static const struct { s8 mx, my, face; } camp_spot[5] = {
    { 6, 4, 3 }, { 8, 4, 2 }, { 7, 5, 1 }, { 6, 5, 1 }, { 8, 5, 1 },
};

static void camp_add(int k, const PMember* p) {
    MemberLook L = member_look(p->face, p->cls);
    n_camp[k] = field_add_npc(camp_spot[k].mx, camp_spot[k].my,
                              L.objt, L.pal, camp_spot[k].face, 0);
    camp_member[k] = p;
}

/* wandering on-map encounters: per-room patrol slots. Each remembers its
 * stat block, bounty, and the story bit that keeps it dead (ship kills live
 * in G.flags, beach kills in G.bflags -- u16 and u32 words, so the slot
 * carries which word rather than a pointer of either width). */
static int n_wander[2] = { -1, -1 };
static u8  wander_mon[2];
static u16 wander_xp[2];
static u8  wander_ship[2];           /* 1 = G.flags, 0 = G.bflags */
static u32 wander_bit[2];

static u32 wander_word(int s) {
    return wander_ship[s] ? G.flags : G.bflags;
}
static void wander_latch(int s) {
    if (wander_ship[s]) G.flags |= (u16)wander_bit[s];
    else G.bflags |= wander_bit[s];
}

static void add_wanderer(int slot, int mx, int my, int objt, int pal, int face,
                         int mon, u16 xp, int ship, u32 bit, int radius) {
    wander_ship[slot] = (u8)ship;
    wander_bit[slot] = bit;
    if (wander_word(slot) & bit) return;   /* already slain: stays slain */
    int i = field_add_npc(mx, my, objt, pal, face, NPC_2FRAME);
    if (i < 0) return;
    field_npc_patrol(i, radius);
    n_wander[slot] = i;
    wander_mon[slot] = (u8)mon;
    wander_xp[slot] = xp;
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
/* The field's echo of combat's dice tray: the skill-check d20, tumbled over
 * the open dialog box through the SAME dice_ui primitive. It claims field OAM
 * slot OBJ_FDIE (face + up to two digits) at the box's right margin -- clear
 * of the text column (max col 27) and the more-arrow (row 4) on every page --
 * and is hidden on every return to the field loop (ev_fdie_hide, called from
 * field_run) so it can never linger into a menu or the next room. */
#define FDIE_X 224        /* col 28: right of the widest dialog line */
#define FDIE_Y 14

void ev_fdie_hide(void) { for (int i = 0; i < 3; i++) obj_hide(OBJ_FDIE + i); }

/* the roll is already decided (d20): stage the digits the field hasn't, spin
 * the face a beat, land on the real value -- gold on a nat 20, a red thud on a
 * nat 1, else the plain grey die. Presentational only: dice_ui spins a
 * throwaway counter, so nothing here draws from fchk_rng or moves the result. */
static void fdie_show(int d20) {
    if (!dlg_is_open()) dlg_open();          /* a box for the die to sit over */
    dice_stage_digits();
    u8 v = (u8)d20;
    int pal = d20 == 20 ? 9 : d20 == 1 ? 11 : 10;
    dice_roll_headline(OBJ_FDIE, FDIE_X, 0, FDIE_Y, 20, &v, 1, d20, pal, frame);
    for (int i = 0; i < 8; i++) frame();     /* a beat on the landed value */
    mgba_logf("field-die d20=%d shown", d20);
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
    fdie_show(d20);                     /* the die tumbles and lands over the box */
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
    if (id == RM_BEACH || id == RM_DUNES) return SONG_TADPOLE;  /* surf, on the surf */
    if (id == RM_CHAPEL || id == RM_GATES) return SONG_GAIA;    /* hymn to the door */
    if (id == RM_CAMP) return SONG_SELUNE;   /* the song was written for here */
    return SONG_EXPLORE;
}

/* The darkvision doctrine's field half (docs/character2.md): DARK rooms dim
 * the screen a half-step, keyed off Tav's own darkvision; the equipped
 * Everburn Blade lights the room outright (encounter_dark answers both, and
 * the battle-side disadvantage keys per-actor off the same flag). */
static int room_dark(int id) {
    return id == RM_CRYPT || id == RM_OSSUARY || id == RM_SANCTUM;
}

void ev_light(void) {
    int dim = encounter_dark() && !(party5[0].traits & TR_DARKVISION);
    if (dim) {
        REG_BLDCNT = 0x00DC;   /* darken field + sky + sprites (BG2|BG3|OBJ);
                                * the text layers stay readable */
        REG_BLDY = 8;          /* the half-step */
    } else {
        REG_BLDCNT = 0;
        REG_BLDY = 0;
    }
    mgba_logf("dark room=%d dim=%d", cur_room, dim);
}

void room_enter(int id, int sx, int sy, int face) {
    mgba_logf("room_enter %d at %d,%d flags=%x", id, sx, sy, G.flags);
    music(room_song(id));
    cur_room = id;
    encounter_set_dark(room_dark(id));
    crumb(CR_ROOM, id);
    n_us = n_lz = n_sh = n_zh = n_fl = -1;
    n_imp[0] = n_imp[1] = n_imp[2] = -1;
    n_wander[0] = n_wander[1] = -1;
    n_shb = n_scav[0] = n_scav[1] = -1;
    n_ast = n_boar = -1;
    n_loot[0] = n_loot[1] = n_loot[2] = -1;
    n_warryn = -1;
    n_withers = -1;
    n_zev = n_wyllg = -1;
    for (int i = 0; i < 6; i++) n_gob[i] = -1;
    for (int i = 0; i < 5; i++) n_camp[i] = -1;

    /* the camp is the arc's one night room: the beach tile family re-lit
     * by moonlight over BG palette 3; every other room restores daylight
     * (the fire indices stay warm -- see field_tiles.NIGHT_PAL) */
    memcpy16(PAL_BG + 48, id == RM_CAMP ? pal_field_night : pal_bg + 48, 16);

    switch (id) {
        case RM_NURSERY: field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H); break;
        case RM_SURGERY: field_load(map_surgery, MAP_SURGERY_W, MAP_SURGERY_H); break;
        case RM_DECK:    field_load(map_deck, MAP_DECK_W, MAP_DECK_H); break;
        case RM_PODS:    field_load(map_pods, MAP_PODS_W, MAP_PODS_H); break;
        case RM_HELM:    field_load(map_helm, MAP_HELM_W, MAP_HELM_H); break;
        case RM_BEACH:   field_load(map_beach, MAP_BEACH_W, MAP_BEACH_H); break;
        case RM_DUNES:   field_load(map_dunes, MAP_DUNES_W, MAP_DUNES_H); break;
        case RM_CHAPEL:  field_load(map_chapel, MAP_CHAPEL_W, MAP_CHAPEL_H); break;
        case RM_CRYPT:   field_load(map_crypt, MAP_CRYPT_W, MAP_CRYPT_H); break;
        case RM_OSSUARY: field_load(map_ossuary, MAP_OSSUARY_W, MAP_OSSUARY_H); break;
        case RM_SANCTUM: field_load(map_sanctum, MAP_SANCTUM_W, MAP_SANCTUM_H); break;
        case RM_CAMP:    field_load(map_camp, MAP_CAMP_W, MAP_CAMP_H); break;
        case RM_GATES:   field_load(map_gates, MAP_GATES_W, MAP_GATES_H); break;
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
                             R5M_LESSER_IMP, 40, 1, GF_W_DECK, 28);
            break;
        case RM_PODS:
            if (sh_room_open) field_set_meta(6, 2, MT_POD_O);
            if (G.flags & GF_RUNE) field_set_meta(5, 4, MT_CONSOLE_LIT);
            if (sh_waiting) n_sh = field_add_npc(6, 3, OBJT_SHADOW, 2, 0, 0);
            if (us_with_us()) n_us = field_add_npc(14, 8, OBJT_US, 3, 0, NPC_2FRAME);
            add_wanderer(1, 15, 2, OBJT_IMPF, 4, 0,
                         R5M_LESSER_IMP, 40, 1, GF_W_PODS, 28);
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
            if (G.origin == ORIG_ASTARION) {
                /* story surgery: the lurker with the knife is the player.
                 * What remains of his beat is the kill he staked out. */
                if (!(G.bflags & BF_BOAR_DRAINED))
                    n_boar = field_add_npc(2, 5, OBJT_BOARW, 3, 2, 0);
                mgba_log("beach reroute astarion");
            } else if (!(G.bflags & BF_AST_RECRUITED)) {
                /* the rogue walker IS his recruited look (member_look) */
                MemberLook L = member_look(ORIG_ASTARION, CLS_ROGUE);
                n_ast = field_add_npc(2, 5, L.objt, L.pal, 0, 0);
            }
            add_wanderer(0, 15, 2, OBJT_DEVF, 5, 0,
                         R5M_DEVOURER, 50, 0, BF_DEV_CRASH, 28);
            break;
        case RM_DUNES:
            if (G.origin == ORIG_GALE) mgba_log("beach reroute gale");
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
                         R5M_DEVOURER, 50, 0, BF_DEV_DUNE, 28);
            add_wanderer(1, 5, 9, OBJT_DEVF, 5, 0,
                         R5M_DEVOURER, 50, 0, BF_DEV_DUNE2, 28);
            break;
        case RM_CHAPEL:
            if (G.bflags & BF_TOMB_OPEN) {
                field_set_meta(7, 1, MT_TOMB_DOOR_O);
                field_set_meta(8, 1, MT_TOMB_DOOR_O);
            }
            if (!(G.bflags & BF_LOOTERS_GONE)) {
                /* the band argues before the door: human tomb-robbers in
                 * their own bare-headed, pry-bar silhouette -- not the
                 * hooded tiefling scavengers of the beach */
                n_loot[0] = field_add_npc(6, 2, OBJT_LOOTER, 7, 0, NPC_2FRAME);
                n_loot[1] = field_add_npc(8, 2, OBJT_LOOTER, 7, 0, NPC_2FRAME);
                n_loot[2] = field_add_npc(7, 3, OBJT_LOOTER, 7, 0, NPC_2FRAME);
                /* and one small figure at the edge of the ring, masked.
                 * The game never explains him -- the sprite is the story. */
                n_warryn = field_add_npc(5, 2, OBJT_WARRYN, 5, 0, NPC_2FRAME);
            }
            break;
        case RM_SANCTUM:
            if (G.bflags & BF_WITHERS_AWAKE) {
                field_set_meta(6, 2, MT_SARC_OT);
                field_set_meta(6, 3, MT_SARC_OB);
                n_withers = field_add_npc(7, 4, OBJT_WITHERSF, 5, 0, NPC_2FRAME);
            }
            break;
        case RM_CAMP: {
            /* the roster knows who made it back: the walking two plus the
             * bench take the spots around the fire, up to all five */
            int k = 0;
            for (int i = 1; i < G.nparty && k < 5; i++) camp_add(k++, &G.pm[i]);
            for (int r = 0; r < G.nreserve && k < 5; r++) camp_add(k++, &G.reserve[r]);
            mgba_logf("camp souls=%d", k + 1);
            break;
        }
        case RM_GATES:
            /* Zevlor holds the door, before and after */
            n_zev = field_add_npc(7, 2, OBJT_ZEVLOR, 6, 0, 0);
            if (!(G.bflags & BF_GATES_WON)) {
                /* the assault, mid-swing: the Blade of Frontiers out front
                 * with two goblins on him, the rest pressing the wall */
                if (G.origin != ORIG_WYLL) {
                    n_wyllg = field_add_npc(5, 5, OBJT_WYLL, 4, 3, 0);
                    mgba_log("gates wyll dueling");
                } else {
                    mgba_log("gates reroute wyll");
                }
                n_gob[0] = field_add_npc(4, 5, OBJT_GOBF, 1, 3, NPC_2FRAME);
                n_gob[1] = field_add_npc(6, 5, OBJT_GOBF, 1, 2, NPC_2FRAME);
                n_gob[2] = field_add_npc(6, 3, OBJT_GOBF, 1, 1, NPC_2FRAME);
                n_gob[3] = field_add_npc(9, 4, OBJT_GOBF, 1, 2, NPC_2FRAME);
                n_gob[4] = field_add_npc(8, 6, OBJT_GOBF, 1, 1, NPC_2FRAME);
                n_gob[5] = field_add_npc(9, 3, OBJT_GOBBOSS, 1, 1, NPC_2FRAME);
            }
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
                SAY_FL("A cold voice floods your mind: \"Thrall. The transponder. CONNECT THE NERVES.\"");
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
            /* her kit, per class -- sized by CLS_COUNT and designated so a
             * new class is a hole the compiler can see, not a wild read
             * (find[4] indexed by twelve classes handed Wyll a garbage
             * weapon id and a wild-pointer Equip screen) */
            static const u8 find[CLS_COUNT] = {
                [CLS_BARD]      = R5W_RAPIER,
                [CLS_ROGUE]     = R5W_SHORTSWORD,
                [CLS_RANGER]    = R5W_TRIDENT,
                [CLS_WIZARD]    = R5W_DAGGER,
                [CLS_FIGHTER]   = R5W_LONGSWORD,
                [CLS_CLERIC]    = R5W_DAGGER,
                [CLS_BARBARIAN] = R5W_LONGSWORD,
                [CLS_DRUID]     = R5W_DAGGER,
                [CLS_MONK]      = R5W_SHORTSWORD,
                [CLS_PALADIN]   = R5W_LONGSWORD,
                [CLS_SORCERER]  = R5W_DAGGER,
                [CLS_WARLOCK]   = R5W_DAGGER,
            };
            int w = find[G.pm[0].cls];
            mgba_logf("duelist find w=%d", w);
            say("A githyanki duelist, dead at her post. Her kit survived the fire.");
            if (isclass(CLS_BARD))
                say("[BARD] Beneath the ash: a dueling rapier, silvered and balanced. It suits your hand like it was made for it.");
            if (isclass(CLS_ROGUE))
                say("[ROGUE] A silvered shortsword, weighted for quick, quiet work.");
            if (isclass(CLS_RANGER))
                say("[RANGER] A boarding trident -- made for ship-to-ship slaughter. It throws true.");
            if (isclass(CLS_WIZARD))
                say("[WIZARD] A balanced throwing dagger. Considerably better than a stick.");
            if (G.pm[0].cls > CLS_WIZARD)
                say("Her silvered sidearm survived too. Good steel, and better luck than its last owner's.");
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
    SAY_FL("The mind flayer's voice cuts through the din: \"NOW, thrall. The nerves!\"");
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

static void camp_scene(void);   /* stone 5: the night under Selune */

/* open-air room change: no sphincter doors out here */
static void beach_go(int next, int sx, int sy) {
    G_FIELD_IDLE = 0;
    sfx_play(SFX_CONFIRM);
    fade_out(14);
    dlg_close();
    room_enter(next, sx, sy, 0);
    field_draw();
    fade_in(14);
    ev_light();
    if (!(seen & (1 << next))) {
        seen |= (u16)(1 << next);
        if (next == RM_DUNES) {
            say("The dunes swallow the surf-sound. Small tracks stitch every slope: clawed, busy, WRONG.");
            dlg_close();
        }
        if (next == RM_CHAPEL) {
            say("The scree tops out on a bluff. A ruined chapel leans over its own graveyard; crows keep the roofline like sentries.");
            if (!(G.bflags & BF_LOOTERS_GONE))
                say("Voices ahead. Living ones -- and arguing about a door.");
            if (!(G.bflags & BF_GATES_WON))
                say("And west along the bluff, under a smudge of smoke: the far-off ring of steel on steel.");
            if (!(G.bflags & BF_CAMP_SCENE))
                say("East, in the lee of the bluff, a low fire-glow throbs against the dark -- a sheltered hollow, a laid ring, a place to gather before the road runs out.");
            dlg_close();
        }
        if (next == RM_GATES) {
            say("The bluff path opens onto beaten ground. A palisade of sharpened logs walls the north -- and a great banded door, shut.");
            if (!(G.bflags & BF_GATES_WON)) {
                say("Goblins swarm the wall. Arrows rattle off the logs from above; something inside is screaming orders in two languages.");
                if (G.origin == ORIG_WYLL) {
                    say("An old tiefling holds the door itself, longsword up, buying the wall time with his own arms.");
                    SAY_ZV("ZEVLOR: \"The Blade of Frontiers?! Then the gulls owe me an apology. TO THE DOOR!\"");
                } else {
                    say("And out front, ALONE, a young man duels two goblins at once -- rapier flashing, coat snapping, entirely too pleased about it.");
                    SAY_WY("WYLL: \"Company! Wonderful -- pick a goblin, friend, they're going spare!\"");
                }
            } else {
                say("Goblin dead litter the beaten ground. The door has not opened; the grove keeps its own counsel.");
            }
            dlg_close();
        }
        if (next == RM_CAMP) {
            say("The path drops into a hollow in the bluff's lee, out of the wind. Driftwood, stacked. A fire ring, laid. The sea keeps time below.");
            say("It is night here the way it hasn't been anywhere else: soft, blue, and quiet. Camp.");
            dlg_close();
        }
    }
    if (next == RM_CAMP && !(G.bflags & BF_CAMP_SCENE)) camp_scene();
}

/* stone transitions under the chapel: fade through the dark, then let the
 * room's darkness (or the Everburn) set the light */
static void crypt_go(int next, int sx, int sy) {
    G_FIELD_IDLE = 0;
    sfx_play(SFX_CONFIRM);
    fade_out(14);
    dlg_close();
    room_enter(next, sx, sy, 0);
    field_draw();
    fade_in(14);
    ev_light();
    if (!(seen & (1 << next))) {
        seen |= (u16)(1 << next);
        switch (next) {
            case RM_CRYPT:
                say("A buried chapel-hall. The air tastes of cold stone and colder prayers.");
                if (encounter_dark() && !(party5[0].traits & TR_DARKVISION))
                    say("The dark presses close. Torchless, aim past arm's reach is a gamble in here.");
                dlg_close();
                break;
            case RM_OSSUARY:
                say("Bones. Drifts of them, raked against the walls like autumn leaves.");
                say("Some are stacked with care. Some are... arranged.");
                dlg_close();
                break;
            case RM_SANCTUM:
                say("A vaulted chamber. One great sarcophagus holds it the way a throne holds a court.");
                say("The cold flames lean toward it. Respectfully.");
                dlg_close();
                break;
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
    SAY_FL("Pinned under a rib of the hull: a mind flayer. Dying. Its eyes find yours --");
    SAY_FL("-- and COLD FINGERS close around your mind. THRALL. KNEEL. SERVE.");
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

/* --- Astarion by the shore (2,5): the knife at your throat --- */

static void astarion_beat(void) {
    field_face_npc(n_ast, field_player_mx() > 2 ? 3 : 0);
    say("A pale elf waves you over, all urgency and excellent posture.");
    SAY_AST("ASTARION: \"You there! One of those brain-things -- I have it cornered, there, in the grass. Kill it and I am forever grateful.\"");
    say_keep("He points. The grass is very still.");
    {
        static const char* const o[] = { "Peer into the grass", "Watch him, not the grass" };
        if (choose(2, o) == 0) {
            say("You lean in. Nothing but wind --");
            say("-- then sand in your knees and an arm like a bar across your chest. Steel settles under your jaw.");
        } else {
            if (isclass(CLS_ROGUE))
                say("[ROGUE] You watch his weight, not his finger. He moves anyway -- smoother than anyone you've ever robbed.");
            say("You keep your eyes on him. It does not help. The world tips, the sand takes you, and steel settles under your jaw.");
        }
    }
    SAY_AST("ASTARION: \"Shhh. Not a sound. I saw you on that ship, friend. So tell me: what do you and your tentacled masters want with me?\"");
    field_shake(16);
    sfx_noise(12);
    say("Behind your eye the passenger LUNGES at him -- and behind his eye, something lunges back.");
    SAY_AST("ASTARION: \"Agh!\" He recoils as if your blood burned him. \"You have one of those worms. Squirming little stowaway -- I FELT it.\"");
    URGE("His grip is broken and his throat is bare. You find yourself counting the ways, warmly.");
    SAY_AST("ASTARION: \"Well. This is awkward. I meant to carve answers out of a thrall, and instead I find -- a fellow victim.\"");
    say_keep("The knife vanishes like a conjurer's card. He offers the same hand, open.");
    {
        static const char* const o[] = { "Together, then.", "Point that at me again and lose it." };
        if (choose(2, o) == 1)
            SAY_AST("ASTARION: \"Noted and filed. You'll find I make a marvelous ally -- and a genuinely tiresome enemy.\"");
        else
            SAY_AST("ASTARION: \"Splendid. Misery adores company, and I am DELIGHTFUL company.\"");
    }
    SAY_AST("ASTARION: \"Astarion. Of Baldur's Gate -- a magistrate, before all this. Now let's find someone who can dig these things out, before the wriggling gets ambitious.\"");
    say("Astarion joins the party!");
    party_add_astarion();
    G.bflags |= BF_AST_RECRUITED;
    if (n_ast >= 0) field_remove_npc(n_ast);
    n_ast = -1;
    mgba_logf("beach recruit astarion walk=%d reserve=%d", G.nparty, G.nreserve);
    dlg_close();
}

/* origin Astarion: the lure spot holds his kill instead of him */
static void boar_beat(void) {
    say("The boar stands where you staked it: hobbled, heart hammering, downwind and doomed.");
    say("Your gums ache around the old points. The sea is loud. No one is watching.");
    say_keep("It would only take a moment.");
    {
        static const char* const o[] = { "Feed", "Master it -- for now" };
        if (choose(2, o) == 0) {
            say("Quick, and quiet, and warm. The hunger curls up satisfied, like a cat.");
            say("You smooth the sand over what's left. Habit.");
            G.bflags |= BF_BOAR_DRAINED;
            mgba_log("boar beat fed");
        } else {
            say("You cut the hobble instead. The boar bolts, and the hunger files the debt under LATER.");
            mgba_log("boar beat spared");
        }
    }
    if (n_boar >= 0) field_remove_npc(n_boar);
    n_boar = -1;
    dlg_close();
}

/* --- the portal sigil cut into the dunes' rocks (9,2), on the chapel-exit
 * approach: a player crossing the dunes for the door passes within its
 * pull. sigil_draw is the SIGNPOST -- a one-shot flare + hum + a line that
 * turns the head, fired on proximity (ev_step) so the missable recruit gets
 * noticed. It never recruits: taking the hand at the stone stays a choice
 * (sigil_beat, on an A-press). Skipped for origin Gale (no one to pull) and
 * once Gale already walks with you. --- */

static void sigil_draw(void) {
    G.bflags |= BF_SIGIL_SEEN;
    field_shake(8);
    sfx_noise(8);
    say("Off toward the pass a standing stone wakes: a rune flares wet-bright across its face, and behind the rock something STIRS, straining to be seen.");
    dlg_close();
    mgba_log("gale sigil noticed");
}

static void sigil_beat(void) {
    if (G.origin == ORIG_GALE) {
        /* story surgery: the wizard the stone would deliver is the player --
         * the anchor has nothing for him but a professional opinion */
        say("A weave-anchor, cut deep and recently. Waterdhavian work: fussy, elegant, nearly excellent.");
        say("Nearly. The exit matrix sits a hair off-true -- whoever routed through here arrived somewhere else, or not at all.");
        say("You could have built it better in your sleep. You resolve, quietly, never to admit how much that cheers you.");
        mgba_log("sigil beat rerouted");
        dlg_close();
        return;
    }
    if (G.bflags & BF_GALE_RECRUITED) {
        say("The sigil stone stands dark and spent. Whatever it was holding, you already pulled it free.");
        dlg_close();
        return;
    }
    say("A sigil glows on the rock face, wet-bright, humming like a bell that refuses to finish ringing.");
    if (isclass(CLS_WIZARD))
        say("[WIZARD] A travel rune, and it is under strain. Something is stuck in the doorway.");
    field_shake(14);
    sfx_noise(10);
    say("The rune FLARES. A hand thrusts out of solid stone, fingers spread, grasping at the air.");
    say("A voice from nowhere, courteous and slightly compressed: \"Ah, contact! Could I trouble you for a hand? Mine is spoken for.\"");
    say_keep("The hand waves, beckoning. The stone around it groans.");
    {
        static const char* const o[] = { "Take the hand and PULL", "Question the hand first" };
        if (choose(2, o) == 1) {
            say("\"And you would be?\" you ask the hand. It wilts, wounded.");
            SAY_GA("GALE: \"A wizard of absolutely no ill intent, wedged in a failing portal. Details AFTER the rescue, please.\"");
        }
    }
    say("You brace a heel on the rock and HAUL. The stone argues -- then gives him up all at once, mid-sentence:");
    SAY_GA("GALE: \"--which is why one never portals off an exploding nautiloid without an exit anchor. Gale, of Waterdeep. Charmed, truly.\"");
    say("He shakes the grit from his sleeves. Behind your eye the passenger stirs -- and behind his, something answers.");
    SAY_GA("GALE: \"Ah. You too? The little mind-worm, courtesy of our late hosts. Then our problems are entangled -- I'd hoped for a nicer word.\"");
    SAY_GA("GALE: \"Traveling alone out here rates somewhere between unwise and posthumous. Shall we?\"");
    say("Gale joins the party!");
    party_add_gale();
    G.bflags |= BF_GALE_RECRUITED;
    mgba_logf("beach recruit gale walk=%d reserve=%d", G.nparty, G.nreserve);
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
    if (m == MT_SIGIL) { sigil_beat(); return; }
    if (m == MT_CAGE || m == MT_CAGE_OPEN) { cage_beat(); return; }
    if (m == MT_ROCK) {
        if (my == 0) {
            say("The rockfall that choked this pass lies levered aside -- fresh pry-marks, boot prints going UP.");
            say("On the bluff above: the ruined chapel, crows wheeling over it. Somebody beat you to the climb.");
        } else say("Sea-worn stone, warm in the sun.");
        dlg_close();
        return;
    }
    if (m == MT_SCREE) {
        say("Broken rock, settled into a climbable ramp. The chapel bluff is a scramble up.");
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

/* ------------------------------------------------------------ the chapel
 * Three looters argue before the sealed tomb door -- four, counting the
 * small one in the mask. Fight them, or talk past on a field check --
 * both outcomes open the way; the check route pays less xp (the fight is
 * the consolation prize for a failed bluff). Stand close to the masked
 * one first and the tadpole opens a third way through the parley. */

static void looters_gone(void) {
    for (int i = 0; i < 3; i++) {
        if (n_loot[i] >= 0) field_remove_npc(n_loot[i]);
        n_loot[i] = -1;
    }
    if (n_warryn >= 0) field_remove_npc(n_warryn);
    n_warryn = -1;
    G.bflags |= BF_LOOTERS_GONE;
    mgba_log("looters resolved");
}

/* stand within a step of the masked looter and the passenger reacts. One
 * line, once; the game never explains what it recognized. */
static void warryn_stir(void) {
    G.bflags |= BF_WARRYN_SEEN;
    mgba_log("warryn stirs");
    sfx_noise(12);
    field_shake(10);
    say("Behind your eye the passenger LUNGES -- kin straining toward kin. Under the small looter's mask, something strains back.");
    dlg_close();
}

static void looter_fight(void) {
    dlg_close();
    if (G.bflags & BF_WARRYN_SEEN)
        say("The small one draws a knife without a word. The other three give it room as they come.");
    dlg_close();
    EncSpawn band[4];
    int n = 0;
    for (int i = 0; i < 3; i++) {
        band[n].mon = R5M_BANDIT; band[n].npc = (s8)n_loot[i];
        band[n].xp = 25; band[n].side = 1; n++;
    }
    if (n_warryn >= 0) {   /* the masked one fights with the band, wordless */
        band[n].mon = R5M_BANDIT; band[n].npc = (s8)n_warryn;
        band[n].xp = 25; band[n].side = 1; n++;
    }
    encounter_run(band, n, 0, 0);
    looters_gone();
    music(room_song(cur_room));
    ev_light();
    say("The chapel yard is quiet again, minus four careers in tomb-robbery.");
    say("The small one's mask has come half away. You put it back. Some answers the gulls can keep.");
    dlg_close();
}

static void looter_beat(void) {
    if (G.bflags & BF_LOOTERS_GONE) return;
    say("Three looters ring the tomb door, all crowbars and sunburn. A fourth stands apart -- small, masked, silent.");
    say("\"The seal's half-cracked already! One more pry and we're eating silver for a YEAR --\"");
    say("The tall one sees you first. \"Oi. This dig's CLAIMED. Walk away, hero.\"");
    URGE("Four pulses. Three soft throats, and one that sounds... crowded. The counting starts on its own.");
    say_keep("Crowbars shift in dirty hands.");
    const char* o[4];
    int n = 0, lever = -1;
    if (G.bflags & BF_WARRYN_SEEN) { o[n] = "[Illithid] LEAVE."; lever = n; n++; }
    o[n++] = "[Intimidate] Leave. NOW.";
    o[n++] = "[Persuade] Tombs bite back.";
    o[n++] = "Draw steel";
    int c = choose(n, o);
    if (c == n - 1) {
        say("\"Wrong answer,\" the tall one grins, and the crowbars stop being tools.");
        looter_fight();
        return;
    }
    if (c == lever) {
        /* the passenger speaks through you; the masked one hears it twice */
        mgba_log("warryn lever");
        sfx_noise(16);
        say("You do not raise your voice. Something behind your eye raises it for you, in a register bones understand.");
        say("The looters' faces empty like tipped cups. The masked one is already walking. The rest follow it down the bluff, and none of them look back.");
        looters_gone();
        say("Gained 40 XP.");
        beat_xp(40);
        dlg_close();
        return;
    }
    if (field_check(c == lever + 1 ? SK_INTIMIDATION : SK_PERSUASION, 12)) {
        if (c == lever + 1)
            say("You say it the way graves say things. The crowbars hit the sand before your hand finds a hilt.");
        else
            say("You sketch the curse in loving detail -- the withering, the slow rot, fingernails first. They go pale by rank.");
        say("\"...The gulls can keep this dump.\" One last greedy look at the door, and they bolt down the bluff. The masked one goes last, unhurried.");
        looters_gone();
        say("Gained 40 XP.");
        beat_xp(40);
        dlg_close();
    } else {
        say(c == lever + 1 ? "The tall one spits. \"Heard scarier from the gulls. GET 'EM!\""
                           : "\"A curse, is it? Funny -- silver cures those. GET 'EM!\"");
        looter_fight();
    }
}

static void tomb_door_beat(void) {
    if (!(G.bflags & BF_LOOTERS_GONE)) { looter_beat(); return; }
    if (!(G.bflags & BF_TOMB_OPEN)) {
        G.bflags |= BF_TOMB_OPEN;
        say("The looters' crowbars finished what the centuries started: the ward-seal hangs split.");
        field_shake(16);
        sfx_noise(14);
        field_set_meta(7, 1, MT_TOMB_DOOR_O);
        field_set_meta(8, 1, MT_TOMB_DOOR_O);
        sfx_play(SFX_CONFIRM);
        say("Stone grinds over stone. Cold air pours out: old dark, older quiet.");
        say("Press onward into the tomb?");
        mgba_log("tomb door opens");
        static const char* const o[] = { "Descend", "Not yet" };
        if (choose(2, o) == 0) { crypt_go(RM_CRYPT, 6, 7); return; }
        dlg_close();
        return;
    }
    /* the door is a solid slab: interacting with the open tomb steps
     * through (there is no walkable door tile to trip an ev_step) */
    crypt_go(RM_CRYPT, 6, 7);
}

static void chapel_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_TOMB_DOOR || m == MT_TOMB_DOOR_O) { tomb_door_beat(); return; }
    if (m == MT_GRAVESTONE) {
        say("A leaning headstone. The name is weathered away; the moss keeps whatever is left.");
        dlg_close();
        return;
    }
    if (m == MT_CHAPEL_WALL) {
        say("Chapel masonry, older than the dunes beneath it. Someone built to last, and lost anyway.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.bflags & BF_CHEST_CHAPEL)) {
            G.bflags |= BF_CHEST_CHAPEL;
            sfx_play(SFX_CONFIRM);
            say("The looters' camp chest, packed for a long dig: two Potions, barely watered down.");
            G.potions = (u8)(G.potions + 2);
        } else say("Empty. Their canteens you leave on principle.");
        dlg_close();
        return;
    }
    if (m == MT_SCREE) {
        say("The cleared rockfall. The dunes are a scramble below.");
        dlg_close();
        return;
    }
    if (m == MT_ROCK) {
        say("Wind-bitten bluff stone. The crows relocate, unhurried, and watch you from one ledge over.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ the camp
 * Stone 5: one quiet night under the moon. First arrival plays Under
 * Selune as a story scene through game.c's story-mode karaoke -- the song
 * shipped two releases before the scene it was written for. BF_CAMP_SCENE
 * marks it played; it never replays. The campfire is the long rest
 * thereafter: full heal, zero xp, repeatable. */

static int camp_has(int face) {
    for (int i = 1; i < G.nparty; i++) if (G.pm[i].face == face) return 1;
    for (int r = 0; r < G.nreserve; r++) if (G.reserve[r].face == face) return 1;
    return 0;
}

static void camp_scene(void) {
    G.bflags |= BF_CAMP_SCENE;
    mgba_logf("camp scene begins souls=%d", G.nparty + G.nreserve);
    field_wait(30);
    say("The driftwood catches. Warmth climbs out of the pit and finds every face that made it this far.");
    if (G.nparty + G.nreserve > 1)
        say("Nobody talks about the ship, or the worms, or tomorrow. Tonight the fire does the talking.");
    else
        say("You feed the fire alone and let the surf argue with the silence. Alive is alive.");
    URGE("Even the thing behind your eye has gone still tonight. Counting stars, maybe. Or just counting.");
    if (camp_has(ORIG_SHADOW))
        SAY_SH("SHADOWHEART: \"The moon is watching us. Selune. ...She and I have history. Tonight I'll allow her the view.\"");
    else if (G.origin == ORIG_SHADOW)
        say("You don't look up at her. But you let the moonlight sit on your shoulders without shrugging it off. Tonight, that is the whole truce.");
    else
        say("The moon stands full over the water. Selune, sailors call her: the one who stays.");
    say("Someone hums against the surf, low, half to themselves. The tune knows where it's going.");
    dlg_close();
    music(SONG_SELUNE);                   /* from the top, for the sync */
    game_story_karaoke(SONG_SELUNE);
    say("The last note hangs, and the surf takes it. One quiet night under the moon -- whatever comes next.");
    dlg_close();
    mgba_log("camp scene played");
}

/* the long rest: full heal for walkers and bench, zero xp, repeatable */
static void campfire_beat(void) {
    say("The fire has settled to steady coals. The bedrolls are claimed, argued over, and re-claimed.");
    say_keep("Rest until you're whole?");
    static const char* const o[] = { "Rest", "Not yet" };
    if (choose(2, o) == 1) {
        dlg_close();
        return;
    }
    int from = G.pm[0].hp;
    dlg_close();
    sfx_play(SFX_HEAL);
    fade_out(24);
    field_wait(30);
    party_heal_full();
    G.rests++;                       /* a camp night, counted for the tally */
    mgba_logf("camp rest from=%d full=%d rests=%d", from,
              (G.pm[0].hp == G.pm[0].hpmax &&
               party5[0].hp == party5[0].hpmax) ? 1 : 0, G.rests);
    field_draw();
    fade_in(24);
    say("Sleep takes you the way the tide takes the beach: completely. You wake mended, under the same patient moon.");
    dlg_close();
}

static void camp_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_CAMPFIRE) { campfire_beat(); return; }
    if (m == MT_BEDROLL) {
        say("A bedroll, sand-proofed by optimism. It smells of smoke and borrowed peace.");
        dlg_close();
        return;
    }
    if (m == MT_ROCK) {
        say("A ring-stone, fire-warm on one side and night-cold on the other.");
        dlg_close();
        return;
    }
    if (m == MT_SURF || m == MT_SEA) {
        say("The sea works the dark shore, patient as ever. Moonlight rides in on every swell.");
        dlg_close();
    }
}

/* a companion by the fire: a word, not a quest */
static int camp_npc_talk(int idx) {
    for (int k = 0; k < 5; k++) {
        if (idx < 0 || idx != n_camp[k] || !camp_member[k]) continue;
        char m[56];   /* name (7) + the 41-char line + NUL, with headroom */
        char* d = m;
        const char* s = camp_member[k]->name;
        while (*s) *d++ = *s++;
        s = " watches the fire burn low. \"Still here.\"";
        while (*s) *d++ = *s++;
        *d = 0;
        mgba_logf("camp talk %s", camp_member[k]->name);
        say(m);
        dlg_close();
        return 1;
    }
    return 0;
}

/* The camp is a side room off the chapel yard's east gap -- easy to walk
 * straight past on the way to the gates. Signpost it: on the approach to the
 * grove road, one call back to the banked fire in the bluff's lee. A nudge,
 * never a shove -- the rest stays optional; BF_CAMP_SEEN fires it once, and
 * BF_CAMP_SCENE (already slept) mutes it. */
static void camp_signpost(void) {
    G.bflags |= BF_CAMP_SEEN;
    if (G.nparty > 1)
        say("One of the others catches your sleeve and tips a head east, to the fire-glow banked in the bluff's lee. \"Before that door -- we rest. While we still can.\"");
    else
        say("West waits the grove road and the ring of steel. East, a fire-glow banks low in the bluff's lee -- a night's rest there for the taking, before the door.");
    dlg_close();
    mgba_log("camp signposted");
}

/* ------------------------------------------------------------ the gates
 * Stone 6, the finale: a goblin assault caught mid-swing. Zevlor holds
 * the door and Wyll duels out front -- both fight as side-2 allies (the
 * helm's machinery). The warchief's band is the last battle between the
 * arc and its accounting; Wyll joins after it, the roster's sixth soul.
 * The great door itself never opens: the grove interior is another
 * release's story, and nothing here presumes its shape. */

/* The prologue's accounting, resurrected as the whole arc's: the ledger of
 * the road from the wreck to the held gates, drawn over the field. G_DONE
 * layers in the order the story reaches its moments (2 = crash, 1 = wake);
 * 3 is the arc's true end, and the full-run scenarios sync here exactly as
 * they once did on the crash tally. START (or the demo's patience) returns
 * to the field -- main stays finishable, and the beach stays walkable
 * after its credits moment. */
static void gates_tally(void) {
    music(SONG_PRELUDE);             /* the title theme closes the ledger */
    scr_tally();
    static const char* const clsn[CLS_COUNT] = { "Bard", "Rogue", "Ranger",
        "Wizard", "Fighter", "Cleric", "Barbarian", "Druid", "Monk",
        "Paladin", "Sorcerer", "Warlock" };
    txt_put_n(SCR_TALLY_WHO_X, SCR_TALLY_WHO_Y, G.pm[0].name, 0, SCR_TALLY_WHO_W);
    txt_put_n(SCR_TALLY_CLS_X, SCR_TALLY_CLS_Y, clsn[G.pm[0].cls], 0, SCR_TALLY_CLS_W);
#define TVAL(slot, cond, yes, no) \
    txt_put_n(SCR_TALLY_##slot##_X, SCR_TALLY_##slot##_Y, (cond) ? (yes) : (no), \
              (cond) ? 1 : 2, SCR_TALLY_##slot##_W)
    TVAL(V_US, us_with_us(), "YES", (G.flags & GF_US_MUTILATED) ? "HURT" : "no");
    if (G.origin == ORIG_LAEZEL)
        txt_put_n(SCR_TALLY_V_LZ_X, SCR_TALLY_V_LZ_Y, "YOU", 1, SCR_TALLY_V_LZ_W);
    else
        TVAL(V_LZ, G.bflags & BF_LZ_RECOVERED, "ALLY", "caged");
    if (G.origin == ORIG_SHADOW)
        txt_put_n(SCR_TALLY_V_SH_X, SCR_TALLY_V_SH_Y, "YOU", 1, SCR_TALLY_V_SH_W);
    else
        TVAL(V_SH, G.bflags & BF_SH_RECOVERED, "SAVED", "lost");
    TVAL(V_ZH, G.flags & GF_ZHALK_DEAD, "SLAIN", "fled");
    TVAL(V_FL, G.bflags & BF_FLAYER_SLAIN, "ENDED",
         (G.bflags & BF_FLAYER_DONE) ? "tide" : "-");
    TVAL(V_WR, G.bflags & BF_WARRYN_SEEN, "SEEN", "unseen");
#undef TVAL
    txt_put_n(SCR_TALLY_V_GT_X, SCR_TALLY_V_GT_Y, "HELD", 1, SCR_TALLY_V_GT_W);
    {   /* the numbers: souls gathered, camp nights slept */
        char b[8]; char* d;
        d = put_num(b, G.nparty + G.nreserve); *d = 0;
        txt_put_n(SCR_TALLY_V_SO_X, SCR_TALLY_V_SO_Y, b, 1, SCR_TALLY_V_SO_W);
        d = put_num(b, G.rests); *d = 0;
        txt_put_n(SCR_TALLY_V_CN_X, SCR_TALLY_V_CN_Y, b,
                  G.rests ? 1 : 2, SCR_TALLY_V_CN_W);
    }
    mgba_logf("gates tally souls=%d rests=%d flags=%x",
              G.nparty + G.nreserve, G.rests, G.flags);
    G_DONE = 3;
    for (;;) {
        frame();
        if (key_hit() & KEY_START) break;
        if (G_DEMO) { field_wait(120); break; }
    }
    sfx_play(SFX_CONFIRM);
    win_clear(0, 0, 30, 20);
    music(room_song(cur_room));
    field_draw();
}

static void gates_victory(void) {
    int zev_up = n_zev >= 0 && !(npcs[n_zev].flags & NPC_GONE);
    say("The last goblin folds. For one long breath the beaten ground holds still -- then the wall above erupts in ragged tiefling cheering.");
    if (zev_up)
        SAY_ZV("ZEVLOR: \"Held. HELD! See to the wounded! And you four -- whatever wind blew you up this bluff, I am in its debt.\"");
    else
        say("Hellriders spill from a sally gap and drag their commander clear of the door -- breathing, cursing, giving orders flat on his back.");
    if (G.origin == ORIG_WYLL) {
        /* story surgery: the Blade of Frontiers is the player. The duel out
         * front never happened; the legend arrived from the beach instead. */
        SAY_ZV("ZEVLOR: \"The Blade of Frontiers, at my door, with the timing of a ballad. When the grove tells this story, no one will believe a word.\"");
        URGE("The cheering washes over you. Somewhere under it, the old quiet thing wonders what the wall would sound like, falling.");
    } else {
        if (n_wyllg >= 0 && (npcs[n_wyllg].flags & NPC_GONE))
            say("By the door, the young duelist picks himself up off the sand, rapier for a cane, dignity nearly intact.");
        SAY_WY("WYLL: \"Wyll. The Blade of Frontiers -- you may have heard the ballads. Kindly ignore the verses about the ogre.\"");
        SAY_WY("WYLL: \"You fight like a story worth following, and I find myself... between causes. Say the word and my blade is yours.\"");
        say("Behind your eye the passenger stirs, curious: this one owes a debt, and something far away is holding the other end of it.");
        say("Wyll joins the party!");
        party_add_wyll();
        if (n_wyllg >= 0) field_remove_npc(n_wyllg);
        n_wyllg = -1;
        mgba_logf("gates recruit wyll walk=%d reserve=%d", G.nparty, G.nreserve);
    }
    say("The great door stays barred. Beyond it: voices, weeping, counting. The grove keeps its own story -- this one ends at the wall.");
    dlg_close();
    gates_tally();                   /* the accounting returns; G_DONE = 3 */
}

static void gates_battle(void) {
    SAY_ZV("ZEVLOR: \"Hellriders! The door HOLDS or the grove dies. TO ME!\"");
    if (n_wyllg >= 0)
        SAY_WY("WYLL: \"You heard the commander -- dance card's open, cut in anywhere!\"");
    URGE("So many spines, all conveniently gathered. Behind your eye, the counting starts, and it is not counting goblins.");
    dlg_close();
    EncSpawn gs[8];
    int n = 0;
    for (int i = 0; i < 5; i++) {
        gs[n].mon = R5M_GOBLIN; gs[n].npc = (s8)n_gob[i];
        gs[n].xp = 50; gs[n].side = 1; n++;
    }
    gs[n].mon = R5M_GOB_BOSS; gs[n].npc = (s8)n_gob[5];
    gs[n].xp = 100; gs[n].side = 1; n++;
    gs[n].mon = R5M_ZEVLOR; gs[n].npc = (s8)n_zev;
    gs[n].xp = 0; gs[n].side = 2; n++;
    if (n_wyllg >= 0) {
        gs[n].mon = R5M_WYLL; gs[n].npc = (s8)n_wyllg;
        gs[n].xp = 0; gs[n].side = 2; n++;
    }
    encounter_song(SONG_GATES);          /* the finale gets its anthem */
    encounter_run(gs, n, 0, 0);
    G.bflags |= BF_GATES_WON;
    mgba_log("gates held");
    music(room_song(cur_room));
    ev_light();
    gates_victory();
}

static void gates_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_GATE_L || m == MT_GATE_R) {
        if (!(G.bflags & BF_GATES_WON))
            say("The great door shudders under fists and hatchets. It is losing the argument slowly.");
        else {
            say("The great door stays barred; behind it, the grove counts its dead and its luck.");
            say("Whatever waits inside is another story. This one held the wall.");
        }
        dlg_close();
        return;
    }
    if (m == MT_PALIS) {
        say("Sharpened ship-timber and driftwood, lashed and earth-set. Half of Elturel's carpentry is in this wall, and all of its stubbornness.");
        dlg_close();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.bflags & BF_CHEST_GATES)) {
            G.bflags |= BF_CHEST_GATES;
            sfx_play(SFX_CONFIRM);
            say("A Hellrider supply cache, half-buried by the fighting: two Potions, stamped with Elturel's sun.");
            G.potions = (u8)(G.potions + 2);
        } else say("Empty. The sun stamp stays with the crate.");
        dlg_close();
        return;
    }
    if (m == MT_ROCK) {
        say("Bluff stone, chipped by arrows. The grove chose its ground well.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ the crypt
 * The first DARK rooms (darkvision doctrine, docs/character2.md): the
 * screen dims unless Tav has darkvision or someone carries the lit
 * Everburn; in battle, no-darkvision actors shoot and cast at
 * disadvantage while the crypt's dead see just fine. */

static void bones_rise(void) {
    say("The drifts here run deeper. Sword-hilts stick out of them like grave-flowers.");
    field_shake(20);
    sfx_noise(20);
    say("The bones KNIT. Three of the dead stand up, wearing the crypt's silence like armor.");
    dlg_close();
    int s0 = field_add_npc(4, 3, OBJT_SKELF, 1, 0, NPC_2FRAME);
    int s1 = field_add_npc(8, 3, OBJT_SKELF, 1, 0, NPC_2FRAME);
    int s2 = field_add_npc(6, 2, OBJT_SKELF, 1, 0, NPC_2FRAME);
    EncSpawn sk[3] = {
        { R5M_SKELETON, (s8)s0, 30, 1 },
        { R5M_SKELETON, (s8)s1, 30, 1 },
        { R5M_SKELETON, (s8)s2, 30, 1 },
    };
    encounter_run(sk, 3, 0, 2);        /* they rise around you: ambush */
    G.bflags |= BF_CRYPT_BONES;
    music(room_song(cur_room));
    ev_light();
    say("The bones lie back down -- disarmed, in every sense that matters.");
    dlg_close();
}

/* --- the sarcophagus, and the one who does not sleep in it --- */

static void withers_wake(void) {
    if (G.bflags & BF_WITHERS_AWAKE) {
        say("The sarcophagus stands open. Its tenant keeps office three feet to the left.");
        dlg_close();
        return;
    }
    say("The great sarcophagus. Its lid is carved with a sleeping judge, scales folded across the chest.");
    URGE("Something under that lid is not dead enough. Your hands itch to amend it -- one way, or the other.");
    say_keep("Old instinct says leave it. Older curiosity says knock.");
    static const char* const o[] = { "Open the sarcophagus", "Leave it sealed" };
    if (choose(2, o) == 1) {
        say("You let the judge sleep. For now.");
        dlg_close();
        return;
    }
    field_shake(24);
    sfx_noise(20);
    say("The lid grinds aside on its own. Your hands, it turns out, were only invited.");
    field_set_meta(6, 2, MT_SARC_OT);
    field_set_meta(6, 3, MT_SARC_OB);
    G.bflags |= BF_WITHERS_AWAKE;
    n_withers = field_add_npc(7, 4, OBJT_WITHERSF, 5, 0, NPC_2FRAME);
    mgba_log("withers wakes");
    say("A desiccated figure sits up, unhurried, and regards you with two points of lamplight where eyes retired.");
    SAY_WI("WITHERS: \"Ah. Visitors. Answer me this, o breathing one: what price would thee set on a single mortal life?\"");
    say_keep("The lamplit sockets wait. They have had practice.");
    static const char* const a[] = { "Everything.", "Nothing.",
                                     "Whatever's in its pockets." };
    int c = choose(3, a);
    if (c == 0)
        SAY_WI("WITHERS: \"Spoken like one who has not yet had to pay it. Acceptable.\"");
    else if (c == 1)
        SAY_WI("WITHERS: \"Spoken like coin that resents being spent. Also acceptable.\"");
    else
        SAY_WI("WITHERS: \"...Thou art fortunate that death finds honesty refreshing.\"");
    SAY_WI("WITHERS: \"Long have I kept the ledgers of the dead, and long enough slept on the job. The world, it seems, still needs its accountant.\"");
    SAY_WI("WITHERS: \"I shall walk again in time. Until then -- my services, such as they are, stand open to thee. Ask.\"");
    dlg_close();
}

/* Withers' service: revival flavor, and the subclass re-pick. The re-pick
 * CALLS the existing machinery -- unmake the choice, then level_up_choices
 * re-makes it through the same screen every level-up uses. No class
 * change. No race change. Those are not paths; they are the walker. */
static void withers_service(void) {
    SAY_WI("WITHERS: \"Speak. What does the living world want of Withers?\"");
    say_keep("The sockets glow, patient as ledgers.");
    static const char* const o[] = { "Change a chosen path",
                                     "Ask about death", "Nothing today" };
    int c = choose(3, o);
    if (c == 2) {
        SAY_WI("WITHERS: \"A rare wisdom. Walk on.\"");
        dlg_close();
        return;
    }
    if (c == 1) {
        SAY_WI("WITHERS: \"Death is a door, not a wall. Shouldst thou lose a companion, bring flame, or scroll, or patience -- the dead keep every appointment.\"");
        SAY_WI("WITHERS: \"For the deeper arrangements, my rates are famously reasonable. In time.\"");
        dlg_close();
        return;
    }
    const char* names[3];
    int map[3], n = 0;
    for (int i = 0; i < G.nparty; i++)
        if (G.pm[i].subclass != 255) { names[n] = G.pm[i].name; map[n] = i; n++; }
    if (!n) {
        SAY_WI("WITHERS: \"None of thee has yet chosen a path worth unchoosing. Walk further first.\"");
        dlg_close();
        return;
    }
    SAY_WI("WITHERS: \"Thy class is thy spine and thy blood is thy blood; Withers reorders neither. But a path chosen within them? That, I unmake.\"");
    say_keep("\"Whose path shall be unwalked?\"");
    int mi = map[choose(n, names)];
    SAY_WI("WITHERS: \"So. The road walked becomes a road unwalked. Choose again -- and mean it, this time.\"");
    mgba_logf("withers repick %s", G.pm[mi].name);
    G.pm[mi].subclass = 255;           /* unmake the choice... */
    level_up_choices();                /* ...and the usual machinery re-makes it */
}

static void crypt_interact(int mx, int my, int m) {
    (void)mx; (void)my;
    if (m == MT_SARC_T || m == MT_SARC_B || m == MT_SARC_OT || m == MT_SARC_OB) {
        withers_wake();
        return;
    }
    if (m == MT_CHEST) {
        if (!(G.bflags & BF_CHEST_CRYPT)) {
            G.bflags |= BF_CHEST_CRYPT;
            sfx_play(SFX_CONFIRM);
            say("Grave-gifts for a journey no one took: a Scroll of Revivify, wax-sealed against the damp.");
            G.revivify++;
        } else say("The rest of the gifts are dust, and the dust is spoken for.");
        dlg_close();
        return;
    }
    if (m == MT_SCONCE) {
        say("A sconce of cold teal flame. It has burned since before the language you swear in.");
        dlg_close();
        return;
    }
    if (m == MT_BONES) {
        say(cur_room == RM_OSSUARY && !(G.bflags & BF_CRYPT_BONES)
            ? "Old bones, drifted deep. You could swear the near ones are holding their breath."
            : "Old bones, stacked by patient hands. You leave the arrangement be.");
        dlg_close();
        return;
    }
    if (m == MT_RUBBLE) {
        say("A fallen column drum. The ceiling it held has been managing without it for centuries.");
        dlg_close();
        return;
    }
    if (m == MT_CARCH) {
        say("Worn steps, and the dark keeping them.");
        dlg_close();
        return;
    }
    if (m == MT_CWALL) {
        say("Dressed stone, laid dry and true. The masons are downstairs now, in the drifts.");
        dlg_close();
    }
}

/* ------------------------------------------------------------ dispatch */

/* subclass resolution when a member reaches its subclass level (Char 2.0):
 * the played hero (slot 0) picks; companions auto-take canon (data.c) */
void level_up_choices(void) {
    for (int i = 0; i < G.nparty; i++) {
        PMember* p = &G.pm[i];
        if (party_canon_subclass(p)) { party5_refresh(i); continue; }
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
    /* benched members resolve at the same moments the walking party does;
     * their twin picks the passives up at its next rebuild (swap-in) */
    for (int r = 0; r < G.nreserve; r++)
        party_canon_subclass(&G.reserve[r]);
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
        case RM_CHAPEL:  chapel_interact(mx, my, m); break;
        case RM_CRYPT:
        case RM_OSSUARY:
        case RM_SANCTUM: crypt_interact(mx, my, m); break;
        case RM_CAMP:    camp_interact(mx, my, m); break;
        case RM_GATES:   gates_interact(mx, my, m); break;
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
        if (n_ast >= 0) {
            /* wander near the pale elf (2,5) and he beckons: the lure IS
             * the beat */
            int dx = mx - 2, dy = my - 5;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx + dy <= 2) { astarion_beat(); return; }
        }
        return;
    }
    if (cur_room == RM_DUNES) {
        if (my == 11) { beach_go(RM_BEACH, mx, 1); return; }  /* crash site */
        if (my == 0) { beach_go(RM_CHAPEL, mx, 8); return; }  /* the bluff */
        if (!(G.bflags & (BF_SIGIL_SEEN | BF_GALE_RECRUITED)) &&
            G.origin != ORIG_GALE) {
            /* cross into the sigil's reach (9,2) and the stone calls out --
             * a draw, not the recruit; the pull is what a straight-for-the-
             * chapel player was missing */
            int dx = mx - 9, dy = my - 2;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx + dy <= 3) { sigil_draw(); return; }
        }
        return;
    }
    if (cur_room == RM_CHAPEL) {
        if (my == 9) { beach_go(RM_DUNES, mx, 1); return; }   /* the scree */
        if (mx == 15) { beach_go(RM_CAMP, 1, 4); return; }    /* the camp path */
        if (mx == 0) { beach_go(RM_GATES, 14, 4); return; }   /* the grove road */
        if (m == MT_TOMB_DOOR_O) { crypt_go(RM_CRYPT, 6, 7); return; }
        if (n_warryn >= 0 && !(G.bflags & BF_WARRYN_SEEN)) {
            /* step within reach of the masked looter (5,2): to those who
             * look, the tadpole looks back */
            int dx = mx - 5, dy = my - 2;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx + dy <= 1) { warryn_stir(); return; }
        }
        if (mx <= 1 &&
            !(G.bflags & (BF_CAMP_SEEN | BF_CAMP_SCENE | BF_GATES_WON))) {
            /* nearing the grove road (west gap at mx 0) with no night behind
             * you: the camp in the east lee gets one last call */
            camp_signpost(); return;
        }
        return;
    }
    if (cur_room == RM_CAMP) {
        if (mx == 0) beach_go(RM_CHAPEL, 14, 4);   /* back up to the yard */
        return;
    }
    if (cur_room == RM_GATES) {
        if (mx == 15) { beach_go(RM_CHAPEL, 1, 4); return; }  /* the bluff road */
        /* close with the fight and it closes with you */
        if (!(G.bflags & BF_GATES_WON) && mx <= 10) { gates_battle(); return; }
        return;
    }
    if (cur_room == RM_CRYPT) {
        if (my == 8) { crypt_go(RM_CHAPEL, 7, 2); return; }
        if (my == 0) { crypt_go(RM_OSSUARY, 6, 7); return; }
        return;
    }
    if (cur_room == RM_OSSUARY) {
        if (my == 8) { crypt_go(RM_CRYPT, 6, 1); return; }
        if (my == 0) { crypt_go(RM_SANCTUM, 6, 7); return; }
        if (!(G.bflags & BF_CRYPT_BONES) && my <= 4) { bones_rise(); return; }
        return;
    }
    if (cur_room == RM_SANCTUM) {
        if (my == 8) crypt_go(RM_OSSUARY, 6, 1);
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
        wander_latch(s);
    music(room_song(cur_room));
    ev_light();
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
    if (cur_room == RM_CAMP && camp_npc_talk(idx)) return;
    if (idx >= 0 && idx == n_zev) {
        if (!(G.bflags & BF_GATES_WON)) {
            SAY_ZV("ZEVLOR: \"Less talk. More steel!\"");
            dlg_close();
            gates_battle();
            return;
        }
        SAY_ZV("ZEVLOR: \"The wall stands, and so, against all odds, do I. Elturel taught me to repay debts -- the grove will hear how this door held.\"");
        dlg_close();
        return;
    }
    if (idx >= 0 && idx == n_wyllg) {
        SAY_WY("WYLL: \"Introductions AFTER the goblins, friend!\"");
        dlg_close();
        gates_battle();
        return;
    }
    if (idx >= 0 && !(G.bflags & BF_GATES_WON) &&
        (idx == n_gob[0] || idx == n_gob[1] || idx == n_gob[2] ||
         idx == n_gob[3] || idx == n_gob[4] || idx == n_gob[5])) {
        say("The goblin wheels on you, delighted. Fresh meat volunteers!");
        dlg_close();
        gates_battle();
        return;
    }
    if (idx >= 0 && idx == n_ast) { astarion_beat(); return; }
    if (idx >= 0 && idx == n_boar) { boar_beat(); return; }
    if (idx >= 0 && idx == n_warryn) {
        if (!(G.bflags & BF_WARRYN_SEEN)) { warryn_stir(); return; }
        say("The small looter says nothing. The mask shifts -- the way a sleeve shifts when the arm inside is wrong.");
        say("At its edge, where jaw should be: skin the grey-violet of deep water. You find somewhere else to look.");
        dlg_close();
        return;
    }
    if (idx >= 0 && (idx == n_loot[0] || idx == n_loot[1] || idx == n_loot[2])) {
        looter_beat();
        return;
    }
    if (idx >= 0 && idx == n_withers) { withers_service(); return; }
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
        SAY_FL("The mind flayer's eyes flick to you, and your tadpole THRUMS in answer. \"The nerves. Go.\"");
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

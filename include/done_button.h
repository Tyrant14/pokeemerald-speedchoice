#ifndef _GUARD_DONE_BUTTON_H
#define _GUARD_DONE_BUTTON_H

// The done stat fields are defined in global.h.

// This is kept seperate and then loaded when the save is loaded.
struct FrameTimers
{
    u32 frameCount;  
    u32 owFrameCount;
    u32 battleFrameCount;
    u32 menuFrameCount;
    u32 introsFrameCount;
};

enum DoneButtonStat
{
    // DoneButtonStats1
    DB_FRAME_COUNT_TOTAL,
    DB_FRAME_COUNT_OW,
    DB_FRAME_COUNT_BATTLE,
    DB_FRAME_COUNT_MENU, // count start menu + PC
    /*accounted*/ DB_FRAME_COUNT_INTROS,
    DB_SAVE_COUNT,
    DB_RELOAD_COUNT,
    DB_CLOCK_RESET_COUNT,
    DB_STEP_COUNT,
    DB_STEP_COUNT_WALK,
    DB_STEP_COUNT_SURF,
    DB_STEP_COUNT_BIKE,
    DB_STEP_COUNT_RUN, // Gen 3 exclusive
    DB_BONKS,
    DB_TOTAL_DAMAGE_DEALT,
    DB_ACTUAL_DAMAGE_DEALT,
    DB_TOTAL_DAMAGE_TAKEN,
    DB_ACTUAL_DAMAGE_TAKEN,
    DB_OWN_MOVES_HIT,
    DB_OWN_MOVES_MISSED,
    DB_ENEMY_MOVES_HIT,
    DB_ENEMY_MOVES_MISSED,
    // DoneButtonStats2
    DB_OWN_MOVES_SE,
    DB_OWN_MOVES_NVE,
    DB_ENEMY_MOVES_SE,
    DB_ENEMY_MOVES_NVE,
    DB_CRITS_DEALT,
    DB_OHKOS_DEALT,
    DB_CRITS_TAKEN,
    DB_OHKOS_TAKEN,
    DB_PLAYER_HP_HEALED,
    DB_ENEMY_HP_HEALED,
    DB_PLAYER_POKEMON_FAINTED,
    DB_ENEMY_POKEMON_FAINTED,
    DB_EXP_GAINED,
    DB_SWITCHOUTS,
    DB_BATTLES,
    DB_TRAINER_BATTLES,
    DB_WILD_BATTLES,
    DB_BATTLES_FLED,
    DB_FAILED_RUNS,
    DB_MONEY_MADE,
    DB_MONEY_SPENT,
    DB_MONEY_LOST,
    DB_ITEMS_PICKED_UP,
    DB_ITEMS_BOUGHT,
    DB_ITEMS_SOLD,
    DB_MOVES_LEARNT,
    DB_BALLS_THROWN,
    DB_POKEMON_CAUGHT_IN_BALLS
};

void TryIncrementButtonStat(enum DoneButtonStat stat);
u32 GetDoneButtonStat(enum DoneButtonStat stat);

extern struct FrameTimers gFrameTimers;
extern bool8 sInSubMenu;
extern bool8 sInBattle;
extern bool8 sInField;

#endif // _GUARD_DONE_BUTTON_H

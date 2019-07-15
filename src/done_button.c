#include "global.h"
#include "main.h"
#include "task.h"
#include "bg.h"
#include "window.h"
#include "palette.h"
#include "alloc.h"
#include "sound.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "gpu_regs.h"
#include "scanline_effect.h"
#include "text.h"
#include "menu.h"
#include "done_button.h"
#include "overworld.h"
#include "text_window.h"

struct DoneButton
{
    MainCallback doneCallback;
    u8 taskId;
    u8 page;
    u16 tilemapBuffer[0x800];
};

extern u16 sUnknown_0855C6A0[1];
extern u16 sUnknown_0855C604[16];
extern u16 sMainMenuTextPal[16];

#define NELEMS ARRAY_COUNT

EWRAM_DATA bool8 sInSubMenu = FALSE;
EWRAM_DATA bool8 sInBattle = FALSE;
EWRAM_DATA bool8 sInField = FALSE;

EWRAM_DATA struct FrameTimers gFrameTimers = {0};


static EWRAM_DATA struct DoneButton *doneButton = NULL;

static void DoneButtonCB(void);
static void PrintGameStatsPage(void);
static void Task_DoneButton(u8 taskId);
static void Task_DestroyDoneButton(u8 taskId);

void OpenDoneButton(MainCallback doneCallback);
void DrawDoneButtonFrame(void);

struct DoneButtonLineItem
{
    const u8 * name;
    const u8 * (*printfn)(void); // string formatter for each type.
};

#define TRY_INC_GAME_STAT(saveBlock, statName, max)                   \
do {                                                                  \
    if(gSaveBlock##saveBlock##Ptr->doneButtonStats.statName < max)    \
        gSaveBlock##saveBlock##Ptr->doneButtonStats.statName++;       \
}while(0)
    
// max is unused, copy paste from other macro
#define GET_GAME_STAT(saveBlock, statName, max)                  \
do {                                                             \
    return gSaveBlock##saveBlock##Ptr->doneButtonStats.statName; \
}while(0)
    
#define TRY_INC_GAME_STAT_BY(saveBlock, statName, add, max)                \
do {                                                                       \
    u32 diff = max - gSaveBlock##saveBlock##Ptr->doneButtonStats.statName; \
    if(diff > add)                                                         \
        gSaveBlock##saveBlock##Ptr->doneButtonStats.statName += add;       \
    else                                                                   \
        gSaveBlock##saveBlock##Ptr->doneButtonStats.statName = max;        \
}while(0)

// Hmm, 3 giant switches. Repetitive?
// TODO: Better way of handling?
void TryAddButtonStatBy(enum DoneButtonStat stat, u32 add)
{
    switch(stat)
    {
        // DoneButtonStats1
        case DB_FRAME_COUNT_TOTAL:
            TRY_INC_GAME_STAT_BY(1, frameCount, add, UINT_MAX);
            break;
        case DB_FRAME_COUNT_OW:
            TRY_INC_GAME_STAT_BY(1, owFrameCount, add, UINT_MAX);
            break;
        case DB_FRAME_COUNT_BATTLE:
            TRY_INC_GAME_STAT_BY(1, battleFrameCount, add, UINT_MAX);
            break;
        case DB_FRAME_COUNT_MENU:
            TRY_INC_GAME_STAT_BY(1, menuFrameCount, add, UINT_MAX);
            break;
        case DB_FRAME_COUNT_INTROS:
            TRY_INC_GAME_STAT_BY(1, introsFrameCount, add, UINT_MAX);
            break;
        case DB_SAVE_COUNT:
            TRY_INC_GAME_STAT_BY(1, saveCount, add, USHRT_MAX);
            break;
        case DB_RELOAD_COUNT:
            TRY_INC_GAME_STAT_BY(1, reloadCount, add, USHRT_MAX);
            break;
        case DB_CLOCK_RESET_COUNT:
            TRY_INC_GAME_STAT_BY(1, clockResetCount, add, USHRT_MAX);
            break;
        case DB_STEP_COUNT:
            TRY_INC_GAME_STAT_BY(1, stepCount, add, UINT_MAX);
            break;
        case DB_STEP_COUNT_WALK:
            TRY_INC_GAME_STAT_BY(1, stepCountWalk, add, UINT_MAX);
            break;
        case DB_STEP_COUNT_SURF:
            TRY_INC_GAME_STAT_BY(1, stepCountSurf, add, UINT_MAX);
            break;
        case DB_STEP_COUNT_BIKE:
            TRY_INC_GAME_STAT_BY(1, stepCountBike, add, UINT_MAX);
            break;
        case DB_STEP_COUNT_RUN:
            TRY_INC_GAME_STAT_BY(1, stepCountRun, add, UINT_MAX);
            break;
        case DB_BONKS:
            TRY_INC_GAME_STAT_BY(1, bonks, add, USHRT_MAX);
            break;
        case DB_TOTAL_DAMAGE_DEALT:
            TRY_INC_GAME_STAT_BY(1, totalDamageDealt, add, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_DEALT:
            TRY_INC_GAME_STAT_BY(1, actualDamageDealt, add, UINT_MAX);
            break;
        case DB_TOTAL_DAMAGE_TAKEN:
            TRY_INC_GAME_STAT_BY(1, totalDamageTaken, add, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_TAKEN:
            TRY_INC_GAME_STAT_BY(1, actualDamageTaken, add, UINT_MAX);
            break;
        case DB_OWN_MOVES_HIT:
            TRY_INC_GAME_STAT_BY(1, ownMovesHit, add, USHRT_MAX);
            break;
        case DB_OWN_MOVES_MISSED:
            TRY_INC_GAME_STAT_BY(1, ownMovesMissed, add, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_HIT:
            TRY_INC_GAME_STAT_BY(1, enemyMovesHit, add, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_MISSED:
            TRY_INC_GAME_STAT_BY(1, enemyMovesMissed, add, USHRT_MAX);
            break;
        // DoneButtonStats2
        case DB_OWN_MOVES_SE:
            TRY_INC_GAME_STAT_BY(2, ownMovesSE, add, USHRT_MAX);
            break;
        case DB_OWN_MOVES_NVE:
            TRY_INC_GAME_STAT_BY(2, ownMovesNVE, add, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_SE:
            TRY_INC_GAME_STAT_BY(2, enemyMovesSE, add, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_NVE:
            TRY_INC_GAME_STAT_BY(2, enemyMovesNVE, add, USHRT_MAX);
            break;
        case DB_CRITS_DEALT:
            TRY_INC_GAME_STAT_BY(2, critsDealt, add, USHRT_MAX);
            break;
        case DB_OHKOS_DEALT:
            TRY_INC_GAME_STAT_BY(2, OHKOsDealt, add, USHRT_MAX);
            break;
        case DB_CRITS_TAKEN:
            TRY_INC_GAME_STAT_BY(2, critsTaken, add, USHRT_MAX);
            break;
        case DB_OHKOS_TAKEN:
            TRY_INC_GAME_STAT_BY(2, OHKOsTaken, add, USHRT_MAX);
            break;
        case DB_PLAYER_HP_HEALED:
            TRY_INC_GAME_STAT_BY(2, playerHPHealed, add, UINT_MAX);
            break;
        case DB_ENEMY_HP_HEALED:
            TRY_INC_GAME_STAT_BY(2, enemyHPHealed, add, UINT_MAX);
            break;
        case DB_PLAYER_POKEMON_FAINTED:
            TRY_INC_GAME_STAT_BY(2, playerPokemonFainted, add, USHRT_MAX);
            break;
        case DB_ENEMY_POKEMON_FAINTED:
            TRY_INC_GAME_STAT_BY(2, enemyPokemonFainted, add, USHRT_MAX);
            break;
        case DB_EXP_GAINED:
            TRY_INC_GAME_STAT_BY(2, expGained, add, UINT_MAX);
            break;
        case DB_SWITCHOUTS:
            TRY_INC_GAME_STAT_BY(2, switchouts, add, USHRT_MAX);
            break;
        case DB_BATTLES:
            TRY_INC_GAME_STAT_BY(2, battles, add, USHRT_MAX);
            break;
        case DB_TRAINER_BATTLES:
            TRY_INC_GAME_STAT_BY(2, trainerBattles, add, USHRT_MAX);
            break;
        case DB_WILD_BATTLES:
            TRY_INC_GAME_STAT_BY(2, wildBattles, add, USHRT_MAX);
            break;
        case DB_BATTLES_FLED:
            TRY_INC_GAME_STAT_BY(2, battlesFled, add, USHRT_MAX);
            break;
        case DB_FAILED_RUNS:
            TRY_INC_GAME_STAT_BY(2, failedRuns, add, USHRT_MAX);
            break;
        case DB_MONEY_MADE:
            TRY_INC_GAME_STAT_BY(2, moneyMade, add, UINT_MAX);
            break;
        case DB_MONEY_SPENT:
            TRY_INC_GAME_STAT_BY(2, moneySpent, add, UINT_MAX);
            break;
        case DB_MONEY_LOST:
            TRY_INC_GAME_STAT_BY(2, moneyLost, add, UINT_MAX);
            break;
        case DB_ITEMS_PICKED_UP:
            TRY_INC_GAME_STAT_BY(2, itemsPickedUp, add, USHRT_MAX);
            break;
        case DB_ITEMS_BOUGHT:
            TRY_INC_GAME_STAT_BY(2, itemsBought, add, USHRT_MAX);
            break;
        case DB_ITEMS_SOLD:
            TRY_INC_GAME_STAT_BY(2, itemsSold, add, USHRT_MAX);
            break;
        case DB_MOVES_LEARNT:
            TRY_INC_GAME_STAT_BY(2, movesLearnt, add, USHRT_MAX);
            break;
        case DB_BALLS_THROWN:
            TRY_INC_GAME_STAT_BY(2, ballsThrown, add, USHRT_MAX);
            break;
        case DB_POKEMON_CAUGHT_IN_BALLS:
            TRY_INC_GAME_STAT_BY(2, pokemonCaughtInBalls, add, USHRT_MAX);
            break;
    }
}

void TryIncrementButtonStat(enum DoneButtonStat stat)
{
    switch(stat)
    {
        // DoneButtonStats1
        case DB_FRAME_COUNT_TOTAL:
            TRY_INC_GAME_STAT(1, frameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_OW:
            TRY_INC_GAME_STAT(1, owFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_BATTLE:
            TRY_INC_GAME_STAT(1, battleFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_MENU:
            TRY_INC_GAME_STAT(1, menuFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_INTROS: // This needs special handling due to intro not having loaded save block yet.
            TRY_INC_GAME_STAT(1, introsFrameCount, UINT_MAX);
            break;
        case DB_SAVE_COUNT:
            TRY_INC_GAME_STAT(1, saveCount, USHRT_MAX);
            break;
        case DB_RELOAD_COUNT:
            TRY_INC_GAME_STAT(1, reloadCount, USHRT_MAX);
            break;
        case DB_CLOCK_RESET_COUNT:
            TRY_INC_GAME_STAT(1, clockResetCount, USHRT_MAX);
            break;
        case DB_STEP_COUNT:
            TRY_INC_GAME_STAT(1, stepCount, UINT_MAX);
            break;
        case DB_STEP_COUNT_WALK:
            TRY_INC_GAME_STAT(1, stepCountWalk, UINT_MAX);
            break;
        case DB_STEP_COUNT_SURF:
            TRY_INC_GAME_STAT(1, stepCountSurf, UINT_MAX);
            break;
        case DB_STEP_COUNT_BIKE:
            TRY_INC_GAME_STAT(1, stepCountBike, UINT_MAX);
            break;
        case DB_STEP_COUNT_RUN:
            TRY_INC_GAME_STAT(1, stepCountRun, UINT_MAX);
            break;
        case DB_BONKS:
            TRY_INC_GAME_STAT(1, bonks, USHRT_MAX);
            break;
        case DB_TOTAL_DAMAGE_DEALT:
            TRY_INC_GAME_STAT(1, totalDamageDealt, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_DEALT:
            TRY_INC_GAME_STAT(1, actualDamageDealt, UINT_MAX);
            break;
        case DB_TOTAL_DAMAGE_TAKEN:
            TRY_INC_GAME_STAT(1, totalDamageTaken, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_TAKEN:
            TRY_INC_GAME_STAT(1, actualDamageTaken, UINT_MAX);
            break;
        case DB_OWN_MOVES_HIT:
            TRY_INC_GAME_STAT(1, ownMovesHit, USHRT_MAX);
            break;
        case DB_OWN_MOVES_MISSED:
            TRY_INC_GAME_STAT(1, ownMovesMissed, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_HIT:
            TRY_INC_GAME_STAT(1, enemyMovesHit, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_MISSED:
            TRY_INC_GAME_STAT(1, enemyMovesMissed, USHRT_MAX);
            break;
        // DoneButtonStats2
        case DB_OWN_MOVES_SE:
            TRY_INC_GAME_STAT(2, ownMovesSE, USHRT_MAX);
            break;
        case DB_OWN_MOVES_NVE:
            TRY_INC_GAME_STAT(2, ownMovesNVE, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_SE:
            TRY_INC_GAME_STAT(2, enemyMovesSE, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_NVE:
            TRY_INC_GAME_STAT(2, enemyMovesNVE, USHRT_MAX);
            break;
        case DB_CRITS_DEALT:
            TRY_INC_GAME_STAT(2, critsDealt, USHRT_MAX);
            break;
        case DB_OHKOS_DEALT:
            TRY_INC_GAME_STAT(2, OHKOsDealt, USHRT_MAX);
            break;
        case DB_CRITS_TAKEN:
            TRY_INC_GAME_STAT(2, critsTaken, USHRT_MAX);
            break;
        case DB_OHKOS_TAKEN:
            TRY_INC_GAME_STAT(2, OHKOsTaken, USHRT_MAX);
            break;
        case DB_PLAYER_HP_HEALED:
            TRY_INC_GAME_STAT(2, playerHPHealed, UINT_MAX);
            break;
        case DB_ENEMY_HP_HEALED:
            TRY_INC_GAME_STAT(2, enemyHPHealed, UINT_MAX);
            break;
        case DB_PLAYER_POKEMON_FAINTED:
            TRY_INC_GAME_STAT(2, playerPokemonFainted, USHRT_MAX);
            break;
        case DB_ENEMY_POKEMON_FAINTED:
            TRY_INC_GAME_STAT(2, enemyPokemonFainted, USHRT_MAX);
            break;
        case DB_EXP_GAINED:
            TRY_INC_GAME_STAT(2, expGained, UINT_MAX);
            break;
        case DB_SWITCHOUTS:
            TRY_INC_GAME_STAT(2, switchouts, USHRT_MAX);
            break;
        case DB_BATTLES:
            TRY_INC_GAME_STAT(2, battles, USHRT_MAX);
            break;
        case DB_TRAINER_BATTLES:
            TRY_INC_GAME_STAT(2, trainerBattles, USHRT_MAX);
            break;
        case DB_WILD_BATTLES:
            TRY_INC_GAME_STAT(2, wildBattles, USHRT_MAX);
            break;
        case DB_BATTLES_FLED:
            TRY_INC_GAME_STAT(2, battlesFled, USHRT_MAX);
            break;
        case DB_FAILED_RUNS:
            TRY_INC_GAME_STAT(2, failedRuns, USHRT_MAX);
            break;
        case DB_MONEY_MADE:
            TRY_INC_GAME_STAT(2, moneyMade, UINT_MAX);
            break;
        case DB_MONEY_SPENT:
            TRY_INC_GAME_STAT(2, moneySpent, UINT_MAX);
            break;
        case DB_MONEY_LOST:
            TRY_INC_GAME_STAT(2, moneyLost, UINT_MAX);
            break;
        case DB_ITEMS_PICKED_UP:
            TRY_INC_GAME_STAT(2, itemsPickedUp, USHRT_MAX);
            break;
        case DB_ITEMS_BOUGHT:
            TRY_INC_GAME_STAT(2, itemsBought, USHRT_MAX);
            break;
        case DB_ITEMS_SOLD:
            TRY_INC_GAME_STAT(2, itemsSold, USHRT_MAX);
            break;
        case DB_MOVES_LEARNT:
            TRY_INC_GAME_STAT(2, movesLearnt, USHRT_MAX);
            break;
        case DB_BALLS_THROWN:
            TRY_INC_GAME_STAT(2, ballsThrown, USHRT_MAX);
            break;
        case DB_POKEMON_CAUGHT_IN_BALLS:
            TRY_INC_GAME_STAT(2, pokemonCaughtInBalls, USHRT_MAX);
            break;
    }
}

u32 GetDoneButtonStat(enum DoneButtonStat stat)
{
    switch(stat)
    {
        // DoneButtonStats1
        case DB_FRAME_COUNT_TOTAL:
            GET_GAME_STAT(1, frameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_OW:
            GET_GAME_STAT(1, owFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_BATTLE:
            GET_GAME_STAT(1, battleFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_MENU:
            GET_GAME_STAT(1, menuFrameCount, UINT_MAX);
            break;
        case DB_FRAME_COUNT_INTROS: // This needs special handling due to intro not having loaded save block yet.
            GET_GAME_STAT(1, introsFrameCount, UINT_MAX);
            break;
        case DB_SAVE_COUNT:
            GET_GAME_STAT(1, saveCount, USHRT_MAX);
            break;
        case DB_RELOAD_COUNT:
            GET_GAME_STAT(1, reloadCount, USHRT_MAX);
            break;
        case DB_CLOCK_RESET_COUNT:
            GET_GAME_STAT(1, clockResetCount, USHRT_MAX);
            break;
        case DB_STEP_COUNT:
            GET_GAME_STAT(1, stepCount, UINT_MAX);
            break;
        case DB_STEP_COUNT_WALK:
            GET_GAME_STAT(1, stepCountWalk, UINT_MAX);
            break;
        case DB_STEP_COUNT_SURF:
            GET_GAME_STAT(1, stepCountSurf, UINT_MAX);
            break;
        case DB_STEP_COUNT_BIKE:
            GET_GAME_STAT(1, stepCountBike, UINT_MAX);
            break;
        case DB_STEP_COUNT_RUN:
            GET_GAME_STAT(1, stepCountRun, UINT_MAX);
            break;
        case DB_BONKS:
            GET_GAME_STAT(1, bonks, USHRT_MAX);
            break;
        case DB_TOTAL_DAMAGE_DEALT:
            GET_GAME_STAT(1, totalDamageDealt, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_DEALT:
            GET_GAME_STAT(1, actualDamageDealt, UINT_MAX);
            break;
        case DB_TOTAL_DAMAGE_TAKEN:
            GET_GAME_STAT(1, totalDamageTaken, UINT_MAX);
            break;
        case DB_ACTUAL_DAMAGE_TAKEN:
            GET_GAME_STAT(1, actualDamageTaken, UINT_MAX);
            break;
        case DB_OWN_MOVES_HIT:
            GET_GAME_STAT(1, ownMovesHit, USHRT_MAX);
            break;
        case DB_OWN_MOVES_MISSED:
            GET_GAME_STAT(1, ownMovesMissed, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_HIT:
            GET_GAME_STAT(1, enemyMovesHit, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_MISSED:
            GET_GAME_STAT(1, enemyMovesMissed, USHRT_MAX);
            break;
        // DoneButtonStats2
        case DB_OWN_MOVES_SE:
            GET_GAME_STAT(2, ownMovesSE, USHRT_MAX);
            break;
        case DB_OWN_MOVES_NVE:
            GET_GAME_STAT(2, ownMovesNVE, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_SE:
            GET_GAME_STAT(2, enemyMovesSE, USHRT_MAX);
            break;
        case DB_ENEMY_MOVES_NVE:
            GET_GAME_STAT(2, enemyMovesNVE, USHRT_MAX);
            break;
        case DB_CRITS_DEALT:
            GET_GAME_STAT(2, critsDealt, USHRT_MAX);
            break;
        case DB_OHKOS_DEALT:
            GET_GAME_STAT(2, OHKOsDealt, USHRT_MAX);
            break;
        case DB_CRITS_TAKEN:
            GET_GAME_STAT(2, critsTaken, USHRT_MAX);
            break;
        case DB_OHKOS_TAKEN:
            GET_GAME_STAT(2, OHKOsTaken, USHRT_MAX);
            break;
        case DB_PLAYER_HP_HEALED:
            GET_GAME_STAT(2, playerHPHealed, UINT_MAX);
            break;
        case DB_ENEMY_HP_HEALED:
            GET_GAME_STAT(2, enemyHPHealed, UINT_MAX);
            break;
        case DB_PLAYER_POKEMON_FAINTED:
            GET_GAME_STAT(2, playerPokemonFainted, USHRT_MAX);
            break;
        case DB_ENEMY_POKEMON_FAINTED:
            GET_GAME_STAT(2, enemyPokemonFainted, USHRT_MAX);
            break;
        case DB_EXP_GAINED:
            GET_GAME_STAT(2, expGained, UINT_MAX);
            break;
        case DB_SWITCHOUTS:
            GET_GAME_STAT(2, switchouts, USHRT_MAX);
            break;
        case DB_BATTLES:
            GET_GAME_STAT(2, battles, USHRT_MAX);
            break;
        case DB_TRAINER_BATTLES:
            GET_GAME_STAT(2, trainerBattles, USHRT_MAX);
            break;
        case DB_WILD_BATTLES:
            GET_GAME_STAT(2, wildBattles, USHRT_MAX);
            break;
        case DB_BATTLES_FLED:
            GET_GAME_STAT(2, battlesFled, USHRT_MAX);
            break;
        case DB_FAILED_RUNS:
            GET_GAME_STAT(2, failedRuns, USHRT_MAX);
            break;
        case DB_MONEY_MADE:
            GET_GAME_STAT(2, moneyMade, UINT_MAX);
            break;
        case DB_MONEY_SPENT:
            GET_GAME_STAT(2, moneySpent, UINT_MAX);
            break;
        case DB_MONEY_LOST:
            GET_GAME_STAT(2, moneyLost, UINT_MAX);
            break;
        case DB_ITEMS_PICKED_UP:
            GET_GAME_STAT(2, itemsPickedUp, USHRT_MAX);
            break;
        case DB_ITEMS_BOUGHT:
            GET_GAME_STAT(2, itemsBought, USHRT_MAX);
            break;
        case DB_ITEMS_SOLD:
            GET_GAME_STAT(2, itemsSold, USHRT_MAX);
            break;
        case DB_MOVES_LEARNT:
            GET_GAME_STAT(2, movesLearnt, USHRT_MAX);
            break;
        case DB_BALLS_THROWN:
            GET_GAME_STAT(2, ballsThrown, USHRT_MAX);
            break;
        case DB_POKEMON_CAUGHT_IN_BALLS:
            GET_GAME_STAT(2, pokemonCaughtInBalls, USHRT_MAX);
            break;
    }
}

const u8 *GetStringSample(void)
{
    return NULL; // TODO
}

const u8 gTODOString[] = _("TODO");

const struct DoneButtonLineItem sLineItems[8][8] = {
    { // PAGE 1 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 2 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 3 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 4 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 5 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 6 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 7 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    },
    { // PAGE 8 (TODO)
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL},
        {gTODOString, NULL}
    }
};

#define NPAGES (NELEMS(sLineItems))

static const struct BgTemplate sSpeedchoiceDoneButtonTemplates[3] =
{
   {
       .bg = 1,
       .charBaseIndex = 1,
       .mapBaseIndex = 30,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 1,
       .baseTile = 0
   },
   {
       .bg = 0,
       .charBaseIndex = 1,
       .mapBaseIndex = 31,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 2,
       .baseTile = 0
   },
   // 0x000001ec
   {
       .bg = 2,
       .charBaseIndex = 1,
       .mapBaseIndex = 29,
       .screenSize = 0,
       .paletteMode = 0,
       .priority = 0,
       .baseTile = 0
   }
};

/*
    {1, 2, 1, 0x1A, 2, 1, 2},
    {0, 2, 5, 0x1A, 14, 1, 0x36},
    {2, 2, 15, 0x1A, 4, 15, 427},
    {2, 23, 9, 4, 4, 15, 531}, // YES/NO
    DUMMY_WIN_TEMPLATE
*/

static const struct WindowTemplate sWinTemplates[3] =
{
    {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 0x1A,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2,
    },
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 0x1A,
        .height = 14,
        .paletteNum = 1,
        .baseBlock = 0x36,
    },
    DUMMY_WIN_TEMPLATE,
};

void Task_InitDoneButtonMenu(u8 taskId)
{
    OpenDoneButton(CB2_ReturnToField);
    DestroyTask(taskId);
}

void OpenDoneButton(MainCallback doneCallback)
{
    doneButton = AllocZeroed(sizeof(*doneButton));
    if (doneButton == NULL)
        SetMainCallback2(doneCallback);
    else
    {
        doneButton->doneCallback = doneCallback;
        doneButton->taskId = 0xFF;
        doneButton->page = 0;
        SetMainCallback2(DoneButtonCB);
    }
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

extern const struct BgTemplate sMainMenuBgTemplates[];
extern const struct WindowTemplate sSpeedchoiceMenuWinTemplates[];

void Task_DoneButtonFadeIn(u8 taskId);

void DoneButtonCB(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
    {
        u8 *addr;
        u32 size;

        addr = (u8 *)VRAM;
        size = 0x18000;
        while (1)
        {
            DmaFill16(3, 0, addr, 0x1000);
            addr += 0x1000;
            size -= 0x1000;
            if (size <= 0x1000)
            {
                DmaFill16(3, 0, addr, size);
                break;
            }
        }
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sSpeedchoiceDoneButtonTemplates, ARRAY_COUNT(sSpeedchoiceDoneButtonTemplates));
        ChangeBgX(0, 0, 0);
        ChangeBgY(0, 0, 0);
        ChangeBgX(1, 0, 0);
        ChangeBgY(1, 0, 0);
        ChangeBgX(2, 0, 0);
        ChangeBgY(2, 0, 0);
        ChangeBgX(3, 0, 0);
        ChangeBgY(3, 0, 0);
        InitWindows(sWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 5);
        SetGpuReg(REG_OFFSET_WINOUT, 39);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        gMain.state++;
        break;
    }
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sUnknown_0855C6A0, 0, sizeof(sUnknown_0855C6A0));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, 0x70, 0x20);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sUnknown_0855C604, 0x10, sizeof(sUnknown_0855C604));
        LoadPalette(sMainMenuTextPal, 0xF0, sizeof(sMainMenuTextPal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(0);
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        //PutWindowTilemap(1);
        //DrawOptionMenuTexts();
        gMain.state++;
    case 9:
        DrawDoneButtonFrame();
        gMain.state++;
        break;
    case 10:
        //FillWindowPixelBuffer(0, PIXEL_FILL(0));
        PlayBGM(MUS_PCC);
        PrintGameStatsPage();
        doneButton->taskId = CreateTask(Task_DoneButtonFadeIn, 0);
        gMain.state++;
        break;
    case 11:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        break;
    }
}

void Task_DoneButtonFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_DoneButton;
}

static void Task_DoneButton(u8 taskId)
{
    struct DoneButton *data = doneButton;

    if (JOY_NEW(DPAD_RIGHT))
    {
        PlaySE(SE_SELECT);
        if (++data->page >= NPAGES)
            data->page = 0;
        PrintGameStatsPage();
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        PlaySE(SE_SELECT);
        if (data->page == 0)
            data->page = NPAGES - 1;
        else
            data->page--;
        PrintGameStatsPage();
    }
    else if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON))
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].func = Task_DestroyDoneButton;
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    }
}

static void Task_DestroyDoneButton(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        SetMainCallback2(doneButton->doneCallback);
        Free(doneButton);
        DestroyTask(taskId);
    }
}

static void PrintGameStatsPage(void)
{
    const struct DoneButtonLineItem * items = sLineItems[doneButton->page];
    s32 i;

    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    for (i = 0; i < 8; i++)
    {
        s32 width;
        const char * value_s;
        if (items[i].name != NULL)
        {
            AddTextPrinterParameterized(0, 1, items[i].name, 1, 18 * i + 1, -1, NULL);
        }
        if (items[i].printfn != NULL)
        {
            value_s = items[i].printfn();
        }
        else
        {
            value_s = gTODOString;
        }
        width = GetStringWidth(0, value_s, 0);
        AddTextPrinterParameterized(0, 1, items[i].name, 200 - width, 18 * i + 1, -1, NULL);
    }
    PutWindowTilemap(0);
    CopyWindowToVram(0, 3);
}

void DrawDoneButtonFrame(void)
{
    //                   bg, tileNum, x,    y,    width, height,  pal
    FillBgTilemapBufferRect(1, 0x1A2, 0,    0,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A3, 1,    0,      0x1C,   1,      7);
    FillBgTilemapBufferRect(1, 0x1A4, 29,   0,      1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A5, 0,    1,      1,      0x16,   7);
    FillBgTilemapBufferRect(1, 0x1A7, 29,   1,      1,      0x16,   7);
    FillBgTilemapBufferRect(1, 0x1A8, 0,    19,     1,      1,      7);
    FillBgTilemapBufferRect(1, 0x1A9, 1,    19,     0x1C,   1,      7);
    FillBgTilemapBufferRect(1, 0x1AA, 29,   19,     1,      1,      7);

    CopyBgTilemapBufferToVram(1);
}
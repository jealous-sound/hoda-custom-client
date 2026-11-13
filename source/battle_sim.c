
typedef enum {
    AIL_NONE = 0,
    AIL_CURSE_POWER = 1 << 0,
    AIL_STUNNED = 1 << 1,
    AIL_CURSE_HEALTH = 1 << 2,
    AIL_UNCURSE_POWER = 1 << 3,
    AIL_UNSTUNNED = 1 << 4,
    AIL_UNCURSE_HEALTH = 1 << 5
} BattleAilment;

typedef struct {
    int unit;
    int health_delta;
    int power_delta;
    int initiative_delta;
    int health_faction_delta;
    float crit_delta;
    float damage_bonus_delta;
} BattleSimTarget;

#define SIM_UNIT_COUNT (2 * SQUAD_SLOT_COUNT)
#define SIM_MAX_TARGETS SIM_UNIT_COUNT
#define SIM_MAX_ACTIONS 4096
#define SIM_INIT_CLASS_SPAN 40
#define SIM_RUNE_TARGET_WEIGHT_PER_LEVEL 0.5f
#define SIM_RUNE_SHIELDED_HIT_MIN_DAMAGE 1
#define SIM_GEAR_TARGET_WEIGHT 2.0f
#define SIM_GEAR_STRONG_TARGET_WEIGHT 3.0f
#define SIM_GEAR_FLANK_WEIGHT 3.0f
#define SIM_GEAR_FLANK_REDUCTION 0.5f
#define SIM_GEAR_DEATH_PASSES 4
#define SIM_GEAR_ACTION_RESERVE 64
#define SIM_DEFENDER_ABSORB_PCT 25

typedef enum { SIM_INIT_CLASS_SLOW, SIM_INIT_CLASS_MEDIUM, SIM_INIT_CLASS_FAST } SimInitiativeClass;

typedef struct {
    int actor;
    bool newround;
    bool crit;
    bool replay_attack;
    bool same_swing;
    BattleAilment ailments;
    BattleSimTarget targets[SIM_MAX_TARGETS];
    int target_count;
} BattleSimAction;

typedef struct {
    int damage_resist_pct, raw_damage_resist;
    int stun_resist_pct, slow_resist_pct, drain_resist_pct;
    int curse_power_resist_pct, curse_health_resist_pct;
    int defender_pct;
    bool stealth, perceptive, haste, flank_reduction;
    bool target_highest_power, target_lowest_health, target_back_row, target_quick, target_slowest;
} SimGearState;

typedef struct {
    const Character *ch;
    const PassiveCharacter *passives;
    SimGearState gear;
    bool present, alive;
    int side, slot;
    int max_health, health;
    int power, start_power;
    int potency_cut_pct;
    int passive_cut_pct;
    float crit_pct;
    float damage_bonus_pct;
    int init_class;
    int cur_initiative;
    bool stunned;
    bool curse_power, curse_health;
    unsigned char curse_power_turns, curse_health_turns;
    int marked;
    unsigned faction;
    bool game_changer_applied;
    long long gear_trigger_worth[GEAR_SLOT_CAPACITY];
} SimUnit;

typedef struct {
    SimUnit units[SIM_UNIT_COUNT];
    BattleSimAction actions[SIM_MAX_ACTIONS];
    int action_count;
    int rounds;
    bool player_won;
    unsigned formation_faction[2];
    float formation_bonus_pct[2];
    RuneBonus rune_bonus[2];
    int rune_defs[RUNE_SLOT_COUNT];
    int rune_def_count;
    unsigned rng;
    unsigned gear_pending_deaths;
    int gear_last_damage;
    const StatBudget *stat_budget;
} BattleSim;

typedef enum {
    RECIPE_TARGET_ABILITY,
    RECIPE_TARGET_ONE,
    RECIPE_TARGET_ROW,
    RECIPE_TARGET_COLUMN,
    RECIPE_TARGET_ALL,
    RECIPE_TARGET_ONE_ALLY,
    RECIPE_TARGET_BACK_ROW,
    RECIPE_TARGET_ANY
} RecipeTarget;

typedef struct {
    short dmg_pct;
    short stun_pct;
    short slow_pct, slow_amt;
    short drain_pct, drain_chance;
    short curse_power_pct, curse_health_pct;
    short curse_turns;
    short levy_pct;
    short assassinate_pct;
    short mark_pct;
    short self_heal_pct;
    short self_crit_pct;
    RecipeTarget target;
    bool status_only;
} AttackRecipe;

typedef struct {
    short self_power_pct, self_power_chance;
    short self_heal_pct, self_heal_chance;
    short self_damage_bonus_pct;
    short self_damage_pct;
    short ally_heal_pct, ally_heal_chance;
    short ally_power_pct, ally_power_chance;
    short ally_power_from_actor_pct;
    short ally_slow_amt;
    short ally_crit_pct;
    short ally_damage_bonus_pct;
    short ally_damage_pct;
} SecondaryEffects;

#define SIM_RECIPE_MAX_STEPS 8
typedef struct {
    AttackRecipe attack;
    short chance_pct, repeat, chance_group, final_damage_pct;
    bool stop_on_fail, effects_once;
    short self_crit_pct;
    SecondaryEffects fx;
} RecipeStep;

typedef struct {
    char key[160];
    AttackRecipe hit;
    SecondaryEffects fx;
    short effect_pct;
    AttackRecipe effect;
    short followup_pct, followup_count;
    short followup_damage_step_pct;
    short followup_final_damage_pct;
    AttackRecipe followup;
    bool generated;
    unsigned char generated_step_count;
    RecipeStep generated_steps[SIM_RECIPE_MAX_STEPS];
} AbilityRecipe;

static const AbilityRecipe SIM_RECIPES[] = {
    {.key = "ABILITY_DAMAGE_ONE"},
    {.key = "ABILITY_DAMAGE_ONE_INCREASE_POWER", .fx.self_power_pct = 25},
    {.key = "ABILITY_DAMAGE_ROW"},
    {.key = "ABILITY_DAMAGE_ROW_INCREASE_POWER", .fx.self_power_pct = 50},
    {.key = "ABILITY_DAMAGE_COLUMN"},
    {.key = "ABILITY_DAMAGE_ALL"},
    {.key = "ABILITY_DAMAGE_ALL_GAINPOWER", .fx.self_power_pct = 25},
    {.key = "ABILITY_DAMAGE_ALL_HEAL_SELF", .fx.self_heal_pct = 25},
    {.key = "ABILITY_DAMAGE_HEAL_SELF", .fx.self_heal_pct = 10},
    {.key = "damage_one_heal_self", .fx.self_heal_pct = 25},
    {.key = "ABILITY_DAMAGE_COLUMN_HEAL_SELF", .fx.self_heal_pct = 25},
    {.key = "ABILITY_DAMAGE_AND_HALF_POWER", .hit = {.drain_pct = 50}},
    {.key = "ABILITY_DAMAGE_ALL_AND_DRAINPOWER", .hit = {.drain_pct = 25}},
    {.key = "ABILITY_DAMAGE_ONE_DRAINPOWER_SELFDRAINPOWER", .hit = {.drain_pct = 25}, .fx.self_power_pct = -10},
    {.key = "ABILITY_DAMAGE_INIT_50", .hit = {.slow_pct = 100, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_INIT_100", .hit = {.slow_pct = 100, .slow_amt = -100}},
    {.key = "ABILITY_DAMAGE_INIT_50_ALL", .hit = {.slow_pct = 100, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_INIT_50_DRAINPOWER_ALL", .hit = {.slow_pct = 100, .slow_amt = -50, .drain_pct = 25}},
    {.key = "ABILITY_DAMAGE_INIT_50_COLUMN", .hit = {.slow_pct = 100, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_INIT_50_STUN25_ALL", .hit = {.stun_pct = 25, .slow_pct = 100, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_ALL_SLOW25", .hit = {.slow_pct = 25, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_ONE_STUN25_SLOW25", .hit = {.stun_pct = 25, .slow_pct = 25, .slow_amt = -50}},
    {.key = "ABILITY_DAMAGE_HIGH_STUN_CHANCE", .hit = {.stun_pct = 75}},
    {.key = "ABILITY_DAMAGE_STUN25_ALL", .hit = {.stun_pct = 25}},
    {.key = "ABILITY_DAMAGE_STUN25_ROW", .hit = {.stun_pct = 25}},
    {.key = "ABILITY_DAMAGE_STUN25_COLUMN", .hit = {.stun_pct = 25}},
    {.key = "ABILITY_DAMAGE_STUN50_ALL", .hit = {.stun_pct = 50}},
    {.key = "ABILITY_DAMAGE_ONE_DAMAGE_SELF", .hit = {.dmg_pct = 150}, .fx.self_damage_pct = -10},
    {.key = "ABILITY_DAMAGE_ALL_POWERGAIN25_HEAL25",
     .fx.self_power_pct = 25,
     .fx.self_power_chance = 25,
     .fx.self_heal_pct = 25,
     .fx.self_heal_chance = 25},
    {.key = "ABILITY_DAMAGE_COLUMN_POWERGAIN25_POWERDRAIN25",
     .hit = {.drain_pct = 25, .drain_chance = 25},
     .fx.self_power_pct = 25,
     .fx.self_power_chance = 25},
    {.key = "ABILITY_DAMAGE_ONE_POWERGAIN50_ALL", .fx.ally_power_pct = 25, .fx.ally_power_chance = 50},
    {.key = "attack1_single_drainpower_attack2and3_single_300damage",
     .hit = {.drain_pct = 25},
     .followup_pct = 10,
     .followup_count = 2,
     .followup = {.dmg_pct = 300, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_all_25heal50powerallies_20attacktwice",
     .fx.ally_heal_pct = 25,
     .fx.ally_power_pct = 50,
     .followup_pct = 20,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_ALL}},
    {.key = "attack_all_chance75_stun_slow_drain35",
     .hit = {.stun_pct = 75, .slow_pct = 75, .slow_amt = -50, .drain_pct = 35, .drain_chance = 75}},
    {.key = "attack_all_curse_health_chance50_stun_drain50",
     .hit = {.stun_pct = 50, .drain_pct = 50, .drain_chance = 50, .curse_health_pct = 100}},
    {.key = "attack_all_drain30_cursepower_gain20heal20self",
     .hit = {.drain_pct = 30, .curse_power_pct = 100},
     .fx.self_power_pct = 20,
     .fx.self_heal_pct = 20},
    {.key = "attack_all_drain35_stun50_ally5damage",
     .hit = {.stun_pct = 50, .drain_pct = 35},
     .fx.ally_damage_pct = -5},
    {.key = "attack_all_slow_allypowergain20", .hit = {.slow_pct = 100, .slow_amt = -50}, .fx.ally_power_pct = 20},
    {.key = "attack_column50_cursestunrow_followup_attackone300_assassinate20",
     .hit = {.dmg_pct = 50},
     .effect_pct = 100,
     .effect =
         {.stun_pct = 100,
          .curse_power_pct = 100,
          .curse_health_pct = 100,
          .target = RECIPE_TARGET_ROW,
          .status_only = true},
     .followup_pct = 100,
     .followup_count = 1,
     .followup = {.dmg_pct = 300, .assassinate_pct = 20, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_one_drain_gain_60_followup_33",
     .hit = {.drain_pct = 60},
     .fx.self_power_pct = 60,
     .followup_pct = 33,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_ONE}},
    {.key = "attack_row_cursepower_drain50_followup25random50_followup25ally50",
     .hit = {.drain_pct = 50, .curse_power_pct = 100},
     .effect_pct = 25,
     .effect = {.dmg_pct = 50, .target = RECIPE_TARGET_ONE_ALLY},
     .followup_pct = 25,
     .followup_count = 1,
     .followup = {.dmg_pct = 50, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_row_drain60_stun35_mark20", .hit = {.stun_pct = 35, .drain_pct = 60, .mark_pct = 20}},
    {.key = "attack_row_drainpower50_gainpower25", .hit = {.drain_pct = 50}, .fx.self_power_pct = 25},
    {.key = "attack_row_stun50_heal25", .hit = {.stun_pct = 50}, .fx.self_heal_pct = 25},
    {.key = "attack_row_stun50_heal50_all_followup50_single_slow",
     .hit = {.stun_pct = 50},
     .fx.ally_heal_pct = 50,
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_single_curse75all_mark30",
     .hit = {.mark_pct = 30},
     .effect_pct = 75,
     .effect = {.curse_power_pct = 100, .curse_health_pct = 100, .target = RECIPE_TARGET_ALL, .status_only = true}},
    {.key = "attack_single_levy50_heal25gain25_all",
     .hit = {.levy_pct = 50},
     .fx.ally_heal_pct = 25,
     .fx.ally_power_pct = 25},
    {.key = "damage_all_20chance_followup_3xdamage",
     .followup_pct = 20,
     .followup_count = 1,
     .followup = {.dmg_pct = 300, .target = RECIPE_TARGET_ALL}},
    {.key = "damage_all_20percent_powergain_50chance_slow_25chance_stun",
     .hit = {.stun_pct = 25, .slow_pct = 50, .slow_amt = -50},
     .fx.ally_power_pct = 20},
    {.key = "damage_all_25chance_at_slow", .hit = {.slow_pct = 25, .slow_amt = -100}},
    {.key = "damage_init_50_all_gainpower_all", .hit = {.slow_pct = 100, .slow_amt = -50}, .fx.ally_power_pct = 25},
    {.key = "damage_all_drainpower_cursepower", .hit = {.drain_pct = 25, .curse_power_pct = 100}},
    {.key = "damage_gainpower_drainpower_cursepower",
     .hit = {.drain_pct = 30},
     .fx.self_power_pct = 25,
     .effect_pct = 100,
     .effect = {.curse_power_pct = 100, .target = RECIPE_TARGET_COLUMN, .status_only = true}},
    {.key = "damage_one_10x_heal5_crit2_small_chance_neg_ailment",
     .hit =
         {.stun_pct = 20,
          .slow_pct = 20,
          .slow_amt = -50,
          .drain_pct = 20,
          .drain_chance = 20,
          .curse_power_pct = 20,
          .curse_health_pct = 20,
          .self_heal_pct = 5,
          .self_crit_pct = 2},
     .followup_pct = 50,
     .followup_count = 9,
     .followup =
         {.stun_pct = 20,
          .slow_pct = 20,
          .slow_amt = -50,
          .drain_pct = 20,
          .drain_chance = 20,
          .curse_power_pct = 20,
          .curse_health_pct = 20,
          .self_heal_pct = 5,
          .self_crit_pct = 2,
          .target = RECIPE_TARGET_ONE}},
    {.key = "damage_one_drain20_chance20_power200",
     .hit = {.drain_pct = 20},
     .followup_pct = 20,
     .followup_count = 1,
     .followup = {.dmg_pct = 200, .target = RECIPE_TARGET_ONE}},
    {.key = "damage_one_drainpower25", .hit = {.drain_pct = 25}},
    {.key = "damage_one_levy_15pct_allygain_15pct", .hit = {.levy_pct = 15}, .fx.ally_power_pct = 15},
    {.key = "damage_row_attack2x_35pctchance",
     .followup_pct = 35,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_ROW}},
    {.key = "damage_row_drain25power_and_35pctchance_attackagain",
     .hit = {.drain_pct = 25},
     .followup_pct = 35,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_ROW}},

    {.key = "ABILITY_DAMAGE_ALL_CURSEPOWER_AND_CURSEHEALTH", .hit = {.curse_power_pct = 100, .curse_health_pct = 100}},
    {.key = "ABILITY_DAMAGE_ALL_STUN_25", .hit = {.stun_pct = 25}},
    {.key = "ABILITY_DAMAGE_COLUMN_CURSEPOWER_FOLLOWUP25_HEAL_ALL",
     .hit = {.curse_power_pct = 100},
     .fx.ally_heal_pct = 65,
     .followup_pct = 30,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_COLUMN}},
    {.key = "attack_all_alliesgain15power20health_slow50",
     .hit = {.slow_pct = 50, .slow_amt = -50},
     .fx.ally_heal_pct = 20,
     .fx.ally_power_pct = 15},
    {.key = "attack_all_chance20_stun_heavyslow_curseboth_powerdrain50",
     .hit =
         {.stun_pct = 20,
          .slow_pct = 20,
          .slow_amt = -100,
          .drain_pct = 50,
          .drain_chance = 20,
          .curse_power_pct = 20,
          .curse_health_pct = 20}},
    {.key = "attack_all_curse_drain35_gain25all_heal25all",
     .hit = {.drain_pct = 35, .curse_power_pct = 100, .curse_health_pct = 100},
     .fx.ally_heal_pct = 25,
     .fx.ally_power_pct = 25},
    {.key = "attack_all_drain50_stun35curse35_heal40gain40all",
     .hit = {.stun_pct = 35, .drain_pct = 50, .curse_power_pct = 35, .curse_health_pct = 35},
     .fx.ally_heal_pct = 40,
     .fx.ally_power_pct = 40},
    {.key = "attack_all_drain50_plus10critall_followup_single150_stun",
     .hit = {.drain_pct = 50},
     .fx.ally_crit_pct = 10,
     .followup_pct = 100,
     .followup_count = 1,
     .followup = {.dmg_pct = 150, .stun_pct = 100, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_all_drain65_crit7", .hit = {.drain_pct = 65}, .fx.ally_crit_pct = 7},
    {.key = "attack_all_gainhealth25_all_gainpower50_and_15drain_power_all",
     .hit = {.drain_pct = 15},
     .fx.ally_heal_pct = 25,
     .fx.ally_power_pct = 50},
    {.key = "attack_all_slow_Attack_2_heavy_slow",
     .hit = {.slow_pct = 100, .slow_amt = -100},
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.slow_pct = 100, .slow_amt = -100, .target = RECIPE_TARGET_ALL}},
    {.key = "attack_all_slow_cursepower_stun30",
     .hit = {.stun_pct = 30, .slow_pct = 100, .slow_amt = -50, .curse_power_pct = 100}},
    {.key = "attack_all_slow_followup25_attack_row_stun_chance10_heal75",
     .hit = {.slow_pct = 100, .slow_amt = -50},
     .fx.self_heal_pct = 75,
     .fx.self_heal_chance = 10,
     .followup_pct = 25,
     .followup_count = 1,
     .followup = {.stun_pct = 100, .target = RECIPE_TARGET_ROW}},
    {.key = "attack_column3x", .followup_pct = 100, .followup_count = 2, .followup = {.target = RECIPE_TARGET_COLUMN}},
    {.key = "attack_column_cursehealth_followup50_attack_row_slow_assassinate30",
     .hit = {.curse_health_pct = 100},
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_ROW},
     .effect_pct = 30,
     .effect = {.assassinate_pct = 100, .target = RECIPE_TARGET_ONE, .status_only = true}},
    {.key = "attack_column_levy35_heavyslowrow_stun50row_attackself5",
     .hit = {.levy_pct = 35},
     .fx.self_damage_pct = -5,
     .effect_pct = 100,
     .effect = {.stun_pct = 50, .slow_pct = 100, .slow_amt = -100, .target = RECIPE_TARGET_ROW, .status_only = true}},
    {.key = "attack_column_stun_slow50all_attack_ally5",
     .hit = {.stun_pct = 100},
     .fx.ally_damage_pct = -5,
     .effect_pct = 50,
     .effect = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_ALL, .status_only = true}},
    {.key = "attack_one_4xfollowup40_gain25all_heal75self",
     .fx.self_heal_pct = 75,
     .fx.ally_power_pct = 25,
     .followup_pct = 40,
     .followup_count = 4,
     .followup = {.target = RECIPE_TARGET_ONE}},
    {.key = "attack_one_gain25all_heal25self_assassinate5",
     .hit = {.assassinate_pct = 5},
     .fx.self_heal_pct = 25,
     .fx.ally_power_pct = 25},
    {.key = "attack_one_levy35_chance40_attack_row_stun",
     .hit = {.levy_pct = 35},
     .followup_pct = 40,
     .followup_count = 1,
     .followup = {.stun_pct = 100, .target = RECIPE_TARGET_ROW}},
    {.key = "attack_one_levy40_followup50_attack_all_stun35",
     .hit = {.levy_pct = 40},
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.stun_pct = 35, .target = RECIPE_TARGET_ALL}},
    {.key = "attack_one_levy50_stun_crit5all_bnsdmg3all_assassinate15",
     .hit = {.stun_pct = 100, .levy_pct = 50, .assassinate_pct = 15},
     .fx.ally_crit_pct = 5,
     .fx.ally_damage_bonus_pct = 3},
    {.key = "attack_single_levy35_heal40all_2bnsdmgall_damage5all",
     .hit = {.levy_pct = 35},
     .fx.ally_heal_pct = 40,
     .fx.ally_damage_bonus_pct = 2,
     .fx.ally_damage_pct = -5},
    {.key = "attack_row_drain10self_stun50all_gain5bnsdmg",
     .fx.self_power_pct = -10,
     .fx.self_damage_bonus_pct = 5,
     .effect_pct = 100,
     .effect = {.stun_pct = 50, .target = RECIPE_TARGET_ALL, .status_only = true}},
    {.key = "attack_all_chance75_slow_drain35_5bnsdmgall_gain5critall",
     .hit = {.slow_pct = 75, .slow_amt = -50, .drain_pct = 35, .drain_chance = 75},
     .fx.ally_crit_pct = 5,
     .fx.ally_damage_bonus_pct = 5},
    {.key = "attack_row_slowall_stun50column_attack_ally5_chance10heal80self",
     .fx.self_heal_pct = 80,
     .fx.self_heal_chance = 10,
     .fx.ally_damage_pct = -5,
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.stun_pct = 100, .target = RECIPE_TARGET_COLUMN, .status_only = true},
     .effect_pct = 100,
     .effect = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_ALL, .status_only = true}},
    {.key = "attack_single_drain_power_and_attack_all",
     .hit = {.drain_pct = 50},
     .followup_pct = 100,
     .followup_count = 1,
     .followup = {.target = RECIPE_TARGET_ALL}},
    {.key = "attackcolumn_20pwrdrn_secondattack30stun_heal20all",
     .hit = {.drain_pct = 20},
     .fx.ally_heal_pct = 20,
     .followup_pct = 30,
     .followup_count = 1,
     .followup = {.stun_pct = 100, .target = RECIPE_TARGET_COLUMN}},
    {.key = "damage_all_and_drainpower_and_curse_power_health_all",
     .hit = {.drain_pct = 25, .curse_power_pct = 100, .curse_health_pct = 100}},
    {.key = "damage_all_curse_health_power", .hit = {.curse_power_pct = 100, .curse_health_pct = 100}},
    {.key = "damage_all_cursehealth_chance_stun", .hit = {.stun_pct = 25, .curse_health_pct = 100}},
    {.key = "damage_all_cursepower_slowdrain50_allygain35",
     .hit = {.slow_pct = 50, .slow_amt = -50, .drain_pct = 50, .drain_chance = 50, .curse_power_pct = 100},
     .fx.ally_power_pct = 35},
    {.key = "damage_all_drainpower_25_and_slow_50_followup_single_followup_mark",
     .hit = {.slow_pct = 50, .slow_amt = -50, .drain_pct = 25},
     .followup_pct = 100,
     .followup_count = 1,
     .followup = {.mark_pct = 30, .target = RECIPE_TARGET_ONE}},
    {.key = "damage_all_gain_power_all", .fx.ally_power_pct = 10},
    {.key = "damage_all_powergain15_heal5_attack_own_all_5",
     .hit = {.curse_power_pct = 100},
     .fx.self_power_pct = 15,
     .fx.self_heal_pct = 5,
     .fx.ally_damage_pct = -5},
    {.key = "damage_all_stun_50_and_curse_power_and_health_all",
     .hit = {.stun_pct = 50, .curse_power_pct = 100, .curse_health_pct = 100}},
    {.key = "damage_init_100_all", .hit = {.slow_pct = 100, .slow_amt = -100}},
    {.key = "damage_one_5x_150",
     .followup_pct = 100,
     .followup_count = 4,
     .followup_final_damage_pct = 150,
     .followup = {.target = RECIPE_TARGET_ONE}},
    {.key = "damage_row_cursepower_stun60", .hit = {.stun_pct = 60, .curse_power_pct = 100}},
    {.key = "damage_row_drain25_stun25_assassinate_single_pct10",
     .hit = {.stun_pct = 25, .drain_pct = 25, .assassinate_pct = 10}},
    {.key = "damage_single_heal_all_medium_chance_followup3x_plus_damage",
     .fx.ally_heal_pct = 30,
     .followup_pct = 50,
     .followup_count = 3,
     .followup_damage_step_pct = 50,
     .followup = {.target = RECIPE_TARGET_ONE}},

    {.key = "attack_column_power_all25", .fx.ally_power_pct = 25},
    {.key = "damage_random_single_2x_powerdrain_each",
     .hit = {.drain_pct = 50},
     .followup_pct = 100,
     .followup_count = 1,
     .followup = {.drain_pct = 50, .target = RECIPE_TARGET_ONE}},
    {.key = "attack_all_3x_heal_all20_stun_25_curse_health50",
     .hit = {.stun_pct = 25, .curse_health_pct = 50},
     .fx.ally_heal_pct = 20,
     .followup_pct = 100,
     .followup_count = 2,
     .followup = {.stun_pct = 25, .curse_health_pct = 50, .target = RECIPE_TARGET_ALL}},
    {.key = "attack_single_3x_heal50_stealth",
     .fx.self_heal_pct = 50,
     .followup_pct = 100,
     .followup_count = 2,
     .followup = {.target = RECIPE_TARGET_ONE}},
    {.key = "attack_column_slow_all_slow_ally",
     .fx.ally_slow_amt = -50,
     .effect_pct = 100,
     .effect = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_ALL, .status_only = true}},

    {.key = "ability_heal_allies_huge_no_attack", .fx.ally_heal_pct = 100},
    {.key = "grant_allies_100power_no_attack", .fx.ally_power_from_actor_pct = 100},
    {.key = "power_drain_all50_assassinate_chance10", .hit = {.drain_pct = 50, .assassinate_pct = 10}},
    {.key = "ability_heal_allies_crit_boost_allies5_no_attack", .fx.ally_heal_pct = 100, .fx.ally_crit_pct = 5},
    {.key = "stun_single_drain_all25_critgain10_allies_no_attack",
     .hit = {.stun_pct = 100},
     .fx.ally_crit_pct = 10,
     .effect_pct = 100,
     .effect = {.drain_pct = 25, .target = RECIPE_TARGET_ALL, .status_only = true}},
    {.key = "heal200self_heal25allies", .fx.self_heal_pct = 200, .fx.ally_heal_pct = 25},
    {.key = "slow_all_drain25_allies_crit7_no_attack",
     .hit = {.slow_pct = 100, .slow_amt = -50, .drain_pct = 25},
     .fx.ally_crit_pct = 7},
    {.key = "slows_row_heavy_powergain_allies_50", .hit = {.slow_pct = 100, .slow_amt = -100}, .fx.ally_power_pct = 50},
    {.key = "heals_and_powergains_allies_15_and_curses_enemies_2_rounds_no_attack",
     .hit = {.curse_power_pct = 100, .curse_health_pct = 100, .curse_turns = 2},
     .fx.ally_heal_pct = 15,
     .fx.ally_power_pct = 15},
    {.key = "slows_all_curse_power_backrow_stun_column",
     .hit = {.slow_pct = 100, .slow_amt = -50},
     .effect_pct = 100,
     .effect = {.curse_power_pct = 100, .target = RECIPE_TARGET_BACK_ROW, .status_only = true},
     .followup_pct = 80,
     .followup_count = 1,
     .followup = {.stun_pct = 100, .target = RECIPE_TARGET_COLUMN, .status_only = true}},
    {.key = "cursepowerall_noattack", .hit = {.curse_power_pct = 100}},
    {.key = "noattack_heal75all_gain50all_stun50row_slow50col",
     .fx.ally_heal_pct = 75,
     .fx.ally_power_pct = 50,
     .effect_pct = 50,
     .effect = {.stun_pct = 100, .target = RECIPE_TARGET_ROW, .status_only = true},
     .followup_pct = 50,
     .followup_count = 1,
     .followup = {.slow_pct = 100, .slow_amt = -50, .target = RECIPE_TARGET_COLUMN, .status_only = true}},
    {.key = "noattack_healall_gain35all", .fx.ally_heal_pct = 100, .fx.ally_power_pct = 35},
    {.key = "noattack_heal100all_gain100all", .fx.ally_heal_pct = 100, .fx.ally_power_from_actor_pct = 100},
    {.key = "noattack_healall50_gainall100_stunrow50_slowrow50_cursepwrrow50",
     .hit = {.stun_pct = 50, .slow_pct = 50, .slow_amt = -50, .curse_power_pct = 50, .target = RECIPE_TARGET_ROW},
     .fx.ally_heal_pct = 50,
     .fx.ally_power_pct = 100},
};
#define SIM_RECIPE_COUNT ((int)(sizeof(SIM_RECIPES) / sizeof(SIM_RECIPES[0])))

static const AbilityRecipe SIM_RECIPE_DEFAULT = {.key = ""};

static const AbilityRecipe *SimExactRecipe(const Character *ch) {
    for (int i = 0; i < SIM_RECIPE_COUNT; i++)
        if (!strcmp(SIM_RECIPES[i].key, ch->ability->key)) return &SIM_RECIPES[i];
    return &SIM_RECIPE_DEFAULT;
}

#define SIM_GENERATED_RECIPE_MAX 512

static RecipeTarget SimGeneratedTarget(const char *name) {
    if (!strcmp(name, "one")) return RECIPE_TARGET_ONE;
    if (!strcmp(name, "row")) return RECIPE_TARGET_ROW;
    if (!strcmp(name, "column")) return RECIPE_TARGET_COLUMN;
    if (!strcmp(name, "all")) return RECIPE_TARGET_ALL;
    if (!strcmp(name, "one_ally")) return RECIPE_TARGET_ONE_ALLY;
    if (!strcmp(name, "back_row")) return RECIPE_TARGET_BACK_ROW;
    if (!strcmp(name, "any")) return RECIPE_TARGET_ANY;
    return RECIPE_TARGET_ABILITY;
}

static void SimGeneratedAttackParse(const cJSON *json, AttackRecipe *attack) {
    attack->target = SimGeneratedTarget(JsonStr(json, "target"));
    attack->dmg_pct = (short)JsonInt(json, "damage_pct");
    attack->stun_pct = (short)JsonInt(json, "stun_pct");
    attack->slow_pct = (short)JsonInt(json, "slow_pct");
    attack->slow_amt = (short)JsonInt(json, "slow_amt");
    attack->drain_pct = (short)JsonInt(json, "drain_pct");
    attack->drain_chance = (short)JsonInt(json, "drain_chance");
    attack->curse_power_pct = (short)JsonInt(json, "curse_power_pct");
    attack->curse_health_pct = (short)JsonInt(json, "curse_health_pct");
    attack->curse_turns = (short)JsonInt(json, "curse_turns");
    attack->levy_pct = (short)JsonInt(json, "levy_pct");
    attack->assassinate_pct = (short)JsonInt(json, "assassinate_pct");
    attack->mark_pct = (short)JsonInt(json, "mark_pct");
    attack->self_heal_pct = (short)JsonInt(json, "self_heal_pct");
    attack->self_crit_pct = (short)JsonInt(json, "self_crit_pct");
    attack->status_only = JsonBool(json, "status_only");
}

static void SimGeneratedEffectsParse(const cJSON *json, RecipeStep *step) {
    if (!json) return;
    SecondaryEffects *fx = &step->fx;
    fx->self_power_pct = (short)JsonInt(json, "self_power_pct");
    fx->self_power_chance = (short)JsonInt(json, "self_power_chance");
    fx->self_heal_pct = (short)JsonInt(json, "self_heal_pct");
    fx->self_heal_chance = (short)JsonInt(json, "self_heal_chance");
    step->self_crit_pct = (short)JsonInt(json, "self_crit_pct");
    fx->self_damage_bonus_pct = (short)JsonInt(json, "self_damage_bonus_pct");
    fx->self_damage_pct = (short)JsonInt(json, "self_damage_pct");
    fx->ally_heal_pct = (short)JsonInt(json, "ally_heal_pct");
    fx->ally_heal_chance = (short)JsonInt(json, "ally_heal_chance");
    fx->ally_power_pct = (short)JsonInt(json, "ally_power_pct");
    fx->ally_power_chance = (short)JsonInt(json, "ally_power_chance");
    fx->ally_power_from_actor_pct = (short)JsonInt(json, "ally_power_from_actor_pct");
    fx->ally_slow_amt = (short)JsonInt(json, "ally_slow_amt");
    fx->ally_crit_pct = (short)JsonInt(json, "ally_crit_pct");
    fx->ally_damage_bonus_pct = (short)JsonInt(json, "ally_damage_bonus_pct");
    fx->ally_damage_pct = (short)JsonInt(json, "ally_damage_pct");
    if (fx->self_power_chance == 100) fx->self_power_chance = 0;
    if (fx->self_heal_chance == 100) fx->self_heal_chance = 0;
    if (fx->ally_heal_chance == 100) fx->ally_heal_chance = 0;
    if (fx->ally_power_chance == 100) fx->ally_power_chance = 0;
}

static const AbilityRecipe *SimGeneratedRecipe(const char *key) {
    static AbilityRecipe recipes[SIM_GENERATED_RECIPE_MAX];
    static int recipe_count = -1;

    if (recipe_count < 0) {
        cJSON *root = JsonLoadFile("data/ability_recipes.json");
        cJSON *rows = cJSON_GetObjectItemCaseSensitive(root, "recipes");
        recipe_count = cJSON_IsArray(rows) ? cJSON_GetArraySize(rows) : 0;
        if (JsonInt(root, "version") != 1 || recipe_count <= 0 || recipe_count > SIM_GENERATED_RECIPE_MAX)
            TraceLog(LOG_FATAL, "Sim: invalid generated ability recipe data");
        for (int i = 0; i < recipe_count; i++) {
            cJSON *row = cJSON_GetArrayItem(rows, i);
            AbilityRecipe *recipe = &recipes[i];
            const char *row_key = JsonStr(row, "key");
            if (strlen(row_key) >= sizeof(recipe->key)) TraceLog(LOG_FATAL, "Sim: recipe key too long: %s", row_key);
            StrCopy(recipe->key, sizeof(recipe->key), row_key);
            recipe->generated = true;
            cJSON *steps = cJSON_GetObjectItemCaseSensitive(row, "steps");
            int count = cJSON_IsArray(steps) ? cJSON_GetArraySize(steps) : 0;
            if (count > SIM_RECIPE_MAX_STEPS) TraceLog(LOG_FATAL, "Sim: too many steps for %s", recipe->key);
            recipe->generated_step_count = (unsigned char)count;
            for (int s = 0; s < count; s++) {
                cJSON *step_json = cJSON_GetArrayItem(steps, s);
                RecipeStep *step = &recipe->generated_steps[s];
                step->chance_pct = (short)JsonInt(step_json, "chance_pct");
                step->repeat = (short)JsonInt(step_json, "repeat");
                step->chance_group = (short)JsonInt(step_json, "chance_group");
                step->final_damage_pct = (short)JsonInt(step_json, "final_damage_pct");
                step->stop_on_fail = JsonBool(step_json, "stop_on_fail");
                step->effects_once = JsonBool(step_json, "effects_once");
                SimGeneratedAttackParse(cJSON_GetObjectItemCaseSensitive(step_json, "attack"), &step->attack);
                SimGeneratedEffectsParse(cJSON_GetObjectItemCaseSensitive(step_json, "effects"), step);
            }
        }
        cJSON_Delete(root);
    }

    for (int i = 0; i < recipe_count; i++)
        if (!strcmp(recipes[i].key, key)) return &recipes[i];
    return &SIM_RECIPE_DEFAULT;
}

static const AbilityRecipe *SimRecipe(const Character *ch) {
    const AbilityRecipe *exact = SimExactRecipe(ch);
    if (exact != &SIM_RECIPE_DEFAULT) return exact;

    const AbilityRecipe *generated = SimGeneratedRecipe(ch->ability->key);
    if (generated != &SIM_RECIPE_DEFAULT) return generated;

    static AbilityRecipe inferred;
    memset(&inferred, 0, sizeof(inferred));
    StrCopy(inferred.key, sizeof(inferred.key), ch->ability->key);
    unsigned effects = ch->ability->mechanics;
    if (effects & ABILITY_POWER_DRAIN) inferred.hit.drain_pct = 25;
    if (effects & ABILITY_STUN) inferred.hit.stun_pct = 25;
    if (effects & ABILITY_SLOW) {
        inferred.hit.slow_pct = 100;
        inferred.hit.slow_amt = -50;
    }
    if (effects & ABILITY_CURSE_POWER) inferred.hit.curse_power_pct = 100;
    if (effects & ABILITY_CURSE_HEALTH) inferred.hit.curse_health_pct = 100;
    if (effects & ABILITY_CURSE_TWO_ROUNDS) inferred.hit.curse_turns = 2;
    if (effects & ABILITY_LEVY) inferred.hit.levy_pct = 15;
    if (effects & ABILITY_MARK) inferred.hit.mark_pct = 20;
    if (effects & ABILITY_ASSASSINATE) inferred.hit.assassinate_pct = 5;
    if (effects & ABILITY_HEAL_ALLIES) inferred.fx.ally_heal_pct = 25;
    if (effects & ABILITY_HEAL_SELF) inferred.fx.self_heal_pct = 25;
    if (effects & ABILITY_GAIN_POWER_ALLIES) inferred.fx.ally_power_pct = 25;
    if (effects & ABILITY_GAIN_POWER_SELF) inferred.fx.self_power_pct = 25;
    if (effects & ABILITY_ATTACK_FRIENDLY) inferred.fx.ally_damage_pct = -5;
    if (effects & ABILITY_DAMAGE_SELF) inferred.fx.self_damage_pct = -5;
    if (effects & ABILITY_LOSE_POWER_SELF) inferred.fx.self_power_pct = -10;
    if (effects & ABILITY_SLOW_ALLIES) inferred.fx.ally_slow_amt = -50;
    if (effects & ABILITY_GAIN_CRIT_ALLIES) inferred.fx.ally_crit_pct = 5;
    if (effects & ABILITY_GAIN_DAMAGE_ALLIES) inferred.fx.ally_damage_bonus_pct = 5;
    if (effects & ABILITY_GAIN_DAMAGE_SELF) inferred.fx.self_damage_bonus_pct = 5;
    if (effects & ABILITY_MULTIATTACK) {
        inferred.followup_pct = 25;
        inferred.followup_count = 1;
        inferred.followup.target = RECIPE_TARGET_ABILITY;
    }
    TraceLog(LOG_DEBUG, "Sim: inferred mechanics for %s (%s)", ch->ability->key, ch->id);
    return &inferred;
}

static unsigned SimRand(BattleSim *s) {
    unsigned x = s->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return s->rng = x;
}
static int SimRange(BattleSim *s, int lo, int hi) {
    return lo + (int)(SimRand(s) % (unsigned)(hi - lo + 1));
}
static bool SimChance(BattleSim *s, int pct) {
    return pct > 0 && SimRange(s, 1, 100) <= pct;
}

static unsigned SimFactionMask(Faction f) {
    switch (f) {
        case FACTION_WHITE:
            return 2;
        case FACTION_RED:
            return 4;
        case FACTION_BLUE:
            return 8;
        case FACTION_BLACK:
            return 1;
        case FACTION_WHITE_RED:
            return 2 | 4;
        case FACTION_WHITE_BLUE:
            return 2 | 8;
        case FACTION_RED_BLUE:
            return 4 | 8;
        case FACTION_WHITE_BLACK:
            return 2 | 1;
        case FACTION_RED_BLACK:
            return 4 | 1;
        case FACTION_BLUE_BLACK:
            return 8 | 1;
        case FACTION_ALL:
            return 15;
        default:
            return 0;
    }
}

static bool SimHasAdvantage(unsigned attacker, unsigned target) {
    unsigned ror4 = ((target >> 1) | (target << 3)) & 15u;
    return (attacker & ror4) != 0;
}

static bool SimHasSpec(const Character *ch, Specialty spec) {
    for (int i = 0; i < ch->specialty_count; i++)
        if (ch->specialties[i] == spec) return true;
    return false;
}

static float SimFormationBonus(Rarity r) {
    switch (r) {
        case RARITY_COMMON:
            return 0.01f;
        case RARITY_UNCOMMON:
            return 0.02f;
        case RARITY_RARE:
            return 0.03f;
        case RARITY_EPIC:
            return 0.04f;
        case RARITY_LEGENDARY:
            return 0.05f;
        default:
            return 0.06f;
    }
}

static short SimCutPct(short pct, int cut_pct) {
    return (short)((pct * (100 - cut_pct) + 50) / 100);
}

static void SimCutAttackPotency(AttackRecipe *attack, int cut_pct) {
    attack->stun_pct = SimCutPct(attack->stun_pct, cut_pct);
    attack->slow_pct = SimCutPct(attack->slow_pct, cut_pct);
    attack->curse_power_pct = SimCutPct(attack->curse_power_pct, cut_pct);
    attack->curse_health_pct = SimCutPct(attack->curse_health_pct, cut_pct);
    attack->levy_pct = SimCutPct(attack->levy_pct, cut_pct);
    attack->assassinate_pct = SimCutPct(attack->assassinate_pct, cut_pct);
    attack->mark_pct = SimCutPct(attack->mark_pct, cut_pct);
}

static void SimCutAllyEffectPotency(SecondaryEffects *fx, int cut_pct) {
    fx->ally_heal_pct = SimCutPct(fx->ally_heal_pct, cut_pct);
    fx->ally_power_pct = SimCutPct(fx->ally_power_pct, cut_pct);
    fx->ally_crit_pct = SimCutPct(fx->ally_crit_pct, cut_pct);
    fx->ally_damage_bonus_pct = SimCutPct(fx->ally_damage_bonus_pct, cut_pct);
}

static const AbilityRecipe *SimUnitRecipe(const SimUnit *u) {
    const AbilityRecipe *recipe = SimRecipe(u->ch);
    if (u->potency_cut_pct <= 0) return recipe;
    static AbilityRecipe cut;
    cut = *recipe;
    SimCutAttackPotency(&cut.hit, u->potency_cut_pct);
    SimCutAttackPotency(&cut.effect, u->potency_cut_pct);
    SimCutAttackPotency(&cut.followup, u->potency_cut_pct);
    SimCutAllyEffectPotency(&cut.fx, u->potency_cut_pct);
    for (int step = 0; step < cut.generated_step_count; step++) {
        SimCutAttackPotency(&cut.generated_steps[step].attack, u->potency_cut_pct);
        SimCutAllyEffectPotency(&cut.generated_steps[step].fx, u->potency_cut_pct);
    }
    return &cut;
}

static void SimUnitsInit(SimUnit *units, int side, int slot, const Character *ch, const StatBudget *hero_rules) {
    SimUnit *u = &units[side * SQUAD_SLOT_COUNT + slot];
    memset(u, 0, sizeof(*u));
    if (!ch) return;
    u->ch = ch;
    u->passives = PassiveManifestFind(CharacterDataId(ch));
    u->present = u->alive = true;
    u->side = side;
    u->slot = slot;
    u->power = CharacterStatPower(ch);
    u->max_health = CharacterStatHealth(ch);
    if (hero_rules) {
        StatBudgetApply(hero_rules, ch, &u->power, &u->max_health);
        u->potency_cut_pct = StatBudgetPotencyCutPct(hero_rules, ch);
        u->passive_cut_pct = StatBudgetPassiveCutPct(hero_rules, ch);
    }
    u->start_power = u->power;
    u->crit_pct = (float)(ch->crit_chance > CONSUME_BASE_CRIT_CHANCE ? ch->crit_chance : CONSUME_BASE_CRIT_CHANCE);
    u->init_class = ch->speed == SPEED_QUICK  ? SIM_INIT_CLASS_FAST
                    : ch->speed == SPEED_SLOW ? SIM_INIT_CLASS_SLOW
                                              : SIM_INIT_CLASS_MEDIUM;
    u->faction = ch->giant ? 0 : SimFactionMask(ch->faction);
    if (!ch->giant && SquadSlotIsFront(slot)) u->max_health = (u->max_health * 3 + 1) / 2;
    if (!ch->giant && SquadSlotIsBack(slot)) u->crit_pct *= 1.5f;
    u->health = u->max_health;
}

static void
SimUnitsFormationBonus(SimUnit *units, int side, unsigned *faction, float *bonus_pct, const StatBudget *stat_budget) {
    unsigned mask = 15u;
    float bonus = 0.0f;
    int count = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        SimUnit *u = &units[side * SQUAD_SLOT_COUNT + slot];
        if (!u->present || u->ch->giant) continue;
        mask &= u->faction;
        if (mask == 0) return;
        bonus += stat_budget ? StatBudgetFormationBonus(stat_budget) : SimFormationBonus(u->ch->rarity);
        count++;
    }
    if (count < 4 || mask == 0) return;

    *faction = mask & (~mask + 1u);
    *bonus_pct = roundf(bonus * 1000.0f) / 10.0f;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        SimUnit *u = &units[side * SQUAD_SLOT_COUNT + slot];
        if (!u->present || u->ch->giant) continue;
        u->power = u->start_power = (int)lroundf(u->power * (1.0f + bonus));
    }
}

static bool SimPassiveHasGroup(const Character *ch, const char *group) {
    for (int i = 0; i < ch->group_count; i++)
        if (!strcmp(ch->groups[i], group)) return true;
    return false;
}

static bool SimPassiveTermMatches(const PassiveMatchTerm *term, const Character *ch) {
    switch (term->kind) {
        case PASSIVE_TERM_GROUP:
            return SimPassiveHasGroup(ch, term->value);
        case PASSIVE_TERM_FACTION: {
            unsigned wanted = !strcmp(term->value, "Black")   ? 1u
                              : !strcmp(term->value, "White") ? 2u
                              : !strcmp(term->value, "Red")   ? 4u
                              : !strcmp(term->value, "Blue")  ? 8u
                                                              : 0u;
            return wanted != 0 && (SimFactionMask(ch->faction) & wanted) != 0;
        }
        case PASSIVE_TERM_CHARACTER_ID:
            return !strcmp(ch->id, term->value);
        case PASSIVE_TERM_SPECIALTY: {
            int specialty = SpecialtyFromString(term->value);
            return specialty >= 0 && SimHasSpec(ch, (Specialty)specialty);
        }
        case PASSIVE_TERM_RARITY:
            return (!strcmp(term->value, "Common") && ch->rarity == RARITY_COMMON) ||
                   (!strcmp(term->value, "Uncommon") && ch->rarity == RARITY_UNCOMMON) ||
                   (!strcmp(term->value, "Rare") && ch->rarity == RARITY_RARE) ||
                   (!strcmp(term->value, "Epic") && ch->rarity == RARITY_EPIC) ||
                   (!strcmp(term->value, "Legendary") && ch->rarity == RARITY_LEGENDARY) ||
                   (!strcmp(term->value, "Mythic") && ch->rarity == RARITY_MYTHIC);
        case PASSIVE_TERM_SPEED:
            return (!strcmp(term->value, "Slow") && ch->speed == SPEED_SLOW) ||
                   (!strcmp(term->value, "Normal") && ch->speed == SPEED_NORMAL) ||
                   (!strcmp(term->value, "Quick") && ch->speed == SPEED_QUICK);
        case PASSIVE_TERM_TAG: {
            if (!strcmp(term->value, "GIANT")) return ch->giant;
            char star_tag[16];
            snprintf(star_tag, sizeof(star_tag), "%dSTAR", ch->stars);
            return !strcmp(term->value, star_tag);
        }
        default:
            return false;
    }
}

static int SimPassiveFilterMatchCount(const PassiveFilter *filter, const SimUnit *source, const SimUnit *candidate) {
    if (!source->present || !candidate->present) return 0;
    if (filter->scope == PASSIVE_SCOPE_SELF) {
        if (source != candidate) return 0;
    } else if (filter->scope == PASSIVE_SCOPE_ALLIES) {
        if (source->side != candidate->side) return 0;
        if (!filter->include_source && source == candidate) return 0;
    } else if (source->side == candidate->side) {
        return 0;
    }

    if (!filter->has_match) return 1;
    int matches = 0;
    for (int i = 0; i < filter->term_count; i++)
        if (SimPassiveTermMatches(&filter->terms[i], candidate->ch)) matches++;
    if (filter->mode == PASSIVE_MATCH_EACH) return matches;
    return matches > 0 ? 1 : 0;
}

static bool SimGearWearerQualifies(const GearEffect *effect, const SimUnit *wearer) {
    return SimPassiveFilterMatchCount(&effect->wearer, wearer, wearer) > 0;
}

static int SimGearVersusCount(const PassiveFilter *filter, const SimUnit *candidate) {
    if (!filter->has_match) return 1;
    int matches = 0;
    for (int i = 0; i < filter->term_count; i++)
        if (SimPassiveTermMatches(&filter->terms[i], candidate->ch)) matches++;
    if (filter->mode == PASSIVE_MATCH_EACH) return matches;
    return matches > 0 ? 1 : 0;
}

static void SimGearApplyStat(SimGearState *gear, GearStat stat, int value) {
    switch (stat) {
        case GEAR_STAT_DAMAGE_RESIST:
            gear->damage_resist_pct += value;
            break;
        case GEAR_STAT_RAW_DAMAGE_RESIST:
            gear->raw_damage_resist += value;
            break;
        case GEAR_STAT_STUN_RESIST:
            gear->stun_resist_pct += value;
            break;
        case GEAR_STAT_SLOW_RESIST:
            gear->slow_resist_pct += value;
            break;
        case GEAR_STAT_DRAIN_RESIST:
            gear->drain_resist_pct += value;
            break;
        case GEAR_STAT_CURSE_POWER_RESIST:
            gear->curse_power_resist_pct += value;
            break;
        case GEAR_STAT_CURSE_HEALTH_RESIST:
            gear->curse_health_resist_pct += value;
            break;
        case GEAR_STAT_DEFENDER:
            gear->defender_pct = value > gear->defender_pct ? value : gear->defender_pct;
            break;
        case GEAR_STAT_STEALTH:
            gear->stealth = true;
            break;
        case GEAR_STAT_PERCEPTIVE:
            gear->perceptive = true;
            break;
        case GEAR_STAT_HASTE:
            gear->haste = true;
            break;
        case GEAR_STAT_FLANK_REDUCTION:
            gear->flank_reduction = true;
            break;
        case GEAR_STAT_TARGET_HIGHEST_POWER:
            gear->target_highest_power = true;
            break;
        case GEAR_STAT_TARGET_LOWEST_HEALTH:
            gear->target_lowest_health = true;
            break;
        case GEAR_STAT_TARGET_BACK_ROW:
            gear->target_back_row = true;
            break;
        case GEAR_STAT_TARGET_QUICK:
            gear->target_quick = true;
            break;
        case GEAR_STAT_TARGET_SLOWEST:
            gear->target_slowest = true;
            break;
        default:
            break;
    }
}

typedef struct {
    int granted, asked;
} SimGearShare;

static const SimGearShare SIM_GEAR_FULL_SHARE = {1, 1};

static bool SimGearStatIsPooled(GearStat stat) {
    return stat == GEAR_STAT_POWER || stat == GEAR_STAT_HEALTH || stat == GEAR_STAT_CRIT ||
           stat == GEAR_STAT_BONUS_DAMAGE || stat == GEAR_STAT_DAMAGE_RESIST || stat == GEAR_STAT_RAW_DAMAGE_RESIST;
}

static int SimGearPooledPoints(const StatBudget *stat_budget, GearStat stat, int value) {
    int size = value < 0 ? -value : value;
    return stat == GEAR_STAT_RAW_DAMAGE_RESIST ? StatBudgetFlatResistPct(stat_budget, size) : size;
}

static int SimGearShared(SimGearShare share, int value) {
    return (int)((long long)value * share.granted / share.asked);
}

static float SimGearSharedPct(SimGearShare share, int value) {
    if (share.granted == share.asked) return (float)value;
    return (float)((long long)value * share.granted) / (float)share.asked;
}

static bool SimGearEffectIsAura(const GearEffect *effect, const SimUnit *wearer) {
    return effect->on == GEAR_ON_BATTLE && !effect->versus.has_match && effect->stat != GEAR_STAT_TARGET_MATCH &&
           SimGearWearerQualifies(effect, wearer);
}

static SimGearShare
SimGearAuraShare(const SimUnit *units, const SimUnit *wearer, const GearDef *def, const StatBudget *stat_budget) {
    if (!stat_budget) return SIM_GEAR_FULL_SHARE;
    int asked = 0;
    for (int e = 0; e < def->effect_count; e++) {
        const GearEffect *effect = &def->effects[e];
        if (!SimGearEffectIsAura(effect, wearer) || !SimGearStatIsPooled(effect->stat)) continue;
        for (int recipient_idx = 0; recipient_idx < SIM_UNIT_COUNT; recipient_idx++) {
            int multiplier = SimPassiveFilterMatchCount(&effect->recipient, wearer, &units[recipient_idx]);
            if (multiplier > 0) asked += SimGearPooledPoints(stat_budget, effect->stat, effect->value * multiplier);
        }
    }
    int cap = StatBudgetGearItemCap(stat_budget);
    return asked > cap ? (SimGearShare){cap, asked} : SIM_GEAR_FULL_SHARE;
}

static void SimUnitsGearAuras(
    SimUnit *units, float health_pct[SIM_UNIT_COUNT], float power_pct[SIM_UNIT_COUNT], const StatBudget *stat_budget
) {
    for (int wearer_idx = 0; wearer_idx < SIM_UNIT_COUNT; wearer_idx++) {
        SimUnit *wearer = &units[wearer_idx];
        if (!wearer->present) continue;
        for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++) {
            const GearDef *def = GearInSlot(wearer->ch, slot);
            if (!def) continue;
            SimGearShare share = SimGearAuraShare(units, wearer, def, stat_budget);
            for (int e = 0; e < def->effect_count; e++) {
                const GearEffect *effect = &def->effects[e];
                if (!SimGearEffectIsAura(effect, wearer)) continue;
                for (int recipient_idx = 0; recipient_idx < SIM_UNIT_COUNT; recipient_idx++) {
                    SimUnit *recipient = &units[recipient_idx];
                    int multiplier = SimPassiveFilterMatchCount(&effect->recipient, wearer, recipient);
                    if (multiplier <= 0) continue;
                    int value = effect->value * multiplier;
                    if (effect->stat == GEAR_STAT_POWER)
                        power_pct[recipient_idx] += SimGearSharedPct(share, value);
                    else if (effect->stat == GEAR_STAT_HEALTH)
                        health_pct[recipient_idx] += SimGearSharedPct(share, value);
                    else if (effect->stat == GEAR_STAT_CRIT)
                        recipient->crit_pct += SimGearSharedPct(share, value);
                    else if (effect->stat == GEAR_STAT_BONUS_DAMAGE)
                        recipient->damage_bonus_pct += SimGearSharedPct(share, value);
                    else if (SimGearStatIsPooled(effect->stat))
                        SimGearApplyStat(&recipient->gear, effect->stat, SimGearShared(share, value));
                    else
                        SimGearApplyStat(&recipient->gear, effect->stat, value);
                }
            }
        }
    }
}

static int SimGearVersusStat(const BattleSim *s, const SimUnit *recipient, const SimUnit *other, GearStat stat) {
    int total = 0;
    for (int wearer_idx = 0; wearer_idx < SIM_UNIT_COUNT; wearer_idx++) {
        const SimUnit *wearer = &s->units[wearer_idx];
        if (!wearer->present) continue;
        for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++) {
            const GearDef *def = GearInSlot(wearer->ch, slot);
            for (int e = 0; def && e < def->effect_count; e++) {
                const GearEffect *effect = &def->effects[e];
                if (effect->stat != stat || effect->on != GEAR_ON_BATTLE || !effect->versus.has_match ||
                    !SimGearWearerQualifies(effect, wearer))
                    continue;
                total += effect->value * SimPassiveFilterMatchCount(&effect->recipient, wearer, recipient) *
                         SimGearVersusCount(&effect->versus, other);
            }
        }
    }
    return total;
}

static bool SimGearPrefersMatchedTarget(const SimUnit *attacker, const SimUnit *target) {
    for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++) {
        const GearDef *def = GearInSlot(attacker->ch, slot);
        for (int e = 0; def && e < def->effect_count; e++) {
            const GearEffect *effect = &def->effects[e];
            if (effect->stat == GEAR_STAT_TARGET_MATCH && SimGearWearerQualifies(effect, attacker) &&
                SimGearVersusCount(&effect->recipient, target) > 0)
                return true;
        }
    }
    return false;
}

static float SimPassiveValuePct(const SimUnit *source, const PassiveEffect *effect) {
    if (source->passive_cut_pct <= 0) return effect->value_pct;
    return effect->value_pct * (float)(100 - source->passive_cut_pct) / 100.0f;
}

static void SimUnitsBattleStartPassives(SimUnit *units, const StatBudget *stat_budget) {
    float gear_health_pct[SIM_UNIT_COUNT] = {0};
    float gear_power_pct[SIM_UNIT_COUNT] = {0};
    float health_pct[SIM_UNIT_COUNT] = {0};
    float power_pct[SIM_UNIT_COUNT] = {0};
    SimUnitsGearAuras(units, gear_health_pct, gear_power_pct, stat_budget);
    for (int i = 0; i < SIM_UNIT_COUNT && !stat_budget; i++) {
        health_pct[i] = gear_health_pct[i];
        power_pct[i] = gear_power_pct[i];
    }

    for (int source_idx = 0; source_idx < SIM_UNIT_COUNT; source_idx++) {
        SimUnit *source = &units[source_idx];
        if (!source->present || !source->passives) continue;
        for (int e = 0; e < source->passives->effect_count; e++) {
            const PassiveEffect *effect = &source->passives->effects[e];
            bool counted = effect->kind == PASSIVE_STAT_PERCENT_PER_MATCHING_ALLY;
            if ((!counted && effect->kind != PASSIVE_STAT_PERCENT) || effect->trigger != PASSIVE_TRIGGER_BATTLE_START)
                continue;
            int composition_count = 1;
            if (counted) {
                composition_count = 0;
                for (int candidate_idx = 0; candidate_idx < SIM_UNIT_COUNT; candidate_idx++)
                    composition_count += SimPassiveFilterMatchCount(&effect->count, source, &units[candidate_idx]);
                if (composition_count <= 0) continue;
            }
            for (int recipient_idx = 0; recipient_idx < SIM_UNIT_COUNT; recipient_idx++) {
                SimUnit *recipient = &units[recipient_idx];
                int multiplier = SimPassiveFilterMatchCount(&effect->recipient, source, recipient);
                if (multiplier <= 0) continue;
                float value = SimPassiveValuePct(source, effect) * multiplier * composition_count;
                if (effect->stats & PASSIVE_STAT_HEALTH) health_pct[recipient_idx] += value;
                if (effect->stats & PASSIVE_STAT_POWER) power_pct[recipient_idx] += value;
            }
        }
    }

    for (int i = 0; i < SIM_UNIT_COUNT; i++) {
        SimUnit *unit = &units[i];
        if (!unit->present) continue;
        if (stat_budget) {
            health_pct[i] = StatBudgetCappedAuraPct(stat_budget, health_pct[i]) +
                            StatBudgetCappedGearPct(stat_budget, gear_health_pct[i]);
            power_pct[i] = StatBudgetCappedAuraPct(stat_budget, power_pct[i]) +
                           StatBudgetCappedGearPct(stat_budget, gear_power_pct[i]);
        }
        if (health_pct[i] != 0.0f) {
            unit->max_health = (int)lroundf(unit->max_health * (1.0f + health_pct[i] / 100.0f));
            if (unit->max_health < 1) unit->max_health = 1;
        }
        if (power_pct[i] != 0.0f) {
            unit->power = (int)lroundf(unit->power * (1.0f + power_pct[i] / 100.0f));
            if (unit->power < 0) unit->power = 0;
        }
        unit->health = unit->max_health;
        unit->start_power = unit->power;
    }
}

static float SimPassiveIncomingTargetMultiplier(const BattleSim *s, const SimUnit *target, bool *found) {
    float multiplier = 1.0f;
    *found = false;
    for (int source_idx = 0; source_idx < SIM_UNIT_COUNT; source_idx++) {
        const SimUnit *source = &s->units[source_idx];
        if (!source->present || !source->passives) continue;
        for (int e = 0; e < source->passives->effect_count; e++) {
            const PassiveEffect *effect = &source->passives->effects[e];
            if (effect->kind != PASSIVE_INCOMING_TARGET_WEIGHT || effect->value_multiplier <= 0.0f) continue;
            int count = SimPassiveFilterMatchCount(&effect->recipient, source, target);
            if (count <= 0) continue;
            *found = true;
            multiplier *= powf(effect->value_multiplier, (float)count);
        }
    }
    return multiplier;
}

static bool SimPassiveLowestHealthTarget(
    const BattleSim *s, const SimUnit *source, const PassiveEffect *effect, const SimUnit *target
) {
    int target_count = SimPassiveFilterMatchCount(&effect->recipient, source, target);
    if (target_count <= 0 || target->max_health <= 0) return false;
    for (int i = 0; i < SIM_UNIT_COUNT; i++) {
        const SimUnit *other = &s->units[i];
        if (!other->alive || other->max_health <= 0 ||
            SimPassiveFilterMatchCount(&effect->recipient, source, other) <= 0)
            continue;
        if ((long long)other->health * target->max_health < (long long)target->health * other->max_health) return false;
    }
    return true;
}

static float SimPassiveOutgoingTargetMultiplier(const BattleSim *s, const SimUnit *attacker, const SimUnit *target) {
    if (!attacker->passives) return 1.0f;
    float multiplier = 1.0f;
    for (int e = 0; e < attacker->passives->effect_count; e++) {
        const PassiveEffect *effect = &attacker->passives->effects[e];
        if (effect->kind != PASSIVE_OUTGOING_TARGET_PREFERENCE || effect->value_multiplier <= 0.0f) continue;
        if (effect->selector == PASSIVE_SELECTOR_LOWEST_HEALTH_PERCENT &&
            SimPassiveLowestHealthTarget(s, attacker, effect, target))
            multiplier *= effect->value_multiplier;
    }
    return multiplier;
}

static float SimPassiveBonusDamagePct(const BattleSim *s, const SimUnit *attacker, const SimUnit *target) {
    float bonus = 0.0f;
    for (int source_idx = 0; source_idx < SIM_UNIT_COUNT; source_idx++) {
        const SimUnit *source = &s->units[source_idx];
        if (!source->present || !source->passives) continue;
        for (int e = 0; e < source->passives->effect_count; e++) {
            const PassiveEffect *effect = &source->passives->effects[e];
            if (effect->kind != PASSIVE_BONUS_DAMAGE_PERCENT || effect->trigger != PASSIVE_TRIGGER_BEFORE_DAMAGE)
                continue;
            int recipients = SimPassiveFilterMatchCount(&effect->recipient, source, attacker);
            if (recipients <= 0) continue;
            int targets = effect->has_target ? SimPassiveFilterMatchCount(&effect->target, source, target) : 1;
            if (targets > 0) bonus += SimPassiveValuePct(source, effect) * recipients * targets;
        }
    }
    return bonus;
}

static bool SimRuneTargetBeats(RuneTargetPreference preference, const SimUnit *a, const SimUnit *b) {
    switch (preference) {
        case RUNE_TARGET_HIGHEST_POWER:
            return a->power > b->power;
        case RUNE_TARGET_HIGHEST_SPEED:
            return a->init_class > b->init_class;
        case RUNE_TARGET_LOWEST_SPEED:
            return a->init_class < b->init_class;
        case RUNE_TARGET_LOWEST_HEALTH:
            return (long long)a->health * b->max_health < (long long)b->health * a->max_health;
        default:
            return false;
    }
}

static bool SimRuneTargetPreferred(const BattleSim *s, RuneTargetPreference preference, const SimUnit *t) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const SimUnit *other = &s->units[t->side * SQUAD_SLOT_COUNT + slot];
        if (other->alive && SimRuneTargetBeats(preference, other, t)) return false;
    }
    return true;
}

static float SimGearTargetMultiplier(const BattleSim *s, const SimUnit *attacker, const SimUnit *t) {
    const SimGearState *gear = &attacker->gear;
    float multiplier = 1.0f;
    if (gear->target_highest_power && SimRuneTargetPreferred(s, RUNE_TARGET_HIGHEST_POWER, t))
        multiplier *= SIM_GEAR_TARGET_WEIGHT;
    if (gear->target_lowest_health && SimRuneTargetPreferred(s, RUNE_TARGET_LOWEST_HEALTH, t))
        multiplier *= SIM_GEAR_TARGET_WEIGHT;
    if (gear->target_slowest && SimRuneTargetPreferred(s, RUNE_TARGET_LOWEST_SPEED, t))
        multiplier *= SIM_GEAR_TARGET_WEIGHT;
    if (gear->target_quick && t->ch->speed == SPEED_QUICK) multiplier *= SIM_GEAR_STRONG_TARGET_WEIGHT;
    if (SimGearPrefersMatchedTarget(attacker, t)) multiplier *= SIM_GEAR_TARGET_WEIGHT;
    return multiplier;
}

static float SimFlankWeight(const SimUnit *attacker, const SimUnit *t) {
    bool flanks = SimHasSpec(attacker->ch, SPEC_FLANK) || attacker->gear.target_back_row;
    if (!flanks || t->ch->giant || !SquadSlotIsBack(t->slot)) return 1.0f;
    return t->gear.flank_reduction ? SIM_GEAR_FLANK_WEIGHT * SIM_GEAR_FLANK_REDUCTION : SIM_GEAR_FLANK_WEIGHT;
}

static float SimRuneTargetMultiplier(const BattleSim *s, const SimUnit *attacker, const SimUnit *t) {
    const RuneBonus *bonus = &s->rune_bonus[attacker->side];
    float multiplier = 1.0f;
    for (int preference = 0; preference < RUNE_TARGET_COUNT; preference++) {
        int level = bonus->target_level[preference];
        if (level > 0 && SimRuneTargetPreferred(s, (RuneTargetPreference)preference, t))
            multiplier *= 1.0f + level * SIM_RUNE_TARGET_WEIGHT_PER_LEVEL;
    }
    return multiplier;
}

static float SimTargetWeight(const BattleSim *s, const SimUnit *attacker, const SimUnit *t) {
    float w = 1.0f;
    bool explicit_incoming = false;
    float incoming = SimPassiveIncomingTargetMultiplier(s, t, &explicit_incoming);
    if (explicit_incoming)
        w *= incoming;
    else {
        if (SimHasSpec(t->ch, SPEC_TAUNT)) w *= 2.0f;
        if (SimHasSpec(t->ch, SPEC_ELUSIVE)) w *= 0.5f;
    }
    w *= SimFlankWeight(attacker, t);
    if (t->marked > 0) w *= 4.0f;
    w *= SimPassiveOutgoingTargetMultiplier(s, attacker, t);
    w *= SimRuneTargetMultiplier(s, attacker, t);
    w *= SimGearTargetMultiplier(s, attacker, t);
    return w;
}

static int SimPickWeighted(BattleSim *s, const SimUnit *attacker, int enemy_side, bool include_giant) {
    float weights[SQUAD_SLOT_COUNT], total = 0.0f;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        const SimUnit *t = &s->units[enemy_side * SQUAD_SLOT_COUNT + slot];
        weights[slot] = 0.0f;
        if (!t->alive) continue;
        if (slot == SQUAD_SLOT_LARGE && !include_giant) continue;
        weights[slot] = SimTargetWeight(s, attacker, t);
        total += weights[slot];
    }
    if (total <= 0.0f) return -1;
    float roll = (SimRand(s) % 10000u) / 10000.0f * total;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        roll -= weights[slot];
        if (weights[slot] > 0.0f && roll <= 0.0f) return enemy_side * SQUAD_SLOT_COUNT + slot;
    }
    for (int slot = SQUAD_SLOT_COUNT - 1; slot >= 0; slot--)
        if (weights[slot] > 0.0f) return enemy_side * SQUAD_SLOT_COUNT + slot;
    return -1;
}

static int SimSelectTargets(BattleSim *s, const SimUnit *attacker, TargetPattern shape, int out[SIM_MAX_TARGETS]) {
    int enemy_side = 1 - attacker->side;
    int n = 0;
    if (shape == TARGET_ALL) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            SimUnit *t = &s->units[enemy_side * SQUAD_SLOT_COUNT + slot];
            if (t->alive) out[n++] = enemy_side * SQUAD_SLOT_COUNT + slot;
        }
        return n;
    }
    if (shape == TARGET_ONE) {
        int pick = SimPickWeighted(s, attacker, enemy_side, true);
        if (pick >= 0) out[n++] = pick;
        return n;
    }
    int pick = SimPickWeighted(s, attacker, enemy_side, false);
    if (pick < 0) {
        pick = SimPickWeighted(s, attacker, enemy_side, true);
        if (pick >= 0) out[n++] = pick;
        return n;
    }
    int slot = pick % SQUAD_SLOT_COUNT;
    int coordinate = shape == TARGET_ROW ? SquadSlotReferenceX(slot) : SquadSlotReferenceY(slot);
    for (int candidate = 0; candidate < SQUAD_SLOT_LARGE; candidate++) {
        int candidate_coordinate =
            shape == TARGET_ROW ? SquadSlotReferenceX(candidate) : SquadSlotReferenceY(candidate);
        if (candidate_coordinate != coordinate) continue;
        if (s->units[enemy_side * SQUAD_SLOT_COUNT + candidate].alive)
            out[n++] = enemy_side * SQUAD_SLOT_COUNT + candidate;
    }
    return n;
}

static BattleSimAction *SimEmit(BattleSim *s, int actor) {
    if (s->action_count >= SIM_MAX_ACTIONS) {
        TraceLog(LOG_FATAL, "Sim: action timeline exceeds %d records", SIM_MAX_ACTIONS);
        return NULL;
    }
    BattleSimAction *a = &s->actions[s->action_count++];
    memset(a, 0, sizeof(*a));
    a->actor = actor;
    return a;
}

static void SimApplyEnemyDeathPassives(BattleSim *s, BattleSimAction *action, int dead_idx);

static void SimApplyTarget(BattleSim *s, BattleSimAction *a, const BattleSimTarget *t) {
    if (!a || a->target_count >= SIM_MAX_TARGETS) return;
    SimUnit *u = &s->units[t->unit];
    BattleSimTarget applied = *t;
    if (applied.power_delta < -u->power) applied.power_delta = -u->power;
    a->targets[a->target_count++] = applied;

    bool was_alive = u->alive;
    u->health += applied.health_delta + applied.health_faction_delta;
    if (u->health > u->max_health) u->health = u->max_health;
    u->power += applied.power_delta;
    u->cur_initiative += applied.initiative_delta;
    u->crit_pct += applied.crit_delta;
    u->damage_bonus_pct += applied.damage_bonus_delta;
    if (u->health <= 0) {
        u->health = 0;
        u->alive = false;
        u->stunned = false;
        u->curse_power = u->curse_health = false;
        u->curse_power_turns = u->curse_health_turns = 0;
        if (was_alive) {
            s->gear_pending_deaths |= 1u << t->unit;
            SimApplyEnemyDeathPassives(s, a, t->unit);
        }
    }
}

static bool SimRecordPassiveHeal(BattleSim *s, BattleSimAction *action, int unit_idx, int requested) {
    SimUnit *unit = &s->units[unit_idx];
    if (!unit->alive || unit->curse_health || requested <= 0 || unit->health >= unit->max_health) return false;
    int applied = requested;
    if (applied > unit->max_health - unit->health) applied = unit->max_health - unit->health;
    for (int i = 0; i < action->target_count; i++) {
        if (action->targets[i].unit != unit_idx) continue;
        action->targets[i].health_delta += applied;
        unit->health += applied;
        return true;
    }
    if (action->target_count >= SIM_MAX_TARGETS) {
        TraceLog(LOG_WARNING, "Sim: no target slot for enemy-death passive heal");
        return false;
    }
    action->targets[action->target_count++] = (BattleSimTarget){.unit = unit_idx, .health_delta = applied};
    unit->health += applied;
    return true;
}

static void SimApplyEnemyDeathPassives(BattleSim *s, BattleSimAction *action, int dead_idx) {
    SimUnit *dead = &s->units[dead_idx];
    for (int source_idx = 0; source_idx < SIM_UNIT_COUNT; source_idx++) {
        SimUnit *source = &s->units[source_idx];
        if (!source->alive || source->side == dead->side || !source->passives) continue;
        for (int e = 0; e < source->passives->effect_count; e++) {
            const PassiveEffect *effect = &source->passives->effects[e];
            if (effect->kind != PASSIVE_HEAL_PERCENT_MAX_HEALTH || effect->trigger != PASSIVE_TRIGGER_ENEMY_DEATH)
                continue;
            int target_multiplier = effect->has_target ? SimPassiveFilterMatchCount(&effect->target, source, dead) : 1;
            if (target_multiplier <= 0) continue;
            for (int recipient_idx = 0; recipient_idx < SIM_UNIT_COUNT; recipient_idx++) {
                SimUnit *recipient = &s->units[recipient_idx];
                int recipient_multiplier = SimPassiveFilterMatchCount(&effect->recipient, source, recipient);
                if (recipient_multiplier <= 0) continue;
                int amount = (int)lroundf(
                    recipient->max_health *
                    (SimPassiveValuePct(source, effect) * recipient_multiplier * target_multiplier / 100.0f)
                );
                SimRecordPassiveHeal(s, action, recipient_idx, amount);
            }
        }
    }
}

static bool SimTeamDead(BattleSim *s, int side) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        if (s->units[side * SQUAD_SLOT_COUNT + slot].alive) return false;
    return true;
}

static float SimTeamHealthFrac(BattleSim *s, int side) {
    long cur = 0, max = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        SimUnit *u = &s->units[side * SQUAD_SLOT_COUNT + slot];
        if (!u->present) continue;
        cur += u->health;
        max += u->max_health;
    }
    return max > 0 ? (float)cur / (float)max : 0.0f;
}

static TargetPattern SimRecipeTarget(const SimUnit *actor, RecipeTarget target) {
    return target == RECIPE_TARGET_ABILITY ? actor->ch->ability->target : (TargetPattern)(target - 1);
}

static int SimPickAlly(BattleSim *s, const SimUnit *actor) {
    int choices[SQUAD_SLOT_COUNT], count = 0;
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        int idx = actor->side * SQUAD_SLOT_COUNT + slot;
        if (slot != actor->slot && s->units[idx].alive) choices[count++] = idx;
    }
    return count > 0 ? choices[SimRange(s, 0, count - 1)] : -1;
}

static bool SimHasHostileEffect(const AttackRecipe *hit) {
    return hit->stun_pct || hit->slow_pct || hit->drain_pct || hit->curse_power_pct || hit->curse_health_pct ||
           hit->assassinate_pct || hit->mark_pct;
}

static void SimApplyPerHitSelfEffects(BattleSim *s, int actor_idx, const AttackRecipe *hit) {
    if (!hit->self_heal_pct && !hit->self_crit_pct) return;
    SimUnit *actor = &s->units[actor_idx];
    BattleSimTarget st = {.unit = actor_idx};
    if (hit->self_heal_pct && !actor->curse_health)
        st.health_delta = (int)lroundf(actor->max_health * (hit->self_heal_pct / 100.0f));
    st.crit_delta = hit->self_crit_pct;
    if (!st.health_delta && !st.crit_delta) return;
    BattleSimAction *a = SimEmit(s, actor_idx);
    if (!a) return;
    a->replay_attack = false;
    SimApplyTarget(s, a, &st);
}

static bool SimRuneResists(BattleSim *s, int resist_pct) {
    return resist_pct > 0 && SimChance(s, resist_pct);
}

static bool SimEffectLands(BattleSim *s, int chance_pct, int resist_pct) {
    if (chance_pct <= 0) return false;
    if (resist_pct < 0) chance_pct -= resist_pct;
    return SimChance(s, chance_pct) && !SimRuneResists(s, resist_pct);
}

static int SimDamageAfterResist(int damage, int shield, int resist) {
    if (damage <= 0 || (!shield && !resist)) return damage;
    int mitigated = damage - shield;
    if (resist > RUNE_RESIST_MAX_PCT) resist = RUNE_RESIST_MAX_PCT;
    if (resist != 0) mitigated = (int)lroundf(mitigated * ((100 - resist) / 100.0f));
    return mitigated > SIM_RUNE_SHIELDED_HIT_MIN_DAMAGE ? mitigated : SIM_RUNE_SHIELDED_HIT_MIN_DAMAGE;
}

static int SimMitigatedDamage(const BattleSim *s, const SimUnit *actor, const SimUnit *t, int damage) {
    const RuneBonus *bonus = &s->rune_bonus[t->side];
    int shield =
        bonus->damage_shield + t->gear.raw_damage_resist + SimGearVersusStat(s, t, actor, GEAR_STAT_RAW_DAMAGE_RESIST);
    int resist =
        bonus->damage_resist_pct + t->gear.damage_resist_pct + SimGearVersusStat(s, t, actor, GEAR_STAT_DAMAGE_RESIST);
    if (s->stat_budget) {
        resist += StatBudgetFlatResistPct(s->stat_budget, shield);
        shield = 0;
    }
    return SimDamageAfterResist(damage, shield, resist);
}

static bool SimHitIsHidden(const BattleSim *s, const SimUnit *actor, const SimUnit *t) {
    bool stealth = s->rune_bonus[actor->side].stealth || actor->gear.stealth;
    bool perceptive = s->rune_bonus[t->side].perceptive || t->gear.perceptive;
    return stealth && !perceptive;
}

static bool SimRuneRetaliates(const BattleSim *s, const SimUnit *actor, const SimUnit *t) {
    if (t->side == actor->side) return false;
    return s->rune_bonus[t->side].retaliation_pct > 0 && !SimHitIsHidden(s, actor, t);
}

static void SimApplyRuneRetaliation(BattleSim *s, int actor_idx, int reflected) {
    if (reflected <= 0 || !s->units[actor_idx].alive) return;
    BattleSimAction *a = SimEmit(s, actor_idx);
    if (!a) return;
    a->replay_attack = false;
    SimApplyTarget(s, a, &(BattleSimTarget){.unit = actor_idx, .health_delta = -reflected});
}

typedef struct {
    BattleSimTarget targets[SIM_UNIT_COUNT];
    bool touched[SIM_UNIT_COUNT];
} SimGearReaction;

static bool SimGearStatIsMetered(GearStat stat) {
    return stat == GEAR_STAT_DAMAGE || stat == GEAR_STAT_DRAIN || SimGearStatIsPooled(stat);
}

static int SimGearTriggerAmount(BattleSim *s, const GearEffect *effect, const SimUnit *recipient, int amount) {
    int base = 0;
    float pct = effect->value / 100.0f;
    switch (effect->stat) {
        case GEAR_STAT_POWER:
            base = effect->basis == GEAR_BASIS_DAMAGE ? amount : recipient->start_power;
            return recipient->curse_power ? 0 : (int)lroundf(base * pct);
        case GEAR_STAT_HEALTH:
            base = effect->basis == GEAR_BASIS_DAMAGE ? amount : recipient->max_health;
            return recipient->curse_health ? 0 : (int)lroundf(base * pct);
        case GEAR_STAT_DAMAGE:
            base = effect->basis == GEAR_BASIS_DAMAGE ? amount : recipient->max_health;
            return (int)lroundf(base * pct);
        case GEAR_STAT_DRAIN:
            if (SimRuneResists(s, s->rune_bonus[recipient->side].drain_resist_pct + recipient->gear.drain_resist_pct))
                return 0;
            return (int)lroundf(recipient->start_power * pct);
        default:
            return effect->value;
    }
}

static void
SimGearReactionAdd(BattleSim *s, SimGearReaction *reaction, const GearEffect *effect, SimUnit *recipient, int amount) {
    int idx = (int)(recipient - s->units);
    BattleSimTarget *t = &reaction->targets[idx];
    t->unit = idx;
    switch (effect->stat) {
        case GEAR_STAT_POWER:
            t->power_delta += amount;
            break;
        case GEAR_STAT_HEALTH:
            t->health_delta += amount;
            break;
        case GEAR_STAT_DAMAGE:
            t->health_delta -= amount;
            break;
        case GEAR_STAT_DRAIN:
            t->power_delta -= amount;
            break;
        case GEAR_STAT_CRIT:
            t->crit_delta += (float)amount;
            break;
        case GEAR_STAT_BONUS_DAMAGE:
            t->damage_bonus_delta += (float)amount;
            break;
        default:
            SimGearApplyStat(&recipient->gear, effect->stat, amount);
            return;
    }
    reaction->touched[idx] = true;
}

static long long SimGearTriggerWorth(BattleSim *s, const GearEffect *effect, const SimUnit *recipient, int amount) {
    switch (effect->stat) {
        case GEAR_STAT_POWER:
        case GEAR_STAT_DRAIN:
            return StatBudgetShareOfStat(amount, recipient->start_power);
        case GEAR_STAT_HEALTH:
        case GEAR_STAT_DAMAGE:
            return StatBudgetShareOfStat(amount, recipient->max_health);
        default:
            return StatBudgetWorthOfPoints(SimGearPooledPoints(s->stat_budget, effect->stat, amount));
    }
}

static int SimGearMissingHealth(const SimGearReaction *reaction, const SimUnit *recipient, int idx) {
    int missing = recipient->max_health - recipient->health - reaction->targets[idx].health_delta;
    return missing > 0 ? missing : 0;
}

static void SimGearMeterTrigger(
    BattleSim *s,
    const SimGearReaction *reaction,
    long long *spent,
    const GearEffect *effect,
    int amounts[SIM_UNIT_COUNT]
) {
    long long asked = 0;
    for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
        const SimUnit *recipient = &s->units[idx];
        if (effect->stat == GEAR_STAT_HEALTH && amounts[idx] > SimGearMissingHealth(reaction, recipient, idx))
            amounts[idx] = SimGearMissingHealth(reaction, recipient, idx);
        asked += SimGearTriggerWorth(s, effect, recipient, amounts[idx]);
    }
    long long left = StatBudgetGearTriggerAllowance(s->stat_budget) - *spent;
    if (asked <= left) {
        *spent += asked;
        return;
    }
    for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) amounts[idx] = (int)(amounts[idx] * left / asked);
    *spent += left;
}

static void SimGearFireEffect(
    BattleSim *s, SimGearReaction *reaction, SimUnit *wearer, int slot, const GearEffect *effect, int amount
) {
    if (!SimGearWearerQualifies(effect, wearer)) return;
    bool metered = SimGearStatIsMetered(effect->stat);
    int amounts[SIM_UNIT_COUNT] = {0};
    bool reached[SIM_UNIT_COUNT] = {false};
    for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
        SimUnit *recipient = &s->units[idx];
        if (!recipient->alive) continue;
        int multiplier = SimPassiveFilterMatchCount(&effect->recipient, wearer, recipient);
        for (int m = 0; m < multiplier; m++) {
            int fired = SimGearTriggerAmount(s, effect, recipient, amount);
            if (metered)
                amounts[idx] += fired;
            else
                SimGearReactionAdd(s, reaction, effect, recipient, fired);
        }
        reached[idx] = metered && multiplier > 0;
    }
    if (!metered) return;
    if (s->stat_budget) SimGearMeterTrigger(s, reaction, &wearer->gear_trigger_worth[slot], effect, amounts);
    for (int idx = 0; idx < SIM_UNIT_COUNT; idx++)
        if (reached[idx]) SimGearReactionAdd(s, reaction, effect, &s->units[idx], amounts[idx]);
}

static void SimGearFireTrigger(BattleSim *s, SimGearReaction *reaction, SimUnit *wearer, GearTrigger on, int amount) {
    for (int slot = 0; slot < GEAR_SLOT_CAPACITY; slot++) {
        const GearDef *def = GearInSlot(wearer->ch, slot);
        for (int e = 0; def && e < def->effect_count; e++)
            if (def->effects[e].on == on) SimGearFireEffect(s, reaction, wearer, slot, &def->effects[e], amount);
    }
}

static void SimGearEmitReaction(BattleSim *s, int actor_idx, const SimGearReaction *reaction) {
    BattleSimAction *a = NULL;
    for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
        const BattleSimTarget *t = &reaction->targets[idx];
        if (!reaction->touched[idx] || !s->units[idx].alive) continue;
        if (!t->health_delta && !t->power_delta && t->crit_delta == 0.0f && t->damage_bonus_delta == 0.0f) continue;
        if (!a) a = SimEmit(s, actor_idx);
        if (!a) return;
        a->replay_attack = false;
        SimApplyTarget(s, a, t);
    }
}

static bool SimGearCanReact(const BattleSim *s) {
    return s->action_count < SIM_MAX_ACTIONS - SIM_GEAR_ACTION_RESERVE;
}

static void SimGearResolveDeaths(BattleSim *s, int actor_idx) {
    for (int pass = 0; pass < SIM_GEAR_DEATH_PASSES && s->gear_pending_deaths && SimGearCanReact(s); pass++) {
        unsigned deaths = s->gear_pending_deaths;
        s->gear_pending_deaths = 0;
        SimGearReaction reaction = {0};
        for (int dead_idx = 0; dead_idx < SIM_UNIT_COUNT; dead_idx++) {
            if (!(deaths & (1u << dead_idx))) continue;
            SimUnit *dead = &s->units[dead_idx];
            SimGearFireTrigger(s, &reaction, dead, GEAR_ON_SELF_DEATH, s->gear_last_damage);
            for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
                SimUnit *wearer = &s->units[idx];
                if (!wearer->alive) continue;
                GearTrigger on = wearer->side == dead->side ? GEAR_ON_ALLY_DEATH : GEAR_ON_ENEMY_DEATH;
                SimGearFireTrigger(s, &reaction, wearer, on, s->gear_last_damage);
            }
        }
        SimGearEmitReaction(s, actor_idx, &reaction);
    }
    s->gear_pending_deaths = 0;
}

static void SimGearReactToAttack(BattleSim *s, int actor_idx, const BattleSimTarget *hits, int hit_count) {
    if (!SimGearCanReact(s)) return;
    const SimUnit *actor = &s->units[actor_idx];
    SimGearReaction reaction = {0};
    int dealt_total = 0;
    for (int h = 0; h < hit_count; h++) {
        const SimUnit *target = &s->units[hits[h].unit];
        int dealt = -(hits[h].health_delta + hits[h].health_faction_delta);
        if (dealt <= 0) continue;
        dealt_total += dealt;
        bool hidden = target->side != actor->side && SimHitIsHidden(s, actor, target);
        for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
            SimUnit *wearer = &s->units[idx];
            if (!wearer->alive) continue;
            if (wearer->side != target->side)
                SimGearFireTrigger(s, &reaction, wearer, GEAR_ON_ENEMY_HIT, dealt);
            else if (hidden)
                continue;
            else if (wearer == target)
                SimGearFireTrigger(s, &reaction, wearer, GEAR_ON_SELF_HIT, dealt);
            else
                SimGearFireTrigger(s, &reaction, wearer, GEAR_ON_ALLY_HIT, dealt);
        }
    }
    if (dealt_total > 0) {
        s->gear_last_damage = dealt_total;
        for (int idx = 0; idx < SIM_UNIT_COUNT; idx++) {
            SimUnit *wearer = &s->units[idx];
            if (!wearer->alive || wearer->side != actor->side) continue;
            SimGearFireTrigger(
                s, &reaction, wearer, idx == actor_idx ? GEAR_ON_SELF_ATTACK : GEAR_ON_ALLY_ATTACK, dealt_total
            );
            if (idx == actor_idx) SimGearFireTrigger(s, &reaction, wearer, GEAR_ON_ALLY_ATTACK, dealt_total);
        }
    }
    SimGearEmitReaction(s, actor_idx, &reaction);
    SimGearResolveDeaths(s, actor_idx);
}

static void SimAttack(
    BattleSim *s, int actor_idx, const AbilityRecipe *r, const AttackRecipe *hit, bool attack, bool apply_secondary
) {
    SimUnit *actor = &s->units[actor_idx];
    int picked[SIM_MAX_TARGETS];
    int n = 0;
    bool friendly = hit->target == RECIPE_TARGET_ONE_ALLY;

    if (attack) {
        if (friendly) {
            int pick = SimPickAlly(s, actor);
            if (pick >= 0) picked[n++] = pick;
        } else if (hit->target == RECIPE_TARGET_ANY) {
            int choices[SIM_UNIT_COUNT], count = 0;
            for (int idx = 0; idx < SIM_UNIT_COUNT; idx++)
                if (idx != actor_idx && s->units[idx].alive) choices[count++] = idx;
            if (count > 0) picked[n++] = choices[SimRange(s, 0, count - 1)];
        } else if (hit->target == RECIPE_TARGET_BACK_ROW) {
            int enemy_side = 1 - actor->side;
            for (int slot = 0; slot < SQUAD_SLOT_LARGE; slot++) {
                if (!SquadSlotIsBack(slot)) continue;
                int idx = enemy_side * SQUAD_SLOT_COUNT + slot;
                if (s->units[idx].alive) picked[n++] = idx;
            }
        } else {
            n = SimSelectTargets(s, actor, SimRecipeTarget(actor, hit->target), picked);
        }
        if (n <= 0) return;
    }

    bool crit =
        attack && !friendly && !hit->status_only && SimRange(s, 1, 1000) <= (int)lroundf(actor->crit_pct * 10.0f);
    int dmg_pct = hit->status_only ? 0 : (hit->dmg_pct ? hit->dmg_pct : 100);

    int base[SIM_MAX_TARGETS];
    int defender = -1;
    int defender_pct = 0;
    for (int i = 0; i < n; i++) {
        SimUnit *t = &s->units[picked[i]];
        float bonus_pct = actor->damage_bonus_pct + SimPassiveBonusDamagePct(s, actor, t) +
                          (float)SimGearVersusStat(s, actor, t, GEAR_STAT_BONUS_DAMAGE);
        float damage_scale = (dmg_pct / 100.0f) * (1.0f + bonus_pct / 100.0f);
        base[i] = (int)lroundf(actor->power * damage_scale) * (crit ? 2 : 1);
        if (!hit->status_only && hit->levy_pct > 0) base[i] += (int)lroundf(t->power * (hit->levy_pct / 100.0f));
        if (t->marked > 0) base[i] += (int)lroundf(base[i] * (t->marked / 100.0f));
        int absorb_pct = SimHasSpec(t->ch, SPEC_DEFENDER) ? SIM_DEFENDER_ABSORB_PCT : t->gear.defender_pct;
        if (n > 1 && absorb_pct > 0) {
            defender = i;
            defender_pct = absorb_pct;
        }
    }
    if (defender >= 0) {
        for (int i = 0; i < n; i++) {
            if (i == defender) continue;
            int moved = (int)((long long)base[i] * defender_pct / 100);
            base[i] -= moved;
            base[defender] += moved;
        }
    }
    for (int i = 0; i < n; i++) {
        const SimUnit *t = &s->units[picked[i]];
        if (t->side != actor->side) base[i] = SimMitigatedDamage(s, actor, t, base[i]);
    }

    struct {
        BattleSimTarget t;
        BattleAilment ailments;
    } hits[SIM_MAX_TARGETS];
    int hit_count = 0;
    int reflected = 0;
    int assassinate_target = -1;
    if (hit->assassinate_pct > 0 && SimChance(s, hit->assassinate_pct)) {
        int choices[SIM_MAX_TARGETS], choice_count = 0;
        for (int i = 0; i < n; i++)
            if (!s->units[picked[i]].ch->giant) choices[choice_count++] = picked[i];
        if (choice_count > 0) assassinate_target = choices[SimRange(s, 0, choice_count - 1)];
    }

    for (int i = 0; i < n; i++) {
        SimUnit *t = &s->units[picked[i]];
        int extra =
            t->side != actor->side && SimHasAdvantage(actor->faction, t->faction) ? (int)lroundf(0.25f * base[i]) : 0;

        BattleSimTarget bt = {.unit = picked[i], .health_delta = -base[i], .health_faction_delta = -extra};
        BattleAilment ailments = AIL_NONE;
        const RuneBonus *defense = &s->rune_bonus[t->side];
        if (SimEffectLands(s, hit->stun_pct, defense->stun_resist_pct + t->gear.stun_resist_pct))
            ailments |= AIL_STUNNED;
        if (SimEffectLands(s, hit->slow_pct, defense->slow_resist_pct + t->gear.slow_resist_pct))
            bt.initiative_delta += hit->slow_amt;
        if (hit->drain_pct > 0 && (hit->drain_chance == 0 || SimChance(s, hit->drain_chance)) &&
            !SimRuneResists(s, defense->drain_resist_pct + t->gear.drain_resist_pct))
            bt.power_delta -= (int)lroundf(actor->power * (hit->drain_pct / 100.0f));
        if (SimEffectLands(s, hit->curse_power_pct, t->gear.curse_power_resist_pct) && !defense->curse_power_immune)
            ailments |= AIL_CURSE_POWER;
        if (SimEffectLands(s, hit->curse_health_pct, t->gear.curse_health_resist_pct)) ailments |= AIL_CURSE_HEALTH;
        if (picked[i] == assassinate_target) {
            bt.health_delta = extra < t->health ? extra - t->health : 0;
            bt.health_faction_delta = extra < t->health ? -extra : -t->health;
        }
        if (SimRuneRetaliates(s, actor, t)) {
            float dealt = -(float)(bt.health_delta + bt.health_faction_delta);
            reflected += (int)lroundf(dealt * (defense->retaliation_pct / 100.0f));
        }

        hits[hit_count].t = bt;
        hits[hit_count].ailments = ailments;
        hit_count++;
    }

    bool first_record = true;
    for (int mask = 0; mask < 8; mask++) {
        BattleSimAction *a = NULL;
        for (int i = 0; i < hit_count; i++) {
            if ((int)hits[i].ailments != mask) continue;
            if (!a) {
                a = SimEmit(s, actor_idx);
                if (!a) return;
                a->crit = crit;
                a->ailments = (BattleAilment)mask;
                a->same_swing = !first_record;
                a->replay_attack = first_record && !hit->status_only;
                first_record = false;
            }
            SimApplyTarget(s, a, &hits[i].t);
            SimUnit *t = &s->units[hits[i].t.unit];
            if (t->alive) {
                if (mask & AIL_STUNNED) t->stunned = true;
                int curse_turns = hit->curse_turns > 0 ? hit->curse_turns : 1;
                if (mask & AIL_CURSE_POWER) {
                    t->curse_power = true;
                    if (t->curse_power_turns < curse_turns) t->curse_power_turns = curse_turns;
                }
                if (mask & AIL_CURSE_HEALTH) {
                    t->curse_health = true;
                    if (t->curse_health_turns < curse_turns) t->curse_health_turns = curse_turns;
                }
            }
        }
    }
    if (hit->mark_pct > 0)
        for (int i = 0; i < n; i++)
            if (s->units[picked[i]].alive) s->units[picked[i]].marked = hit->mark_pct;

    SimApplyPerHitSelfEffects(s, actor_idx, hit);

    BattleSimTarget landed[SIM_MAX_TARGETS];
    for (int i = 0; i < hit_count; i++) landed[i] = hits[i].t;
    SimGearReactToAttack(s, actor_idx, landed, hit_count);

    if (!apply_secondary) {
        SimApplyRuneRetaliation(s, actor_idx, reflected);
        return;
    }

    const SecondaryEffects *fx = &r->fx;
    bool self_power = fx->self_power_pct != 0 && (fx->self_power_chance == 0 || SimChance(s, fx->self_power_chance));
    bool self_heal = fx->self_heal_pct != 0 && (fx->self_heal_chance == 0 || SimChance(s, fx->self_heal_chance));
    bool ally_heal = fx->ally_heal_pct != 0 && (fx->ally_heal_chance == 0 || SimChance(s, fx->ally_heal_chance));
    bool ally_power = fx->ally_power_pct != 0 && (fx->ally_power_chance == 0 || SimChance(s, fx->ally_power_chance));
    bool has_ally = ally_heal || ally_power || fx->ally_power_from_actor_pct || fx->ally_slow_amt ||
                    fx->ally_crit_pct || fx->ally_damage_bonus_pct || fx->ally_damage_pct;
    if (self_power || self_heal || fx->self_damage_bonus_pct || fx->self_damage_pct || has_ally) {
        BattleSimAction *sec = SimEmit(s, actor_idx);
        if (!sec) return;
        sec->replay_attack = false;
        BattleSimTarget st = {.unit = actor_idx};
        if (self_power) st.power_delta += (int)lroundf(actor->start_power * (fx->self_power_pct / 100.0f));
        if (actor->curse_power && st.power_delta > 0) st.power_delta = 0;
        if (self_heal && !actor->curse_health)
            st.health_delta += (int)lroundf(actor->max_health * (fx->self_heal_pct / 100.0f));
        if (fx->self_damage_bonus_pct) st.damage_bonus_delta += fx->self_damage_bonus_pct;
        if (fx->self_damage_pct) st.health_delta += (int)lroundf(actor->power * (fx->self_damage_pct / 100.0f));
        if (st.power_delta || st.health_delta || st.damage_bonus_delta) SimApplyTarget(s, sec, &st);
        if (has_ally) {
            for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
                int idx = actor->side * SQUAD_SLOT_COUNT + slot;
                SimUnit *ally = &s->units[idx];
                if (!ally->alive || idx == actor_idx) continue;
                BattleSimTarget at = {.unit = idx};
                if (ally_heal && !ally->curse_health)
                    at.health_delta += (int)lroundf(actor->power * (fx->ally_heal_pct / 100.0f));
                if (ally_power && !ally->curse_power)
                    at.power_delta += (int)lroundf(ally->start_power * (fx->ally_power_pct / 100.0f));
                if (fx->ally_power_from_actor_pct && !ally->curse_power)
                    at.power_delta += (int)lroundf(actor->power * (fx->ally_power_from_actor_pct / 100.0f));
                if (fx->ally_slow_amt) at.initiative_delta += fx->ally_slow_amt;
                if (fx->ally_crit_pct) at.crit_delta += fx->ally_crit_pct;
                if (fx->ally_damage_bonus_pct) at.damage_bonus_delta += fx->ally_damage_bonus_pct;
                if (fx->ally_damage_pct) at.health_delta += (int)lroundf(actor->power * (fx->ally_damage_pct / 100.0f));
                if (at.health_delta || at.power_delta || at.initiative_delta || at.crit_delta || at.damage_bonus_delta)
                    SimApplyTarget(s, sec, &at);
            }
        }
        if (sec->target_count == 0) s->action_count--;
    }
    SimApplyRuneRetaliation(s, actor_idx, reflected);
}

static AttackRecipe SimFollowupAttack(const AbilityRecipe *recipe, int index) {
    AttackRecipe hit = recipe->followup;
    int damage = hit.dmg_pct ? hit.dmg_pct : 100;
    if (recipe->followup_damage_step_pct) damage += recipe->followup_damage_step_pct * (index + 1);
    if (recipe->followup_final_damage_pct && index == recipe->followup_count - 1)
        damage = recipe->followup_final_damage_pct;
    hit.dmg_pct = (short)damage;
    return hit;
}

static void SimExpireAilments(BattleSim *s, int actor_idx, bool unstun) {
    SimUnit *u = &s->units[actor_idx];
    BattleAilment ailments = unstun ? AIL_UNSTUNNED : AIL_NONE;
    if (u->curse_power) {
        if (u->curse_power_turns > 1) {
            u->curse_power_turns--;
        } else {
            ailments |= AIL_UNCURSE_POWER;
            u->curse_power = false;
            u->curse_power_turns = 0;
        }
    }
    if (u->curse_health) {
        if (u->curse_health_turns > 1) {
            u->curse_health_turns--;
        } else {
            ailments |= AIL_UNCURSE_HEALTH;
            u->curse_health = false;
            u->curse_health_turns = 0;
        }
    }
    if (ailments != AIL_NONE) {
        BattleSimAction *a = SimEmit(s, actor_idx);
        if (a) a->ailments = ailments;
    }
    u->stunned = false;
}

static void SimGeneratedTakeTurn(BattleSim *s, int actor_idx, const AbilityRecipe *recipe, bool no_attack) {
    SimUnit *actor = &s->units[actor_idx];
    signed char chance_group[16];
    memset(chance_group, -1, sizeof(chance_group));
    for (int i = 0; i < recipe->generated_step_count; i++) {
        const RecipeStep *step = &recipe->generated_steps[i];
        for (int repeat = 0; repeat < step->repeat; repeat++) {
            if (!actor->alive) return;
            bool proc = true;
            if (step->chance_pct < 100) {
                if (step->chance_group > 0 && step->chance_group < (int)sizeof(chance_group)) {
                    if (chance_group[step->chance_group] < 0)
                        chance_group[step->chance_group] = SimChance(s, step->chance_pct);
                    proc = chance_group[step->chance_group] != 0;
                } else {
                    proc = SimChance(s, step->chance_pct);
                }
            }
            if (!proc) {
                if (step->stop_on_fail) break;
                continue;
            }

            AttackRecipe hit = step->attack;
            if (step->final_damage_pct && repeat == step->repeat - 1) hit.dmg_pct = step->final_damage_pct;
            if (no_attack) hit.status_only = true;
            bool apply_effects = !step->effects_once || repeat == 0;
            if (apply_effects) hit.self_crit_pct += step->self_crit_pct;
            if (hit.drain_chance == 100) hit.drain_chance = 0;
            AbilityRecipe effects = {0};
            if (apply_effects) effects.fx = step->fx;
            bool hostile = SimHasHostileEffect(&hit);
            bool attacks_enemy = !hit.status_only;
            if (attacks_enemy && SimTeamDead(s, 1 - actor->side)) return;
            SimAttack(s, actor_idx, &effects, &hit, attacks_enemy || hostile, true);
        }
    }
}

static void SimTakeTurn(BattleSim *s, int actor_idx) {
    SimUnit *u = &s->units[actor_idx];
    if (!u->alive) return;

    if (u->stunned) {
        SimExpireAilments(s, actor_idx, true);
        return;
    }

    const AbilityRecipe *r = SimUnitRecipe(u);
    bool no_attack = (u->ch->ability->mechanics & ABILITY_ATTACK_NONE) || SimHasSpec(u->ch, SPEC_ATTACKNONE);
    if (r->generated) {
        SimGeneratedTakeTurn(s, actor_idx, r, no_attack);
    } else {
        AttackRecipe primary = r->hit;
        if (no_attack) primary.status_only = true;
        SimAttack(s, actor_idx, r, &primary, !no_attack || SimHasHostileEffect(&primary), true);

        if (r->effect_pct > 0 && u->alive && !SimTeamDead(s, 1 - u->side) && SimChance(s, r->effect_pct)) {
            AttackRecipe effect = r->effect;
            if (no_attack) effect.status_only = true;
            SimAttack(s, actor_idx, r, &effect, !no_attack || SimHasHostileEffect(&effect), false);
        }

        for (int f = 0; f < r->followup_count; f++) {
            if (SimTeamDead(s, 1 - u->side) || !u->alive) break;
            if (!SimChance(s, r->followup_pct)) break;
            AttackRecipe followup = SimFollowupAttack(r, f);
            if (no_attack) followup.status_only = true;
            SimAttack(s, actor_idx, r, &followup, !no_attack || SimHasHostileEffect(&followup), false);
        }
    }
    SimExpireAilments(s, actor_idx, false);
}

static int SimRollInitiative(BattleSim *s, const SimUnit *u) {
    int lo = u->init_class * SIM_INIT_CLASS_SPAN, hi = lo + SIM_INIT_CLASS_SPAN - 1;
    int rune_initiative = s->rune_bonus[u->side].initiative_steps * SIM_INIT_CLASS_SPAN;
    if (s->stat_budget) rune_initiative = StatBudgetRuneInitiative(s->stat_budget, rune_initiative);
    int roll = SimRange(s, lo, hi);
    if (u->gear.haste) {
        int second = SimRange(s, lo, hi);
        if (second > roll) roll = second;
    }
    return roll + u->cur_initiative + rune_initiative;
}

static int SimRollRoundOrder(BattleSim *s, int order[SIM_UNIT_COUNT]) {
    int inits[SIM_UNIT_COUNT], count = 0;
    for (int i = 0; i < SIM_UNIT_COUNT; i++) {
        if (!s->units[i].alive) continue;
        order[count] = i;
        inits[count] = SimRollInitiative(s, &s->units[i]);
        s->units[i].cur_initiative = 0;
        count++;
    }
    for (int i = 1; i < count; i++) {
        int oi = order[i], vi = inits[i], j = i - 1;
        while (j >= 0 && (inits[j] < vi || (inits[j] == vi && order[j] > oi))) {
            order[j + 1] = order[j];
            inits[j + 1] = inits[j];
            j--;
        }
        order[j + 1] = oi;
        inits[j + 1] = vi;
    }
    return count;
}

#define SIM_ROUND_CAP 10

static int SimRuneStat(int stat, int rune_pct, const StatBudget *stat_budget) {
    return stat_budget ? StatBudgetRuneStat(stat_budget, stat, rune_pct) : RuneApplyPct(stat, rune_pct);
}

static void SimUnitsRuneBonus(SimUnit *units, int side, const RuneBonus *bonus, const StatBudget *stat_budget) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        SimUnit *u = &units[side * SQUAD_SLOT_COUNT + slot];
        if (!u->present) continue;
        u->power = u->start_power = SimRuneStat(u->power, bonus->power_pct, stat_budget);
        u->max_health = u->health = SimRuneStat(u->max_health, bonus->health_pct, stat_budget);
    }
}

static bool SimChangerEffectMatches(const BattleChangerEffect *effect, const SimUnit *u) {
    for (int i = 0; i < effect->group_count; i++)
        if (!SimPassiveHasGroup(u->ch, effect->groups[i])) return false;
    if (effect->faction_mask && !(u->faction & effect->faction_mask)) return false;
    if (effect->unit_count == 0) return true;
    for (int i = 0; i < effect->unit_count; i++)
        if (!strcmp(u->ch->id, effect->units[i])) return true;
    return false;
}

static void SimUnitsGameChanger(SimUnit *units, const BattleGameChanger *changer, const StatBudget *stat_budget) {
    if (!changer) return;
    for (int i = 0; i < SIM_UNIT_COUNT; i++) {
        SimUnit *u = &units[i];
        if (!u->present) continue;
        for (int e = 0; e < changer->effect_count; e++) {
            const BattleChangerEffect *effect = &changer->effects[e];
            if (!SimChangerEffectMatches(effect, u)) continue;
            int power_pct = effect->power_pct, health_pct = effect->health_pct;
            if (stat_budget) {
                power_pct = StatBudgetCappedChangerPct(stat_budget, power_pct);
                health_pct = StatBudgetCappedChangerPct(stat_budget, health_pct);
            }
            u->power = u->start_power = (int)lroundf(u->power * (1.0f + power_pct / 100.0f));
            u->max_health = u->health = (int)lroundf(u->max_health * (1.0f + health_pct / 100.0f));
            u->game_changer_applied = true;
        }
    }
}

typedef struct {
    const Character *chars[SQUAD_SLOT_COUNT];
    int rune_defs[RUNE_SLOT_COUNT];
    int rune_def_count;
} BattleSimSide;

static BattleSimSide SimPlayerSide(const Player *player) {
    BattleSimSide side = {0};
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) side.chars[slot] = PlayerSquadCharacter(player, slot);
    side.rune_def_count = RuneActiveDefs(&player->runes, player->current_squad, RuneNow(), side.rune_defs);
    return side;
}

static BattleSimSide SimRosterSide(const BattleRoster *roster) {
    BattleSimSide side = {0};
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++)
        side.chars[slot] = roster->present[slot] ? &roster->chars[slot] : NULL;
    for (int i = 0; i < roster->rune_def_count && i < RUNE_SLOT_COUNT; i++)
        side.rune_defs[side.rune_def_count++] = roster->rune_defs[i];
    return side;
}

typedef struct {
    const BattleGameChanger *changer;
    const StatBudget *stat_budget;
    const StatBudget *hero_rules;
} BattleSimRules;

static BattleSimRules SimRosterRules(const BattleRoster *enemy) {
    return (BattleSimRules){
        .changer = enemy->game_changer,
        .stat_budget = StatBudgetIfScaling(enemy->stat_budget),
        .hero_rules = enemy->stat_budget,
    };
}

static RuneBonus SimSideRuneBonus(const BattleSimSide *side, const StatBudget *stat_budget) {
    RuneBonus bonus = RuneDefsBonus(side->rune_defs, side->rune_def_count);
    if (stat_budget) StatBudgetLimitRuneBonus(stat_budget, &bonus);
    return bonus;
}

static void SimUnitsSetup(
    SimUnit *units,
    unsigned faction[2],
    float bonus_pct[2],
    const BattleSimSide *player,
    const BattleSimSide *enemy,
    const BattleSimRules *rules
) {
    PassiveManifestEnsure();
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        SimUnitsInit(units, 0, slot, player->chars[slot], rules->hero_rules);
        SimUnitsInit(units, 1, slot, enemy->chars[slot], rules->hero_rules);
    }
    SimUnitsBattleStartPassives(units, rules->stat_budget);
    SimUnitsFormationBonus(units, 0, &faction[0], &bonus_pct[0], rules->stat_budget);
    SimUnitsFormationBonus(units, 1, &faction[1], &bonus_pct[1], rules->stat_budget);
    RuneBonus rune_bonus = SimSideRuneBonus(player, rules->stat_budget);
    SimUnitsRuneBonus(units, 0, &rune_bonus, rules->stat_budget);
    RuneBonus enemy_rune_bonus = SimSideRuneBonus(enemy, rules->stat_budget);
    SimUnitsRuneBonus(units, 1, &enemy_rune_bonus, rules->stat_budget);
    SimUnitsGameChanger(units, rules->changer, rules->stat_budget);
}

static void BattleSimSetupSides(
    BattleSim *s, const BattleSimSide *player, const BattleSimSide *enemy, const BattleSimRules *rules
) {
    memset(s, 0, sizeof(*s));
    s->stat_budget = rules->stat_budget;
    SimUnitsSetup(s->units, s->formation_faction, s->formation_bonus_pct, player, enemy, rules);
    s->rune_bonus[0] = SimSideRuneBonus(player, rules->stat_budget);
    s->rune_bonus[1] = SimSideRuneBonus(enemy, rules->stat_budget);
    s->rune_def_count = player->rune_def_count;
    memcpy(s->rune_defs, player->rune_defs, sizeof(s->rune_defs));
}

void BattleSimSetup(BattleSim *s, const Player *player, const BattleRoster *enemy) {
    BattleSimSide player_side = SimPlayerSide(player), enemy_side = SimRosterSide(enemy);
    BattleSimRules rules = SimRosterRules(enemy);
    BattleSimSetupSides(s, &player_side, &enemy_side, &rules);
}

void BattleSimPreview(SimUnit units[SIM_UNIT_COUNT], const Player *player, const BattleRoster *enemy) {
    unsigned faction[2] = {0};
    float bonus_pct[2] = {0};
    BattleSimSide player_side = SimPlayerSide(player), enemy_side = SimRosterSide(enemy);
    BattleSimRules rules = SimRosterRules(enemy);
    SimUnitsSetup(units, faction, bonus_pct, &player_side, &enemy_side, &rules);
}

static void BattleSimRunSides(
    BattleSim *s, const BattleSimSide *player, const BattleSimSide *enemy, const BattleSimRules *rules, unsigned seed
);

void BattleSimRun(BattleSim *s, const Player *player, const BattleRoster *enemy, unsigned seed) {
    BattleSimSide player_side = SimPlayerSide(player), enemy_side = SimRosterSide(enemy);
    BattleSimRules rules = SimRosterRules(enemy);
    BattleSimRunSides(s, &player_side, &enemy_side, &rules, seed);
}

void BattleSimRunRosters(BattleSim *s, const BattleRoster *player, const BattleRoster *enemy, unsigned seed) {
    BattleSimSide player_side = SimRosterSide(player), enemy_side = SimRosterSide(enemy);
    BattleSimRules rules = SimRosterRules(enemy);
    BattleSimRunSides(s, &player_side, &enemy_side, &rules, seed);
}

static void BattleSimRunSides(
    BattleSim *s, const BattleSimSide *player, const BattleSimSide *enemy, const BattleSimRules *rules, unsigned seed
) {
    BattleSimSetupSides(s, player, enemy, rules);
    s->rng = seed ? seed : 0x9E3779B9u;

    for (int round = 1; round <= SIM_ROUND_CAP; round++) {
        s->rounds = round;
        if (round > 1) {
            BattleSimAction *nr = SimEmit(s, -1);
            if (nr) nr->newround = true;
        }
        int order[SIM_UNIT_COUNT];
        int count = SimRollRoundOrder(s, order);
        for (int i = 0; i < count; i++) {
            SimTakeTurn(s, order[i]);
            SimGearResolveDeaths(s, order[i]);
            if (SimTeamDead(s, 0) || SimTeamDead(s, 1)) {
                s->player_won = SimTeamDead(s, 1);
                TraceLog(
                    LOG_INFO,
                    "Sim: %s by death, round %d, %d actions",
                    s->player_won ? "WIN" : "LOSS",
                    round,
                    s->action_count
                );
                return;
            }
        }
    }
    s->player_won = SimTeamHealthFrac(s, 0) > SimTeamHealthFrac(s, 1);
    TraceLog(LOG_INFO, "Sim: %s by round cap, %d actions", s->player_won ? "WIN" : "LOSS", s->action_count);
}

void BattleSimDump(const BattleSim *s) {
    int round = 1;
    for (int i = 0; i < s->action_count; i++) {
        const BattleSimAction *a = &s->actions[i];
        if (a->newround) {
            round++;
            TraceLog(LOG_INFO, "Sim: ---- round %d ----", round);
            continue;
        }
        const SimUnit *actor = &s->units[a->actor];
        char ailments[96] = "";
        if (a->ailments & AIL_STUNNED) strcat(ailments, " STUN");
        if (a->ailments & AIL_CURSE_POWER) strcat(ailments, " CURSE_PWR");
        if (a->ailments & AIL_CURSE_HEALTH) strcat(ailments, " CURSE_HP");
        if (a->ailments & AIL_UNSTUNNED) strcat(ailments, " UNSTUN(turn lost)");
        if (a->ailments & AIL_UNCURSE_POWER) strcat(ailments, " UNCURSE_PWR");
        if (a->ailments & AIL_UNCURSE_HEALTH) strcat(ailments, " UNCURSE_HP");
        char line[512];
        int len = snprintf(
            line,
            sizeof(line),
            "Sim: [%3d] %s s%d %-24s%s%s%s",
            i,
            actor->side == 0 ? "P" : "E",
            actor->slot,
            actor->ch->id,
            a->crit ? " CRIT" : "",
            a->replay_attack ? "" : " (chain)",
            ailments
        );
        for (int t = 0; t < a->target_count && len < (int)sizeof(line) - 64; t++) {
            const BattleSimTarget *bt = &a->targets[t];
            const SimUnit *tu = &s->units[bt->unit];
            len += snprintf(
                line + len,
                sizeof(line) - (size_t)len,
                " | %s s%d hp%+d%s pw%+d in%+d",
                tu->side == 0 ? "P" : "E",
                tu->slot,
                bt->health_delta,
                bt->health_faction_delta ? TextFormat("(%+d)", bt->health_faction_delta) : "",
                bt->power_delta,
                bt->initiative_delta
            );
        }
        TraceLog(LOG_INFO, "%s", line);
    }
    TraceLog(
        LOG_INFO, "Sim: outcome=%s rounds=%d actions=%d", s->player_won ? "WIN" : "LOSS", s->rounds, s->action_count
    );
}

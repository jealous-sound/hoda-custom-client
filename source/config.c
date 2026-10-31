#define CAMPAIGN_MAX_MAPS 16
#define CAMPAIGN_MAX_NODES 12

#define NEXUS_MAX_EVENTS 40
#define NEXUS_MAX_DIFFICULTIES 6
#define NEXUS_MAX_NODES 8
#define NEXUS_KEY_COUNT 4
#define NEXUS_KEY_FIRST_ID 2001
#define NEXUS_KEY_MAX 99
#define NEXUS_SECONDS_PER_DAY 86400LL

typedef struct {
    const int game_width;
    const int game_height;
} Config;

static const Config config = {.game_width = 1280, .game_height = 720};

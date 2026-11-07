typedef struct {
    int current;
} MusicPlayer;

void MusicPlayerPlayRandom(MusicPlayer *mp) {
    StopMusicStream(GuiMusic(mp->current));
    int next = GetRandomValue(0, MUS_HOME_COUNT - 1);
    while (next == mp->current) next = GetRandomValue(0, MUS_HOME_COUNT - 1);
    mp->current = next;
    PlayMusicStream(GuiMusic(mp->current));
}

void MusicPlayerInitialize(MusicPlayer *mp) {
    mp->current = GetRandomValue(0, MUS_HOME_COUNT - 1);
    PlayMusicStream(GuiMusic(mp->current));
}

void MusicPlayerUpdate(MusicPlayer *mp) {
    Music m = GuiMusic(mp->current);
    AudioUpdateMusic(m, 1.0f);
    if (!IsMusicStreamPlaying(m)) MusicPlayerPlayRandom(mp);
}

#ifndef HOD_TEST_SQUAD_META_H
#define HOD_TEST_SQUAD_META_H

#define SQUAD_META_CAPACITY 192
#define SQUAD_META_EVEN_SHARE 0.5
#define SQUAD_META_SOLVER_STEP_LIMIT 200000
#define SQUAD_META_SOLVER_STEPS_PER_CHECK 2000
#define SQUAD_META_SOLVER_TOLERANCE 0.002

typedef struct {
    int count;
    float win_share[SQUAD_META_CAPACITY][SQUAD_META_CAPACITY];
    double weight[SQUAD_META_CAPACITY];
} SquadMeta;

static int SquadMetaAdd(SquadMeta *meta) {
    int squad = meta->count++;
    meta->win_share[squad][squad] = (float)SQUAD_META_EVEN_SHARE;
    meta->weight[squad] = 0.0;
    return squad;
}

static void SquadMetaRecord(SquadMeta *meta, int squad, int rival, float squad_win_share) {
    meta->win_share[squad][rival] = squad_win_share;
    meta->win_share[rival][squad] = 1.0f - squad_win_share;
}

static void SquadMetaWeighEvenly(SquadMeta *meta, int squads) {
    for (int squad = 0; squad < meta->count; squad++) meta->weight[squad] = squad < squads ? 1.0 / squads : 0.0;
}

static double SquadMetaWinShareAgainst(const SquadMeta *meta, int squad, const double *weight) {
    double share = 0.0;
    for (int rival = 0; rival < meta->count; rival++) share += weight[rival] * meta->win_share[squad][rival];
    return share;
}

static double SquadMetaWinShare(const SquadMeta *meta, int squad) {
    return SquadMetaWinShareAgainst(meta, squad, meta->weight);
}

static double SquadMetaBestWinShare(const SquadMeta *meta) {
    double best = 0.0;
    for (int squad = 0; squad < meta->count; squad++) {
        double share = SquadMetaWinShare(meta, squad);
        if (share > best) best = share;
    }
    return best;
}

static void SquadMetaSolve(SquadMeta *meta) {
    static double regret[SQUAD_META_CAPACITY], mix[SQUAD_META_CAPACITY], played[SQUAD_META_CAPACITY];
    static double gain[SQUAD_META_CAPACITY];
    int count = meta->count;
    for (int squad = 0; squad < count; squad++) regret[squad] = played[squad] = 0.0;
    for (int step = 1; step <= SQUAD_META_SOLVER_STEP_LIMIT; step++) {
        double regretted = 0.0;
        for (int squad = 0; squad < count; squad++) regretted += regret[squad];
        for (int squad = 0; squad < count; squad++)
            mix[squad] = regretted > 0.0 ? regret[squad] / regretted : 1.0 / count;
        for (int squad = 0; squad < count; squad++)
            gain[squad] = SquadMetaWinShareAgainst(meta, squad, mix) - SQUAD_META_EVEN_SHARE;
        for (int squad = 0; squad < count; squad++) {
            regret[squad] = regret[squad] + gain[squad] > 0.0 ? regret[squad] + gain[squad] : 0.0;
            played[squad] += mix[squad];
        }
        if (step % SQUAD_META_SOLVER_STEPS_PER_CHECK) continue;
        for (int squad = 0; squad < count; squad++) meta->weight[squad] = played[squad] / step;
        if (SquadMetaBestWinShare(meta) <= SQUAD_META_EVEN_SHARE + SQUAD_META_SOLVER_TOLERANCE) return;
    }
}

#endif

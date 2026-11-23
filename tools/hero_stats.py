def stat_at_level(stats, stat, level):
    base = stats[f"base_{stat}"]
    growth_levels = (stats.get("max_level") or 0) - 1
    if growth_levels < 1:
        return base
    return base + ((level - 1) * (stats[f"max_{stat}"] - base) + growth_levels // 2) // growth_levels


def stat_total_at_level(stats, level):
    return stat_at_level(stats, "power", level) + stat_at_level(stats, "health", level)

#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import original_cache

CLI_DESCRIPTION = 'Build data/store_packs.json, the single pack catalog, and the store art the 4.4.0 cache lacks.'

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'data' / 'store_packs.json'
LOCALIZATION = ROOT / '.claude' / 'skills' / 'hoda-reference' / 'data' / 'localization' / 'Loc_RUS_RU.json'
FOOTAGE = ROOT / 'client-additional-data' / 'store_research' / 'footage'
EXTRA_STORE_DIR = ROOT / 'client-additional-data' / 'extra' / 'gui' / 'store'
ASSET_PACK_IMAGES = Path.home() / 'Downloads' / 'HoDA_game_assets_pack' / 'images'

BALANCE_NOTE = (
    'Каталог магазина сервер отдавал удалённо, и в дампе его нет. Названия паков взяты из локализации 5.4.4. '
    'Цены, виденные в записях, сохранены; все остальные цены, шансы, состав, расписание и описания заданы '
    'как локальный баланс. Лестница самоцветов восстановлена точно из строк локализации.'
)

FOOTAGE_ASPECT_CORRECTION = 4.0 / 3.0
TILE_ART_HEIGHT = 720
TILE_CROPS = {
    'tile_heroes.png': ('2016_landing_f_C54RVmu8k_t302.jpg', (150, 190, 303, 452)),
    'tile_items.png': ('2016_landing_f_C54RVmu8k_t302.jpg', (497, 190, 650, 452)),
    'tile_vault.png': ('2021_landing_78daeqhxqZQ_t150.jpg', (505, 373, 615, 485)),
    'tile_featured.png': ('2021_landing_78daeqhxqZQ_t150.jpg', (187, 180, 345, 480)),
}

VOUCHERS = (
    ('bronze', 'cache:tex.Voucher_BronzePack'),
    ('silver', 'cache:tex.Voucher_SilverPack'),
    ('champion', 'cache:tex.Voucher_ChampPack'),
    ('iron', 'cache:tex.Voucher_IronPack'),
    ('bloodstone', 'cache:tex.Voucher_BloodstonePack'),
    ('goat', 'cache:tex.Voucher_Goat'),
    ('nug', 'gui/store/tex.Voucher_Nug.png'),
    ('dragon', 'gui/store/tex.Voucher_Dragon.png'),
    ('romance', 'gui/store/tex.Voucher_Romance.png'),
    ('wintersend', 'gui/store/tex.Voucher_Wintersend.png'),
    ('harvest', 'gui/store/tex.Voucher_Harvest.png'),
    ('anniversary', 'gui/store/tex.Voucher_Anniversary.png'),
    ('library', 'gui/store/tex.Voucher_Library.png'),
    ('lyrium_mine', 'gui/store/tex.Voucher_LyriumMine.png'),
)

GEM_SKUS = [
    {'id': 'sackofgems', 'gems': 22, 'icon': 0},
    {'id': 'bagofgems', 'gems': 46, 'icon': 1},
    {'id': 'pileofgems', 'gems': 121, 'icon': 2, 'banner': 'STORE_MOST_POPULAR'},
    {'id': 'boxofgems', 'gems': 275, 'icon': 3},
    {'id': 'heapofgems', 'gems': 425, 'icon': 4},
    {'id': 'chestofgems', 'gems': 720, 'icon': 5},
    {'id': 'bagfulloffunk', 'gems': 1600, 'icon': 6, 'banner': 'STORE_BEST_VALUE'},
]

GREEN = 'cache:tex.Store_HeaderBGGreenPack'
GOLD = 'cache:tex.Store_HeaderBGGoldPack'
SILVER = 'cache:tex.Store_HeaderBGSilverPack'
BRONZE = 'cache:tex.Store_HeaderBGBronzePack'
PURPLE = 'cache:tex.Store_HeaderChampionPack'
BLUE = 'cache:tex.Store_HeaderBGBlueGreenPack'
FEATURED = 'cache:tex.Store_HeaderBGFeaturedPack'

COMMON_TIER = {'Common': 7500, 'Uncommon': 2000, 'Rare': 400, 'Epic': 90, 'Legendary': 10}
UNCOMMON_TIER = {'Common': 0, 'Uncommon': 7000, 'Rare': 2200, 'Epic': 700, 'Legendary': 100}
RARE_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 6600, 'Epic': 2700, 'Legendary': 700}
BULK_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 6000, 'Epic': 3200, 'Legendary': 800}
EPIC_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 0, 'Epic': 8500, 'Legendary': 1500}
LEGENDARY_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 0, 'Epic': 0, 'Legendary': 10000}
MINIMUM = {id(COMMON_TIER): 'Common', id(UNCOMMON_TIER): 'Uncommon', id(RARE_TIER): 'Rare', id(BULK_TIER): 'Rare',
           id(EPIC_TIER): 'Epic', id(LEGENDARY_TIER): 'Legendary'}

RUNE_FAMILIES = ['rune-power', 'rune-health', 'rune-initiative', 'rune-stun']

WEEKLY_FLASH_DAYS = 7


def offer(currency, price):
    return {'currency': currency, 'price': price}


def voucher(name):
    return {'currency': 'voucher', 'voucher': name, 'price': 1}


def hero(key, loc, order, category, header, icon, offers, tier, count=1, bullets=(), info=None, opened_counter=0,
         schedule=None, rotation=None, epic=0, legendary=0, pity=0, rune_pct=0, rune=(1, 2), rune_count=1,
         gear_pct=0, gear=(1, 3)):
    return {
        'id': key, 'kind': 'hero', 'loc_id': loc, 'category': category, 'order': order, 'header': header,
        'icon': icon, 'offers': offers, 'bullets': list(bullets), 'info_ru': info, 'schedule': schedule,
        'rotation': rotation, 'opened_counter': opened_counter, 'hero_count': count,
        'minimum_rarity': MINIMUM[id(tier)], 'guaranteed_legendary': legendary, 'guaranteed_epic': epic,
        'legendary_pity': pity, 'weights': tier, 'rune_chance_pct': rune_pct, 'rune_min_rarity': rune[0],
        'rune_max_rarity': rune[1], 'rune_count': rune_count, 'gear_chance_pct': gear_pct,
        'gear_min_rarity': gear[0], 'gear_max_rarity': gear[1],
    }


def rune(key, loc, order, header, icon, offers, slots, bullets, info, schedule=None):
    return {
        'id': key, 'kind': 'rune', 'loc_id': loc, 'category': 'ITEMS', 'order': order, 'header': header, 'icon': icon,
        'offers': offers, 'bullets': bullets, 'info_ru': info, 'schedule': schedule, 'rotation': None, 'slots': slots,
    }


def gear(key, loc, order, header, icon, offers, count, rarity, bullets, info, tome_pct=0):
    return {
        'id': key, 'kind': 'gear', 'loc_id': loc, 'category': 'ITEMS', 'order': order, 'header': header, 'icon': icon,
        'offers': offers, 'bullets': bullets, 'info_ru': info, 'schedule': None, 'rotation': None, 'count': count,
        'min_rarity': rarity[0], 'max_rarity': rarity[1], 'tome_chance_pct': tome_pct,
    }


def bullet(text, rarity=0):
    return {'text_ru': text, 'rarity': rarity}


def season(start, end):
    return {'start': start, 'end': end}


def slot(families, low, high):
    return {'families': families, 'min_rarity': low, 'max_rarity': high}


def weekly(index):
    return {'group': 'flash', 'index': index}


def hero_packs():
    gold, gems = (lambda p: [offer('gold', p)]), (lambda p: [offer('gems', p)])
    icon = 'cache:tex.Store_Icon'
    chance = 'Шанс на'
    rows = [
        hero('recruit', 'ITEM_BRONZE_IDENTIFIER', 10, 'UNITS', BRONZE, icon + 'BronzePack',
             gold(350) + [voucher('bronze')], COMMON_TIER, rune_pct=15,
             info='Дает 1 героя. Как минимум обычный боец; есть шанс на необычного, редкого, эпического или легендарного.'),
        hero('soldier', 'ITEM_SILVER_IDENTIFIER', 20, 'UNITS', SILVER, icon + 'SilverPack',
             gold(1500) + [voucher('silver')], UNCOMMON_TIER, opened_counter=1, rune_pct=30, rune=(2, 3),
             info='Дает 1 героя или крупное существо. Как минимум необычный боец; есть шанс на редкого, эпического или легендарного.'),
        hero('champion', 'ITEM_GOLD_IDENTIFIER', 30, 'UNITS', PURPLE, icon + 'GoldPack',
             gems(30) + [voucher('champion')], RARE_TIER, opened_counter=2, pity=4, rune_pct=100, rune=(3, 5),
             bullets=[bullet('Легендарный не позже 4-го', 5)],
             info='Дает 1 героя и 1 руну. Как минимум редкий боец; есть шанс на эпического или легендарного.'),
        hero('grand_champion', 'GRAND_CHAMPION', 40, 'UNITS', 'cache:tex.Store_HeaderGrandChampionPack',
             icon + 'LegendaryPack', gems(270), BULK_TIER, count=10, opened_counter=3, legendary=2, epic=3,
             rune_pct=100, rune=(3, 5), rune_count=10,
             bullets=[bullet('10 наборов по цене 9!'), bullet('2 легендарных и 3 эпических гарантировано', 5)],
             info='10 наборов защитника: 2 легендарных и 3 эпических героя гарантированы.'),
        hero('mercenary', 'MERCENARY', 50, 'UNITS', BRONZE, icon + 'WarriorPack', gold(5000), UNCOMMON_TIER,
             opened_counter=1, rune_pct=20, rune=(1, 3), bullets=[bullet(chance, 3)]),
        hero('knight', 'KNIGHT', 60, 'UNITS', SILVER, icon + 'GreyWardenKnight', gold(25000), UNCOMMON_TIER,
             opened_counter=1, rune_pct=40, rune=(2, 4), gear_pct=10, bullets=[bullet(chance, 4)]),
        hero('commander', 'COMMANDER', 70, 'UNITS', GOLD, icon + 'CommanderDuhaime', gems(60), RARE_TIER,
             opened_counter=2, rune_pct=100, rune=(3, 5), gear_pct=20, gear=(2, 4), bullets=[bullet(chance, 5)]),
        hero('elite', 'ELITE', 80, 'UNITS', PURPLE, icon + 'GreyWardenFiona', gems(1280), EPIC_TIER, count=10,
             opened_counter=3, legendary=3, rune_pct=100, rune=(4, 5), rune_count=10, gear_pct=50, gear=(3, 5),
             bullets=[bullet('Гарантировано', 5)]),
        hero('iron', 'IRON', 90, 'UNITS', GOLD, 'cache:tex.Store_IronPack', [voucher('iron')], RARE_TIER,
             opened_counter=2, pity=6, rune_pct=50, rune=(2, 4), bullets=[bullet('Гарантировано', 5)]),
        hero('bloodstone', 'BLOODSTONE', 100, 'UNITS', GOLD, icon + 'BloodstonePack', [voucher('bloodstone')],
             EPIC_TIER, opened_counter=2, rune_pct=50, rune=(3, 5), bullets=[bullet('Гарантировано', 4)]),
        hero('lyrium_vein', 'LYRIUM_MINE', 110, 'UNITS', GREEN, 'gui/store/tex.Voucher_LyriumMine.png',
             [voucher('lyrium_mine')], RARE_TIER, opened_counter=2, rune_pct=40, rune=(3, 4),
             bullets=[bullet(chance, 5)]),
        hero('starter_conversion', 'MINNOW_CONVERSION_V2', 120, 'UNITS', BRONZE, icon + 'SerJory', gems(121),
             RARE_TIER, count=3, opened_counter=2, epic=1, rune_pct=100, rune=(3, 4), rune_count=3,
             bullets=[bullet('Особое предложение!')]),
        hero('starter_beasts', 'STARTER_PACK_BEASTS', 130, 'UNITS', BRONZE, icon + 'Dragonling', gems(275),
             RARE_TIER, count=5, opened_counter=2, epic=2, rune_pct=100, rune=(3, 4), rune_count=5,
             bullets=[bullet('Особое предложение!')]),
        hero('wisp', 'WISP', 140, 'UNITS', GREEN, icon + 'RuneWIsp', gold(800), UNCOMMON_TIER,
             bullets=[bullet('Для обучения')]),
        hero('divine_wisp', 'DIVINE_WISP', 150, 'UNITS', GREEN, icon + 'DivineWisp', gems(40), RARE_TIER,
             opened_counter=2, bullets=[bullet('Для обучения', 4)]),
        hero('training', 'ITEM_CONSUME_IDENTIFIER', 160, 'UNITS', GREEN, icon + 'RuneWIsp', gold(300), COMMON_TIER,
             count=3, bullets=[bullet('Для обучения')]),
        hero('free_wisp', 'ITEM_FREE_WISP_PROMO', 170, 'UNITS', GREEN, icon + 'Snowball', [offer('gold', 0)],
             COMMON_TIER, schedule=season('01-01', '01-07'), bullets=[bullet('Подарок')]),
        hero('booster_blackwhite', 'HERO_BOOSTER_BLACKWHITE', 180, 'UNITS', BLUE, 'cache:tex.Store_BlackWhitePack',
             gems(45), RARE_TIER, opened_counter=2, bullets=[bullet(chance, 4)]),
        hero('booster_redblue', 'HERO_BOOSTER_REDBLUE', 190, 'UNITS', BLUE, 'cache:tex.Store_BlueRedPack', gems(45),
             RARE_TIER, opened_counter=2, bullets=[bullet(chance, 4)]),
        hero('booster_giant', 'HERO_BOOSTER_GIANT', 200, 'UNITS', BLUE, icon + 'AbyssalHighDragon', gems(45),
             RARE_TIER, opened_counter=2, bullets=[bullet(chance, 4)]),
        hero('booster_prestige', 'HERO_BOOSTER_PRESTIGE', 210, 'UNITS', BLUE, icon + 'SpiritofFaith', gems(90),
             EPIC_TIER, opened_counter=2, bullets=[bullet(chance, 5)]),
    ]
    seasonal = (
        ('goat', 'GOAT', 'cache:tex.Voucher_Goat', 'goat', ('04-01', '04-14'), 'Осторожно, козы!'),
        ('nug', 'NUG', icon + 'GoldenNug', 'nug', ('04-15', '04-30'), 'Осторожно, наги!'),
        ('dragon', 'DRAGON', icon + 'DragonMalvernis', 'dragon', ('08-01', '08-21'), 'Шанс на дракона'),
        ('romance', 'ROMANCE', 'gui/store/tex.Voucher_Romance.png', 'romance', ('02-01', '02-20'), 'Отрада сердца'),
        ('wintersend', 'WINTERSEND', icon + 'WintersEndPack', 'wintersend', ('12-10', '01-15'), 'Украсьте залы'),
        ('harvest', 'HARVEST', 'gui/store/tex.Voucher_Harvest.png', 'harvest', ('10-01', '10-31'), 'Шанс на'),
        ('anniversary', 'ANNIVERSARY', 'gui/store/tex.Voucher_Anniversary.png', 'anniversary', ('12-01', '12-09'),
         'Шанс на'),
        ('library', 'LIBRARY', 'gui/store/tex.Voucher_Library.png', 'library', ('09-01', '09-21'), 'Шанс на'),
    )
    for index, (key, loc, art, ticket, dates, text) in enumerate(seasonal):
        rows.append(hero(key, loc, 300 + index * 10, 'UNITS', GREEN, art, [voucher(ticket)], RARE_TIER,
                         opened_counter=2, schedule=season(*dates), rune_pct=40, rune=(3, 4),
                         bullets=[bullet(text, 4)]))
    rows.append(hero('mac_tir', 'TEMP_MAC_TIR', 380, 'UNITS', GREEN, icon + 'Maryden', gems(75), RARE_TIER,
                     opened_counter=2, schedule=season('06-01', '06-21'), bullets=[bullet(chance, 5)]))
    rows.append(hero('new_release', 'NEW_RELEASE', 390, 'UNITS', GREEN, icon + 'FlashPack', gems(75), RARE_TIER,
                     opened_counter=2, schedule=season('05-01', '05-21'), bullets=[bullet(chance, 5)]))
    paid = (
        ('heroic', 'MTX_PACK_HEROIC', icon + 'GoldPack', 150, 5, RARE_TIER, 0, 1),
        ('legendary', 'MTX_PACK_LEGENDARY', icon + 'LegendaryPack', 360, 5, BULK_TIER, 1, 1),
        ('ultra', 'MTX_PACK_ULTRA', 'cache:tex.PackPromo_Darkspawn', 720, 5, EPIC_TIER, 2, 2),
        ('ultimate', 'ITEM_ULTIMATE_IDENTIFIER_V2', 'cache:tex.PackPromo_SebastianVael', 1600, 10, EPIC_TIER, 4, 2),
        ('special', 'SPECIAL', 'cache:tex.flashpack', 275, 3, RARE_TIER, 1, 0),
    )
    for index, (key, loc, art, price, count, tier, legendary, epic) in enumerate(paid):
        rows.append(hero(key, loc, 400 + index * 10, 'FEATURED', FEATURED, art, gems(price), tier, count=count,
                         opened_counter=3, legendary=legendary, epic=epic, rune_pct=100, rune=(3, 5),
                         rune_count=count, gear_pct=40, gear=(2, 5), bullets=[bullet('Гарантировано', 5)]))
    flash = ('FLASH', 'FLASH_BLACK_FRIDAY', 'FLASH_DRAGON_HUNT', 'FLASH_EXCLUSIVELY', 'FLASH_FINE_DWARVEN_CRAFTS',
             'FLASH_FIREWORKS', 'FLASH_GOOD_COMPANY', 'FLASH_STAR_STRUCK', 'FLASH_STOCKING_STUFFERS',
             'FLASH_UNIVERSITY_SUPPLIES', 'FLASH_VALENTINES_GIFTS')
    for index, loc in enumerate(flash):
        rows.append(hero(loc.lower(), loc, 500 + index, 'FEATURED', FEATURED, icon + 'FlashPack', gems(90), RARE_TIER,
                         count=2, opened_counter=2, epic=1, rotation=weekly(index), rune_pct=100, rune=(3, 4),
                         rune_count=2, gear_pct=25, gear=(2, 4), bullets=[bullet('Только на этой неделе!', 4)]))
    return rows


def rune_packs():
    knowledge = ['rune-knowledge']
    return [
        rune('speed_kills', 'SPEED_KILLS_RUNE', 610, GREEN, 'cache:tex.Store_IconSpeedRune', [offer('gems', 13)],
             [slot(['rune-initiative'], 5, 6), slot(RUNE_FAMILIES[:3], 4, 5), slot(RUNE_FAMILIES[:3], 4, 5)],
             [bullet('Гарантирована руна скорости!')], 'Руна скорости и две другие руны.'),
        rune('ancient_lore', 'ANCIENT_LORE_RUNE', 620, GREEN, 'cache:tex.Store_IconXPRune', [offer('gems', 13)],
             [slot(knowledge, 5, 6), slot(knowledge, 4, 5), slot(knowledge, 4, 5)],
             [bullet('3 руны знаний')], 'Три руны знаний.'),
        rune('grab_bag', 'GRAB_BAG_RUNE', 630, GOLD, 'cache:tex.Store_IconRuneWIsp', [offer('gold', 10000)],
             [slot(knowledge, 3, 5), slot(RUNE_FAMILIES, 3, 5), slot(RUNE_FAMILIES, 3, 5)],
             [bullet('3 редкие или лучше руны')], 'Три руны редкого качества или выше.'),
        rune('legendary_runes', 'LEGENDARY_RUNE', 640, PURPLE, 'cache:tex.Store_RuneCert', [offer('gems', 120)],
             [slot([], 5, 5), slot([], 5, 5), slot([], 5, 5)],
             [bullet('3 легендарные руны', 5)], 'Три легендарные руны.'),
        rune('prestige_runes', 'PRESTIGE_RUNE', 650, PURPLE, 'cache:tex.Store_IconGoldRune', [offer('gems', 150)],
             [slot([], 7, 7)], [bullet('1 престижная руна', 5)], 'Одна престижная руна для особой ячейки отряда.'),
    ]


def gear_packs():
    return [
        gear('peasant', 'PEASANT_GEAR', 700, BRONZE, 'gui/gear/tex.Ico_Gear_QunariWaraxe.png', [offer('gold', 2000)], 1, (1, 2),
             [bullet('Шанс на', 2)], '1 предмет снаряжения.'),
        gear('novice', 'NOVICE_GEAR', 710, GREEN, 'gui/gear/tex.Ico_Gear_QunariWaraxe.png', [offer('gold', 30000)], 1, (1, 3),
             [bullet('Шанс на', 3)], '1 предмет снаряжения.'),
        gear('journeyman', 'JOURNEYMAN_GEAR', 720, PURPLE, 'gui/gear/tex.Ico_Gear_TomeOfKoslun.png', [offer('gems', 22)], 2,
             (2, 3), [bullet('Шанс на', 4)], '2 предмета снаряжения.', tome_pct=2),
        gear('merchant', 'MERCHANT_GEAR', 730, PURPLE, 'gui/gear/tex.Ico_Gear_TomeOfRifts.png', [offer('gems', 35)], 3,
             (2, 4), [bullet('Шанс на', 4)], '3 предмета снаряжения.', tome_pct=5),
        gear('masterwork', 'MASTERWORK_GEAR', 740, PURPLE, 'gui/gear/tex.Ico_Gear_BowOfTheRighteous.png', [offer('gems', 55)], 3,
             (3, 5), [bullet('Гарантировано', 5)], '3 предмета снаряжения.', tome_pct=10),
        gear('armory', 'ARMORY_GEAR', 750, GOLD, 'gui/gear/tex.Ico_Gear_AxeOfTheDragonHunter.png', [offer('gems', 150)], 5, (4, 6),
             [bullet('Гарантировано', 5)], '5 предметов снаряжения.', tome_pct=25),
    ]


def pack_info(pack, russian):
    key = f"{pack['loc_id']}_PACK_INFO"
    text = pack.pop('info_ru') or russian.get(key)
    if text:
        return text
    count = pack.get('hero_count', 1)
    return f'Дает героев: {count}. Как минимум {RARITY_RU[pack["minimum_rarity"]]} боец.'


RARITY_RU = {'Common': 'обычный', 'Uncommon': 'необычный', 'Rare': 'редкий', 'Epic': 'эпический',
             'Legendary': 'легендарный'}


def build(russian):
    packs = hero_packs() + rune_packs() + gear_packs()
    for pack in packs:
        pack['name_ru'] = russian.get(f"{pack['loc_id']}_PACK_NAME", pack['id'])
        pack['info_ru'] = pack_info(pack, russian)
    return {
        'version': 3,
        'balance_note_ru': BALANCE_NOTE,
        'flash_days': WEEKLY_FLASH_DAYS,
        'vouchers': [{'id': key, 'icon': icon} for key, icon in VOUCHERS],
        'gem_skus': GEM_SKUS,
        'packs': packs,
    }


def validate(catalog):
    errors = []
    tickets = {row['id'] for row in catalog['vouchers']}
    seen = set()
    for pack in catalog['packs']:
        if pack['id'] in seen:
            errors.append(f"duplicate pack {pack['id']}")
        seen.add(pack['id'])
        refs = [pack['header'], pack['icon']]
        for ref in refs:
            if ref.startswith('cache:') and not original_cache.resolves(ref, 28):
                errors.append(f"{pack['id']}: {ref} is not in the cache")
        for row in pack['offers']:
            if row['currency'] == 'voucher' and row['voucher'] not in tickets:
                errors.append(f"{pack['id']}: unknown voucher {row['voucher']}")
        if pack['kind'] == 'hero' and sum(pack['weights'].values()) != 10000:
            errors.append(f"{pack['id']}: weights do not total 10000")
    return errors


def build_art():
    EXTRA_STORE_DIR.mkdir(parents=True, exist_ok=True)
    for _, icon in VOUCHERS:
        if icon.startswith('gui/store/'):
            name = Path(icon).name
            (EXTRA_STORE_DIR / name).write_bytes((ASSET_PACK_IMAGES / name).read_bytes())
    for name, (frame, box) in TILE_CROPS.items():
        crop = Image.open(FOOTAGE / frame).convert('RGB').crop(box)
        width = round(crop.width * FOOTAGE_ASPECT_CORRECTION * TILE_ART_HEIGHT / crop.height)
        crop.resize((width, TILE_ART_HEIGHT), Image.Resampling.LANCZOS).save(EXTRA_STORE_DIR / name)


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail if the catalog on disk is stale')
    parser.add_argument('--art', action='store_true', help='also rebuild the git-ignored store art')
    args = parser.parse_args()
    russian = json.loads(LOCALIZATION.read_text(encoding='utf-8'))
    catalog = build(russian)
    errors = validate(catalog)
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    text = json.dumps(catalog, ensure_ascii=False, indent=2) + '\n'
    if args.check:
        if OUTPUT.read_text(encoding='utf-8') != text:
            print('data/store_packs.json is stale; run tools/build_store_catalog.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    if args.art:
        build_art()
    print(f"{len(catalog['packs'])} packs")
    return 0


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import economy_localization as economy
import original_cache

CLI_DESCRIPTION = 'Build data/store_packs.json, the single pack catalog, and the store art the 4.4.0 cache lacks.'

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'data' / 'store_packs.json'
CHARACTERS = ROOT / 'client-additional-data' / 'characters.json'
FOOTAGE = ROOT / 'client-additional-data' / 'store_research' / 'footage'
EXTRA_STORE_DIR = ROOT / 'client-additional-data' / 'extra' / 'gui' / 'store'
ASSET_PACK_IMAGES = Path.home() / 'Downloads' / 'HoDA_game_assets_pack' / 'images'

BALANCE_NOTE = (
    'The server sent the store catalog remotely and the dump lacks it. Pack names come from the 5.4.4 localization. '
    'Prices seen in recordings are kept; every other price, odds, contents, schedule and description is local '
    'balance. The gem ladder is restored exactly from the localization strings.'
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
CHAMPION_GOLD = 'cache:tex.Store_HeaderChampionPack'
BLUE = 'cache:tex.Store_HeaderBGBlueGreenPack'
FEATURED = 'cache:tex.Store_HeaderBGFeaturedPack'
PURPLE_TINT_OVER_SILVER = '#B070FF'

COMMON_TIER = {'Common': 5500, 'Uncommon': 3800, 'Rare': 650, 'Epic': 50, 'Legendary': 0}
UNCOMMON_TIER = {'Common': 0, 'Uncommon': 6000, 'Rare': 3600, 'Epic': 400, 'Legendary': 0}
RARE_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 8400, 'Epic': 1500, 'Legendary': 100}
BULK_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 8000, 'Epic': 1850, 'Legendary': 150}
EPIC_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 0, 'Epic': 9700, 'Legendary': 300}
WISP_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 6600, 'Epic': 2700, 'Legendary': 700}
LEGENDARY_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 0, 'Epic': 0, 'Legendary': 10000}
UNCOMMON_ONLY_TIER = {'Common': 0, 'Uncommon': 10000, 'Rare': 0, 'Epic': 0, 'Legendary': 0}
RARE_ONLY_TIER = {'Common': 0, 'Uncommon': 0, 'Rare': 10000, 'Epic': 0, 'Legendary': 0}
MINIMUM = {id(COMMON_TIER): 'Common', id(UNCOMMON_TIER): 'Uncommon', id(RARE_TIER): 'Rare', id(BULK_TIER): 'Rare',
           id(WISP_TIER): 'Rare',
           id(EPIC_TIER): 'Epic', id(LEGENDARY_TIER): 'Legendary', id(UNCOMMON_ONLY_TIER): 'Uncommon',
           id(RARE_ONLY_TIER): 'Rare'}

RARITY_NUMBER = {'Common': 1, 'Uncommon': 2, 'Rare': 3, 'Epic': 4, 'Legendary': 5}
WISP_POOL = 'WISP'
GOAT_POOL = 'GOAT'

RUNE_FAMILIES = ['rune-power', 'rune-health', 'rune-initiative', 'rune-stun']

WEEKLY_FLASH_DAYS = 7
FLASH_ROTATION = 'flash'
BOOSTER_ROTATION = 'booster'

CHANCE_AT = 'HODA_STORE_BULLET_CHANCE_AT'
GUARANTEED = 'FEATURED_INTERSTITIAL_GUARANTEED'
TRAINING = 'ROSTER_FILTER_TUTORIAL'
TEN_FOR_THE_PRICE_OF_NINE = 'gcp_rotating_text_3'
THIS_WEEK_ONLY = 'HODA_STORE_BULLET_THIS_WEEK_ONLY'


def pack_text(loc, fallback_info):
    name, info, description = (f'{loc}_PACK_{part}' for part in ('NAME', 'INFO', 'DESCRIPTION'))
    text = {
        'name_key': name if economy.has_original(name) else economy.project_key(name),
        'info_key': info if economy.has_original(info) else fallback_info,
    }
    if economy.has_original(description):
        text['description_key'] = description
    return text


def hero_info(count, tier):
    return f'HODA_STORE_HERO_PACK_INFO_{count}_{MINIMUM[id(tier)].upper()}'


def gear_info(count):
    return f'HODA_STORE_GEAR_PACK_INFO_{count}'


def offer(currency, price):
    return {'currency': currency, 'price': price}


def voucher(name):
    return {'currency': 'voucher', 'voucher': name, 'price': 1}


def hero(key, loc, order, category, header, icon, offers, tier, count=1, bullets=(), info=None, opened_counter=0,
         schedule=None, rotation=None, epic=0, legendary=0, pity=0, rune_pct=0, rune=(1, 2), rune_count=1,
         gear_pct=0, gear=(1, 3), rare=0, fixed=(), pool=None, unique=False, header_tint=None):
    optional = {'guaranteed_rare': rare, 'fixed_heroes': list(fixed), 'pool_tag': pool, 'unique_heroes': unique,
                'header_tint': header_tint}
    return {
        'id': key, 'kind': 'hero', **pack_text(loc, info or hero_info(count, tier)), 'category': category,
        'order': order, 'header': header, 'icon': icon, 'offers': offers, 'bullets': list(bullets), 'schedule': schedule,
        'rotation': rotation, 'opened_counter': opened_counter, 'hero_count': count,
        'minimum_rarity': MINIMUM[id(tier)], 'guaranteed_legendary': legendary, 'guaranteed_epic': epic,
        'legendary_pity': pity, 'weights': tier, 'rune_chance_pct': rune_pct, 'rune_min_rarity': rune[0],
        'rune_max_rarity': rune[1], 'rune_count': rune_count, 'gear_chance_pct': gear_pct,
        'gear_min_rarity': gear[0], 'gear_max_rarity': gear[1],
        **{key: value for key, value in optional.items() if value},
    }


def rune(key, loc, order, header, icon, offers, slots, bullets, info=None, schedule=None):
    return {
        'id': key, 'kind': 'rune', **pack_text(loc, info), 'category': 'ITEMS', 'order': order, 'header': header,
        'icon': icon, 'offers': offers, 'bullets': bullets, 'schedule': schedule, 'rotation': None, 'slots': slots,
    }


def gear(key, loc, order, header, icon, offers, count, rarity, bullets, tome_pct=0):
    return {
        'id': key, 'kind': 'gear', **pack_text(loc, gear_info(count)), 'category': 'ITEMS', 'order': order,
        'header': header, 'icon': icon, 'offers': offers, 'bullets': bullets, 'schedule': None, 'rotation': None,
        'count': count,
        'min_rarity': rarity[0], 'max_rarity': rarity[1], 'tome_chance_pct': tome_pct,
    }


def bullet(key, rarity=0):
    return {'text_key': key, 'rarity': rarity}


def season(start, end):
    return {'start': start, 'end': end}


def slot(families, low, high):
    return {'families': families, 'min_rarity': low, 'max_rarity': high}


def weekly(group, index):
    return {'group': group, 'index': index}


TUTORIAL_CONSUME_ICON = 'cache:tex.Store_IconGreyWardenKnight'


def tutorial_consume_pack():
    row = hero('tutorial_consume', 'TUTORIAL_CONSUME', 5, 'UNITS', GOLD, TUTORIAL_CONSUME_ICON,
               [offer('gold', 0)], RARE_TIER, count=2, bullets=[bullet(TRAINING)],
               info='HODA_TUTORIAL_CONSUME_PACK_INFO')
    row['tutorial_only'] = True
    return row


def hero_packs():
    gold, gems = (lambda p: [offer('gold', p)]), (lambda p: [offer('gems', p)])
    icon = 'cache:tex.Store_Icon'
    chance = CHANCE_AT
    rows = [
        hero('recruit', 'ITEM_BRONZE_IDENTIFIER', 10, 'UNITS', BRONZE, icon + 'BronzePack',
             gold(350) + [voucher('bronze')], COMMON_TIER, rune_pct=15),
        hero('soldier', 'ITEM_SILVER_IDENTIFIER', 20, 'UNITS', SILVER, icon + 'SilverPack',
             gold(1500) + [voucher('silver')], UNCOMMON_TIER, opened_counter=1, rune_pct=30, rune=(2, 3)),
        hero('champion', 'ITEM_GOLD_IDENTIFIER', 30, 'UNITS', CHAMPION_GOLD, icon + 'GoldPack',
             gems(30) + [voucher('champion')], RARE_TIER, opened_counter=2, rune_pct=100, rune=(3, 5),
             bullets=[bullet(chance, 5)]),
        hero('grand_champion', 'GRAND_CHAMPION', 40, 'UNITS', 'cache:tex.Store_HeaderGrandChampionPack',
             icon + 'LegendaryPack', gems(270), BULK_TIER, count=10, opened_counter=3, epic=3,
             rune_pct=100, rune=(3, 5), rune_count=10,
             bullets=[bullet(TEN_FOR_THE_PRICE_OF_NINE), bullet('HODA_STORE_BULLET_GRAND_CHAMPION', 4)],
             info='HODA_GRAND_CHAMPION_PACK_INFO'),
        hero('mercenary', 'MERCENARY', 50, 'UNITS', SILVER, icon + 'WarriorPack', gold(5000), UNCOMMON_TIER,
             header_tint=PURPLE_TINT_OVER_SILVER, opened_counter=1, rune_pct=20, rune=(1, 3), bullets=[bullet(chance, 3)]),
        hero('knight', 'KNIGHT', 60, 'UNITS', SILVER, icon + 'GreyWardenKnight', gold(25000), RARE_TIER,
             opened_counter=1, rune_pct=100, rune=(2, 5), bullets=[bullet(chance, 5)]),
        hero('iron', 'IRON', 90, 'UNITS', GOLD, 'cache:tex.Store_IronPack', [voucher('iron')], RARE_TIER,
             opened_counter=2, rune_pct=50, rune=(2, 4), bullets=[bullet(chance, 5)]),
        hero('bloodstone', 'BLOODSTONE', 100, 'UNITS', GOLD, icon + 'BloodstonePack', [voucher('bloodstone')],
             LEGENDARY_TIER, opened_counter=2, rune_pct=50, rune=(3, 5), bullets=[bullet(GUARANTEED, 5)]),
        hero('lyrium_vein', 'LYRIUM_MINE', 110, 'UNITS', GREEN, 'gui/store/tex.Voucher_LyriumMine.png',
             [voucher('lyrium_mine')], RARE_TIER, opened_counter=2, rune_pct=40, rune=(3, 4),
             bullets=[bullet(chance, 5)]),
        hero('wisp', 'WISP', 140, 'UNITS', GREEN, icon + 'RuneWIsp', gold(800), WISP_TIER,
             count=10, epic=1, pool=WISP_POOL, bullets=[bullet(TRAINING)]),
        hero('divine_wisp', 'DIVINE_WISP', 150, 'UNITS', GREEN, icon + 'DivineWisp', gems(40), WISP_TIER,
             count=10, legendary=1, epic=2, pool=WISP_POOL, opened_counter=2, bullets=[bullet(TRAINING, 4)]),
        hero('training', 'ITEM_CONSUME_IDENTIFIER', 160, 'UNITS', GREEN, icon + 'RuneWIsp', gold(300), UNCOMMON_ONLY_TIER,
             count=2, rare=1, bullets=[bullet(TRAINING)]),
        hero('free_wisp', 'ITEM_FREE_WISP_PROMO', 170, 'UNITS', GREEN, icon + 'Snowball', [offer('gold', 0)],
             RARE_ONLY_TIER, count=4, pool=WISP_POOL, unique=True, rune_pct=100, rune=(3, 3), rune_count=4, schedule=season('01-01', '01-07'), bullets=[bullet('HODA_STORE_BULLET_GIFT')]),
        hero('booster_blackwhite', 'HERO_BOOSTER_BLACKWHITE', 180, 'UNITS', BLUE, 'cache:tex.Store_BlackWhitePack',
             gems(45), RARE_TIER, opened_counter=2, rotation=weekly(BOOSTER_ROTATION, 0),
             bullets=[bullet(chance, 4), bullet(THIS_WEEK_ONLY)]),
        hero('booster_redblue', 'HERO_BOOSTER_REDBLUE', 190, 'UNITS', BLUE, 'cache:tex.Store_BlueRedPack', gems(45),
             RARE_TIER, opened_counter=2, rotation=weekly(BOOSTER_ROTATION, 1),
             bullets=[bullet(chance, 4), bullet(THIS_WEEK_ONLY)]),
        hero('booster_giant', 'HERO_BOOSTER_GIANT', 200, 'UNITS', BLUE, icon + 'AbyssalHighDragon', gems(45),
             RARE_TIER, opened_counter=2, rotation=weekly(BOOSTER_ROTATION, 2),
             bullets=[bullet(chance, 4), bullet(THIS_WEEK_ONLY)]),
        hero('booster_prestige', 'HERO_BOOSTER_PRESTIGE', 210, 'UNITS', BLUE, icon + 'SpiritofFaith', gems(90),
             EPIC_TIER, opened_counter=2, rotation=weekly(BOOSTER_ROTATION, 3),
             bullets=[bullet(chance, 5), bullet(THIS_WEEK_ONLY)]),
    ]
    seasonal = (
        ('goat', 'GOAT', 'cache:tex.Voucher_Goat', 'goat', ('04-01', '04-14'), None),
        ('nug', 'NUG', icon + 'GoldenNug', 'nug', ('04-15', '04-30'), 'HODA_STORE_BULLET_NUGS'),
        ('dragon', 'DRAGON', icon + 'DragonMalvernis', 'dragon', ('08-01', '08-21'), 'HODA_STORE_BULLET_DRAGON_CHANCE'),
        ('romance', 'ROMANCE', 'gui/store/tex.Voucher_Romance.png', 'romance', ('02-01', '02-20'),
         'HODA_STORE_BULLET_ROMANCE'),
        ('wintersend', 'WINTERSEND', icon + 'WintersEndPack', 'wintersend', ('12-10', '01-15'),
         'HODA_STORE_BULLET_WINTERSEND'),
        ('harvest', 'HARVEST', 'gui/store/tex.Voucher_Harvest.png', 'harvest', ('10-01', '10-31'), CHANCE_AT),
        ('anniversary', 'ANNIVERSARY', 'gui/store/tex.Voucher_Anniversary.png', 'anniversary', ('12-01', '12-09'),
         CHANCE_AT),
        ('library', 'LIBRARY', 'gui/store/tex.Voucher_Library.png', 'library', ('09-01', '09-21'), CHANCE_AT),
    )
    pools = {'goat': GOAT_POOL}
    for index, (key, loc, art, ticket, dates, text_key) in enumerate(seasonal):
        pool = pools.get(key)
        rows.append(hero(key, loc, 300 + index * 10, 'UNITS', GREEN, art, [voucher(ticket)],
                         COMMON_TIER if pool else RARE_TIER, pool=pool, opened_counter=2, schedule=season(*dates),
                         rune_pct=40, rune=(3, 4), bullets=[bullet(text_key, 4)] if text_key else []))
    rows.append(hero('mac_tir', 'TEMP_MAC_TIR', 380, 'UNITS', GREEN, icon + 'Maryden', gems(75), RARE_TIER,
                     opened_counter=2, schedule=season('06-01', '06-21'), bullets=[bullet(chance, 5)]))
    rows.append(hero('new_release', 'NEW_RELEASE', 390, 'UNITS', GREEN, icon + 'FlashPack', gems(75), RARE_TIER,
                     opened_counter=2, schedule=season('05-01', '05-21'), bullets=[bullet(chance, 5)]))
    paid = (
        ('legendary', 'MTX_PACK_LEGENDARY', 410, icon + 'LegendaryPack', 360, 5, BULK_TIER, 1, 1),
        ('ultimate', 'ITEM_ULTIMATE_IDENTIFIER_V2', 430, 'cache:tex.PackPromo_SebastianVael', 1600, 10, EPIC_TIER, 2, 2),
    )
    for key, loc, order, art, price, count, tier, legendary, epic in paid:
        rows.append(hero(key, loc, order, 'FEATURED', FEATURED, art, gems(price), tier, count=count,
                         opened_counter=3, legendary=legendary, epic=epic, rune_pct=100, rune=(3, 5),
                         rune_count=count, gear_pct=40, gear=(2, 5),
                         bullets=[bullet(GUARANTEED, RARITY_NUMBER['Legendary' if legendary else 'Epic'])]))
    flash = ('FLASH', 'FLASH_BLACK_FRIDAY', 'FLASH_DRAGON_HUNT', 'FLASH_EXCLUSIVELY', 'FLASH_FINE_DWARVEN_CRAFTS',
             'FLASH_FIREWORKS', 'FLASH_GOOD_COMPANY', 'FLASH_STAR_STRUCK', 'FLASH_STOCKING_STUFFERS',
             'FLASH_UNIVERSITY_SUPPLIES', 'FLASH_VALENTINES_GIFTS')
    for index, loc in enumerate(flash):
        rows.append(hero(loc.lower(), loc, 500 + index, 'FEATURED', FEATURED, icon + 'FlashPack', gems(90), RARE_TIER,
                         count=2, opened_counter=2, epic=1, rotation=weekly(FLASH_ROTATION, index), rune_pct=100, rune=(3, 4),
                         rune_count=2, gear_pct=25, gear=(2, 4), bullets=[bullet(THIS_WEEK_ONLY, 4)]))
    return rows


def rune_packs():
    knowledge = ['rune-knowledge']
    return [
        rune('speed_kills', 'SPEED_KILLS_RUNE', 610, GREEN, 'cache:tex.Store_IconSpeedRune', [offer('gems', 13)],
             [slot(['rune-initiative'], 5, 6), slot(RUNE_FAMILIES[:3], 4, 5), slot(RUNE_FAMILIES[:3], 4, 5)], []),
        rune('ancient_lore', 'ANCIENT_LORE_RUNE', 620, GREEN, 'cache:tex.Store_IconXPRune', [offer('gems', 13)],
             [slot(knowledge, 5, 6), slot(knowledge, 4, 5), slot(knowledge, 4, 5)], []),
        rune('grab_bag', 'GRAB_BAG_RUNE', 630, GOLD, 'cache:tex.Store_IconRuneWIsp', [offer('gold', 10000)],
             [slot(knowledge, 3, 5), slot(RUNE_FAMILIES, 3, 5), slot(RUNE_FAMILIES, 3, 5)], []),
        rune('legendary_runes', 'LEGENDARY_RUNE', 640, CHAMPION_GOLD, 'cache:tex.Store_RuneCert', [offer('gems', 120)],
             [slot([], 5, 5), slot([], 5, 5), slot([], 5, 5)], []),
        rune('prestige_runes', 'PRESTIGE_RUNE', 650, CHAMPION_GOLD, 'cache:tex.Store_IconGoldRune', [offer('gems', 150)],
             [slot([], 7, 7)], [bullet('HODA_STORE_BULLET_PRESTIGE_RUNE', 5)], info='HODA_PRESTIGE_RUNE_PACK_INFO'),
    ]


def gear_packs():
    return [
        gear('peasant', 'PEASANT_GEAR', 700, BRONZE, 'gui/gear/tex.Ico_Gear_QunariWaraxe.png', [offer('gold', 2000)], 1, (1, 2),
             [bullet(CHANCE_AT, 2)]),
        gear('novice', 'NOVICE_GEAR', 710, GREEN, 'gui/gear/tex.Ico_Gear_QunariWaraxe.png', [offer('gold', 30000)], 1, (1, 3),
             [bullet(CHANCE_AT, 3)]),
        gear('journeyman', 'JOURNEYMAN_GEAR', 720, CHAMPION_GOLD, 'gui/gear/tex.Ico_Gear_TomeOfKoslun.png', [offer('gems', 22)], 2,
             (2, 3), [bullet(CHANCE_AT, 4)], tome_pct=2),
        gear('merchant', 'MERCHANT_GEAR', 730, CHAMPION_GOLD, 'gui/gear/tex.Ico_Gear_TomeOfRifts.png', [offer('gems', 35)], 3,
             (2, 4), [bullet(CHANCE_AT, 4)], tome_pct=5),
        gear('masterwork', 'MASTERWORK_GEAR', 740, CHAMPION_GOLD, 'gui/gear/tex.Ico_Gear_BowOfTheRighteous.png', [offer('gems', 55)], 3,
             (3, 5), [bullet(GUARANTEED, 5)], tome_pct=10),
        gear('armory', 'ARMORY_GEAR', 750, GOLD, 'gui/gear/tex.Ico_Gear_AxeOfTheDragonHunter.png', [offer('gems', 150)], 5, (4, 6),
             [bullet(GUARANTEED, 5)], tome_pct=25),
    ]


def build():
    packs = [tutorial_consume_pack()] + hero_packs() + rune_packs() + gear_packs()
    return {
        'version': 3,
        'balance_note': BALANCE_NOTE,
        'flash_days': WEEKLY_FLASH_DAYS,
        'vouchers': [{'id': key, 'icon': icon} for key, icon in VOUCHERS],
        'gem_skus': GEM_SKUS,
        'packs': packs,
    }


def text_keys(catalog):
    for pack in catalog['packs']:
        yield from (pack[field] for field in ('name_key', 'info_key', 'description_key') if field in pack)
        yield from (row['text_key'] for row in pack['bullets'])
    for sku in catalog['gem_skus']:
        yield f"{sku['id']}_cost"
        if 'banner' in sku:
            yield sku['banner']


def validate(catalog):
    errors = []
    heroes = {row['id'] for row in json.loads(CHARACTERS.read_text(encoding='utf-8'))['characters']}
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
        for unit in pack.get('fixed_heroes', []):
            if unit not in heroes:
                errors.append(f"{pack['id']}: fixed hero {unit} is not in the catalog")
        if pack['kind'] == 'hero' and sum(pack['weights'].values()) != 10000:
            errors.append(f"{pack['id']}: weights do not total 10000")
        if not pack['bullets'] and 'description_key' not in pack:
            errors.append(f"{pack['id']}: needs a description or a bullet")
    for group in {pack['rotation']['group'] for pack in catalog['packs'] if pack['rotation']}:
        indexes = sorted(pack['rotation']['index'] for pack in catalog['packs']
                         if pack['rotation'] and pack['rotation']['group'] == group)
        if indexes != list(range(len(indexes))):
            errors.append(f'rotation {group} indexes are not 0..{len(indexes) - 1}')
    errors += [f'unknown localization key {key}' for key in economy.unknown_keys(text_keys(catalog))]
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
    catalog = build()
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

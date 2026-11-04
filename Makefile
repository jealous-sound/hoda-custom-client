CC      = cc
CFLAGS  = -Wall -O2 -std=c99 -I/opt/homebrew/opt/raylib/include
LDFLAGS = -L/opt/homebrew/opt/raylib/lib
LIBS    = -lraylib -framework Cocoa -framework IOKit -framework OpenGL

TARGET  = game
SRC     = source/main.c
DEPS    = $(wildcard source/*.c source/*.h source/vendors/*.c source/vendors/*.h)
TEST_TARGET = tests/battle_sim_test
BACKEND_DIR ?= ../hoda-custom-backend

$(TARGET): $(DEPS) Makefile
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

$(TEST_TARGET): tests/battle_sim_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/battle_sim_test.c -o $(TEST_TARGET) $(LDFLAGS) $(LIBS)

test: $(TEST_TARGET)
	python3 client-additional-data/validate_characters.py
	python3 -m unittest tests/test_current_tier_models.py tests/test_import_fbx.py tests/test_pvp_rewards.py
	python3 tools/audit_character_models.py --deep
	python3 tools/validate_ability_recipes.py
	python3 tools/validate_intrinsic_passives.py
	python3 tools/validate_unit_audio.py
	python3 tools/validate_unit_vfx.py
	python3 tools/validate_gear.py
	python3 tools/build_store_catalog.py --check
	python3 tools/validate_cache_refs.py
	python3 tools/build_campaign_node_layout.py --check
	python3 tools/build_runes.py --check
	python3 tools/build_missions.py --check
	python3 tools/build_news_feed.py --check
	python3 tools/build_pvp_rules.py --check
	python3 tools/build_nexus_game_changers.py --check
	python3 tools/build_consume_tables.py --check
	python3 tools/build_consume_scene.py --check
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

play:
	HOD_CLIENT_DIR="$(CURDIR)" "$(BACKEND_DIR)/scripts/play-local.sh"

clean:
	rm -f $(TARGET) $(TEST_TARGET)

format:
	clang-format -i source/*.c

format-check:
	clang-format --dry-run --Werror source/*.c

.PHONY: run play test clean format format-check

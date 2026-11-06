APP_NAME = Heroes of Dragon Age
HOST_PLATFORM := $(if $(filter Darwin,$(shell uname -s)),macos,linux)
PLATFORM ?= $(HOST_PLATFORM)
RAYLIB_DIR ?= /opt/homebrew/opt/raylib
EXTRA_CFLAGS ?=
RAYLIB_LINK ?= -lraylib
CONTENT_URL ?= https://cdn.hodareborn.com
SERVER_URL ?= https://api.hodareborn.com
CONTENT_BUCKET ?= hoda-assets
CFLAGS  = -Wall -O2 -std=c99 -I$(RAYLIB_DIR)/include -DHOD_CONTENT_URL='"$(CONTENT_URL)"' -DHOD_SERVER_URL='"$(SERVER_URL)"' $(PLATFORM_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS = -L$(RAYLIB_DIR)/lib
SRC     = source/main.c source/platform.c
DEPS    = $(wildcard source/*.c source/*.h source/vendors/*.c source/vendors/*.h)
TEST_TARGET = tests/battle_sim_test
CONTENT_TEST_TARGET = tests/content_update_test
BACKEND_DIR ?= ../hoda-custom-backend
DIST_DIR ?= ../hoda-custom-dist
DIST_BACKEND_URL ?=
APP_ICON = packaging/app_icon.png
.DEFAULT_GOAL := app

ifeq ($(PLATFORM),macos)
LIBS    = $(RAYLIB_LINK) -framework Cocoa -framework IOKit -framework OpenGL -framework CoreAudio -framework CoreVideo
APP_BUNDLE = $(APP_NAME).app
APP_CONTENTS = $(APP_BUNDLE)/Contents
APP_EXECUTABLE = $(APP_CONTENTS)/MacOS/$(APP_NAME)
TARGET  = Heroes\ of\ Dragon\ Age.app/Contents/MacOS/Heroes\ of\ Dragon\ Age
APP_INFO = packaging/macos/Info.plist
ICONSET = build/AppIcon.iconset
ICON_SIZES = 16 32 128 256 512

$(TARGET): $(DEPS) $(APP_INFO) $(APP_ICON) Makefile
	mkdir -p "$(APP_CONTENTS)/MacOS" "$(APP_CONTENTS)/Resources" $(ICONSET)
	$(CC) $(CFLAGS) $(SRC) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
	cp $(APP_INFO) "$(APP_CONTENTS)/Info.plist"
	for size in $(ICON_SIZES); do \
		sips -z $$size $$size $(APP_ICON) --out $(ICONSET)/icon_$${size}x$${size}.png >/dev/null; \
		sips -z $$((size * 2)) $$((size * 2)) $(APP_ICON) --out $(ICONSET)/icon_$${size}x$${size}@2x.png >/dev/null; \
	done
	iconutil -c icns $(ICONSET) -o "$(APP_CONTENTS)/Resources/AppIcon.icns"
	codesign --force --sign - "$(APP_BUNDLE)"
endif

ifeq ($(PLATFORM),linux)
PLATFORM_CFLAGS = -D_DEFAULT_SOURCE
LIBS    = $(RAYLIB_LINK) -lGL -lm -lpthread -ldl -lrt -lX11
APP_EXECUTABLE = build/linux/$(APP_NAME)
TARGET  = build/linux/Heroes\ of\ Dragon\ Age

$(TARGET): $(DEPS) Makefile
	mkdir -p build/linux
	$(CC) $(CFLAGS) $(SRC) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
endif

ifeq ($(PLATFORM),windows)
CC      = x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
LIBS    = $(RAYLIB_LINK) -lopengl32 -lgdi32 -lwinmm -lwinhttp -lpthread -static -mwindows
APP_EXECUTABLE = build/windows/$(APP_NAME).exe
TARGET  = build/windows/Heroes\ of\ Dragon\ Age.exe
WINDOWS_RESOURCES = build/windows/app_resources.o

$(WINDOWS_RESOURCES): packaging/windows/app.rc packaging/windows/app_icon.ico
	mkdir -p build/windows
	$(WINDRES) packaging/windows/app.rc -O coff -o $(WINDOWS_RESOURCES)

$(TARGET): $(DEPS) $(WINDOWS_RESOURCES) Makefile
	$(CC) $(CFLAGS) $(SRC) $(WINDOWS_RESOURCES) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
endif

app: $(TARGET)

$(TEST_TARGET): tests/battle_sim_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/battle_sim_test.c source/platform.c -o $(TEST_TARGET) $(LDFLAGS) $(LIBS)

$(CONTENT_TEST_TARGET): tests/content_update_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/content_update_test.c source/platform.c -o $(CONTENT_TEST_TARGET) $(LDFLAGS) $(LIBS)

test: $(TEST_TARGET) $(CONTENT_TEST_TARGET)
	python3 client-additional-data/validate_characters.py
	python3 -m unittest tests/test_current_tier_models.py tests/test_import_fbx.py tests/test_pvp_rewards.py tests/test_build_dist.py tests/test_build_custom_heroes.py tests/test_content_packs.py
	python3 tools/audit_character_models.py --deep
	python3 tools/validate_ability_recipes.py
	python3 tools/validate_intrinsic_passives.py
	python3 tools/validate_unit_audio.py
	python3 tools/validate_unit_vfx.py
	python3 tools/validate_gear.py
	python3 tools/build_store_catalog.py --check
	python3 tools/build_hero_pool.py --check
	python3 tools/build_custom_heroes.py --check
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
	./$(CONTENT_TEST_TARGET)

run: $(TARGET)
	"./$(APP_EXECUTABLE)"

play:
	HOD_CLIENT_DIR="$(CURDIR)" "$(BACKEND_DIR)/scripts/play-local.sh"

dist: $(TARGET)
	python3 tools/build_dist.py --replace --out "$(DIST_DIR)" --backend-url "$(DIST_BACKEND_URL)"

content:
	python3 tools/publish_content.py --bucket "$(CONTENT_BUCKET)" --content-url "$(CONTENT_URL)"

clean:
	rm -rf "$(APP_NAME).app" build $(TEST_TARGET) $(CONTENT_TEST_TARGET)

format:
	clang-format -i source/*.c

format-check:
	clang-format --dry-run --Werror source/*.c

.PHONY: app run play dist content test clean format format-check

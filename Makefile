APP_NAME = Heroes of Dragon Age
HOST_PLATFORM := $(if $(filter Darwin,$(shell uname -s)),macos,linux)
PLATFORM ?= $(HOST_PLATFORM)
RAYLIB_DIR ?= /opt/homebrew/opt/raylib
EXTRA_CFLAGS ?=
RAYLIB_LINK ?= -lraylib
CONTENT_URL ?= https://cdn.hodareborn.com
SERVER_URL ?= https://api.hodareborn.com
CONTENT_BUCKET ?= hoda-assets
RELEASE_URL ?= https://github.com/jealous-sound/hoda-custom-client/releases/download
RELEASE_PUBLIC_KEY ?= ce5d7dc891a76fa5e7b306d8023d327f46ff106e764b66e7721f3898dc0e7459
CLIENT_VERSION ?= $(shell git describe --tags --match 'v[0-9]*' --always --dirty 2>/dev/null || echo dev)
BUNDLE_VERSION ?= $(shell git describe --tags --match 'v[0-9]*' --abbrev=0 2>/dev/null | sed 's/^v//;s/-.*//')
CFLAGS  = -Wall -O2 -std=c99 -ffp-contract=off -I$(RAYLIB_DIR)/include -DHOD_CONTENT_URL='"$(CONTENT_URL)"' -DHOD_SERVER_URL='"$(SERVER_URL)"' -DHOD_VERSION='"$(CLIENT_VERSION)"' -DHOD_RELEASE_URL='"$(RELEASE_URL)"' -DHOD_RELEASE_PUBLIC_KEY='"$(RELEASE_PUBLIC_KEY)"' $(PLATFORM_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS = -L$(RAYLIB_DIR)/lib
SUPPORT_SRC = source/platform.c source/release_signature.c
SRC     = source/main.c $(SUPPORT_SRC)
DEPS    = $(wildcard source/*.c source/*.h source/vendors/*.c source/vendors/*.h)
TEST_TARGET = tests/battle_sim_test
GOLDEN_TARGET = tests/battle_replay_golden
GOLDEN_DIR = tests/golden
BALANCE_TARGET = tests/balance_check
STRENGTH_TARGET = tests/pvp_squad_strength
BALANCE_ARGS ?=
CONTENT_TEST_TARGET = tests/content_update_test
UPDATE_TEST_TARGET = tests/client_update_test
INSTALL_TEST_TARGET = tests/client_install_test
SERVER_ACTIONS_TEST_TARGET = tests/server_actions_test
BUTTON_TEST_TARGET = tests/button_test
SETTINGS_TEST_TARGET = tests/settings_test
BACKEND_DIR ?= ../hoda-custom-backend
LOCAL_PLAY = $(BACKEND_DIR)/scripts/play-local.sh
DIST_DIR ?= ../hoda-custom-dist
DIST_BACKEND_URL ?=
APP_ICON = packaging/app_icon.png
STACK_FRAME_LIMIT = 262144
WINDOWS_STACK_RESERVE = 8388608
WEB_CC ?= emcc
WEB_DIR ?= build/web
WEB_RAYLIB_VERSION = 5.5
WEB_RAYLIB_REPO = https://github.com/raysan5/raylib.git
WEB_RAYLIB_SRC ?= build/raylib-$(WEB_RAYLIB_VERSION)-src
WEB_RAYLIB_DIR ?= build/raylib-web
WEB_RAYLIB = $(WEB_RAYLIB_DIR)/libraylib.a
WEB_SHELL = packaging/web/shell.html
WEB_FETCH_BACKEND = packaging/web/fetch_backend.js
WEB_PAGE = $(WEB_DIR)/index.html
WEB_RESOURCE_INDEX = client-additional-data/resource_index.txt
WEB_RESOURCE_INDEX_BUILD = build/resource_index.txt
WEB_PORT ?= 8077
WEB_BACKEND ?=
WEB_RELEASE ?=
WEB_PREVIOUS ?=
WEB_DRY_RUN ?=
WEB_FILES_URL = $(if $(WEB_RELEASE),$(CONTENT_URL)/web/$(WEB_RELEASE)/,)
WEB_BAKED = $(WEB_DIR)-baked.txt
WEB_CFLAGS = -Wall -O2 -std=c99 -ffp-contract=off -D_DEFAULT_SOURCE -pthread -I$(WEB_RAYLIB_DIR) \
	-DHOD_CONTENT_URL='"$(CONTENT_URL)"' -DHOD_SERVER_URL='"$(SERVER_URL)"' -DHOD_VERSION='"$(CLIENT_VERSION)"' \
	-DHOD_RELEASE_URL='"$(RELEASE_URL)"' -DHOD_RELEASE_PUBLIC_KEY='"$(RELEASE_PUBLIC_KEY)"' \
	-DHOD_RESOURCE_INDEX='"$(WEB_RESOURCE_INDEX)"' -DHOD_GAME_FILES_URL='"$(WEB_FILES_URL)"' $(EXTRA_CFLAGS)
WEB_LDFLAGS = -L$(WEB_RAYLIB_DIR) -lraylib -pthread -sENVIRONMENT=web,worker -sUSE_GLFW=3 \
	-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3 -sWASMFS -sFETCH -sASYNCIFY -sASYNCIFY_STACK_SIZE=262144 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=512MB -sMAXIMUM_MEMORY=4GB -sSTACK_SIZE=8MB \
	-sDEFAULT_PTHREAD_STACK_SIZE=2MB -sPTHREAD_POOL_SIZE=32 -sEXPORTED_RUNTIME_METHODS=ENV,HEAPF32 \
	--shell-file $(WEB_SHELL) --js-library $(WEB_FETCH_BACKEND)
.DEFAULT_GOAL := app

ifeq ($(PLATFORM),macos)
STACK_FRAME_GUARD = -Wframe-larger-than=$(STACK_FRAME_LIMIT) -Werror=frame-larger-than
LIBS    = $(RAYLIB_LINK) -framework Cocoa -framework CoreFoundation -framework IOKit -framework OpenGL -framework CoreAudio -framework CoreVideo
APP_BUNDLE = $(APP_NAME).app
APP_CONTENTS = $(APP_BUNDLE)/Contents
APP_EXECUTABLE = $(APP_CONTENTS)/MacOS/$(APP_NAME)
TARGET  = Heroes\ of\ Dragon\ Age.app/Contents/MacOS/Heroes\ of\ Dragon\ Age
APP_INFO = packaging/macos/Info.plist
ICONSET = build/AppIcon.iconset
ICON_SIZES = 16 32 128 256 512

$(TARGET): $(DEPS) $(APP_INFO) $(APP_ICON) Makefile
	mkdir -p "$(APP_CONTENTS)/MacOS" "$(APP_CONTENTS)/Resources" $(ICONSET)
	$(CC) $(CFLAGS) $(STACK_FRAME_GUARD) $(SRC) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
	cp $(APP_INFO) "$(APP_CONTENTS)/Info.plist"
	plutil -replace CFBundleShortVersionString -string "$(or $(BUNDLE_VERSION),0.0.0)" "$(APP_CONTENTS)/Info.plist"
	for size in $(ICON_SIZES); do \
		sips -z $$size $$size $(APP_ICON) --out $(ICONSET)/icon_$${size}x$${size}.png >/dev/null; \
		sips -z $$((size * 2)) $$((size * 2)) $(APP_ICON) --out $(ICONSET)/icon_$${size}x$${size}@2x.png >/dev/null; \
	done
	iconutil -c icns $(ICONSET) -o "$(APP_CONTENTS)/Resources/AppIcon.icns"
	codesign --force --sign - "$(APP_BUNDLE)"
endif

ifeq ($(PLATFORM),linux)
PLATFORM_CFLAGS = -D_DEFAULT_SOURCE
STACK_FRAME_GUARD = -Werror=frame-larger-than=$(STACK_FRAME_LIMIT)
LIBS    = $(RAYLIB_LINK) -lGL -lm -lpthread -ldl -lrt -lX11
APP_EXECUTABLE = build/linux/$(APP_NAME)
TARGET  = build/linux/Heroes\ of\ Dragon\ Age

$(TARGET): $(DEPS) Makefile
	mkdir -p build/linux
	$(CC) $(CFLAGS) $(STACK_FRAME_GUARD) $(SRC) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
endif

ifeq ($(PLATFORM),windows)
CC      = x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
STACK_FRAME_GUARD = -Werror=frame-larger-than=$(STACK_FRAME_LIMIT)
LIBS    = $(RAYLIB_LINK) -lopengl32 -lgdi32 -lwinmm -lwinhttp -lpthread -static -mwindows -Wl,--stack,$(WINDOWS_STACK_RESERVE)
APP_EXECUTABLE = build/windows/$(APP_NAME).exe
TARGET  = build/windows/Heroes\ of\ Dragon\ Age.exe
WINDOWS_RESOURCES = build/windows/app_resources.o

$(WINDOWS_RESOURCES): packaging/windows/app.rc packaging/windows/app_icon.ico
	mkdir -p build/windows
	$(WINDRES) packaging/windows/app.rc -O coff -o $(WINDOWS_RESOURCES)

$(TARGET): $(DEPS) $(WINDOWS_RESOURCES) Makefile
	$(CC) $(CFLAGS) $(STACK_FRAME_GUARD) $(SRC) $(WINDOWS_RESOURCES) -o "$(APP_EXECUTABLE)" $(LDFLAGS) $(LIBS)
endif

app: $(TARGET)

$(TEST_TARGET): tests/battle_sim_test.c tests/squad_meta.h $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/battle_sim_test.c $(SUPPORT_SRC) -o $(TEST_TARGET) $(LDFLAGS) $(LIBS)

$(GOLDEN_TARGET): tests/battle_replay_golden.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/battle_replay_golden.c $(SUPPORT_SRC) -o $(GOLDEN_TARGET) $(LDFLAGS) $(LIBS)

golden: $(GOLDEN_TARGET)
	./$(GOLDEN_TARGET) $(GOLDEN_DIR)

$(BALANCE_TARGET): tests/balance_check.c tests/squad_meta.h $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/balance_check.c $(SUPPORT_SRC) -o $(BALANCE_TARGET) $(LDFLAGS) $(LIBS)

balance: $(BALANCE_TARGET)
	./$(BALANCE_TARGET) $(BALANCE_ARGS)

$(STRENGTH_TARGET): tests/pvp_squad_strength.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/pvp_squad_strength.c $(SUPPORT_SRC) -o $(STRENGTH_TARGET) $(LDFLAGS) $(LIBS)

squad-strength: $(STRENGTH_TARGET)
	./$(STRENGTH_TARGET)
	python3 tools/build_pvp_rules.py

$(CONTENT_TEST_TARGET): tests/content_update_test.c tests/zip_fixture.h $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/content_update_test.c $(SUPPORT_SRC) -o $(CONTENT_TEST_TARGET) $(LDFLAGS) $(LIBS)

$(UPDATE_TEST_TARGET): tests/client_update_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/client_update_test.c $(SUPPORT_SRC) -o $(UPDATE_TEST_TARGET) $(LDFLAGS) $(LIBS)

$(INSTALL_TEST_TARGET): tests/client_install_test.c tests/zip_fixture.h $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/client_install_test.c $(SUPPORT_SRC) -o $(INSTALL_TEST_TARGET) $(LDFLAGS) $(LIBS)

$(SERVER_ACTIONS_TEST_TARGET): tests/server_actions_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/server_actions_test.c $(SUPPORT_SRC) -o $(SERVER_ACTIONS_TEST_TARGET) $(LDFLAGS) $(LIBS)

$(BUTTON_TEST_TARGET): tests/button_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/button_test.c $(SUPPORT_SRC) -o $(BUTTON_TEST_TARGET) $(LDFLAGS) $(LIBS)

$(SETTINGS_TEST_TARGET): tests/settings_test.c $(DEPS) Makefile
	$(CC) $(CFLAGS) tests/settings_test.c $(SUPPORT_SRC) -o $(SETTINGS_TEST_TARGET) $(LDFLAGS) $(LIBS)

ci-test: $(CONTENT_TEST_TARGET) $(UPDATE_TEST_TARGET) $(INSTALL_TEST_TARGET) $(SERVER_ACTIONS_TEST_TARGET) $(BUTTON_TEST_TARGET) $(SETTINGS_TEST_TARGET) $(GOLDEN_TARGET) $(STRENGTH_TARGET)
	python3 tools/validate_ability_recipes.py
	python3 tools/validate_intrinsic_passives.py
	python3 tools/build_hero_auras.py --check
	python3 tools/build_hero_pool.py --check
	python3 tools/validate_localization.py
	python3 tools/validate_roster_levels.py
	python3 tools/validate_enemy_rosters.py
	python3 -m unittest tests/test_release_manifest.py tests/test_roster_levels.py tests/test_enemy_rosters.py tests/test_patch_notes.py tests/test_nexus_scenes.py tests/test_campaign_scenes.py tests/test_map_particles.py tests/test_build_hero_auras.py tests/test_web_build.py tests/test_publish_web.py
	./$(CONTENT_TEST_TARGET)
	./$(UPDATE_TEST_TARGET)
	./$(INSTALL_TEST_TARGET)
	./$(SERVER_ACTIONS_TEST_TARGET)
	./$(BUTTON_TEST_TARGET)
	./$(SETTINGS_TEST_TARGET)
	./$(GOLDEN_TARGET) --check $(GOLDEN_DIR)
	./$(STRENGTH_TARGET) --check

test: ci-test $(TEST_TARGET)
	python3 client-additional-data/validate_characters.py
	python3 -m unittest tests/test_current_tier_models.py tests/test_import_fbx.py tests/test_pvp_rewards.py tests/test_build_dist.py tests/test_build_custom_heroes.py tests/test_content_packs.py tests/test_hero_rarity.py tests/test_level_caps.py tests/test_unit_texture.py tests/test_body_frame.py tests/test_human_muscles.py tests/test_stray_scale_folds.py tests/test_audit_character_models.py tests/test_prefab_scaffold.py tests/test_skeleton_binding.py
	python3 tools/audit_character_models.py --deep
	python3 tools/validate_unit_audio.py
	python3 tools/validate_unit_vfx.py
	python3 tools/validate_gear.py
	python3 tools/validate_hero_rarity.py
	python3 tools/build_store_catalog.py --check
	python3 tools/build_custom_heroes.py --check
	python3 tools/validate_cache_refs.py
	python3 tools/build_campaign_node_layout.py --check
	python3 tools/build_runes.py --check
	python3 tools/build_missions.py --check
	python3 tools/build_news_feed.py --check
	python3 tools/build_pvp_rules.py --check
	python3 tools/build_nexus_game_changers.py --check
	python3 tools/build_consume_tables.py --check
	python3 tools/build_fortify_tables.py --check
	python3 tools/build_consume_scene.py --check
	uv run --offline --with UnityPy python tools/build_nexus_scenes.py --check
	uv run --offline --with UnityPy python tools/build_campaign_scenes.py --check
	./$(TEST_TARGET)

run: $(TARGET)
ifneq ($(wildcard $(LOCAL_PLAY)),)
	HOD_CLIENT_DIR="$(CURDIR)" HOD_CLIENT_EXECUTABLE="$(APP_EXECUTABLE)" "$(LOCAL_PLAY)"
else
	@echo "No backend checkout at $(BACKEND_DIR): Online Game plays on $(SERVER_URL)"
	"./$(APP_EXECUTABLE)"
endif

play: run

dist: $(TARGET)
	python3 tools/build_dist.py --replace --out "$(DIST_DIR)" --backend-url "$(DIST_BACKEND_URL)"

content:
	python3 tools/publish_content.py --bucket "$(CONTENT_BUCKET)" --content-url "$(CONTENT_URL)"

$(WEB_RAYLIB):
	test -d "$(WEB_RAYLIB_SRC)" || git clone --depth 1 --branch $(WEB_RAYLIB_VERSION) $(WEB_RAYLIB_REPO) "$(WEB_RAYLIB_SRC)"
	mkdir -p "$(WEB_RAYLIB_DIR)"
	$(MAKE) -C "$(WEB_RAYLIB_SRC)/src" PLATFORM=PLATFORM_WEB GRAPHICS=GRAPHICS_API_OPENGL_ES3 \
		CUSTOM_CFLAGS="-pthread -O2" RAYLIB_BUILD_MODE=RELEASE RAYLIB_RELEASE_PATH="$(CURDIR)/$(WEB_RAYLIB_DIR)"
	cp "$(WEB_RAYLIB_SRC)/src/raylib.h" "$(WEB_RAYLIB_SRC)/src/raymath.h" "$(WEB_RAYLIB_SRC)/src/rlgl.h" "$(WEB_RAYLIB_DIR)/"

$(WEB_BAKED): FORCE
	mkdir -p "$(dir $@)"
	echo "$(CONTENT_URL) $(SERVER_URL) $(WEB_FILES_URL) $(CLIENT_VERSION)" | cmp -s - "$@" || \
		echo "$(CONTENT_URL) $(SERVER_URL) $(WEB_FILES_URL) $(CLIENT_VERSION)" > "$@"

$(WEB_PAGE): $(DEPS) $(WEB_SHELL) $(WEB_FETCH_BACKEND) $(WEB_BAKED) $(WEB_RAYLIB) Makefile
	mkdir -p "$(WEB_DIR)"
	$(WEB_CC) $(WEB_CFLAGS) $(SRC) -o $(WEB_PAGE) $(WEB_LDFLAGS)

$(WEB_RESOURCE_INDEX_BUILD): $(TARGET)
	mkdir -p build
	HOD_RESOURCE_INDEX_OUT="$(WEB_RESOURCE_INDEX_BUILD)" "./$(APP_EXECUTABLE)"

web: $(WEB_PAGE) $(WEB_RESOURCE_INDEX_BUILD)
	python3 tools/build_web.py --out "$(WEB_DIR)" --resource-index "$(WEB_RESOURCE_INDEX_BUILD)" \
		--served-index "$(WEB_RESOURCE_INDEX)"

web-serve: web
	python3 tools/serve_web.py --root "$(WEB_DIR)" --port $(WEB_PORT) $(if $(WEB_BACKEND),--backend "$(WEB_BACKEND)")

web-publish: web
	python3 tools/publish_web.py --root "$(WEB_DIR)" --bucket "$(CONTENT_BUCKET)" --content-url "$(CONTENT_URL)" \
		--release "$(WEB_RELEASE)" $(if $(WEB_PREVIOUS),--previous "$(WEB_PREVIOUS)") $(if $(WEB_DRY_RUN),--dry-run)

FORCE:

clean:
	rm -rf "$(APP_NAME).app" build $(TEST_TARGET) $(CONTENT_TEST_TARGET) $(UPDATE_TEST_TARGET) $(INSTALL_TEST_TARGET) $(SERVER_ACTIONS_TEST_TARGET) $(BUTTON_TEST_TARGET) $(GOLDEN_TARGET) $(BALANCE_TARGET) $(STRENGTH_TARGET)

format:
	clang-format -i source/*.c

format-check:
	clang-format --dry-run --Werror source/*.c

release-public-key:
	@echo $(RELEASE_PUBLIC_KEY)

.PHONY: app run play dist content web web-serve web-publish FORCE ci-test test golden squad-strength clean format format-check release-public-key

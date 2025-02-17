BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = main.c scene.c game.c title.c
assets_xm = $(wildcard assets/*.xm)
assets_wav = $(wildcard assets/*.wav)
assets_png = $(wildcard assets/*.png)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite)))

AUDIOCONV_FLAGS ?=
MKSPRITE_FLAGS ?=

all: asteroids64_new.z64

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/%.wav64: assets/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem $<

$(BUILD_DIR)/asteroids64_new.dfs: $(assets_conv)
$(BUILD_DIR)/asteroids64_new.elf: $(src:%.c=$(BUILD_DIR)/%.o)

asteroids64_new.z64: N64_ROM_TITLE="RSPQ Demo"
asteroids64_new.z64: $(BUILD_DIR)/asteroids64_new.dfs 

clean:
	rm -rf $(BUILD_DIR) asteroids64_new.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean

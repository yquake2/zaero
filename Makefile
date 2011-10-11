# ----------------------------------------------------- #
# Makefile for the zaero game module for Quake II       #
#                                                       #
# Just type "make" to compile the                       #
#  - Zaero Game (game.so)                               #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theorie every one should work              #
#   Yamagi Quake II ist recommended.                    #
#                                                       #
# Platforms:                                            #
# - Linux                                               #
# - FreeBSD                                             #
# ----------------------------------------------------- #

# Check the OS type
OSTYPE := $(shell uname -s)

# Some plattforms call it "amd64" and some "x86_64"
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/amd64/x86_64/)

# Refuse all other plattforms as a firewall against PEBKAC
# (You'll need some #ifdef for your unsupported  plattform!)
ifneq ($(ARCH),i386)
ifneq ($(ARCH),x86_64)
$(error arch $(ARCH) is currently not supported)
endif
endif

# ----------

# The compiler
CC := gcc

# ----------

# Base CFLAGS. 
#
# -O2 are enough optimizations.
# 
# -fno-strict-aliasing since the source doesn't comply
#  with strict aliasing rules and it's next to impossible
#  to get it there...
#
# -fomit-frame-pointer since the framepointer is mostly
#  useless for debugging Quake II and slows things down.
#
# -g to build allways with debug symbols. Please do not
#  change this, since it's our only chance to debug this
#  crap when random crashes happen!
#
# -fPIC for position independend code.
#
# -MMD to generate header dependencies.
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -fPIC -Wall -pipe -g -MMD

# ----------

# Base LDFLAGS.
LDFLAGS := -shared

# ----------

# Builds everything
all: zaero

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	@rm -Rf build release

# ----------

# The zaero game
zaero:
	@echo '===> Building game.so'
	@mkdir -p release/
	$(MAKE) release/game.so

build/%.o: %.c
	@echo '===> CC $<'
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) -o $@ $<

# ----------

ZAERO_OBJS_ = \
	src/g_ai.o \
	src/g_cmds.o \
	src/g_combat.o \
	src/g_func.o \
	src/g_items.o \
	src/g_main.o \
	src/g_misc.o \
	src/g_monster.o \
	src/g_phys.o \
	src/g_spawn.o \
	src/g_svcmds.o \
	src/g_target.o \
	src/g_trigger.o \
	src/g_turret.o \
	src/g_utils.o \
	src/g_weapon.o \
	src/monster/actor/actor.o \
	src/monster/berserker/berserker.o \
	src/monster/boss/boss.o \
	src/monster/boss2/boss2.o \
	src/monster/boss3/boss3.o \
	src/monster/boss3/boss31.o \
	src/monster/boss3/boss32.o \
	src/monster/brain/brain.o \
	src/monster/chick/chick.o \
	src/monster/flipper/flipper.o \
	src/monster/float/float.o \
	src/monster/flyer/flyer.o \
	src/monster/gladiator/gladiator.o \
	src/monster/gunner/gunner.o \
	src/monster/handler/handler.o \
	src/monster/hound/hound.o \
	src/monster/hover/hover.o \
	src/monster/infantry/infantry.o \
	src/monster/insane/insane.o \
	src/monster/medic/medic.o \
	src/monster/misc/move.o \
	src/monster/mutant/mutant.o \
	src/monster/parasite/parasite.o \
	src/monster/sentien/sentien.o \
	src/monster/soldier/soldier.o \
	src/monster/supertank/supertank.o \
	src/monster/tank/tank.o \
	src/player/client.o \
	src/player/hud.o \
	src/player/trail.o \
	src/player/view.o \
	src/player/weapon.o \
	src/savegame/savegame.o \
	src/shared/flash.o \
	src/shared/shared.o \
    src/zaero/acannon.o \
    src/zaero/ai.o \
    src/zaero/anim.o \
    src/zaero/camera.o \
    src/zaero/frames.o \
    src/zaero/item.o \
    src/zaero/mtest.o \
    src/zaero/spawn.o \
    src/zaero/trigger.o \
    src/zaero/weapon.o

# ----------

# Rewrite pathes to our object directory
ZAERO_OBJS = $(patsubst %,build/%,$(ZAERO_OBJS_))

# ----------

# Generate header dependencies
ZAERO_DEPS= $(ZAERO_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(ZAERO_DEPS)

# ----------

release/game.so : $(ZAERO_OBJS)
	@echo '===> LD $@'
	@$(CC) $(LDFLAGS) -o $@ $(ZAERO_OBJS)

# ----------

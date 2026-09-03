# Любовь Эндора — сборка
#
#   make            обычная сборка в ./andors-love (терминал)
#   make gui        графическая сборка на SDL2 в ./andors-love-gui
#   make run        собрать и запустить
#   make test       регрессионные тесты
#   make debug      сборка с санитайзерами (ASan + UBSan)
#   make clean
#
# Переопределяется так:  make CXX=clang++ CXXFLAGS="-Os"
# Стандарт языка при этом НЕ теряется — см. CXXSTD ниже.

# Termux и часть систем ставят только clang++, а встроенное значение CXX в make
# всегда g++. Подменяем лишь значение по умолчанию: явный «make CXX=...» и
# CXX из окружения по-прежнему уважаются. Если $(shell) недоступен, остаётся g++.
ifeq ($(origin CXX),default)
  DETECTED_CXX := $(shell command -v g++ 2>/dev/null || command -v clang++ 2>/dev/null)
  ifneq ($(DETECTED_CXX),)
    CXX := $(notdir $(DETECTED_CXX))
  endif
endif

# Стандарт держим отдельно от CXXFLAGS. Иначе «export CXXFLAGS=-Os» в окружении
# перебивает CXXFLAGS ?= целиком и молча уносит -std=..., после чего сборка
# падает на непонятных ошибках. Коду достаточно C++11 — так он собирается и
# старыми компиляторами вроде GCC из C4Droid.
CXXSTD   ?= -std=c++11
WARN     := -Wall -Wextra
CXXFLAGS ?= -O2 $(WARN)
DEPFLAGS := -MMD -MP
LDFLAGS  ?=

ALL_CXXFLAGS := $(CXXSTD) $(CXXFLAGS) $(DEPFLAGS)

# Логика игры и терминальная оболочка. Графическая оболочка лежит в src/gfx
# и собирается отдельной целью: логика у них общая, различаются оболочки.
SRC     := $(wildcard src/*.cpp)
OBJ     := $(SRC:src/%.cpp=build/%.o)
CORE    := $(filter-out build/main.o build/ui.o,$(OBJ))
GFX_SRC := $(wildcard src/gfx/*.cpp)
GFX_OBJ := $(GFX_SRC:src/gfx/%.cpp=build/gfx/%.o)
DEP     := $(OBJ:.o=.d) $(GFX_OBJ:.o=.d)
# Переопределяется: make BIN=result — некоторые сборщики (в том числе C4Droid)
# ожидают исполняемый файл под своим именем.
BIN ?= andors-love
GUI_BIN ?= andors-love-gui

# SDL2 ищется через sdl2-config; на Android его нет, там флаги задаёт C4Droid,
# поэтому значения переопределяемы.
SDL_CFLAGS ?= $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   ?= $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2)

.PHONY: all gui run debug test embed font clean

all: $(BIN)

# CXXFLAGS передаём и в линковку: флаги вроде -pie и -s действуют именно на
# этом шаге, а на Android исполняемый файл обязан быть PIE. Если их потерять,
# система откажется запускать собранный бинарник.
$(BIN): $(OBJ)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

# --- графическая сборка ---
gui: $(GUI_BIN)

$(GUI_BIN): $(GFX_OBJ) $(CORE)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(GFX_OBJ) $(CORE) -o $@ $(SDL_LIBS) $(LDFLAGS)

build/gfx/%.o: src/gfx/%.cpp | build
	$(CXX) $(ALL_CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

build:
	@mkdir -p build build/gfx

run: $(BIN)
	./$(BIN)

# Тесты линкуются со всеми объектами, кроме main.o (там своя точка входа).
# Тесты линкуются со всей логикой и с той частью графического слоя, которую
# можно проверить без окна: шрифт, жесты, ходьба. Рисование проверяется
# отдельно — прогоном `./andors-love-gui --script` со снимками экрана.
GFX_TESTABLE := build/gfx/font.o build/gfx/font_data.o build/gfx/touch.o build/gfx/walk.o
TEST_SRC := tests/test_game.cpp
TEST_OBJ := $(filter-out build/main.o,$(OBJ)) $(GFX_TESTABLE)
TEST_BIN := build/run-tests

test: $(TEST_BIN)
	@./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_OBJ) | build
	$(CXX) $(ALL_CXXFLAGS) $(TEST_SRC) $(TEST_OBJ) -o $@ $(LDFLAGS)

debug: CXXFLAGS := -g -O0 $(WARN) -fsanitize=address,undefined
debug: LDFLAGS  := -fsanitize=address,undefined
debug: clean $(BIN)

# Карты вшиты в src/embedded_maps.cpp, чтобы игра работала без внешних файлов
# (сборка APK, копирование одним бинарником). Файл лежит в репозитории —
# эта цель нужна только после правки карт и требует python3.
embed:
	@python3 tools/embed_maps.py

# Растровый шрифт вшит в src/gfx/font_data.cpp и лежит в репозитории —
# эта цель нужна только после правки набора символов и требует python3-pil.
font:
	@python3 tools/gen_font.py

clean:
	@rm -rf build $(BIN) $(GUI_BIN)

-include $(DEP)

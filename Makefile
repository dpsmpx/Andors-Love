# Любовь Эндора — сборка
#
#   make            обычная сборка в ./andors-love (терминал)
#   make gui        графическая сборка на SDL2 в ./andors-love-gui
#   make gui-so     то же для Android: разделяемая библиотека для SDL-активности
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

# Платформа определяется по компилятору, а не по машине, на которой запущен
# make: так же верно отвечает и кросс-сборка. Спрашиваем сам компилятор —
# «x86_64-w64-mingw32» и подобные значат Windows. Если -dumpmachine недоступен
# (сборщик без $(shell)), остаётся переменная OS, которую задаёт сама Windows.
TARGET_TRIPLE := $(shell $(CXX) -dumpmachine 2>/dev/null)
ifneq (,$(findstring mingw,$(TARGET_TRIPLE)))
  WINDOWS := 1
endif
ifneq (,$(findstring cygwin,$(TARGET_TRIPLE)))
  WINDOWS := 1
endif
ifeq ($(TARGET_TRIPLE),)
  ifeq ($(OS),Windows_NT)
    WINDOWS := 1
  endif
endif

ifdef WINDOWS
  # Компоновщик Windows сам дописывает .exe, если у выходного файла нет
  # расширения. Без этого make каждый раз не находит цель и пересобирает её
  # заново: файл-то называется иначе, чем то, что он ждал.
  EXE := .exe
  # Код PE и так позиционно независим — -fPIC компилятор для Windows либо
  # ругает как бесполезный, либо молча игнорирует.
  PIC :=
else
  EXE :=
  PIC := -fPIC
endif

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
BIN ?= andors-love$(EXE)
GUI_BIN ?= andors-love-gui$(EXE)
# Android-сборка графики: там приложение — не исполняемый файл, а разделяемая
# библиотека. Её загружает SDL-активность и вызывает в ней SDL_main.
GUI_SO ?= libandors-love-gui.so

# SDL2 ищется через sdl2-config; если его нет, остаётся простой -lSDL2.
# Значения переопределяемы: на Android окружение задаёт C4Droid.
SDL_CFLAGS ?= $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   ?= $(shell sdl2-config --libs 2>/dev/null || echo -lSDL2)

# На Android SDL2 подменяет точку входа: main становится SDL_main, и вызывает
# его загрузчик, а не система. Чтобы он нашёл символ в готовом файле, тот
# должен остаться видимым — иначе получаем «your app doesn't properly link to
# SDL2». На настольной машине флаг ничего не ломает, только чуть увеличивает
# бинарник; выключается через SDL_EXPORT=.
#
# На Windows его нет вовсе: -rdynamic — флаг компоновщика ELF, а MinGW ругается
# «unrecognized command-line option». Точку входа там разворачивает не он, а
# библиотека SDL2main: она даёт настоящий main/WinMain, который зовёт SDL_main.
ifdef WINDOWS
  SDL_EXPORT ?=
else
  SDL_EXPORT ?= -rdynamic
endif

.PHONY: all gui gui-so run debug test check-maps embed font clean

all: $(BIN)

# CXXFLAGS передаём и в линковку: флаги вроде -pie и -s действуют именно на
# этом шаге, а на Android исполняемый файл обязан быть PIE. Если их потерять,
# система откажется запускать собранный бинарник.
$(BIN): $(OBJ)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(ALL_CXXFLAGS) $(PIC) -c $< -o $@

# --- графическая сборка ---
# gui    — обычный исполняемый файл: настольная машина, Termux с X-сервером.
# gui-so — разделяемая библиотека: так устроено SDL2-приложение на Android.
#          Активность SDL загружает файл и зовёт в нём SDL_main, поэтому
#          исполняемый файл ей не подходит — отсюда «your app doesn't
#          properly link to SDL2» при внешне успешной сборке.
gui: $(GUI_BIN)
gui-so: $(GUI_SO)

$(GUI_BIN): $(GFX_OBJ) $(CORE)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(SDL_EXPORT) $(GFX_OBJ) $(CORE) -o $@ $(SDL_LIBS) $(LDFLAGS)

# -pie и -shared противоречат друг другу: у C4Droid -pie лежит в CXXFLAGS
# ради исполняемых файлов, и для библиотеки его надо убрать, иначе компоновщик
# Android откажется собирать.
SO_CXXFLAGS := $(filter-out -pie -fpie -fPIE,$(CXXFLAGS))

$(GUI_SO): $(GFX_OBJ) $(CORE)
	$(CXX) $(CXXSTD) $(SO_CXXFLAGS) -shared $(GFX_OBJ) $(CORE) -o $@ $(SDL_LIBS) $(LDFLAGS)

# -fPIC нужен объектам, которые пойдут в разделяемую библиотеку; на код
# исполняемого файла он не влияет, поэтому ставится всем без разбора — кроме
# Windows, где позиционно независим весь код и флаг лишний.
build/gfx/%.o: src/gfx/%.cpp | build
	$(CXX) $(ALL_CXXFLAGS) $(PIC) $(SDL_CFLAGS) -c $< -o $@

build:
	@mkdir -p build build/gfx

run: $(BIN)
	./$(BIN)

# Тесты линкуются со всеми объектами, кроме main.o (там своя точка входа).
# Тесты линкуются со всей логикой и с той частью графического слоя, которую
# можно проверить без окна: шрифт, жесты, ходьба. Рисование проверяется
# отдельно — прогоном `./andors-love-gui --script` со снимками экрана.
GFX_TESTABLE := build/gfx/font.o build/gfx/font_data.o build/gfx/touch.o \
                build/gfx/walk.o build/gfx/png.o build/gfx/tiles.o
TEST_SRC := tests/test_game.cpp
TEST_OBJ := $(filter-out build/main.o,$(OBJ)) $(GFX_TESTABLE)
TEST_BIN := build/run-tests$(EXE)

test: $(TEST_BIN)
	@./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_OBJ) | build
	$(CXX) $(ALL_CXXFLAGS) $(TEST_SRC) $(TEST_OBJ) -o $@ $(LDFLAGS)

debug: CXXFLAGS := -g -O0 $(WARN) -fsanitize=address,undefined
debug: LDFLAGS  := -fsanitize=address,undefined
debug: clean $(BIN)

# Проверка карт. Файлы data/maps/*.map первичны и правятся руками, поэтому
# ошибку в них ловит не сборка, а этот скрипт: он ничего не пишет.
check-maps:
	@python3 tools/check_maps.py

# Карты вшиты в src/embedded_maps.cpp, чтобы игра работала без внешних файлов
# (сборка APK, копирование одним бинарником). Файл лежит в репозитории —
# эта цель нужна только после правки карт и требует python3. Проверка идёт
# первой: вшить сломанную карту молча — худшее, что тут может случиться.
embed: check-maps
	@python3 tools/embed_maps.py

# Растровый шрифт вшит в src/gfx/font_data.cpp и лежит в репозитории —
# эта цель нужна только после правки набора символов и требует python3-pil.
font:
	@python3 tools/gen_font.py

clean:
	@rm -rf build $(BIN) $(GUI_BIN) $(GUI_SO)

-include $(DEP)

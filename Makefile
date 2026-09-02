# Любовь Эндора — сборка
#
#   make            обычная сборка в ./andors-love
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

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)
BIN := andors-love

.PHONY: all run debug test embed clean

all: $(BIN)

# CXXFLAGS передаём и в линковку: флаги вроде -pie и -s действуют именно на
# этом шаге, а на Android исполняемый файл обязан быть PIE. Если их потерять,
# система откажется запускать собранный бинарник.
$(BIN): $(OBJ)
	$(CXX) $(CXXSTD) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(ALL_CXXFLAGS) -c $< -o $@

build:
	@mkdir -p build

run: $(BIN)
	./$(BIN)

# Тесты линкуются со всеми объектами, кроме main.o (там своя точка входа).
TEST_SRC := tests/test_game.cpp
TEST_OBJ := $(filter-out build/main.o,$(OBJ))
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

clean:
	@rm -rf build $(BIN)

-include $(DEP)

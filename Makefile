# Любовь Эндора — сборка
#
#   make            обычная сборка в ./andors-love
#   make run        собрать и запустить
#   make debug      сборка с санитайзерами (ASan + UBSan)
#   make clean

# Termux и часть систем ставят только clang++, а встроенное значение CXX в make
# всегда g++ — из-за этого сборка падала бы на «g++: command not found».
# Явный «make CXX=...» по-прежнему уважается: подменяем только значение по умолчанию.
ifeq ($(origin CXX),default)
  CXX := $(shell command -v g++ >/dev/null 2>&1 && echo g++ ||                  (command -v clang++ >/dev/null 2>&1 && echo clang++ || echo c++))
endif
WARN     := -Wall -Wextra -Wpedantic -Wshadow
CXXFLAGS ?= -std=c++17 $(WARN) -O2
LDFLAGS  ?=

# -MMD -MP порождают .d-файлы: правка заголовка пересобирает всё, что его
# включает. Без этого правка ui.h оставляла main.o собранным по старой
# сигнатуре и ломала линковку.
DEPFLAGS := -MMD -MP

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)
BIN := andors-love

.PHONY: all run debug test clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

build:
	@mkdir -p build

run: $(BIN)
	./$(BIN)

debug: CXXFLAGS := -std=c++17 $(WARN) -g -O0 -fsanitize=address,undefined
debug: LDFLAGS  := -fsanitize=address,undefined
debug: clean $(BIN)

# Тесты линкуются со всеми объектами, кроме main.o (там своя точка входа).
TEST_SRC := tests/test_game.cpp
TEST_OBJ := $(filter-out build/main.o,$(OBJ))
TEST_BIN := build/run-tests

test: $(TEST_BIN)
	@./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_OBJ) | build
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(TEST_SRC) $(TEST_OBJ) -o $@ $(LDFLAGS)

clean:
	@rm -rf build $(BIN)

-include $(DEP)

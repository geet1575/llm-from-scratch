SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BUILD_DIR)/main

NVCC ?= nvcc

CUDA_ARCH ?=
STD ?= c++17

CPPFLAGS ?=
NVCCFLAGS ?= -O2 -std=$(STD)
LDFLAGS ?=
LDLIBS ?=

ARCH_FLAGS := $(if $(CUDA_ARCH),-arch=$(CUDA_ARCH),)

CPP_SRCS := $(shell find $(SRC_DIR) -type f -name '*.cpp')
CU_SRCS := $(shell find $(SRC_DIR) -type f -name '*.cu')
SRCS := $(CPP_SRCS) $(CU_SRCS)
OBJS := $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(SRCS)))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(NVCC) $(ARCH_FLAGS) $(NVCCFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.cpp.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(NVCC) $(ARCH_FLAGS) $(CPPFLAGS) $(NVCCFLAGS) -x cu -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.cu.o: %.cu | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(NVCC) $(ARCH_FLAGS) $(CPPFLAGS) $(NVCCFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

-include $(DEPS)

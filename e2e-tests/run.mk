E2E_DIR       := e2e-tests
E2E_CASES_DIR := $(E2E_DIR)/cases
E2E_OUT_DIR   := build/$(BUILD)/e2e
E2E_RUNNER    := $(BIN_DIR)/e2e-runner$(EXE_EXT)

E2E_RAW_TESTS  := $(wildcard $(E2E_CASES_DIR)/raw/*.tasm)
E2E_TOBJ_TESTS := $(wildcard $(E2E_CASES_DIR)/tobj/*.tasm)

E2E_RAW_RESULTS  := $(patsubst $(E2E_CASES_DIR)/raw/%.tasm,$(E2E_OUT_DIR)/raw/%.done,$(E2E_RAW_TESTS))
E2E_TOBJ_RESULTS := $(patsubst $(E2E_CASES_DIR)/tobj/%.tasm,$(E2E_OUT_DIR)/tobj/%.done,$(E2E_TOBJ_TESTS))
E2E_RESULTS      := $(E2E_RAW_RESULTS) $(E2E_TOBJ_RESULTS)

.PHONY: e2e-test e2e-clean

e2e-test: $(E2E_RESULTS)

$(E2E_OUT_DIR)/raw/%.done: $(E2E_CASES_DIR)/raw/%.tasm $(E2E_RUNNER) $(TARGET)
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(E2E_RUNNER) $(TARGET) $< $(patsubst %.done,%.t48b,$@) raw/$* raw
	@touch $@

$(E2E_OUT_DIR)/tobj/%.done: $(E2E_CASES_DIR)/tobj/%.tasm $(E2E_RUNNER) $(TARGET)
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(E2E_RUNNER) $(TARGET) $< $(patsubst %.done,%.t48b,$@) tobj/$* tobj
	@touch $@

$(E2E_RUNNER): $(E2E_DIR)/run.c $(EMU_LIB_STATIC) $(TOBJ_LIB_STATIC)
	@$(call CMD_MKDIR_P,$(dir $@))
	$(CC) $(CFLAGS) $< $(EMU_LIB_STATIC) $(TOBJ_LIB_STATIC) $(LDFLAGS) -o $@

e2e-clean:
	@$(call CMD_RM_RF,$(E2E_OUT_DIR))
	@rm -f $(E2E_RUNNER)

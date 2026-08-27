SHELL := /bin/bash

P01_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NODE_ROOT := $(abspath $(P01_DIR)/../..)
OUTPUT_DIR ?= $(NODE_ROOT)/build/p01-candidate
KERNEL_SOURCE ?= $(abspath $(NODE_ROOT)/../Linux-kernel-node-runtime)
KERNEL_REVISION ?= 94515f3a7d4256a5062176b7d6ed0471938cd51a

.PHONY: all micro-os initramfs kernel image-tree image records inspect boundaries qemu qemu-bios qemu-uefi host-tests shell-syntax validate

all: validate

micro-os:
	"$(P01_DIR)/scripts/build-micro-os.sh" "$(OUTPUT_DIR)"

host-tests: micro-os
	ctest --test-dir "$(OUTPUT_DIR)/node-build" -R '^p01_' --output-on-failure

initramfs: micro-os
	"$(P01_DIR)/scripts/build-initramfs.sh" "$(OUTPUT_DIR)"

kernel:
	"$(P01_DIR)/scripts/build-kernel.sh" "$(OUTPUT_DIR)" \
		"$(KERNEL_SOURCE)" "$(KERNEL_REVISION)"

image-tree: initramfs kernel
	"$(P01_DIR)/scripts/assemble-image-tree.sh" "$(OUTPUT_DIR)"

image: image-tree
	@status=0; "$(P01_DIR)/scripts/construct-image.sh" "$(OUTPUT_DIR)" || status=$$?; \
		if [[ $$status -ne 0 && $$status -ne 77 ]]; then exit $$status; fi

records: image
	"$(P01_DIR)/scripts/emit-candidate-records.sh" "$(OUTPUT_DIR)"
	"$(P01_DIR)/scripts/validate-records.sh" "$(OUTPUT_DIR)"

inspect: records
	"$(P01_DIR)/scripts/inspect-candidate.sh" "$(OUTPUT_DIR)"

boundaries: records
	"$(P01_DIR)/scripts/validate-boundaries.sh" "$(OUTPUT_DIR)"

qemu: inspect
	@status=0; "$(P01_DIR)/scripts/qemu-smoke.sh" "$(OUTPUT_DIR)" || status=$$?; \
		if [[ $$status -ne 0 && $$status -ne 77 ]]; then exit $$status; fi

qemu-bios: image
	@status=0; "$(P01_DIR)/scripts/qemu-bios-iso-smoke.sh" "$(OUTPUT_DIR)" || status=$$?; \
		if [[ $$status -ne 0 && $$status -ne 77 ]]; then exit $$status; fi

qemu-uefi: image
	@status=0; "$(P01_DIR)/scripts/qemu-uefi-smoke.sh" "$(OUTPUT_DIR)" || status=$$?; \
		if [[ $$status -ne 0 && $$status -ne 77 ]]; then exit $$status; fi

shell-syntax:
	find "$(P01_DIR)" -type f -name '*.sh' -print0 | \
		xargs -0 -r -n1 bash -n

validate: shell-syntax host-tests inspect boundaries qemu qemu-bios qemu-uefi
	git -C "$(NODE_ROOT)" diff --check -- assembly/p01_boot assembly/micro_os

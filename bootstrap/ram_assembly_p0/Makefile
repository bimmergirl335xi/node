SHELL := /bin/bash

P0_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
HOST_BUILD_DIR ?= $(abspath $(P0_DIR)/../../build/ram-assembly-p0-host)
HOST_ROOT := $(HOST_BUILD_DIR)/root

.PHONY: all host-build host-test package inspect shell-syntax validate clean

all: validate

host-build:
	"$(P0_DIR)/scripts/compile-payloads.sh" --host-validation --output "$(HOST_ROOT)"
	install -D -m 0644 "$(P0_DIR)/manifests/tests.manifest" \
		"$(HOST_ROOT)/etc/node-p0/tests.manifest"

host-test: host-build
	"$(HOST_ROOT)/bin/p0_test_runner" \
		--manifest "$(HOST_ROOT)/etc/node-p0/tests.manifest" \
		--root "$(HOST_ROOT)"
	@if "$(HOST_ROOT)/bin/p0_test_runner" \
		--manifest "$(P0_DIR)/manifests/invalid.manifest" \
		--root "$(HOST_ROOT)" \
		> "$(HOST_BUILD_DIR)/invalid-manifest.out" 2>&1; then \
		echo 'malformed manifest was unexpectedly accepted' >&2; \
		exit 1; \
	fi
	grep -F '"outcome":"manifest_failed"' "$(HOST_BUILD_DIR)/invalid-manifest.out"

package: host-build
	mkdir -p "$(HOST_BUILD_DIR)/artifacts"
	cd "$(HOST_ROOT)" && \
		find . -print0 | LC_ALL=C sort -z | \
		cpio --null --create --quiet --format=newc --owner=0:0 --reproducible | \
		gzip -9n > "$(HOST_BUILD_DIR)/artifacts/node-p0-host-initramfs.cpio.gz"

inspect: package
	gzip -dc "$(HOST_BUILD_DIR)/artifacts/node-p0-host-initramfs.cpio.gz" | \
		cpio -it --quiet > "$(HOST_BUILD_DIR)/artifacts/initramfs.list"
	grep -Eq '^(\./)?init$$' "$(HOST_BUILD_DIR)/artifacts/initramfs.list"
	grep -Eq '^(\./)?bin/p0_test_runner$$' "$(HOST_BUILD_DIR)/artifacts/initramfs.list"
	test -x "$(HOST_ROOT)/init"
	test -x "$(HOST_ROOT)/bin/p0_test_runner"

shell-syntax:
	find "$(P0_DIR)/scripts" -type f -name '*.sh' -print0 | \
		xargs -0 -r -n1 bash -n

validate: shell-syntax host-test inspect
	git -C "$(P0_DIR)" diff --check -- .

clean:
	@echo "Host build retained at $(HOST_BUILD_DIR); remove it explicitly if desired."

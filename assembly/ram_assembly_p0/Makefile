SHELL := /bin/bash

P0_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
HOST_BUILD_DIR ?= $(abspath $(P0_DIR)/../../build/ram-assembly-p0-host)
HOST_ROOT := $(HOST_BUILD_DIR)/root

.PHONY: all host-build host-test component-manifest-test provider-record-test package inspect privilege-model shell-syntax validate clean

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

component-manifest-test: host-build
	"$(HOST_ROOT)/bin/node_component_manifest_validator" \
		> "$(HOST_BUILD_DIR)/component-manifest-absent.out"
	grep -F '"declaration_state":"declaration_not_supplied"' \
		"$(HOST_BUILD_DIR)/component-manifest-absent.out"
	"$(HOST_ROOT)/bin/node_component_manifest_validator" \
		--manifest "$(P0_DIR)/../manifests/public/example.manifest" \
		> "$(HOST_BUILD_DIR)/component-manifest-valid.out"
	grep -F '"declaration_state":"declaration_structurally_valid"' \
		"$(HOST_BUILD_DIR)/component-manifest-valid.out"
	grep -F '"declaration_revision_ref":"component-revision:example-metrics:1"' \
		"$(HOST_BUILD_DIR)/component-manifest-valid.out"
	@if "$(HOST_ROOT)/bin/node_component_manifest_validator" \
		--manifest "$(P0_DIR)/../tests/fixtures/malformed.manifest" \
		> "$(HOST_BUILD_DIR)/component-manifest-malformed.out" 2>&1; then \
		echo 'malformed external component manifest was unexpectedly accepted' >&2; \
		exit 1; \
	fi
	grep -F '"failure":"malformed"' \
		"$(HOST_BUILD_DIR)/component-manifest-malformed.out"
	@if "$(HOST_ROOT)/bin/node_component_manifest_validator" \
		--manifest "$(P0_DIR)/../tests/fixtures/incompatible.manifest" \
		> "$(HOST_BUILD_DIR)/component-manifest-incompatible.out" 2>&1; then \
		echo 'incompatible external component manifest was unexpectedly accepted' >&2; \
		exit 1; \
	fi
	grep -F '"failure":"incompatible_abi"' \
		"$(HOST_BUILD_DIR)/component-manifest-incompatible.out"
	grep -F '"declaration_state":"declaration_abi_incompatible"' \
		"$(HOST_BUILD_DIR)/component-manifest-incompatible.out"

provider-record-test:
	mkdir -p "$(HOST_BUILD_DIR)/provider-record-examples"
	install -m 0644 "$(P0_DIR)/contracts/p0-conformance-authority-v1.json" \
		"$(HOST_BUILD_DIR)/provider-record-examples/p0-conformance-authority-v1.json"
	cp -f "$(P0_DIR)"/records/examples/*.json \
		"$(HOST_BUILD_DIR)/provider-record-examples/"
	"$(P0_DIR)/scripts/validate-provider-records.sh" --host-validation \
		--records "$(HOST_BUILD_DIR)/provider-record-examples"
	python3 "$(P0_DIR)/tests/validate_provider_records.py" \
		"$(HOST_BUILD_DIR)/provider-record-examples"

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

privilege-model:
	"$(P0_DIR)/scripts/test-privilege-model.sh"

validate: shell-syntax privilege-model host-test component-manifest-test provider-record-test inspect
	git -C "$(P0_DIR)" diff --check -- .

clean:
	@echo "Host build retained at $(HOST_BUILD_DIR); remove it explicitly if desired."

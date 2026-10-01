if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED TOOLCHAIN_IDENTITY)
    message(FATAL_ERROR "CPU runtime provenance requires INPUT, OUTPUT, and TOOLCHAIN_IDENTITY")
endif()
if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "CPU runtime candidate is unavailable: ${INPUT}")
endif()
file(SIZE "${INPUT}" ARTIFACT_SIZE)
if(ARTIFACT_SIZE LESS 1 OR ARTIFACT_SIZE GREATER 4194304)
    message(FATAL_ERROR "CPU runtime candidate exceeds its bounded size")
endif()
file(SHA256 "${INPUT}" ARTIFACT_SHA256)
string(LENGTH "${TOOLCHAIN_IDENTITY}" TOOLCHAIN_IDENTITY_LENGTH)
if(TOOLCHAIN_IDENTITY_LENGTH LESS 1 OR
   TOOLCHAIN_IDENTITY_LENGTH GREATER 128 OR
   NOT TOOLCHAIN_IDENTITY MATCHES "^[A-Za-z0-9._:+-]+$")
    message(FATAL_ERROR "CPU runtime toolchain identity is not a bounded token")
endif()
file(WRITE "${OUTPUT}"
    "schema=node.cpu-runtime-candidate.v1\n"
    "component=node.cpu.runtime\n"
    "abi=node.cpu-runtime.conformance.v1\n"
    "architecture=x86_64\n"
    "profile=avx2\n"
    "source_identity=node.cpu-runtime-conformance-source\n"
    "source_revision=1\n"
    "toolchain_identity=${TOOLCHAIN_IDENTITY}\n"
    "configuration=p01.cpu-runtime.x86_64.avx2.static.v1\n"
    "compile_location=host-canonical-image-build\n"
    "artifact_sha256=${ARTIFACT_SHA256}\n"
    "artifact_size=${ARTIFACT_SIZE}\n"
    "parallelism=1\n"
    "source_available=true\n"
    "toolchain_available=true\n"
    "compile_result=success\n"
    "link_result=success\n"
)

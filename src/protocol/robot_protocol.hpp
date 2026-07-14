#ifndef PROMETHEUS_PROTOCOL_ROBOT_PROTOCOL_HPP
#define PROMETHEUS_PROTOCOL_ROBOT_PROTOCOL_HPP

#include <cstddef>
#include <cstdint>

namespace prometheus::robot_protocol {

inline constexpr std::uint32_t kGigaMagic = 0x47494741u;
inline constexpr std::uint32_t kTensorMagic = 0x50524F4Du;

inline constexpr std::uint16_t kTensorProtocolVersion = 2u;
inline constexpr std::uint32_t kLidarBins = 360u;

enum class PiSourceId : std::uint16_t {
    AiGlobalShutter = 0,
    NoIrGlobalShutter = 1,
};

enum class TensorStreamId : std::uint16_t {
    Imx500Appearance = 0,
    AiHailoState = 1,

    WideGsAppearance = 2,
    WideGsTemporal = 3,
    WideGsMotion = 4,

    NoIrAppearance = 5,
    NoIrHailoState = 6,

    TeleGsAppearance = 7,
    TeleGsTemporal = 8,
    TeleGsMotion = 9,

    PiHealth = 10,
    Count,
};

enum TensorFlags : std::uint16_t {
    TensorFlagValid = 1u << 0,
    TensorFlagCameraMoving = 1u << 1,
    TensorFlagFocusMoving = 1u << 2,
    TensorFlagExternalTrigger = 1u << 3,
    TensorFlagIrIlluminated = 1u << 4,
    TensorFlagFrameDropped = 1u << 5,
    TensorFlagTimingUncertain = 1u << 6,
    TensorFlagDegraded = 1u << 7,
};

#pragma pack(push, 1)

struct ImuData {
    float accel[3];
    float gyro[3];
    float mag[3];
};

struct GigaTelemetryPacket {
    std::uint32_t magic;
    std::uint32_t frame_id;
    std::uint64_t timestamp_ns;

    ImuData imu_head;
    ImuData imu_body;

    float light_lux;
    float lidar_distances[kLidarBins];
};

struct TensorPacketHeader {
    std::uint32_t magic;

    std::uint16_t protocol_version;
    std::uint16_t header_bytes;

    std::uint16_t source_id;
    std::uint16_t stream_id;

    std::uint16_t camera_id;
    std::uint16_t flags;

    std::uint64_t pico_epoch;
    std::uint64_t sensor_timestamp_ns;
    std::uint64_t sender_timestamp_ns;

    std::uint32_t sequence;
    std::uint32_t feature_count;

    std::uint32_t payload_bytes;
    std::uint32_t payload_crc32;
};

#pragma pack(pop)

static_assert(sizeof(ImuData) == 36);
static_assert(sizeof(GigaTelemetryPacket) == 1532);
static_assert(sizeof(TensorPacketHeader) == 56);

}  // namespace prometheus::robot_protocol

#endif
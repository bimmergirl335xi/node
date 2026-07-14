#include "protocol/robot_protocol.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
    static_assert(sizeof(ImuData) == 36);
    static_assert(sizeof(GigaTelemetryPacket) == 1532);
    static_assert(sizeof(TensorPacketHeader) == 56);

    if (TENSOR_PROTOCOL_VERSION != 2u) {
        std::cerr << "Unexpected tensor protocol version\n";
        return EXIT_FAILURE;
    }

    if (GIGA_MAGIC != 0x47494741u) {
        std::cerr << "Unexpected GIGA magic\n";
        return EXIT_FAILURE;
    }

    if (TENSOR_MAGIC != 0x50524F4Du) {
        std::cerr << "Unexpected tensor magic\n";
        return EXIT_FAILURE;
    }

    std::cout << "Robot protocol layout verified\n";
    return EXIT_SUCCESS;
}
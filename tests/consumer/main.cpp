#include <eolib/eolib.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    using namespace eolib;
    using namespace eolib::protocol::net;

    client::WalkPlayerClientPacket packet;
    packet.walk_action.direction = protocol::Direction::Up;
    packet.walk_action.coords.x = 5;
    packet.walk_action.coords.y = 7;

    data::EoWriter writer;
    packet.Serialize(writer);

    data::EoReader reader(writer.ToByteArray());
    const auto deserialized = client::PacketFactory::Deserialize(PacketFamily::Walk, PacketAction::Player, reader);
    if (deserialized == nullptr || deserialized->ToString() != packet.ToString())
    {
        std::cerr << "Round trip failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "eolib " << GetVersionString() << ": " << deserialized->ToString() << "\n";
    return EXIT_SUCCESS;
}

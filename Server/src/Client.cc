#include <Client.hh>

#include <iostream>

#include <BinaryCoder/BinaryCoder.hh>
#include <BinaryCoder/NativeTypes.hh>

#include <Server.hh>
#include <Simulation.hh>

namespace app
{
    Client::Client(websocketpp::connection_hdl hdl, Simulation &simulation)
        : m_Hdl(hdl),
          m_Simulation(simulation)
    {
        std::cout << "client create\n";
        m_Player.emplace(m_Simulation.Create());
        m_Simulation.AddComponent<component::Flower>(*m_Player);
        m_Simulation.AddComponent<component::Physical>(*m_Player);
        m_Simulation.AddComponent<component::Life>(*m_Player);
        m_Simulation.AddComponent<component::Render>(*m_Player);
        m_Simulation.Get<component::Physical>(*m_Player).Radius(25.0f);
    }

    Client::~Client()
    {
        std::cout << "client destroy\n";

        if (m_Player)
            m_Simulation.Remove(*m_Player);
    }

    void Client::BroadcastUpdate()
    {
        static uint8_t data[1024 * 1024];
        bc::BinaryCoder coder{data};
        coder.Write<bc::Uint8>(0);

        coder.Write<bc::Float32>(m_Camera.m_Fov);
        coder.Write<bc::Float32>(m_Camera.m_X);
        coder.Write<bc::Float32>(m_Camera.m_Y);

        m_Simulation.WriteUpdate(coder, m_Camera);

        SendPacket(coder);
    }

    void Client::Tick()
    {
        m_Simulation.Get<component::Physical>(*m_Player).m_Acceleration = m_PlayerAcceleration;
        m_Camera.m_X = m_Simulation.Get<component::Physical>(*m_Player).X();
        m_Camera.m_Y = m_Simulation.Get<component::Physical>(*m_Player).Y();
        BroadcastUpdate();
    }

    void Client::SendPacket(bc::BinaryCoder const &data) const
    {
        std::string packet{data.Data(), data.Data() + data.At()};
        m_Simulation.m_Server.m_Server.send(m_Hdl, packet, websocketpp::frame::opcode::binary);
    }

    void Client::ReadPacket(uint8_t *data, size_t size)
    {
        if (size < 1)
        {
            std::cout << "someone sent packet with size < 1\n";
            return;
        }
        bc::BinaryCoder coder{data};
        uint8_t type = coder.Read<bc::Uint8>();
        if (type == 0)
        {
            if (size != 3) {
                std::cout << "someone sent update packet with size not equal to 3\n";
                return;
            }

            bool mouseMovement = coder.Read<bc::Uint8>();

            if (mouseMovement)
            {
                std::cout << "mouse movement doesn't exist yet\n";
                return;
            }

            uint8_t movementFlags = coder.Read<bc::Uint8>();

            float x = 0;
            float y = 0;

            if (movementFlags & 1) y--;
            if (movementFlags & 2) x--;
            if (movementFlags & 4) y++;
            if (movementFlags & 8) x++;

            m_PlayerAcceleration.Set(x, y);
            m_PlayerAcceleration.Normalize();
        }
    }

    websocketpp::connection_hdl Client::GetHdl() { return m_Hdl; }
}

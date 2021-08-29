#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>

#include <botcraft/Game/World/World.hpp>
#include <botcraft/Game/World/Chunk.hpp>
#include <botcraft/Game/World/Block.hpp>
#include <botcraft/Game/Entities/EntityManager.hpp>
#include <botcraft/Game/Inventory/InventoryManager.hpp>
#include <botcraft/Game/Inventory/Window.hpp>
#include <botcraft/Game/Entities/LocalPlayer.hpp>
#include <botcraft/Network/NetworkManager.hpp>
#include <botcraft/Game/AssetsManager.hpp>

#include <protocolCraft/Types/NBT/NBT.hpp>
#include <protocolCraft/Types/NBT/TagList.hpp>

#include "ResidentSleeperBot.hpp"
#include <protocolCraft/Types/NBT/TagString.hpp>
#include <protocolCraft/Types/NBT/TagInt.hpp>

using namespace Botcraft;
using namespace ProtocolCraft;

namespace 
{
    bool endsWith (std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }
} // namespace 


ResidentSleeperBot::ResidentSleeperBot(const bool use_renderer_) : InterfaceClient(use_renderer_, false)
{
    started = false;
}

ResidentSleeperBot::~ResidentSleeperBot()
{
    
}

void ResidentSleeperBot::Handle(ClientboundChatPacket &msg)
{
    BaseClient::Handle(msg);
    
    // Split the message 
    std::istringstream ss{ msg.GetMessage().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });
    
    if (splitted.size() > 0 && splitted[0] == "start")
    {
        LaunchSleeping();
    }
    else
    {
        return;
    }
}


void ResidentSleeperBot::LaunchSleeping()
{
    std::thread sleeping_thread(&ResidentSleeperBot::StayAfk, this);
    sleeping_thread.detach();
}

const std::vector<Position> ResidentSleeperBot::GetAllBedsAround() const
{
    std::cout<<"GetAllBedsAround"<<std::endl;
    std::vector<Position> output;

    std::shared_ptr<LocalPlayer> local_player = entity_manager->GetLocalPlayer();
    const Position player_position(local_player->GetX(), local_player->GetY(), local_player->GetZ());


    Position checked_position;
    std::map<double, Position> distancesMap;
    {
        std::lock_guard<std::mutex> world_guard(world->GetMutex());
        const Block* block;
        const std::map<std::pair<int, int>, std::shared_ptr<Chunk> >& all_chunks = world->GetAllChunks();

        for (auto it = all_chunks.begin(); it != all_chunks.end(); ++it)
        {
            for (int x = 0; x < CHUNK_WIDTH; ++x)
            {
                checked_position.x = it->first.first * CHUNK_WIDTH + x;
                for (int y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    checked_position.y = y;
                    for (int z = 0; z < CHUNK_WIDTH; ++z)
                    {
                        checked_position.z = it->first.second * CHUNK_WIDTH + z;
                        block = world->GetBlock(checked_position);
                        if (block &&  endsWith(block->GetBlockstate()->GetName(), "bed"))
                        {
                            auto playerToBed = checked_position - player_position;
                            double distance = checked_position.dot(checked_position);
                            // I don't care if there are an collision, I just want to take closest one
                            distancesMap[distance] = checked_position;
                        }
                    }
                }
            }
        }
    }

    for(auto distanceToBed: distancesMap){
        output.push_back(distanceToBed.second);
    }
    
    return output;
}


void ResidentSleeperBot::GetSomeSleep()
{
    std::cout<<"GetSomeSleep"<<std::endl;
    std::vector<Position> beds = GetAllBedsAround();

    short container_id;

    if( beds.size() == 0){
        std::cout<<"No beds found"<<std::endl;
    }

    for (int i = 0; i < beds.size(); ++i)
    {
        std::cout<<"Trying bed bed ["<< i <<"]"<<std::endl;
        // If we can't open this chest for a reason
        if (!GoTo(beds[i], true))
        {
            std::cout<<"Cant go to that bed ["<< i <<"]"<<std::endl;
            continue;
        }else{
            std::cout<<"Go to bed ["<< i <<"]"<<std::endl;
        }

       if( InteractWithBlock(beds[i],PlayerDiggingFace::Top)){
           std::cout<<"Interacted successfully with bed ["<< i <<"]"<<std::endl;
       }
       else{
           std::cout<<"Cant iteract with bed ["<< i <<"]"<<std::endl;
       }
       break;
    }
}

void ResidentSleeperBot::StayAfk()
{
    if (started)
    {
        return;
    }

    try
    {
        started = true;

        while (true)
        {
            GetSomeSleep();
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        started = false;
    }
    catch (...)
    {
        std::cout << "Gotcha!" << std::endl;
        started = false;
        should_be_closed = true;
        return;
    }
}

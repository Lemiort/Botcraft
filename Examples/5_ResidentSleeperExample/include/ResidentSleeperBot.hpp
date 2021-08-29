#ifndef EXAMPLES_5_RESIDENTSLEEPEREXAMPLE_INCLUDE_RESIDENTSLEEPERBOT
#define EXAMPLES_5_RESIDENTSLEEPEREXAMPLE_INCLUDE_RESIDENTSLEEPERBOT

#include <botcraft/Game/Vector3.hpp>
#include <botcraft/Game/InterfaceClient.hpp>
#include <botcraft/Game/World/Blockstate.hpp>

#include <random>
#include <set>
#include <map>

class ResidentSleeperBot : public Botcraft::InterfaceClient
{
public:
    ResidentSleeperBot(const bool use_renderer_);
    ~ResidentSleeperBot();

    void LaunchSleeping();

protected:
    virtual void Handle(ProtocolCraft::ClientboundChatPacket &msg) override;

private:

    // Get the positions of all loaded beds
    // returns: a vector with all the found positions
    const std::vector<Botcraft::Position> GetAllBedsAround() const;
    void GetSomeSleep();
    void StayAfk();
private:
    bool started;

};


#endif /* EXAMPLES_5_RESIDENTSLEEPEREXAMPLE_INCLUDE_RESIDENTSLEEPERBOT */

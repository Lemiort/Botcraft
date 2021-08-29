#pragma once

#include "botcraft/Version.hpp"
#include "protocolCraft/Handler.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace Botcraft
{
    class Entity;
    class LocalPlayer;

#if USE_GUI
    namespace Renderer
    {
        class RenderingManager;
    }
#endif

    class EntityManager : public ProtocolCraft::Handler
    {
    public:
        EntityManager();

        std::shared_ptr<LocalPlayer> GetLocalPlayer();
        const std::unordered_map<int, std::shared_ptr<Entity> >& GetEntities() const;

#if USE_GUI
        void SetRenderingManager(std::shared_ptr<Renderer::RenderingManager> rendering_manager_);
#endif
        std::mutex& GetMutex();

    protected:
        virtual void Handle(ProtocolCraft::ClientboundLoginPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundPlayerPositionPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundAddEntityPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundAddMobPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundAddPlayerPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundSetHealthPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundTeleportEntityPacket& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundPlayerAbilitiesPacket& msg) override;
#if PROTOCOL_VERSION < 755
        virtual void Handle(ProtocolCraft::ClientboundMoveEntityPacket& msg) override;
#endif
        virtual void Handle(ProtocolCraft::ClientboundMoveEntityPacketPos& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundMoveEntityPacketPosRot& msg) override;
        virtual void Handle(ProtocolCraft::ClientboundMoveEntityPacketRot& msg) override;
#if PROTOCOL_VERSION == 755
        virtual void Handle(ProtocolCraft::ClientboundRemoveEntityPacket& msg) override;
#else
        virtual void Handle(ProtocolCraft::ClientboundRemoveEntitiesPacket& msg) override;
#endif


    private:
        std::unordered_map<int, std::shared_ptr<Entity> > entities;
        // The current player is stored independently
        std::shared_ptr<LocalPlayer> local_player;

        std::mutex entity_manager_mutex;

#if USE_GUI
        std::shared_ptr<Renderer::RenderingManager> rendering_manager;
#endif
    };
} // Botcraft
#pragma once

#include "botcraft/Game/World/Biome.hpp"
#include "botcraft/Game/World/Blockstate.hpp"
#include "botcraft/Game/Inventory/Item.hpp"

#include <vector>

namespace Botcraft
{
    class AssetsManager
    {
    public:
        static AssetsManager& getInstance();

        AssetsManager(AssetsManager const&) = delete;
        void operator=(AssetsManager const&) = delete;

#ifdef USE_GUI
        const std::vector<std::pair<std::string, std::string> > GetTexturesPathsNames() const;
#endif
#if PROTOCOL_VERSION < 347
        const std::map<int, std::map<unsigned char, std::shared_ptr<Blockstate> > >& Blockstates() const;
#else
        const std::map<int, std::shared_ptr<Blockstate> >& Blockstates() const;
#endif
        
#if PROTOCOL_VERSION < 358
        const std::map<unsigned char, std::shared_ptr<Biome> >& Biomes() const;
        const std::shared_ptr<Biome> GetBiome(const unsigned char id);
#else
        const std::map<int, std::shared_ptr<Biome> >& Biomes() const;
        const std::shared_ptr<Biome> GetBiome(const int id);
#endif

#if PROTOCOL_VERSION < 347
        const std::map<int, std::map<unsigned char, std::shared_ptr<Item> > >& Items() const;
#else
        const std::map<int, std::shared_ptr<Item> >& Items() const;
#endif

    private:
        AssetsManager();

        void LoadBlocksFile();
        void LoadBiomesFile();
        void LoadItemsFile();
        void ClearCaches();

    private:
#if PROTOCOL_VERSION < 347
        std::map<int, std::map<unsigned char, std::shared_ptr<Blockstate> > > blockstates;
#else
        std::map<int, std::shared_ptr<Blockstate> > blockstates;
#endif
#if PROTOCOL_VERSION < 358
        std::map<unsigned char, std::shared_ptr<Biome> > biomes;
#else
        std::map<int, std::shared_ptr<Biome> > biomes;
#endif
#if PROTOCOL_VERSION < 347
        std::map<int, std::map<unsigned char, std::shared_ptr<Item> > > items;
#else
        std::map<int, std::shared_ptr<Item> > items;
#endif
    };
} // Botcraft
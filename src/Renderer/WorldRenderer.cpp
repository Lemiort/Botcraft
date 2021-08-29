#include "botcraft/Renderer/Atlas.hpp"
#include "botcraft/Renderer/Camera.hpp"
#include "botcraft/Renderer/Chunk.hpp"
#include "botcraft/Renderer/TransparentChunk.hpp"
#include "botcraft/Renderer/WorldRenderer.hpp"

#include "botcraft/Game/AssetsManager.hpp"
#include "botcraft/Game/World/Biome.hpp"
#include "botcraft/Game/World/Block.hpp"
#include "botcraft/Game/World/Blockstate.hpp"
#include "botcraft/Game/World/Chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>

namespace Botcraft
{
    namespace Renderer
    {
        WorldRenderer::WorldRenderer(const unsigned int section_height_,
            const std::vector<std::pair<std::string, std::string> >& textures_path_names)
        {
            section_height = section_height_;

            chunks = std::unordered_map<Position, std::shared_ptr<Chunk> >();
            transparent_chunks = std::unordered_map<Position, std::shared_ptr<TransparentChunk> >();

            faces_should_be_updated = true;

            camera = std::shared_ptr<Camera>(new Camera);

            //Build atlas
            atlas = std::shared_ptr<Atlas>(new Atlas);
            atlas->LoadData(textures_path_names);
        }

        WorldRenderer::~WorldRenderer()
        {
            glDeleteTextures(1, &atlas_texture);
            atlas.reset();
            camera.reset();
        }

        std::shared_ptr<Camera> WorldRenderer::GetCamera()
        {
            return camera;
        }

        void WorldRenderer::InitGL()
        {
            glGenBuffers(1, &view_uniform_buffer);
            glBindBuffer(GL_UNIFORM_BUFFER, view_uniform_buffer);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glBindBufferRange(GL_UNIFORM_BUFFER, 0, view_uniform_buffer, 0, sizeof(glm::mat4));

            //Create a texture
            glGenTextures(1, &atlas_texture);
            glBindTexture(GL_TEXTURE_2D, atlas_texture);

            //Options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->GetWidth(), atlas->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->Get());
            glGenerateMipmap(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        void WorldRenderer::UpdateViewMatrix()
        {
            if (camera->GetHasChangedOrientation() || camera->GetHasChangedPosition())
            {
                m_mutex_camera.lock();
                glm::mat4 view_matrix = camera->GetViewMatrix();
                if (camera->GetHasChangedPosition())
                {
                    transparent_chunks_mutex.lock();
                    for (auto it = transparent_chunks.begin(); it != transparent_chunks.end(); ++it)
                    {
                        it->second->SetDisplayStatus(BufferStatus::Updated);
                    }
                    transparent_chunks_mutex.unlock();
                }
                camera->ResetHasChangedPosition();
                camera->ResetHasChangedOrientation();
                m_mutex_camera.unlock();
                glBindBuffer(GL_UNIFORM_BUFFER, view_uniform_buffer);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view_matrix));
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
            }
        }

        void WorldRenderer::SetCameraProjection(const glm::mat4& proj)
        {
            std::lock_guard<std::mutex> lock(m_mutex_camera);
            camera->SetProjection(proj);
        }

        void WorldRenderer::UpdateFaces()
        {
            if (faces_should_be_updated)
            {
                faces_should_be_updated = false;
                {
                    std::lock_guard<std::mutex> lock(chunks_mutex);
                    for (auto it = chunks.begin(); it != chunks.end();)
                    {
                        it->second->Update();
                        if (it->second->GetNumFace() == 0)
                        {
                            it = chunks.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(transparent_chunks_mutex);
                    for (auto it = transparent_chunks.begin(); it != transparent_chunks.end();)
                    {
                        it->second->Update();
                        if (it->second->GetNumFace() == 0)
                        {
                            it = transparent_chunks.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
        }

        void WorldRenderer::UpdateChunk(const int x_, const int z_, const std::shared_ptr<const Botcraft::Chunk> chunk)
        {
            // Remove any previous version of this chunk
            {
                std::lock_guard<std::mutex> lock(chunks_mutex);
                Position pos(x_, 0, z_);
                for (int y = 0; y < CHUNK_HEIGHT / section_height; ++y)
                {
                    pos.y = y;
                    auto it = chunks.find(pos);
                    if (it != chunks.end())
                    {
                        it->second->ClearFaces();
                    }
                }
            }
            {
                std::lock_guard<std::mutex> lock(transparent_chunks_mutex);
                Position pos(x_, 0, z_);
                for (int y = 0; y < CHUNK_HEIGHT / CHUNK_WIDTH; ++y)
                {
                    pos.y = y;
                    auto it = transparent_chunks.find(pos);
                    if (it != transparent_chunks.end())
                    {
                        it->second->ClearFaces();
                    }
                }
            }

            if (chunk == nullptr)
            {
                faces_should_be_updated = true;
                return;
            }

            auto& AssetsManager_ = AssetsManager::getInstance();

            // For each block in the chunk, check its neighbours
            // to see which face to draw
            const std::vector<Position> neighbour_positions({ Position(0, -1, 0), Position(0, 0, -1),
                            Position(-1, 0, 0), Position(1, 0, 0), Position(0, 0, 1), Position(0, 1, 0) });

            std::vector<std::shared_ptr<Blockstate> > neighbour_blockstates(6);
            std::vector<unsigned char> neighbour_model_ids(6);

            Position pos;
            for (int y = 0; y < CHUNK_HEIGHT; ++y)
            {
                pos.y = y;
                for (int z = 0; z < CHUNK_WIDTH; ++z)
                {
                    pos.z = z;
                    for (int x = 0; x < CHUNK_WIDTH; ++x)
                    {
                        pos.x = x;

                        // If this block is air, just skip it
                        const Block* this_block = chunk->GetBlock(pos);
                        if (this_block == nullptr ||
                            this_block->GetBlockstate()->IsAir())
                        {
                            continue;
                        }

                        // Else check its neighbours to find which face to draw
                        for (int i = 0; i < 6; ++i)
                        {
                            const Block* neighbour_block = chunk->GetBlock(pos + neighbour_positions[i]);
                            if (neighbour_block == nullptr)
                            {
                                neighbour_blockstates[i] = nullptr;
                                neighbour_model_ids[i] = 0;
                            }
                            else
                            {
                                neighbour_blockstates[i] = neighbour_block->GetBlockstate();
                                neighbour_model_ids[i] = neighbour_block->GetModelId();
                            }
                        }

                        //Check if this block is sourrounded by non transparent blocks
                        for (int i = 0; i < neighbour_positions.size(); ++i)
                        {
                            if (neighbour_blockstates[i] == nullptr ||
                                neighbour_blockstates[i]->IsTransparent())
                            {
                                break;
                            }

                            //If we are here, all the neigbhours are non transparent blocks, so
                            //there is no face to add to the renderer
                            if (i == neighbour_positions.size() - 1)
                            {
                                continue;
                            }
                        }

                        //Add all faces of the current state
                        const std::vector<FaceDescriptor>& current_faces = this_block->GetBlockstate()->GetModel(this_block->GetModelId()).GetFaces();
#if PROTOCOL_VERSION < 552
                        const std::shared_ptr<Biome> current_biome = AssetsManager_.GetBiome(chunk->GetBiome(x, z));
#else
                        const std::shared_ptr<Biome> current_biome = AssetsManager_.GetBiome(chunk->GetBiome(x, y, z));
#endif

                        for (int i = 0; i < current_faces.size(); ++i)
                        {
                            //Check if the neighbour in this direction is hidding this face
                            // We also remove the faces between two transparent blocks with the same names
                            // (example: faces between two water blocks)
                            if (current_faces[i].cullface_direction == Orientation::None ||
                                !neighbour_blockstates[(int)current_faces[i].cullface_direction] ||
                                (neighbour_blockstates[(int)current_faces[i].cullface_direction]->IsTransparent() &&
                                    neighbour_blockstates[(int)current_faces[i].cullface_direction]->GetName() != this_block->GetBlockstate()->GetName())
                                )
                            {
                                AddFace(pos.x + CHUNK_WIDTH * x_, pos.y, pos.z + CHUNK_WIDTH * z_,
                                    current_faces[i].face, current_faces[i].texture_names,
                                    GetColorModifier(pos.y, current_biome, this_block->GetBlockstate(),
                                        current_faces[i].use_tintindexes));
                            }
                        }
                    }
                }
            }
            faces_should_be_updated = true;
        }

        void WorldRenderer::UseAtlasTextureGL()
        {
            glBindTexture(GL_TEXTURE_2D, atlas_texture);
        }

        void WorldRenderer::ClearFaces()
        {
            // We simply remove all the faces from all the chunks
            // The chunks will be deleted on the next frame
            {
                std::lock_guard<std::mutex> lock(chunks_mutex);
                for (auto it = chunks.begin(); it != chunks.end(); it++)
                {
                    it->second->ClearFaces();
                }
            }
            {
                std::lock_guard<std::mutex> lock(transparent_chunks_mutex);
                for (auto it = transparent_chunks.begin(); it != transparent_chunks.end(); it++)
                {
                    it->second->ClearFaces();
                }
            }
            faces_should_be_updated = true;
        }

        void WorldRenderer::SetPosOrientation(const double x_, const double y_, const double z_, const float yaw_, const float pitch_)
        {
            if (camera)
            {
                std::lock_guard<std::mutex> lock(m_mutex_camera);
                camera->SetPosition((float)x_, (float)y_, (float)z_);
                camera->SetRotation(pitch_, yaw_);
            }
        }

        void WorldRenderer::RenderFaces(int* num_chunks_, int* num_rendered_chunks_,
            int* num_faces_, int* num_rendered_faces_)
        {
            const std::array<glm::vec4, 6>& frustum_planes = camera->GetFrustumPlanes();

            // Get a list of all loaded chunks
            std::unordered_set<Position> all_loaded_chunks;
            all_loaded_chunks.reserve(chunks.size() + transparent_chunks.size());
            int num_faces = 0;

            chunks_mutex.lock();
            for (auto it = chunks.begin(); it != chunks.end(); ++it)
            {
                all_loaded_chunks.insert(it->first);
                num_faces += it->second->GetNumFace();
            }
            chunks_mutex.unlock();
            transparent_chunks_mutex.lock();
            for (auto it = transparent_chunks.begin(); it != transparent_chunks.end(); ++it)
            {
                all_loaded_chunks.insert(it->first);
                num_faces += it->second->GetNumFace();
            }
            transparent_chunks_mutex.unlock();
            const int num_chunks = all_loaded_chunks.size();

            // Apply frustum culling to render only the visible ones
            std::vector<Position> chunks_to_render;
            chunks_to_render.reserve(all_loaded_chunks.size());
            // Frustum culling algorithm from http://old.cescg.org/CESCG-2002/DSykoraJJelinek/
            for (auto it = all_loaded_chunks.begin(); it != all_loaded_chunks.end(); ++it)
            {
                FrustumResult result = FrustumResult::Inside;

                const float min_x = CHUNK_WIDTH * (*it).x;
                const float max_x = CHUNK_WIDTH * ((*it).x + 1);
                const float min_y = (int)section_height * (*it).y;
                const float max_y = (int)section_height * ((*it).y + 1);
                const float min_z = CHUNK_WIDTH * (*it).z;
                const float max_z = CHUNK_WIDTH * ((*it).z + 1);

                for (int i = 0; i < 6; ++i)
                {
                    const bool sign_x = frustum_planes[i].x > 0.0f;
                    const bool sign_y = frustum_planes[i].y > 0.0f;
                    const bool sign_z = frustum_planes[i].z > 0.0f;

                    const glm::vec4 p_vertex = glm::vec4(sign_x ? max_x : min_x, sign_y ? max_y : min_y, sign_z ? max_z : min_z, 1.0f);

                    if (glm::dot(frustum_planes[i], p_vertex) < 0.0f)
                    {
                        result = FrustumResult::Outside;
                        break;
                    }

                    const glm::vec4 n_vertex = glm::vec4(sign_x ? min_x : max_x, sign_y ? min_y : max_y, sign_z ? min_z : max_z, 1.0f);

                    if (glm::dot(frustum_planes[i], n_vertex) < 0.0f)
                    {
                        result = FrustumResult::Intersect;
                    }
                }

                if (result != FrustumResult::Outside)
                {
                    chunks_to_render.push_back(*it);
                }
            }

            // Sort the chunks from far to near the camera
            m_mutex_camera.lock();
            std::sort(chunks_to_render.begin(), chunks_to_render.end(), [this](const Position& p1, const Position& p2) {return this->DistanceToCamera(p1) > this->DistanceToCamera(p2); });
            m_mutex_camera.unlock();

            // Render all non partially transparent faces
            const int num_rendered_chunks = chunks_to_render.size();
            int num_rendered_faces = 0;
            chunks_mutex.lock();
            for (int i = 0; i < num_rendered_chunks; ++i)
            {
                auto found_it = chunks.find(chunks_to_render[i]);
                if (found_it != chunks.end())
                {
                    found_it->second->Render();
                    num_rendered_faces += found_it->second->GetNumFace();
                }
            }
            chunks_mutex.unlock();

            m_mutex_camera.lock();
            const glm::vec3 cam_pos = camera->GetPosition();
            m_mutex_camera.unlock();

            // Render all partially transparent faces
            transparent_chunks_mutex.lock();
            for (int i = 0; i < num_rendered_chunks; ++i)
            {
                auto found_it = transparent_chunks.find(chunks_to_render[i]);
                if (found_it != transparent_chunks.end())
                {
                    found_it->second->Sort(cam_pos);
                    found_it->second->Render();
                    num_rendered_faces += found_it->second->GetNumFace();
                }
            }
            transparent_chunks_mutex.unlock();

            if (num_chunks_)
            {
                *num_chunks_ = num_chunks;
            }
            if (num_rendered_chunks_)
            {
                *num_rendered_chunks_ = num_rendered_chunks;
            }
            if (num_faces_)
            {
                *num_faces_ = num_faces;
            }
            if (num_rendered_faces_)
            {
                *num_rendered_faces_ = num_rendered_faces;
            }
        }

        void WorldRenderer::AddFace(const int x_, const int y_, const int z_, const Face& face_, const std::vector<std::string>& texture_identifiers_, const std::vector<unsigned int>& texture_multipliers_)
        {
            std::array<unsigned short, 4> atlas_pos = { 0 };
            std::array<unsigned short, 4> atlas_size = { 0 };
            std::array<Transparency, 2> transparencies = { Transparency::Opaque };
            std::array<Animation, 2> animated = { Animation::Static };

            for (int i = 0; i < std::min(2, (int)texture_identifiers_.size()); ++i)
            {
                const std::pair<int, int>& atlas_coords = atlas->GetPosition(texture_identifiers_[i]);
                atlas_pos[2 * i + 0] = atlas_coords.first;
                atlas_pos[2 * i + 1] = atlas_coords.second;
                const std::pair<int, int>& atlas_dims = atlas->GetSize(texture_identifiers_[i]);
                atlas_size[2 * i + 0] = atlas_dims.first;
                atlas_size[2 * i + 1] = atlas_dims.second;
                transparencies[i] = atlas->GetTransparency(texture_identifiers_[i]);
                animated[i] = atlas->GetAnimation(texture_identifiers_[i]);
            }

            std::array<unsigned int, 2> texture_multipliers = { 0xFFFFFFFF };
            for (int i = 0; i < std::min(2, (int)texture_multipliers_.size()); ++i)
            {
                texture_multipliers[i] = texture_multipliers_[i];
            }

            Face face(face_);
            face.SetTextureMultipliers(texture_multipliers);

            std::array<float, 4> coords = face.GetTextureCoords(false);
            unsigned short height_normalizer = animated[0] == Animation::Animated ? atlas_size[0] : atlas_size[1];
            for (int i = 0; i < 2; ++i)
            {
                coords[2 * i + 0] = (atlas_pos[0] + coords[2 * i + 0] / 16.0f * atlas_size[0]) / atlas->GetWidth();
                coords[2 * i + 1] = (atlas_pos[1] + coords[2 * i + 1] / 16.0f * height_normalizer) / atlas->GetHeight();
            }
            face.SetTextureCoords(coords, false);

            if (texture_identifiers_.size() > 1)
            {
                coords = face.GetTextureCoords(true);
                height_normalizer = animated[1] == Animation::Animated ? atlas_size[2] : atlas_size[3];
                for (int i = 0; i < 2; ++i)
                {
                    coords[2 * i + 0] = (atlas_pos[2] + coords[2 * i + 0] / 16.0f * atlas_size[2]) / atlas->GetWidth();
                    coords[2 * i + 1] = (atlas_pos[3] + coords[2 * i + 1] / 16.0f * height_normalizer) / atlas->GetHeight();
                }
                face.SetTextureCoords(coords, true);
            }

            //Add 0.5 because the origin of the block is at the center
            //but the coordinates start from the block corner
            face.GetMatrix()[12] += x_ + 0.5f;
            face.GetMatrix()[13] += y_ + 0.5f;
            face.GetMatrix()[14] += z_ + 0.5f;

            const Transparency transparency = transparencies[0];
            if (transparency == Transparency::Opaque)
            {
                face.SetDisplayBackface(false);
            }

            const Position position((int)floor(x_ / (double)CHUNK_WIDTH), (int)floor(y_ / (double)section_height), (int)floor(z_ / (double)CHUNK_WIDTH));

            if (transparency == Transparency::Partial)
            {
                std::lock_guard<std::mutex> lock(transparent_chunks_mutex);
                auto chunk_it = transparent_chunks.find(position);
                if (chunk_it == transparent_chunks.end())
                {
                    transparent_chunks[position] = std::shared_ptr<TransparentChunk>(new TransparentChunk);
                }
                transparent_chunks[position]->AddFace(face);
            }
            else
            {
                std::lock_guard<std::mutex> lock(chunks_mutex);
                auto chunk_it = chunks.find(position);
                if (chunk_it == chunks.end())
                {
                    chunks[position] = std::shared_ptr<Chunk>(new Chunk);
                }
                chunks[position]->AddFace(face);
            }
        }

        const std::vector<unsigned int> WorldRenderer::GetColorModifier(const int y, const std::shared_ptr<Biome> biome, const std::shared_ptr<Blockstate> blockstate, const std::vector<bool>& use_tintindex) const
        {
            std::vector<unsigned int> texture_modifier(use_tintindex.size(), 0xFFFFFFFF);
            for (int i = 0; i < use_tintindex.size(); ++i)
            {
                switch (blockstate->GetTintType())
                {
                case TintType::None:
                    break;
                case TintType::Grass:
                    if (use_tintindex[i])
                    {
                        if (biome)
                        {
                            texture_modifier[i] = biome->GetColorMultiplier(y, true);
                        }
                    }
                    break;
                case TintType::Leaves:
                    if (use_tintindex[i])
                    {
                        if (biome)
                        {
                            texture_modifier[i] = biome->GetColorMultiplier(y, false);
                        }
                    }
                    break;
                    //Something like black when signal strength is 0 and red when it's 15
                case TintType::Redstone:
#if PROTOCOL_VERSION == 340 // 1.12.2
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * blockstate->GetMetadata());
#elif PROTOCOL_VERSION == 393 // 1.13
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 1752) / 9) % 16));
#elif PROTOCOL_VERSION == 401 || PROTOCOL_VERSION == 404 // 1.13.1 && 1.13.2
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 1753) / 9) % 16));
#elif PROTOCOL_VERSION == 477 || PROTOCOL_VERSION == 480 || PROTOCOL_VERSION == 485 || PROTOCOL_VERSION == 490 || PROTOCOL_VERSION == 498 // 1.14.X
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 2056) / 9) % 16));
#elif PROTOCOL_VERSION == 573 || PROTOCOL_VERSION == 575 || PROTOCOL_VERSION == 578 // 1.15.X
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 2056) / 9) % 16));
#elif PROTOCOL_VERSION == 735 || PROTOCOL_VERSION == 736 || PROTOCOL_VERSION == 751 || PROTOCOL_VERSION == 753  || PROTOCOL_VERSION == 754 // 1.16.X
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 2058) / 9) % 16));
#elif PROTOCOL_VERSION == 755 || PROTOCOL_VERSION == 756 // 1.17.X
                    texture_modifier[i] = 0xFF000000 | (25 + 15 * (((blockstate->GetId() - 2114) / 9) % 16));
#else
                    #error "Protocol version not implemented"
#endif
                    break;
                case TintType::Water:
                    if (biome)
                    {
                        texture_modifier[i] = biome->GetWaterColorMultiplier();
                    }
                    break;
                default:
                    break;
                }
            }
            return texture_modifier;
        }

        const float WorldRenderer::DistanceToCamera(const Position& chunk) const
        {
            return camera->GetDistance(CHUNK_WIDTH * (chunk.x + 0.5f), section_height * (chunk.y + 0.5f), CHUNK_WIDTH * (chunk.z + 0.5f));
        }
    } // Renderer
} // Botcraft
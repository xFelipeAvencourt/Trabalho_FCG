#include "../include/Collisions.h"
#include "../include/Personagem.h"
#include "../include/Constantes.h"
#include "../include/SceneObject.h"
#include <glm/glm.hpp>
#include <algorithm>

void PlayerWallCollision(Camera &player, bool ghostMode) {
    if (ghostMode) return;

    const float margin = PLAYER_HEIGHT * 2.0f + 0.05f;

    const float minX = -SCALE_FLOUR + margin;
    const float maxX =  SCALE_FLOUR - margin;
    const float minZ = -SCALE_FLOUR + margin;
    const float maxZ =  SCALE_FLOUR - margin;
    
    if(player.Position.x < minX) player.Position.x = minX;
    if(player.Position.x > maxX) player.Position.x = maxX;
    if(player.Position.z < minZ) player.Position.z = minZ;
    if(player.Position.z > maxZ) player.Position.z = maxZ;
    
}

void PlayerObjectCollision(Camera &player, const std::map<int, struct AABB>& aabbList) {
    
    const float margin = PLAYER_HEIGHT / 2.0f;

    
    for (const auto& pair : aabbList) {

        const AABB& aabb = pair.second;

        const float playerMinX = player.Position.x - PLAYER_HEIGHT / 2.0f;
        const float playerMaxX = player.Position.x + PLAYER_HEIGHT / 2.0f;
        const float playerMinZ = player.Position.z - PLAYER_HEIGHT / 2.0f;
        const float playerMaxZ = player.Position.z + PLAYER_HEIGHT / 2.0f;
        const float playerMinY = player.Position.y - PLAYER_HEIGHT / 2.0f;
        const float playerMaxY = player.Position.y + PLAYER_HEIGHT / 2.0f;

        const float objectMinX = aabb.min.x - margin;
        const float objectMaxX = aabb.max.x + margin;
        const float objectMinZ = aabb.min.z - margin;
        const float objectMaxZ = aabb.max.z + margin;
        const float objectMinY = aabb.min.y - margin;
        const float objectMaxY = aabb.max.y + margin;


        bool collisionX = playerMaxX >= objectMinX && playerMinX <= objectMaxX;
        bool collisionZ = playerMaxZ >= objectMinZ && playerMinZ <= objectMaxZ;
        bool collisionY = playerMaxY >= objectMinY && playerMinY <= objectMaxY;

        bool check = collisionX && collisionZ && collisionY;


        if (check){
            float overlapLeft   = (player.Position.x + PLAYER_HEIGHT / 2.0f) - aabb.min.x;
            float overlapRight  = aabb.max.x - (player.Position.x - PLAYER_HEIGHT / 2.0f);
            float overlapTop    = (player.Position.z + PLAYER_HEIGHT / 2.0f) - aabb.min.z;
            float overlapBottom = aabb.max.z - (player.Position.z - PLAYER_HEIGHT / 2.0f);
            float overlapUp     = (player.Position.y + PLAYER_HEIGHT / 2.0f) - aabb.min.y;
            float overlapDown   = aabb.max.y - (player.Position.y - PLAYER_HEIGHT / 2.0f);

            float minOverlapX = std::min(overlapLeft, overlapRight);
            float minOverlapZ = std::min(overlapTop, overlapBottom);
            float minOverlapY = std::min(overlapUp, overlapDown);

            if (minOverlapX < minOverlapZ && minOverlapX < minOverlapY) {
                if (overlapLeft < overlapRight)
                    player.Position.x -= overlapLeft;
                else
                    player.Position.x += overlapRight;
            } else {
                if (overlapTop < overlapBottom)
                    player.Position.z -= overlapTop;
                else
                    player.Position.z += overlapBottom;
            }
        }
    }
}
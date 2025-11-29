#include "../include/collisions.h"
#include "../include/Personagem.h"
#include "../include/Constantes.h"
#include <glm/glm.hpp>
#include <algorithm>

void PlayerCollision(Camera &player, bool ghostMode) {
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

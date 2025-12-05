#pragma once
#include <map>
#include <glm/glm.hpp>
#include <Constantes.h>

class Camera;
void PlayerWallCollision(Camera &player, bool ghostMode, bool door, GameState* gameState);
void PlayerObjectCollision(Camera &player, bool ghostMode, const std::map<int, struct AABB>& aabbList);
bool CheckSafe(const glm::vec3& position);
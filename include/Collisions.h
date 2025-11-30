#pragma once
#include <map>
#include <glm/glm.hpp>
class Camera;
void PlayerWallCollision(Camera &player, bool ghostMode);
void PlayerObjectCollision(Camera &player, const std::map<int, struct AABB>& aabbList);
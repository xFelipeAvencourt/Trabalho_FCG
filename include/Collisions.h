#pragma once
#include <map>
#include <glm/glm.hpp>
class Camera;
void PlayerWallCollision(Camera &player, bool ghostMode, bool door);
void PlayerObjectCollision(Camera &player, const std::map<int, struct AABB>& aabbList);
bool CheckSafe(const glm::vec3& position);
#ifndef AI_H
#define AI_H
#include "defs.h"
#include "snake.h"
/* Compute best next direction for the AI snake toward food, avoiding obstacles. */
void ai_think(Snake *ai, const Snake *player, Point food,
              const Point *obs, int obs_count);
#endif

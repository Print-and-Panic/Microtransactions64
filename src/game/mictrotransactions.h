#ifndef MICROTRANSACTIONS_H
#define MICROTRANSACTIONS_H

#include <PR/ultratypes.h>

#include "macros.h"
#include "types.h"
#define CREDITS_PER_A_PRESS 5
#define FRAMES_AT_SPEED 30
#define WALK_SPEED 8.0f
s32 can_afford(struct MarioState *m, s32 credits);
void deduct_credits(struct MarioState *m, s32 credits);
void update_credit_deduction(struct MarioState *m);
void credit_deduction(struct MarioState *m);

#endif // MICROTRANSACTIONS_H
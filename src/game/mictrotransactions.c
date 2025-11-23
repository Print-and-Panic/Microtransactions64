#include "mictrotransactions.h"

s32 can_afford(struct MarioState *m, s32 credits) {
    if (m->numCredits < credits) {
        return FALSE;
    }
   deduct_credits(m, credits);
    return TRUE;
}

void deduct_credits(struct MarioState *m, s32 credits) {
    m->numCredits -= credits;
    m->framesSinceCreditDeduction = 0;
}

void update_credit_deduction(struct MarioState *m) {
    m->framesSinceCreditDeduction++;
}


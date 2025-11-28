#include "microtransactions.h"

#include "audio/external.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "sounds.h"
#include "usb/usb.h"

// #define EMULATOR_MODE 1
volatile u8 gDebugLastUsbByte = 0;

// --- THREAD & STACK ---
static u64 gCapitalismThreadStack[0x2000 / sizeof(u64)];
static OSThread gCapitalismThread;

// --- MAILBOXES ---
// 1. To send coins to the Game Thread
static OSMesgQueue gCapitalismMq;
static OSMesg gCapitalismMsgBuf[16]; 

static OSTimer gSleepTimer;
static OSMesgQueue gTimerMq;
static OSMesg gTimerMsgBuf[1];

// --- THE WALLET ---
volatile s32 gGlobalWallet = 0; // Persistent storage


s32 can_afford(struct MarioState *m, s32 credits) {
    if (m->numCredits < credits) {
        return FALSE;
    }
   deduct_credits(m, credits);
    return TRUE;
}

void deduct_credits(struct MarioState *m, s32 credits) {
    m->numCredits -= credits;
    gGlobalWallet -= credits;
    m->framesSinceCreditDeduction = 0;
}

void update_credit_deduction(struct MarioState *m) {
    m->framesSinceCreditDeduction++;
}

// --- THE WORKER THREAD ---

void capitalism_thread_entry(void *arg) {
    
    // Setup: Create a queue for the timer
    osCreateMesgQueue(&gTimerMq, gTimerMsgBuf, 1);
    // Wait for ~2 seconds (60 frames) before entering the read loop
    for (int i = 0; i < 60; i++) {
        osSetTimer(&gSleepTimer, 1562500, 0, &gTimerMq, (OSMesg)1);
        osRecvMesg(&gTimerMq, NULL, OS_MESG_BLOCK);
    }
    
    #ifdef EMULATOR_MODE
    int mockTimer = 0;
    #else
    u8 gCoinBuffer[16];
    usb_initialize();
    #endif

    while (1) {
        // STEP 1: Sleep for 1 Frame (30 FPS Target)
        // N64 CPU Counter Frequency = 46,875,000 Hz
        // 1 Frame = 1/30th of a second
        // 46,875,000 / 30 = 1,562,500 Cycles
        
        // Use 1.5 Million cycles for a stable 30hz heartbeat
        osSetTimer(&gSleepTimer, 1562500, 0, &gTimerMq, (OSMesg)1);
        
        // BLOCK here until timer finishes
        osRecvMesg(&gTimerMq, NULL, OS_MESG_BLOCK);

#ifdef EMULATOR_MODE
        // --- MOCK LOGIC ---
        // Since the loop runs 30 times a second:
        mockTimer++;
        
        // Every 10 seconds (300 frames)
        if (mockTimer >= 300) { 
            mockTimer = 0;
            // Send 25 cents
            osSendMesg(&gCapitalismMq, (OSMesg)(u32)25, OS_MESG_NOBLOCK);
        }
#else
       // 1. POLL
        // usage: usb_poll() reads the header/footer internally.
        // It returns a Header Integer if successful, or 0 if empty/garbage.
        u32 header = usb_poll();
        
        if (header != 0) {
            // Extract size from the header macro
            int size = USBHEADER_GETSIZE(header);
            
            // 2. READ
            // We only expect 1 byte (the coin), but use the size reported by the packet
            if (size > 0) {
                usb_read(gCoinBuffer, size);
                
                u8 val = gCoinBuffer[0];
                
                // Debug to screen
                gDebugLastUsbByte = val;

                // Send to game logic
                if (val != 0) {
                    osSendMesg(&gCapitalismMq, (OSMesg)(u32)val, OS_MESG_NOBLOCK);
                }
            }
        }
#endif
    }
}

void capitalism_init(void) {
    // 1. Initialize the Income Mailbox
    osCreateMesgQueue(&gCapitalismMq, gCapitalismMsgBuf, 16);

    // 2. Start the Thread
    osCreateThread(
        &gCapitalismThread, 
        20, 
        capitalism_thread_entry, 
        NULL, 
        // FIX: Point to the END of the stack (High Address)
        // Removing the "-1" aligns it correctly for most N64 SDKs
        gCapitalismThreadStack + (0x2000 / sizeof(u64)), 
        8 // Priority 8 (Below Game/Audio)
    );
    osStartThread(&gCapitalismThread);
}

void capitalism_update(void) {
    OSMesg msg;
    // Check mailbox without blocking
    while (osRecvMesg(&gCapitalismMq, &msg, OS_MESG_NOBLOCK) == 0) {
        s32 amount = (s32)msg;
        gGlobalWallet += amount;

        // Sync to Mario State if active
        if (gMarioState != NULL) { 
            gMarioState->numCredits = gGlobalWallet;
        }
        
        // Satisfaction Sound
        play_sound(SOUND_GENERAL_COIN_DROP, gGlobalSoundSource);
    }
}

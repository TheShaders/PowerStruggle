#ifndef __MODEL_H__
#define __MODEL_H__

#include <platform_gfx.h>
#include <gfx.h>

#include <types.h>

///////////////////
// Model defines //
///////////////////

typedef struct BoneLayer_t {
    DrawLayer layer;        // Draw layer of this displaylist
    Gfx *displaylist; // Displaylist for this layer of the bone
} BoneLayer;

typedef struct Bone_t {
    uint8_t index;    // Not strictly needed, but helpful to save time during processing
    uint8_t parent;   // Index of the parent bone
    uint8_t numLayers;   // Number of layers this bone is present on
    uint8_t reserved; // For future use
    float posX; // Base positional offset x component (float to save conversion time later on)
    float posY; // Base positional offset y component
    float posZ; // Base positional offset y component
    BoneLayer *layers; // Segmented pointer to the start of the displaylist array (of length layers)
    Gfx *(*beforeCb)(Bone* bone, BoneLayer *layer); // Callback before drawing a layer of this bone, can return a displaylist to prepend before the bone
    Gfx *(*afterCb) (Bone* bone, BoneLayer *layer); // Callback after drawing a layer of this bone, can return a displaylist to append after the bone
    MtxF *matrix; // Used during rendering for hierarchical rendering
} Bone;

typedef struct Model_t {
    uint16_t numBones; // Bone count
    uint16_t reserved; // For future use
    Bone* bones;  // Segmented pointer to the start of the bone array (of length numBones)
} Model;

///////////////////////
// Animation defines //
///////////////////////

// Animation flags
#define ANIM_LOOP (1 << 0)
#define ANIM_HAS_TRIGGERS (1 << 1)

// Bone table flags
#define CHANNEL_POS_X (1 << 0)
#define CHANNEL_POS_Y (1 << 1)
#define CHANNEL_POS_Z (1 << 2)
#define CHANNEL_ROT_X (1 << 3)
#define CHANNEL_ROT_Y (1 << 4)
#define CHANNEL_ROT_Z (1 << 5)
#define CHANNEL_SCALE_X (1 << 6)
#define CHANNEL_SCALE_Y (1 << 7)
#define CHANNEL_SCALE_Z (1 << 8)

typedef struct Animation_t {
    uint16_t frameCount; // The number of frames of data this animation has
    uint8_t boneCount; // Number of bones this animation has data for
    uint8_t flags; // Flags for this animation
    BoneTable *boneTables; // Segmented pointer to the array of bone animation tables
    AnimTrigger *triggers; // Segmented pointer to the array of triggers for this animation
} Animation;

typedef struct BoneTable_t {
    uint32_t flags; // Flags to specify which channels are encoded in this bone's animation data
    int16_t *channels; // Segmented pointer to the array of all channel data
} BoneTable;

typedef struct AnimTrigger_t {
    uint32_t frame; // The frame at which this trigger should run
    void (*triggerCb)(Model* model, uint32_t frame); // The callback to run at the specified frame
} AnimTrigger;

#define ANIM_COUNTER_FACTOR 16.0f
#define ANIM_COUNTER_SHIFT 4
#define ANIM_COUNTER_TO_FRAME(x) ((x) >> (ANIM_COUNTER_SHIFT))

typedef struct AnimState_t {
    Animation* anim;
    uint16_t counter; // Frame counter of format 12.4
    int8_t speed; // Animation playback speed of format s3.4
    int8_t triggerIndex; // Index of the previous trigger
} AnimState;

Gfx *gfxCbBeforeBillboard(Bone* bone, BoneLayer *layer);
Gfx *gfxCbAfterBillboard(Bone* bone, BoneLayer *layer);

#endif

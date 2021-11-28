#include <glm/glm.hpp>
#include <types.h>
#include <gfx.h>
#include <platform_gfx.h>
#include <mem.h>
#include <ecs.h>
#include <model.h>
#include <files.h>
#include <behaviors.h>

#include <cmath>

void drawAnimatedModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_ANIM_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_ANIM_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_ANIM_MODEL)]);
    AnimState *curAnimState = static_cast<AnimState *>(componentArrays[COMPONENT_INDEX(AnimState, ARCHETYPE_ANIM_MODEL)]);

    while (count)
    {
        Animation *anim = segmentedToVirtual(curAnimState->anim);

        gfx::push_mat();
         gfx::apply_translation_affine((*curPos)[0], (*curPos)[1], (*curPos)[2]);
         gfx::rotate_euler_xyz((*curRot)[0], (*curRot)[1], (*curRot)[2]);
          drawModel(*curModel, anim, ANIM_COUNTER_TO_FRAME(curAnimState->counter));
        gfx::pop_mat();

        curAnimState->counter += curAnimState->speed;
#ifdef FPS30
        curAnimState->counter += curAnimState->speed;
#endif
        if (anim)
        {
            if (curAnimState->counter >= (anim->frameCount << (ANIM_COUNTER_SHIFT)))
            {
                if (anim->flags & ANIM_LOOP)
                {
                    curAnimState->counter -= (anim->frameCount << (ANIM_COUNTER_SHIFT));
                }
                else
                {
                    curAnimState->counter = (anim->frameCount - 1) << (ANIM_COUNTER_SHIFT);
                }
            }
        }

        count--;
        curPos++;
        curRot++;
        curModel++;
        curAnimState++;
    }
}

void drawModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_MODEL)]);

    while (count)
    {
        gfx::push_mat();
         gfx::apply_translation_affine((*curPos)[0], (*curPos)[1], (*curPos)[2]);
         gfx::rotate_euler_xyz((*curRot)[0], (*curRot)[1], (*curRot)[2]);
          drawModel(*curModel, nullptr, 0);
        gfx::pop_mat();
        count--;
        curPos++;
        curRot++;
        curModel++;
    }
}

void drawModelsNoRotation(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_MODEL_NO_ROTATION)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_MODEL_NO_ROTATION)]);

    while (count)
    {
        gfx::push_mat();
         gfx::apply_translation_affine((*curPos)[0], (*curPos)[1], (*curPos)[2]);
          drawModel(*curModel, nullptr, 0);
        gfx::pop_mat();
        count--;
        curPos++;
        curModel++;
    }
}

void drawResizableAnimatedModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model, Resizable
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_SCALED_ANIM_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_SCALED_ANIM_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_SCALED_ANIM_MODEL)]);
    AnimState *curAnimState = static_cast<AnimState *>(componentArrays[COMPONENT_INDEX(AnimState, ARCHETYPE_SCALED_ANIM_MODEL)]);
    float *curScale = static_cast<float *>(componentArrays[COMPONENT_INDEX(Scale, ARCHETYPE_SCALED_ANIM_MODEL)]);

    while (count)
    {
        Animation *anim = segmentedToVirtual(curAnimState->anim);

        gfx::push_mat();
         gfx::apply_translation_affine((*curPos)[0], (*curPos)[1], (*curPos)[2]);
         gfx::rotate_euler_xyz((*curRot)[0], (*curRot)[1], (*curRot)[2]);
         gfx::apply_scale_affine(*curScale, *curScale, *curScale);
          drawModel(*curModel, anim, ANIM_COUNTER_TO_FRAME(curAnimState->counter));
        gfx::pop_mat();

        curAnimState->counter += curAnimState->speed;
#ifdef FPS30
        curAnimState->counter += curAnimState->speed;
#endif
        if (anim)
        {
            if (curAnimState->counter >= (anim->frameCount << (ANIM_COUNTER_SHIFT)))
            {
                if (anim->flags & ANIM_LOOP)
                {
                    curAnimState->counter -= (anim->frameCount << (ANIM_COUNTER_SHIFT));
                }
                else
                {
                    curAnimState->counter = (anim->frameCount - 1) << (ANIM_COUNTER_SHIFT);
                }
            }
        }

        count--;
        curPos++;
        curRot++;
        curModel++;
        curAnimState++;
        curScale++;
    }
}

void drawResizableModels(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation, Model, Resizable
    Vec3 *curPos = static_cast<Vec3 *>(componentArrays[COMPONENT_INDEX(Position, ARCHETYPE_SCALED_MODEL)]);
    Vec3s *curRot = static_cast<Vec3s *>(componentArrays[COMPONENT_INDEX(Rotation, ARCHETYPE_SCALED_MODEL)]);
    Model **curModel = static_cast<Model **>(componentArrays[COMPONENT_INDEX(Model, ARCHETYPE_SCALED_MODEL)]);
    float *curScale = static_cast<float *>(componentArrays[COMPONENT_INDEX(Scale, ARCHETYPE_SCALED_MODEL)]);

    while (count)
    {
        gfx::push_mat();
         gfx::apply_translation_affine((*curPos)[0], (*curPos)[1], (*curPos)[2]);
         gfx::rotate_euler_xyz((*curRot)[0], (*curRot)[1], (*curRot)[2]);
         gfx::apply_scale_affine(*curScale, *curScale, *curScale);
          drawModel(*curModel, nullptr, 0);
        gfx::pop_mat();
        count--;
        curPos++;
        curRot++;
        curModel++;
        curScale++;
    }
}

void mtxf_align_camera(MtxF dest, MtxF mtx, Vec3 position, int16_t angle) {
    int16_t xrot;
    int16_t yrot;
    float cx, cy, cz;
    Vec3 colX, colY, colZ; // Column vectors
    float sx, sy, sz; // Scale
    MtxF scaleMat;

    dest[3][0] =
        mtx[0][0] * position[0] + mtx[1][0] * position[1] + mtx[2][0] * position[2] + mtx[3][0];
    dest[3][1] =
        mtx[0][1] * position[0] + mtx[1][1] * position[1] + mtx[2][1] * position[2] + mtx[3][1];
    dest[3][2] =
        mtx[0][2] * position[0] + mtx[1][2] * position[1] + mtx[2][2] * position[2] + mtx[3][2];
    dest[3][3] = 1;
    xrot = -atan2s(dest[3][2], dest[3][0]);
    yrot = atan2s(dest[3][2], dest[3][1]);
    cx = cossf(xrot);
    cy = cossf(yrot);
    cz = cossf(angle);

    colX[0] = mtx[0][0];
    colX[1] = mtx[1][0];
    colX[2] = mtx[2][0];

    colY[0] = mtx[0][1];
    colY[1] = mtx[1][1];
    colY[2] = mtx[2][1];

    colZ[0] = mtx[0][2];
    colZ[1] = mtx[1][2];
    colZ[2] = mtx[2][2];

    sx = std::sqrt(VEC3_DOT(colX, colX));
    sy = std::sqrt(VEC3_DOT(colY, colY));
    sz = std::sqrt(VEC3_DOT(colZ, colZ));

    guScaleF(scaleMat, sx, sy, sz);

    dest[2][0] = sinsf(xrot);
    dest[0][2] = -dest[2][0];
    dest[1][2] = sinsf(yrot);
    dest[2][1] = -dest[1][2];
    dest[0][1] = sinsf(angle);
    dest[1][0] = -dest[0][1];

    dest[0][0] = -cx * cz;
    dest[1][1] = -cy * cz;
    dest[2][2] = -cx * -cy;

    dest[0][3] = 0;
    dest[1][3] = 0;
    dest[2][3] = 0;
    
    mtxfMul(dest, dest, scaleMat);
}

void drawAllEntities()
{
    // Draw all non-resizable entities that have a model and no rotation or animation
    iterateOverEntities(drawModelsNoRotation, nullptr, ARCHETYPE_MODEL_NO_ROTATION, Bit_Rotation | Bit_AnimState | Bit_Scale);
    // Draw all non-resizable entities that have a model and no animation
    iterateOverEntities(drawModels, nullptr, ARCHETYPE_MODEL, Bit_AnimState | Bit_Scale);
    // Draw all non-resizable entities that have a model and an animation
    iterateOverEntities(drawAnimatedModels, nullptr, ARCHETYPE_ANIM_MODEL, Bit_Scale);
    // Draw all resizable entities that have a model and no animation
    iterateOverEntities(drawResizableModels, nullptr, ARCHETYPE_SCALED_MODEL, Bit_AnimState);
    // Draw all resizable entities that have a model and an animation
    iterateOverEntities(drawResizableAnimatedModels, nullptr, ARCHETYPE_SCALED_ANIM_MODEL, 0);

    animateTextures();
    scrollTextures();
}

Model* head_model = nullptr;

void draw_enemy_heads_callback(size_t count, void *arg, void **componentArrays)
{
    Vec3* player_pos = reinterpret_cast<Vec3*>(arg);
    Vec3* cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_CONTROLLABLE);
    BehaviorState* cur_bhv = get_component<Bit_Behavior, BehaviorState>(componentArrays, ARCHETYPE_CONTROLLABLE);

    while (count)
    {
        BaseEnemyState* enemy_state = reinterpret_cast<BaseEnemyState*>(cur_bhv->data.data());

        int rot = atan2s((*player_pos)[2] - (*cur_pos)[2], (*player_pos)[0] - (*cur_pos)[0]);

        gfx::push_mat();
         gfx::apply_translation_affine((*cur_pos)[0], (*cur_pos)[1] + enemy_state->definition->base.head_y_offset, (*cur_pos)[2]);
         gfx::rotate_euler_xyz(0, rot, 0);
         drawModel(head_model, nullptr, 0);

        gfx::pop_mat();

        cur_pos++;
        cur_bhv++;
        count--;
    }
}

extern Entity* g_PlayerEntity;

void draw_enemy_heads()
{
    if (head_model == nullptr)
    {
        head_model = load_model("models/Head_Enemy");
    }
    void* player_components[NUM_COMPONENTS(ARCHETYPE_PLAYER) + 1];
    getEntityComponents(g_PlayerEntity, player_components);
    Vec3* player_pos = get_component<Bit_Position, Vec3>(player_components, ARCHETYPE_PLAYER);
    // Draw all non-resizable entities that have a model and no rotation or animation
    iterateOverEntities(draw_enemy_heads_callback, player_pos, ARCHETYPE_CONTROLLABLE, 0);
}

void drawAllEntitiesHealth()
{
    // Iterate over everything that has a health bar
    iterateOverEntities(drawHealthBars, nullptr, ARCHETYPE_HEALTHBAR, 0);
}

Gfx *gfxCbBeforeBillboard(UNUSED Joint* bone, UNUSED JointMeshLayer *layer)
{
    MtxF billboardMtx;
    Vec3 offset = {0.0f, 0.0f, 0.0f};
    mtxf_align_camera(billboardMtx, *g_curMatFPtr, offset, 0);

    gfx::push_load_mat(&billboardMtx);

    return nullptr;
}

Gfx *gfxCbAfterBillboard(UNUSED Joint* bone, UNUSED JointMeshLayer *layer)
{
    gfx::pop_mat();
    
    return nullptr;
}

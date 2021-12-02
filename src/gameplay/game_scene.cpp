#include <cmath>

#include <gameplay.h>
#include <camera.h>
#include <mathutils.h>
#include <player.h>
#include <physics.h>
#include <ecs.h>
#include <main.h>
#include <files.h>
#include <input.h>
#include <collision.h>
#include <behaviors.h>
#include <control.h>
#include <text.h>

extern "C" {
#include <debug.h>
}

extern GridDefinition get_grid_definition(const char *file);

GameplayScene::GameplayScene() : grid_{}
{
}

// LoadHandle handle;

#define ARCHETYPE_TESTHITBOX (ARCHETYPE_CYLINDER_HITBOX | Bit_Model)

void createHitboxCallback(UNUSED size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Position, Rotation Model, Hitbox
    Vec3* pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_TESTHITBOX);
    Model** model = get_component<Bit_Model, Model*>(componentArrays, ARCHETYPE_TESTHITBOX);
    Hitbox* hitbox = get_component<Bit_Hitbox, Hitbox>(componentArrays, ARCHETYPE_TESTHITBOX);

    Model* sphere = load_model("models/Sphere");

    while (count)
    {
        (*pos)[0] = 2229.0f + -512.0f + 4.0f * RAND(256);
        (*pos)[1] = 0.0f;
        (*pos)[2] = 26620.0f + -512.0f + 4.0f * RAND(256);

        hitbox->radius = 50.0f;
        hitbox->size_y = 100.0f;
        hitbox->mask = player_hitbox_mask;

        *model = sphere;

        pos++;
        hitbox++;
        model++;
        count--;
    }
}

// #define ARCHETYPE_TEST_RECTANGLE_HITBOX (ARCHETYPE_RECTANGLE_HITBOX | Bit_Model)

// Entity* rect_hitbox;

bool GameplayScene::load()
{
    Vec3 pos;
    pos[0] = 2229.0f;
    pos[1] = 512.0f;
    pos[2] = 26620.0f;

    // Create the player entity
    createPlayer(pos);

    // create_enemy(3229.0f, 0.0f, 26620.0f, EnemyType::Slash, 0);
    // create_shoot_enemy(3229.0f, 0.0f, 26620.0f, 0);
    // create_shoot_enemy(2229.0f, 0.0f, 27020.0f, 0);
    // create_slash_enemy(3229.0f, 0.0f, 26620.0f, 0);
    // create_slash_enemy(3229.0f, 0.0f, 26120.0f, 0);
    // create_slash_enemy(3229.0f, 0.0f, 27120.0f, 0);
    // create_slash_enemy(2729.0f, 0.0f, 26120.0f, 0);
    // create_enemy(3729.0f, 0.0f, 27120.0f, EnemyType::Shoot, 0);

    debug_printf("Loading tiles\n");

    dynamic_array<TileType> tiles(24);

    int i = 0;

    tiles[i++] = TileType{load_model("models/Floor"),       TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorBlue"),   TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorBrown"),  TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorGray"),   TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorGreen"),  TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorRed"),    TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorWhite"),  TileCollision::floor};
    tiles[i++] = TileType{load_model("models/FloorYellow"), TileCollision::floor};
    tiles[i++] = TileType{load_model("models/Slope"),       TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeBlue"),   TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeBrown"),  TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeGray"),   TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeGreen"),  TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeRed"),    TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeWhite"),  TileCollision::slope};
    tiles[i++] = TileType{load_model("models/SlopeYellow"), TileCollision::slope};
    tiles[i++] = TileType{load_model("models/Wall"),        TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallBlue"),    TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallBrown"),   TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallGray"),    TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallGreen"),   TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallRed"),     TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallWhite"),   TileCollision::wall};
    tiles[i++] = TileType{load_model("models/WallYellow"),  TileCollision::wall};


    debug_printf("Getting grid definition\n");

    GridDefinition def = get_grid_definition("levels/1");
    grid_ = Grid{def, std::move(tiles)};

    debug_printf("Finished GameplayScene::load\n");

    // createEntitiesCallback(ARCHETYPE_TESTHITBOX, nullptr, 32, createHitboxCallback);

    // rect_hitbox = createEntity(ARCHETYPE_TEST_RECTANGLE_HITBOX);

    // {
    //     void* components[1 + NUM_COMPONENTS(ARCHETYPE_TEST_RECTANGLE_HITBOX)];
    //     getEntityComponents(rect_hitbox, components);

    //     Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_TEST_RECTANGLE_HITBOX);
    //     Hitbox& hitbox = *get_component<Bit_Hitbox, Hitbox>(components, ARCHETYPE_TEST_RECTANGLE_HITBOX);
    //     Model** model = get_component<Bit_Model, Model*>(components, ARCHETYPE_TEST_RECTANGLE_HITBOX);

    //     pos[0] = 2229.0f + -512.0f;
    //     pos[1] = 0.0f;
    //     pos[2] = 26620.0f;

    //     hitbox.mask = player_hitbox_mask;
    //     hitbox.radius = 200;
    //     hitbox.size_y = 100;
    //     hitbox.size_z = 40;

    //     *model = load_model("models/Weapon");
    // }

    grid_.load_objects();

    return true;
}

void GameplayScene::update()
{
    grid_.unload_nonvisible_chunks(g_Camera);
    grid_.load_visible_chunks(g_Camera);
    grid_.process_loading_chunks();
    // if ((g_PlayerInput.buttonsHeld & R_TRIG) || (g_PlayerInput.buttonsPressed & L_TRIG))
    {
        // {
        //     void* components[1 + NUM_COMPONENTS(ARCHETYPE_TEST_RECTANGLE_HITBOX)];
        //     getEntityComponents(rect_hitbox, components);
        //     Vec3s& rot = *get_component<Bit_Rotation, Vec3s>(components, ARCHETYPE_TEST_RECTANGLE_HITBOX);
        //     Vec3& pos = *get_component<Bit_Position, Vec3>(components, ARCHETYPE_TEST_RECTANGLE_HITBOX);

        //     rot[1] += 0x800;
            
        //     pos[0] = 1717.0f  + 150 * cossf(rot[1]);
        //     pos[2] = 26620.0f - 150 * sinsf(rot[1]);
        // }
        // Increment the physics state
        // debug_printf("before physics tick\n");
        physicsTick(grid_);
        find_collisions(grid_);
        control_update();
        // Process all entities that have a behavior
        // debug_printf("before behaviors\n");
        iterateBehaviorEntities();
        tickDestroyTimers();
    }
}

void GameplayScene::draw()
{
    Vec3 lightDir = { 100.0f * sinf((M_PIf / 180.0f) * 30.0f), 100.0f * cosf((M_PIf / 180.0f) * 30.0f), 0.0f};
    // Set up the camera
    // debug_printf("before camera\n");
    setupCameraMatrices(&g_Camera);
    // debug_printf("before light dir\n");
    setLightDirection(lightDir);
    set_text_color(255, 255, 255, 255);

    if (g_gameTimer > 30)
    {
        // debug_printf("before drawing\n");
        grid_.draw(&g_Camera);

        draw_enemy_heads();
        drawAllEntities();
        drawAllEntitiesHealth();
        set_text_color(0, 128, 0, 255);
        print_text(10, screen_height - 8 - 10 - border_height, get_player_controlled_definition()->base.enemy_name);
        draw_all_text();
        // drawAllHitboxes();
    }
}

void GameplayScene::after_gfx()
{
    // debug_printf("After end frame\n");
}

void GameplayScene::unloading_update()
{

}

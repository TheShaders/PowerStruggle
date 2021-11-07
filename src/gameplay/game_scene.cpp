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

extern "C" {
#include <debug.h>
}

extern GridDefinition get_grid_definition(const char *file);

GameplayScene::GameplayScene() : grid_{}
{
}

// LoadHandle handle;

bool GameplayScene::load()
{
    // Create the player entity
    createPlayer(&playerState_);

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

    return true;
}

void GameplayScene::update()
{
    grid_.unload_nonvisible_chunks(g_Camera);
    grid_.load_visible_chunks(g_Camera);
    grid_.process_loading_chunks();
    // if ((g_PlayerInput.buttonsHeld & R_TRIG) || (g_PlayerInput.buttonsPressed & L_TRIG))
    {
        // Increment the physics state
        // debug_printf("before physics tick\n");
        physicsTick(grid_);
        // Process all entities that have a behavior
        // debug_printf("before behaviors\n");
        iterateBehaviorEntities();
    }
}

void GameplayScene::draw()
{
    Vec3 lightDir = { 100.0f * sinf((M_PIf / 180.0f) * 45.0f), 100.0f * cosf((M_PIf / 180.0f) * 45.0f), 0.0f};
    // Set up the camera
    // debug_printf("before camera\n");
    setupCameraMatrices(&g_Camera);
    // debug_printf("before light dir\n");
    setLightDirection(lightDir);

    if (g_gameTimer > 30)
        grid_.draw();

    // debug_printf("before drawing\n");
    drawAllEntities();
}

void GameplayScene::after_gfx()
{
    // debug_printf("After end frame\n");
}

void GameplayScene::unloading_update()
{

}

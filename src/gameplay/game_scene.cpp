#include <cmath>

#include <gameplay.h>
#include <camera.h>
#include <mathutils.h>
#include <player.h>
#include <physics.h>
#include <ecs.h>
#include <main.h>
#include <files.h>

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
    
    tiles[i++] = TileType{load_model("models/Floor")};
    tiles[i++] = TileType{load_model("models/FloorBlue")};
    tiles[i++] = TileType{load_model("models/FloorBrown")};
    tiles[i++] = TileType{load_model("models/FloorGray")};
    tiles[i++] = TileType{load_model("models/FloorGreen")};
    tiles[i++] = TileType{load_model("models/FloorRed")};
    tiles[i++] = TileType{load_model("models/FloorWhite")};
    tiles[i++] = TileType{load_model("models/FloorYellow")};
    tiles[i++] = TileType{load_model("models/Slope")};
    tiles[i++] = TileType{load_model("models/SlopeBlue")};
    tiles[i++] = TileType{load_model("models/SlopeBrown")};
    tiles[i++] = TileType{load_model("models/SlopeGray")};
    tiles[i++] = TileType{load_model("models/SlopeGreen")};
    tiles[i++] = TileType{load_model("models/SlopeRed")};
    tiles[i++] = TileType{load_model("models/SlopeWhite")};
    tiles[i++] = TileType{load_model("models/SlopeYellow")};
    tiles[i++] = TileType{load_model("models/Wall")};
    tiles[i++] = TileType{load_model("models/WallBlue")};
    tiles[i++] = TileType{load_model("models/WallBrown")};
    tiles[i++] = TileType{load_model("models/WallGray")};
    tiles[i++] = TileType{load_model("models/WallGreen")};
    tiles[i++] = TileType{load_model("models/WallRed")};
    tiles[i++] = TileType{load_model("models/WallWhite")};
    tiles[i++] = TileType{load_model("models/WallYellow")};


    debug_printf("Getting grid definition\n");

    GridDefinition def = get_grid_definition("levels/test_");
    grid_ = Grid{def, std::move(tiles)};

    debug_printf("Finished GameplayScene::load\n");

    // LoadHandle
    // handle = start_file_load("models/Wall");
    // handle.join();
    // auto m = load_model("models/Wall");

    // Create the level collision entity
    // // debug_printf("Creating collision entity\n");
    // createEntitiesCallback(Bit_Collision, segmentedToVirtual(&test_collision_collision_tree), 1, setCollision);

    // {
    //     Entity *toDelete;
    //     Entity *lastEntity;
    //     // debug_printf("Creating 5000 position only entities\n");
    //     createEntities(Bit_Position, 5000);

    //     // debug_printf("Creating 1 position only entity to delete\n");
    //     toDelete = createEntity(Bit_Position);
    //     // debug_printf("To delete entity archetype array index: %d\n", toDelete->archetypeArrayIndex);

    //     // debug_printf("Creating 5000 more position only entities\n");
    //     createEntities(Bit_Position, 5000);

    //     // debug_printf("Creating 1 position only entity to test\n");
    //     lastEntity = createEntity(Bit_Position);
    //     // debug_printf("Test entity archetype array index: %d\n", lastEntity->archetypeArrayIndex);
        
    //     // debug_printf("Deleting entity at array position %d\n", toDelete->archetypeArrayIndex);
    //     deleteEntity(toDelete);

    //     // debug_printf("Last entity was moved to %d\n", lastEntity->archetypeArrayIndex);
    // }
    // debug_printf("Processing level header\n");
    // processLevelHeader(segmentedToVirtual(&mainHeader));

    return true;
}

void GameplayScene::update()
{
    grid_.unload_nonvisible_chunks(g_Camera);
    grid_.load_visible_chunks(g_Camera);
    grid_.process_loading_chunks();
    // Increment the physics state
    // debug_printf("before physics tick\n");
    physicsTick();
    // Process all entities that have a behavior
    // debug_printf("before behaviors\n");
    iterateBehaviorEntities();
}

void GameplayScene::draw()
{
    Vec3 lightDir = { 100.0f * sinf((M_PIf / 180.0f) * 45.0f), 100.0f * cosf((M_PIf / 180.0f) * 45.0f), 0.0f};
    // Set up the camera
    // debug_printf("before camera\n");
    setupCameraMatrices(&g_Camera);
    // debug_printf("before light dir\n");
    setLightDirection(lightDir);

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

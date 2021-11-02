#include <cmath>

#include <gameplay.h>
#include <camera.h>
#include <mathutils.h>
#include <player.h>
#include <physics.h>
#include <ecs.h>
#include <main.h>
#include <files.h>

extern GridDefinition get_grid_definition(const char *file);

GameplayScene::GameplayScene() : grid_{}
{
}

// load_handle handle;

bool GameplayScene::load()
{
    // Create the player entity
    // createPlayer(&playerState_);

    dynamic_array<TileType> tiles{
        TileType{load_model("models/Floor")},
        TileType{load_model("models/FloorBlue")},
        TileType{load_model("models/FloorBrown")},
        TileType{load_model("models/FloorGray")},
        TileType{load_model("models/FloorGreen")},
        TileType{load_model("models/FloorRed")},
        TileType{load_model("models/FloorWhite")},
        TileType{load_model("models/FloorYellow")},
        TileType{load_model("models/Slope")},
        TileType{load_model("models/SlopeBlue")},
        TileType{load_model("models/SlopeBrown")},
        TileType{load_model("models/SlopeGray")},
        TileType{load_model("models/SlopeGreen")},
        TileType{load_model("models/SlopeRed")},
        TileType{load_model("models/SlopeWhite")},
        TileType{load_model("models/SlopeYellow")},
        TileType{load_model("models/Wall")},
        TileType{load_model("models/WallBlue")},
        TileType{load_model("models/WallBrown")},
        TileType{load_model("models/WallGray")},
        TileType{load_model("models/WallGreen")},
        TileType{load_model("models/WallRed")},
        TileType{load_model("models/WallWhite")},
        TileType{load_model("models/WallYellow")},
    };

    GridDefinition def = get_grid_definition("levels/test.bin");
    grid_ = Grid{def, std::move(tiles)};
    grid_.load_chunk({0, 0});
    grid_.load_chunk({0, 1});
    grid_.load_chunk({0, 2});
    grid_.load_chunk({0, 3});
    grid_.load_chunk({1, 0});
    grid_.load_chunk({1, 1});
    grid_.load_chunk({1, 2});
    grid_.load_chunk({1, 3});
    grid_.load_chunk({2, 0});
    grid_.load_chunk({2, 1});
    grid_.load_chunk({2, 2});
    grid_.load_chunk({2, 3});

    // load_handle
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

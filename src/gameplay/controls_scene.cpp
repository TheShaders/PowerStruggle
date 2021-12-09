#include <misc_scenes.h>
#include <input.h>
#include <files.h>
#include <mem.h>
#include <gameplay.h>
#include <save.h>

ControlsScene::ControlsScene() : howtoplay_image_(nullptr)
{
}

ControlsScene::~ControlsScene()
{
    if (howtoplay_image_ != nullptr)
    {
        freeAlloc(howtoplay_image_);
        howtoplay_image_ = nullptr;
    }
}

bool ControlsScene::load()
{
    howtoplay_image_ = load_file("textures/howtoplay");
    load_save();
    return true;
}

void ControlsScene::update()
{
    if (g_PlayerInput.buttonsPressed & START_BUTTON)
    {
        // if (g_SaveFile.data.level != 0)
        // {   
            start_scene_load(std::make_unique<FileScene>());
        // }
        // else
        // {
        //     start_scene_load(std::make_unique<GameplayScene>(0));
        // }
    }
}

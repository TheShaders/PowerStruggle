#include <misc_scenes.h>
#include <input.h>
#include <files.h>
#include <mem.h>
#include <gameplay.h>
#include <text.h>
#include <title.h>
#include <save.h>

FileScene::FileScene()
{
}

bool FileScene::load()
{
    return true;
}

void FileScene::update()
{
    if (g_PlayerInput.buttonsPressed & A_BUTTON)
    {
        start_scene_load(std::make_unique<GameplayScene>(0));
    }
    else if (g_PlayerInput.buttonsPressed & B_BUTTON)
    {
        start_scene_load(std::make_unique<GameplayScene>(1));
    }
    else if (g_PlayerInput.buttonsPressed & Z_TRIG)
    {
        start_scene_load(std::make_unique<GameplayScene>(2));
    }
}

void FileScene::draw(bool)
{
    set_text_color(255, 255, 255, 255);
    print_text_centered(screen_width / 2, screen_height / 2 - 25, "Level Select");
    print_text_centered(screen_width / 2, screen_height / 2 - 5, "A: Start from beginning");
    print_text_centered(screen_width / 2, screen_height / 2 + 5, "B: Skip to level 2");
    print_text_centered(screen_width / 2, screen_height / 2 + 15, "Z: Skip to level 3");
    draw_all_text();
}

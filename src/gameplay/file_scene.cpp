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
        start_scene_load(std::make_unique<IntroScene>());
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



IntroScene::IntroScene()
{
}

bool IntroScene::load()
{
    return true;
}

void IntroScene::update()
{
    if (g_PlayerInput.buttonsPressed & START_BUTTON)
    {
        start_scene_load(std::make_unique<GameplayScene>(0));
    }
}

void IntroScene::draw(bool)
{
    set_text_color(0, 128, 0, 255);
    print_text_centered(screen_width / 2, 20, "=======================================");
    print_text_centered(screen_width / 2, 30, "Terraforming Program Status Report:");
    print_text_centered(screen_width / 2, 40, "=======================================");
    print_text(20,  60, "Power generation unstable.");
    print_text(20,  70, "Malfunctioning service robots detected.");
    print_text(20,  90, "Establish link to mainframe and restore power.");
    print_text(20, 100, "Use of force is authorized.");

    print_text(20, 140, "Damage robots to destroy their head modules.");
    print_text(20, 150, "Robot chassis can then be hijacked by aiming");
    print_text(20, 160, "with the analog stick and pressing [L] or [R].");
    set_text_color(255, 255, 255, 255);
    print_text_centered(screen_width / 2, screen_height - 40, "PRESS START");
    draw_all_text();
}

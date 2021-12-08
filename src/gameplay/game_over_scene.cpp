#include <misc_scenes.h>
#include <input.h>
#include <files.h>
#include <mem.h>
#include <gameplay.h>
#include <text.h>
#include <title.h>

GameOverScene::GameOverScene(int level_idx) : timer_{0}, level_idx_{level_idx}
{
}

bool GameOverScene::load()
{
    return true;
}

void GameOverScene::update()
{
    if (timer_ > 120)
    {
        if (g_PlayerInput.buttonsPressed && START_BUTTON)
        {
            start_scene_load(std::make_unique<GameplayScene>(level_idx_));
        }
    }
    timer_++;
}

void GameOverScene::draw(bool)
{
    set_text_color(0, 128, 0, 255);
    print_text_centered(screen_width / 2, screen_height / 2 - 5, "GAME OVER");
    if (timer_ > 120)
    {
        set_text_color(255, 255, 255, 255);
        print_text_centered(screen_width / 2, screen_height - 50, "Continue?");
        print_text_centered(screen_width / 2, screen_height - 40, "PRESS START");
    }
    draw_all_text();
}

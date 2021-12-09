#include <misc_scenes.h>
#include <input.h>
#include <files.h>
#include <mem.h>
#include <gameplay.h>
#include <text.h>
#include <title.h>

EndingScene::EndingScene() : timer_{0}
{
}

bool EndingScene::load()
{
    return true;
}

void EndingScene::update()
{
    if (timer_ > 120)
    {
        if (g_PlayerInput.buttonsPressed && START_BUTTON)
        {
            start_scene_load(std::make_unique<TitleScene>());
        }
    }
    timer_++;
}

void EndingScene::draw(bool)
{
    set_text_color(0, 128, 0, 255);
    print_text_centered(screen_width / 2, screen_height / 2 - 20, "Mainframe link established.");
    print_text_centered(screen_width / 2, screen_height / 2 - 10, "Power generation restored!");
    print_text_centered(screen_width / 2, screen_height / 2 + 10, "Congratulations! Thanks for playing!");
    if (timer_ > 120)
    {
        set_text_color(255, 255, 255, 255);
        print_text_centered(screen_width / 2, screen_height - 40, "PRESS START");
    }
    draw_all_text();
}

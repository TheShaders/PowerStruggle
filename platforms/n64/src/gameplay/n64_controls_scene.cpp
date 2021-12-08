#include <misc_scenes.h>
#include <ultra64.h>
#include <n64_gfx.h>
#include <text.h>

void ControlsScene::draw(bool)
{
    int howtoplay_img_x = 40;
    int howtoplay_img_y = 60;
    extern Gfx* g_gui_dlist_head;
    gDPPipeSync(g_gui_dlist_head++);
    gDPSetRenderMode(g_gui_dlist_head++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
    gDPSetTextureFilter(g_gui_dlist_head++, G_TF_POINT);
    gDPSetEnvColor(g_gui_dlist_head++, 0, 128, 0, 255);
    gDPSetCombineLERP(g_gui_dlist_head++, 0, 0, 0, ENVIRONMENT, 0, 0, 0, TEXEL0, 0, 0, 0, ENVIRONMENT, 0, 0, 0, TEXEL0);
    gDPLoadTextureBlock_4b(g_gui_dlist_head++, howtoplay_image_, G_IM_FMT_I, 64, 64, 0, 0, 0, 0, 0, 0, 0);
    gSPTextureRectangle(g_gui_dlist_head++, (howtoplay_img_x) << 2, (howtoplay_img_y) << 2, (howtoplay_img_x + 64) << 2, (howtoplay_img_y + 64) << 2, 0, 0, 0, 1 << 10, 1 << 10);

    text_reset();
    set_text_color(0, 128, 0, 255);
    print_text_centered(howtoplay_img_x + 32, howtoplay_img_y - 20, "Hold the controller");
    print_text_centered(howtoplay_img_x + 32, howtoplay_img_y - 10, "like this");

    
    print_text_centered(screen_width - howtoplay_img_x - 32, howtoplay_img_y +  0, "Controls");
    print_text_centered(screen_width - howtoplay_img_x - 32, howtoplay_img_y + 10, "Move: D-Pad");
    print_text_centered(screen_width - howtoplay_img_x - 32, howtoplay_img_y + 20, "Aim: Analog");
    print_text_centered(screen_width - howtoplay_img_x - 32, howtoplay_img_y + 30, "Attack: Z  ");
    print_text_centered(screen_width - howtoplay_img_x - 32, howtoplay_img_y + 40, "Hijack: L  ");
    
    print_text_centered(screen_width / 2, screen_height - 80, "Emulator users should bind their");
    print_text_centered(screen_width / 2, screen_height - 70, "right analog stick to the N64 analog stick.");

    set_text_color(255, 255, 255, 255);
    print_text_centered(screen_width / 2, screen_height - 40, "PRESS START");

    draw_all_text();
}

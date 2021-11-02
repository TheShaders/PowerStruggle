#ifndef __GAMEPLAY_H__
#define __GAMEPLAY_H__

#include <memory>

#include <scene.h>
#include <grid.h>
#include <player.h>

class GameplayScene : public Scene {
public:
    GameplayScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw() override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final;
    // Called every frame while the scene is being unloaded
    void unloading_update() override final;
private:
    PlayerState playerState_;
    Grid grid_;
};

#endif

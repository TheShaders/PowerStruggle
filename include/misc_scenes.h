#ifndef __MISC_SCENES_H__
#define __MISC_SCENES_H__

#include <scene.h>

class ControlsScene : public Scene {
public:
    ControlsScene();
    ~ControlsScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final {}
    // Called every frame while the scene is being unloaded
    void unloading_update() override final {}
private:
    void* howtoplay_image_;
};

class EndingScene : public Scene {
public:
    EndingScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final {}
    // Called every frame while the scene is being unloaded
    void unloading_update() override final {}
private:
    int timer_;
};

class GameOverScene : public Scene {
public:
    GameOverScene(int level_idx);
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final {}
    // Called every frame while the scene is being unloaded
    void unloading_update() override final {}
private:
    int timer_;
    int level_idx_;
};

class FileScene : public Scene {
public:
    FileScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final {}
    // Called every frame while the scene is being unloaded
    void unloading_update() override final {}
};

class IntroScene : public Scene {
public:
    IntroScene();
    // Called every frame after the scene is constructed, stops being called once it returns true
    bool load() override final;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    void update() override final;
    // Called every frame while the scene is active every frame for drawing the scene contents
    void draw(bool unloading) override final;
    // Called every frame while the scene is active after graphics processing is complete
    void after_gfx() override final {}
    // Called every frame while the scene is being unloaded
    void unloading_update() override final {}
};

#endif

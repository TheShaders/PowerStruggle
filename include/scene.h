#ifndef __SCENE_H__
#define __SCENE_H__

class Scene {
public:
    // Called every frame after the scene is constructed, stops being called once it returns true
    virtual bool load() = 0;
    // Called every frame while the scene is active at a fixed 60Hz rate for logic handling
    virtual void update() = 0;
    // Called every frame while the scene is active every frame for drawing the scene contents
    virtual void draw() = 0;
    // Called every frame while the scene is active after graphics processing is complete
    virtual void after_gfx() = 0;
    // Called every frame while the scene is being unloaded
    virtual void unloading_update() = 0;
};

#endif

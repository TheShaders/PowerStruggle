#ifndef __ENEMY_BASE_H__
#define __ENEMY_BASE_H__

// The basic info that every enemy definition has
struct BaseEnemyInfo {
    const char* model_name;
    Model* model;
    uint16_t max_health;
    uint16_t controllable_health;
    // Speed the enemy will move at
    float move_speed;
    EnemyType enemy_type;
};

// The base definition that all enemy definitions inherit from
struct BaseEnemyDefinition {
    BaseEnemyInfo base;
};

struct BaseEnemyState {
    BaseEnemyDefinition* definition;
};

#endif

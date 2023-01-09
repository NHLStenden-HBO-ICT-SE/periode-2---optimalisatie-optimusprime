#pragma once
class Grid;

namespace Tmpl8
{
    class Terrain; //forward declare

enum Color
{
    BLUE,
    RED
};

class Tank
{
  public:
    Tank(float pos_x, float pos_y, Color color, Sprite* tank_sprite,
         Sprite* smoke_sprite, float tar_x, float tar_y, float collision_radius,
         int health, float max_speed, Grid* grid);

    ~Tank();

    void tick(Terrain& terrain);

    vec2 get_position() const { return position; };
    float get_collision_radius() const { return collision_radius; };
    bool rocket_reloaded() const { return reloaded; };

    void set_route(const std::vector<vec2>& route);
    void reload_rocket();

    void deactivate();
    bool isDestroyedByHit(int hit_value);

    void draw(Surface* screen);

    int compare_health(const Tank& other) const;


    void push(vec2 direction, float magnitude);

    vec2 position;
    vec2 speed;
    vec2 target;

    Grid* grid;

    vector<vec2> current_route;

    int health;

    float collision_radius;
    vec2 force;

    float max_speed;
    float reload_time;

    bool reloaded;
    bool active;


    Color color;

    int current_frame;
    Sprite* tank_sprite;
    Sprite* smoke_sprite;

    static bool compare_active(const Tank &t1, const Tank &t2);
};

} // namespace Tmpl8
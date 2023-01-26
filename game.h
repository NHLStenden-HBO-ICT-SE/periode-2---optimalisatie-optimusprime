#pragma once

namespace Tmpl8 {
//forward declarations
    class Tank;

    class Rocket;

    class Smoke;

    class Particle_beam;

    class Game {
    public:
        Game() {
        }

        void set_target(Surface *surface) { screen = surface; }

        void init();

        void shutdown();

        void update(float deltaTime);

        void draw();

        void tick(float deltaTime);

        void insertion_sort_tanks_health(const std::vector<Tank> &original, std::vector<const Tank *> &sorted_tanks,
                                         int begin, int end);

        void draw_health_bars(const std::vector<const Tank *> &sorted_tanks, const int team);

        void measure_performance();

        Tank &find_closest_enemy(Tank &current_tank);

        void mouse_up(int button) { /* implement if you want to detect mouse button presses */
        }

        void mouse_down(int button) { /* implement if you want to detect mouse button presses */
        }

        void mouse_move(int x, int y) { /* implement if you want to detect mouse movement */
        }

        void key_up(int key) { /* implement if you want to handle keys */
        }

        void key_down(int key) { /* implement if you want to handle keys */
        }

        Grid grid;

        constexpr const static float tank_radius = 3.f;
        constexpr const static float rocket_radius = 5.f;
    private:
        Surface *screen;
        vector<Tank> tanks;
        vector<Rocket> rockets;
        vector<Smoke> smokes;
        vector<Explosion> explosions;
        vector<Particle_beam> particle_beams;
        Terrain background_terrain;
        std::vector<vec2> forcefield_hull;
        std::mutex tankMutex;
        std::mutex rocketMutex;

    private:
        Font *frame_count_font;
        long long frame_count = 0;
        int inactiveTanks = 0;
        bool lock_update = false;

        //Checks if a point lies on the left of an arbitrary angled line
        bool left_of_line(vec2 line_start, vec2 line_end, vec2 point);

        void initializeTankRoute();

        void tankCollision();

        void updatePartOfTankList(int start, int stop);

        void updateSmoke();

        void calcConvexHull(int first_active, vec2 &point_on_hull);

        int findFirstActiveTank();

        vec2 findLeftPointOnConvexHull();

        void moveRockets();

        void disableRockets();

        void updateParticleBeams();

        void updateExplosionSprites();

        void eraseExplosions();

        void removeInactiveRockets();

        void RocketHitsTank(Rocket &rocket);

        void drawSmokeOnDestroyedTank(Tank &tank);

        void drawExplosionOnHitTank(Tank &tank);

        void sortActiveTanks(vector<Tank> &tanks);

        template<class _Compare>
        _Compare tankComparer(bool *pBoolean);

        template<class _Compare>
        bool compare_Tanks();

        bool compareTanks(Tank T1, Tank T2);

        void removeInactiveTanks();

        void sortActiveTanks();

        void DrawTanks();

        void DrawRockets();

        void DrawSmoke();

        void DrawParticleBeams();

        void DrawExplosions();

        void DrawSprites();

        void DrawForceField();

        void DrawSortedHealthBars();

        void rocketIntersectsTank(Rocket &rocket);

        void doRocketDamage();

        void updateTanksConcurrent();
    };

};
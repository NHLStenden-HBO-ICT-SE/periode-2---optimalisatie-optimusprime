#include "precomp.h" // include (only) this in every .cpp file
#include "game.h"


constexpr auto num_tanks_blue = 2048;
constexpr auto num_tanks_red = 2048;

constexpr auto tank_max_health = 1000;
constexpr auto rocket_hit_value = 60;
constexpr auto particle_beam_hit_value = 50;

constexpr auto tank_max_speed = 1.0;

constexpr auto health_bar_width = 70;

constexpr auto max_frames = 2000;

//Global performance timer

// Timer Sven: 57343.6
// Timer Achmed: 70845.5
constexpr auto REF_PERFORMANCE = 57343.6; //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)

static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface *tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface *tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface *rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface *rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface *particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface *smoke_img = new Surface("assets/Smoke.png");
static Surface *explosion_img = new Surface("assets/Explosion.png");

static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(7, 9);
const static vec2 rocket_size(6, 6);
ThreadPool threadPool(std::thread::hardware_concurrency());

// -----------------------------------------------------------
// Initialize the simulation state
// This function does not count for the performance multiplier
// (Feel free to optimize anyway though ;) )
// -----------------------------------------------------------
void Game::init() {
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(num_tanks_blue + num_tanks_red);

    uint max_rows = 24;

    float start_blue_x = tank_size.x + 40.0f;
    float start_blue_y = tank_size.y + 30.0f;

    float start_red_x = 1088.0f;
    float start_red_y = tank_size.y + 30.0f;

    float spacing = 7.5f;

    //Spawn blue tanks
    for (int i = 0; i < num_tanks_blue; i++) {
        vec2 position{start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing)};
        tanks.push_back(Tank(position.x, position.y, BLUE,
                             &tank_blue, &smoke, 1100.f, position.y + 16,
                             tank_radius, tank_max_health, tank_max_speed, &grid));
    }
    //Spawn red tanks
    for (int i = 0; i < num_tanks_red; i++) {
        vec2 position{start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing)};
        tanks.push_back(Tank(position.x, position.y, RED,
                             &tank_red, &smoke, 100.f, position.y + 16,
                             tank_radius, tank_max_health, tank_max_speed, &grid));
    }

    grid = Grid(std::ceil(screen->get_width() / rocket_radius), rocket_radius);
    // Add tanks to grid
    for (int i = 0; i < tanks.size(); ++i) {
        grid.add(&tanks.at(i));
    }


    particle_beams.push_back(
            Particle_beam(vec2(590, 327), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(
            Particle_beam(vec2(64, 64), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(
            Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));

    // Create a threadPool with the amount of objects the hardware has
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown() {
}

// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank &Game::find_closest_enemy(Tank &current_tank) {
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index = 0;

    for (int i = 0; i < tanks.size(); i++) {
        if (tanks.at(i).color != current_tank.color && tanks.at(i).active) {
            float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqr_dist < closest_distance) {
                closest_distance = sqr_dist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

bool operator<(const Tank &tank1, const Tank &tank2) {
    return tank1.health < tank2.health;
}

//Checks if a point lies on the left of an arbitrary angled line
bool Tmpl8::Game::left_of_line(vec2 line_start, vec2 line_end, vec2 point) {
    return ((line_end.x - line_start.x) * (point.y - line_start.y) -
            (line_end.y - line_start.y) * (point.x - line_start.x)) < 0;
}

// -----------------------------------------------------------
// Update the game state:
// Move all objects
// Update sprite frames
// Collision detection
// Targeting etc..
// -----------------------------------------------------------
void Game::update(float deltaTime) {

    //Calculate the route to the destination for each tank using BFS
    //Initializing routes here, so it gets counted for performance..
    initializeTankRoute();

    //todo make tank.active Vector to stop all a lot of bullshit checks.

    //Check tank collision and nudge tanks away from each other
    tankCollision();

    //Update tanks
    updateTanksConcurrent();

    //Update smoke plumes
    updateSmoke();

    //Calculate "force field" around active tanks
    // forcefield_hull.clear(); // is now done in calcConvexHull()

    //todo findLeftPointOnConvexHull as parameter in calcConvexHull() it's not working yet. Have to find the reason
    vec2 point_on_hull = findLeftPointOnConvexHull();
    //Calculate convex hull for 'rocket barrier'
    calcConvexHull(findFirstActiveTank(), point_on_hull);

    //Update rockets
    moveRocketsConcurrently();

    //Check if rocket collides with enemy tank, spawn explosion, and if tank is destroyed spawn a smoke plume
    doRocketDamage();

    //Disable rockets if they collide with the "force field"
    //Hint: A point to convex hull intersection test might be better here? :) (Disable if outside)
    disableRockets();

    //Remove exploded rockets with remove erase idiom
    removeInactiveRockets();

    //Update particle beams
    updateParticleBeams();

    //Update explosion sprites and remove when done with remove erase idiom
    updateExplosionSprites();

    eraseExplosions();
}


void Game::removeInactiveRockets() {
    rockets.erase(remove_if(
                          rockets.begin(),
                          rockets.end(),
                          [](const Rocket &rocket) { return !rocket.active; }),
                  rockets.end());
}

void Game::eraseExplosions() {
    explosions.erase(remove_if(
                             explosions.begin(),
                             explosions.end(),
                             [](const Explosion &explosion) { return explosion.done(); }),
                     explosions.end());
}

void Game::updateExplosionSprites() {
    for (Explosion &explosion: explosions) {
        explosion.tick();
    }
}

void Game::updateParticleBeams() {
    for (Particle_beam &particle_beam: particle_beams) {
        particle_beam.tick(tanks);

        //Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding square)
        for (Tank &tank: tanks) {
            if (tank.active &&
                particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius())) {
                if (tank.isDestroyedByHit(particle_beam.damage)) {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }
}

void Game::disableRockets() {
    for (Rocket &rocket: rockets) {
        if (rocket.active) {
            for (size_t i = 0; i < forcefield_hull.size(); i++) {
                if (circle_segment_intersect(forcefield_hull.at(i),
                                             forcefield_hull.at((i + 1) % forcefield_hull.size()), rocket.position,
                                             rocket.collision_radius)) {
                    explosions.push_back(Explosion(&explosion, rocket.position));
                    rocket.active = false;
                }
            }
        }
    }
}

void Game::moveRocketsConcurrently() {
    int rocketsPerThread = rockets.size() / threadPool.getAmountOfCores();
    int remainder = rockets.size() % threadPool.getAmountOfCores();
    int start = 0, end = 0;
    std::vector<std::future<void>> workingThreads;

    for (int i = 0; i < threadPool.getAmountOfCores(); i++) {
        start = end;
        end += rocketsPerThread;
        if (remainder > 0) {
            end += 1;
            remainder--;
        }
        workingThreads.push_back(threadPool.enqueue([&, start, end]() { updatePartOfRocketList(start, end); }));
    }

    for (int i = 0; i < workingThreads.size(); ++i) {
        workingThreads.at(i).wait();
    }
}

void Game::updatePartOfRocketList(int start, int end) {
    for (int j = start; j < end; j++) {
        Rocket &rocket = rockets.at(j);
        //const std::lock_guard<std::mutex> tankLock(rocketMutex);
        rocket.tick();
    }
}


void Game::doRocketDamage() {
    for (Rocket &rocket: rockets) {
        rocketIntersectsTank(rocket);
    }
}

void Game::rocketIntersectsTank(Rocket &rocket) {
    vec2 cellPos(rocket.position.x / grid.CELL_SIZE, rocket.position.y / grid.CELL_SIZE);

    for (int x = cellPos.x - 1; x < cellPos.x + 1; ++x) {
        for (int y = cellPos.y - 1; y < cellPos.y + 1; ++y) {
            for (Tank *tank: grid.getTanksAt(cellPos)) {
                if (tank->active && (tank->color != rocket.color) &&
                    rocket.intersects(tank->position, tank->collision_radius)) {
                    drawExplosionOnHitTank(*tank);
                    drawSmokeOnDestroyedTank(*tank);
                    rocket.deactivate();
                    return;
                }
            }
        }
    }
}

void Game::drawExplosionOnHitTank(Tank &tank) { explosions.push_back(Explosion(&explosion, tank.position)); }

void Game::drawSmokeOnDestroyedTank(Tank &tank) {
    if (tank.isDestroyedByHit(rocket_hit_value)) {
        smokes.push_back(Smoke(smoke, tank.position - vec2(7, 24)));
    }
}

vec2 Game::findLeftPointOnConvexHull() {
    vec2 point_on_hull = tanks.at(findFirstActiveTank()).position;
    //Find left most tank position
    for (Tank &tank: tanks) {
        if (tank.active) {
            if (tank.position.x <= point_on_hull.x) {
                point_on_hull = tank.position;
            }
        }
    }
    return point_on_hull;
}


//Find first active tank (this loop is a bit disgusting, fix?)
int Game::findFirstActiveTank() {
    int first_active = 0;
    for (Tank &tank: tanks) {
        if (tank.active) {
            break;
        }
        first_active++;
    }
    return first_active;
}

void Game::calcConvexHull(int first_active, vec2 &point_on_hull) {
    forcefield_hull.clear();
    for (Tank &tank: tanks) {
        if (tank.active) {
            forcefield_hull.push_back(point_on_hull);
            vec2 endpoint = tanks.at(first_active).position;

            for (Tank &tank: tanks) {
                if (tank.active) {
                    if ((endpoint == point_on_hull) || left_of_line(point_on_hull, endpoint, tank.position)) {
                        endpoint = tank.position;
                    }
                }
            }
            point_on_hull = endpoint;

            if (endpoint == forcefield_hull.at(0)) {
                break;
            }
        }
    }
}

void Game::updateSmoke() {
    for (Smoke &smoke: smokes) {
        smoke.tick();
    }
}

void Game::updatePartOfTankList(int start, int end) {
    for (int j = start; j < end; j++) {
        Tank &tank = tanks.at(j);
        if (tank.active) {
            { // to stop lock after tick
                const std::lock_guard<std::mutex> tankLock(tankMutex);
                //Move tanks according to speed and nudges (see above) also reload
                tank.tick(background_terrain);
            }
            //Shoot at closest target if reloaded
            if (tank.rocket_reloaded()) {
                Tank &target = find_closest_enemy(tank);
                { // to stop lock after push_back
                    const std::lock_guard<std::mutex> tankLock(rocketMutex);
                    rockets.push_back(
                            Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3,
                                   rocket_radius,
                                   tank.color, ((tank.color == RED) ? &rocket_red : &rocket_blue)));
                }
                tank.reload_rocket();
            }
        }
    }
}

void Game::updateTanksConcurrent() {
    int tanksPerThread = tanks.size() / threadPool.getAmountOfCores();
    int remainder = tanks.size() % threadPool.getAmountOfCores();
    int start = 0, stop = 0;
    std::vector<std::future<void>> workingThreads;
    for (int i = 0; i < threadPool.getAmountOfCores(); i++) {
        start = stop;
        stop += tanksPerThread;
        if (remainder > 0) {
            stop += 1;
            remainder--;
        }
        workingThreads.push_back(threadPool.enqueue([&, start, stop]() { updatePartOfTankList(start, stop); }));
    }

    for (int i = 0; i < workingThreads.size(); ++i) {
        workingThreads.at(i).wait();
    }
}

void Game::tankCollision() {
    for (Tank &tank: tanks) {
        if (tank.active) {
            for (Tank &other_tank: tanks) {
                if (&tank == &other_tank || !other_tank.active) continue;

                vec2 dir = tank.get_position() - other_tank.get_position();
                float dir_squared_len = dir.sqr_length();

                float col_squared_len = (tank.get_collision_radius() + other_tank.get_collision_radius());
                col_squared_len *= col_squared_len;

                if (dir_squared_len < col_squared_len) {
                    tank.push(dir.normalized(), 1.f);
                }
            }
        }
    }
}

void Game::initializeTankRoute() {
    if (frame_count == 0) {
        for (Tank &t: tanks) {
            t.set_route(background_terrain.get_route(t, t.target));
        }
    }
}

// -----------------------------------------------------------
// Draw all sprites to the screen
// (It is not recommended to multi-thread this function)
// -----------------------------------------------------------
void Game::draw() {
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background_terrain.draw(screen);

    //Draw sprites
    DrawSprites();

    //Draw force field (mostly for debugging, its kinda ugly..)
    DrawForceField();

    //Draw sorted health bars
    DrawSortedHealthBars();
}

void Game::DrawSortedHealthBars() {
    for (int t = 0; t < 2; t++) {
        const int NUM_TANKS = ((t < 1) ? num_tanks_blue : num_tanks_red);

        const int begin = ((t < 1) ? 0 : num_tanks_blue);
        vector<const Tank *> sorted_tanks;
        insertion_sort_tanks_health(tanks, sorted_tanks, begin, begin + NUM_TANKS);
        sorted_tanks.erase(remove_if(sorted_tanks.begin(), sorted_tanks.end(),
                                     [](const Tank *tank) { return !tank->active; }),
                           sorted_tanks.end());

        draw_health_bars(sorted_tanks, t);
    }
}

void Game::DrawForceField() {
    for (size_t i = 0; i < forcefield_hull.size(); i++) {
        vec2 line_start = forcefield_hull.at(i);
        vec2 line_end = forcefield_hull.at((i + 1) % forcefield_hull.size());
        line_start.x += HEALTHBAR_OFFSET;
        line_end.x += HEALTHBAR_OFFSET;
        screen->line(line_start, line_end, 0x0000ff);
    }
}

void Game::DrawSprites() {
    DrawTanks();
    DrawRockets();
    DrawSmoke();
    DrawParticleBeams();
    DrawExplosions();
}

void Game::DrawExplosions() {
    for (Explosion &explosion: explosions) {
        explosion.draw(screen);
    }
}

void Game::DrawParticleBeams() {
    for (Particle_beam &particle_beam: particle_beams) {
        particle_beam.draw(screen);
    }
}

void Game::DrawSmoke() {
    for (Smoke &smoke: smokes) {
        smoke.draw(screen);
    }
}

void Game::DrawRockets() {
    for (Rocket &rocket: rockets) {
        rocket.draw(screen);
    }
}

void Game::DrawTanks() {
    for (int i = 0; i < num_tanks_blue + num_tanks_red; i++) {
        tanks.at(i).draw(screen);
        vec2 tank_pos = tanks.at(i).get_position();
    }
}

// -----------------------------------------------------------
// Sort tanks by health value using insertion sort
// -----------------------------------------------------------
void
Tmpl8::Game::insertion_sort_tanks_health(const std::vector<Tank> &original, std::vector<const Tank *> &sorted_tanks,
                                         int begin, int end) {
    const int NUM_TANKS = end - begin;
    sorted_tanks.reserve(NUM_TANKS);
    sorted_tanks.emplace_back(&original.at(begin));

    for (int i = begin + 1; i < (begin + NUM_TANKS); i++) {
        const Tank &current_tank = original.at(i);

        for (int s = (int) sorted_tanks.size() - 1; s >= 0; s--) {
            const Tank *current_checking_tank = sorted_tanks.at(s);

            if ((current_checking_tank->compare_health(current_tank) <= 0)) {
                sorted_tanks.insert(1 + sorted_tanks.begin() + s, &current_tank);
                break;
            }

            if (s == 0) {
                sorted_tanks.insert(sorted_tanks.begin(), &current_tank);
                break;
            }
        }
    }
}

void Tmpl8::Game::quick_sort_tanks_health(const std::vector<Tank> &original, std::vector<const Tank *> &sorted_tanks,
                                          int begin, int end) {

}


// -----------------------------------------------------------
// Draw the health bars based on the given tanks health values
// -----------------------------------------------------------
void Tmpl8::Game::draw_health_bars(const std::vector<const Tank *> &sorted_tanks, const int team) {
    int health_bar_start_x = (team < 1) ? 0 : (SCRWIDTH - HEALTHBAR_OFFSET) - 1;
    int health_bar_end_x = (team < 1) ? health_bar_width : health_bar_start_x + health_bar_width - 1;

    for (int i = 0; i < SCRHEIGHT - 1; i++) {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
    }

    //Draw the <SCRHEIGHT> least healthy tank health bars
    int draw_count = std::min(SCRHEIGHT, (int) sorted_tanks.size());
    for (int i = 0; i < draw_count - 1; i++) {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        float health_fraction = (1 - ((double) sorted_tanks.at(i)->health / (double) tank_max_health));

        if (team == 0) {
            screen->bar(health_bar_start_x + (int) ((double) health_bar_width * health_fraction), health_bar_start_y,
                        health_bar_end_x, health_bar_end_y, GREENMASK);
        } else {
            screen->bar(health_bar_start_x, health_bar_start_y,
                        health_bar_end_x - (int) ((double) health_bar_width * health_fraction), health_bar_end_y,
                        GREENMASK);
        }
    }
}

// -----------------------------------------------------------
// When we reach max_frames print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance() {
    char buffer[128];
    if (frame_count >= max_frames) {
        if (!lock_update) {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update) {
        screen->bar(420 + HEALTHBAR_OFFSET, 170, 870 + HEALTHBAR_OFFSET, 430, 0x030000);
        int ms = (int) duration % 1000, sec = ((int) duration / 1000) % 60, min = ((int) duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime) {
    if (!lock_update) {
        update(deltaTime);
    }
    draw();

    measure_performance();

    // print something in the graphics window
    //screen->Print("hello world", 2, 2, 0xffffff);

    // print something to the text window
    //cout << "This goes to the console window." << std::endl;

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}


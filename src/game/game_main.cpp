// Tell the engine that this is loaded
#define FOG_GAME
#include <vector>
#include <fstream>
#include <algorithm>

const u32 NO_ASSET = 1024;
const f32 WORLD_LEFT_EDGE  = -100;
const f32 WORLD_RIGHT_EDGE =  100;
const f32 WORLD_TOP_EDGE = 34;
const f32 WORLD_BOTTOM_EDGE = -34;
const f32 PIXEL_TO_WORLD = 1.0 / 3.0;

namespace Game {

using namespace Input;
Physics::ShapeID square;
bool game_over = false;
bool dead = false;
bool music_muted = false;
Mixer::AudioID music_id;

void explode_truck();
Vec2 get_truck_pos();
Vec2 paralax(Vec2 position, f32 distance);

void set_game_over() {
    game_over = true;
}

void end_game() {
    if (dead) return;
    Logic::add_callback(Logic::At::PRE_UPDATE, set_game_over, Logic::now() + 3);
    explode_truck();
    dead = true;
}

const float START_TRASH_LEVEL = -50;
const float COLLISION_TRASH_LEVEL = 20;
const float MAX_TRASH_LEVEL = -15;
const float MIN_TRASH_LEVEL = -43;
const float TRASH_VELOCITY = 0.01;

const float SHOW_CONTROLS_FOR = 12.0;

f32 currentTrashLevel = START_TRASH_LEVEL;
f32 goalTrashLevel = MIN_TRASH_LEVEL;
f32 groundLevel = currentTrashLevel + COLLISION_TRASH_LEVEL;

#include "text.h"
#include "highscore.h"
#include "combo.h"
#include "entity.h"
#include "enemy.h"
#include "truck.h"
#include "truck.cpp"
#include "clouds.h"
#include "gameover.cpp"
#include "star_particles.h"

Truck truck;

void explode_truck() {
    Mixer::play_sound(ASSET_DEATH, 1.0, 0.7);
    truck.smoke_particles.position = truck.body.position;
    truck.smoke_particles.velocity_dir = {0, 2 * PI};
    truck.smoke_particles.velocity = {1, 7};
    for (int i = 0; i < 40; i++) {
        truck.smoke_particles.spawn();
    }

    truck.super_particles.position = truck.body.position;
    truck.super_particles.velocity_dir = {0, 2 * PI};
    truck.super_particles.velocity = {1, 14};
    for (int i = 0; i < 60; i++) {
        truck.super_particles.spawn();
    }
}

f32 show_controls = 0.0;

float CASTLE_DISTANCE = 5;
float TRASH_MOUNTAIN_DISTANCE = -0.5;

float CAMERA_MAX = WORLD_RIGHT_EDGE - 60;
float CAMERA_MIN = WORLD_LEFT_EDGE  + 60;

const std::string VALID_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// TODO(ed): An enum for the game state
bool title_screen = true;
std::vector<HighScore> highscores;

Vec2 get_truck_pos() {
    return truck.body.position;
}

Mixer::AudioID play_music() {
    return Mixer::play_sound(ASSET_BEEPBOX_SONG, 1.0, 5.0
              ,Mixer::AUDIO_DEFAULT_VARIANCE, Mixer::AUDIO_DEFAULT_VARIANCE, true);
}

void setup() {
    highscores = read_highscores();

    // Bind wasd
    add(K(w), Player::P1, Name::BOOST);
    add(K(d), Player::P1, Name::UP);
    add(K(s), Player::P1, Name::CYCLEDOWN);
    add(K(a), Player::P1, Name::DOWN);

    // Bind arrow keys
    add(K(UP), Player::P1, Name::BOOST);
    add(K(RIGHT), Player::P1, Name::UP);
    add(K(DOWN), Player::P1, Name::CYCLEDOWN);
    add(K(LEFT), Player::P1, Name::DOWN);

    // Shoot!
    add(K(SPACE), Player::P1, Name::SHOOT);

    // Confirm
    add(K(RETURN), Player::P1, Name::CONFIRM);

    // Mute
    add(K(m), Player::P1, Name::MUTE);

    music_id = play_music();
    // Fullscreen
    add(K(f), Player::P1, Name::FULLSCREEN);

    Renderer::set_window_size(1200, 670);
    Renderer::set_window_position(200, 100);

    {
        Vec2 points[] = {
            V2(-0.5, -0.5),
            V2( 0.5, -0.5),
            V2( 0.5,  0.5),
            V2(-0.5,  0.5),
        };
        square = Physics::add_shape(LEN(points), points);
    }

    truck = create_truck();
    initalize_bullets();

    initalize_enemies();
    createCloudSystems();
    createStarSystem();

    Renderer::global_camera.zoom = 3.335 / 200.0;

    Logic::add_callback(Logic::At::PRE_UPDATE, spawnCloud, 0, Logic::FOREVER, 2);

    reset_score();

    // Warm the cloud particles
    for (u32 i = 0; i < 100; i++) {
        spawnCloud();
        updateClouds(2);
    }
}

void camera_follow(Vec2 target, f32 delta) {
    target.x = CLAMP(CAMERA_MIN, CAMERA_MAX, -target.x);
    Vec2 curr = Renderer::global_camera.position;
    Renderer::global_camera.position.x = LERP(curr.x, 0.05, target.x);
}

Vec2 paralax(Vec2 position, f32 distance) {
    return position - Renderer::global_camera.position / distance;
}

void update_title_screen() {
    if (pressed(Player::P1, Name::CONFIRM))
        title_screen = false;
}

f32 time_pressed = 0;
f32 update_speed = 0;
int highscore_index[] = { 10, 10, 10 };
u32 highscore_space = 0;
std::string highscore_name = "AAA";
bool first_pass = true;
void update_game_over_screen(f32 delta) {
    stars.update(Logic::delta());
    if (highscores.empty() || score > highscores[0].score) {
        spawnStar();
        if (first_pass) {
            first_pass = false;
            Mixer::play_sound(ASSET_NEW_HIGHSCORE, 1.0, 0.8);
        }
    }
    if (pressed(Player::P1, Name::CYCLEDOWN)) {
        Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
        highscore_index[highscore_space] += 1;
        highscore_index[highscore_space] %= VALID_CHARS.size();
        highscore_name[highscore_space] = VALID_CHARS[highscore_index[highscore_space]];
    }

    if (down(Player::P1, Name::CYCLEDOWN)) {
        time_pressed += delta * update_speed;
        update_speed += delta;
        if (time_pressed >= 0.2) {
            Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
            highscore_index[highscore_space] += 1;
            highscore_index[highscore_space] %= VALID_CHARS.size();
            highscore_name[highscore_space] = VALID_CHARS[highscore_index[highscore_space]];
            time_pressed = 0;
        }
    }

    if (released(Player::P1, Name::CYCLEDOWN)) {
        time_pressed = 0;
        update_speed = 1;
    }

    if (pressed(Player::P1, Name::BOOST)) {
        Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
        highscore_index[highscore_space] -= 1;
        if (highscore_index[highscore_space] == -1)
            highscore_index[highscore_space] = VALID_CHARS.size() - 1;
        highscore_name[highscore_space] = VALID_CHARS[highscore_index[highscore_space]];
    }

    if (down(Player::P1, Name::BOOST)) {
        time_pressed += delta * update_speed;
        update_speed += delta;
        if (time_pressed >= 0.2) {
            Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
            highscore_index[highscore_space] -= 1;
            if (highscore_index[highscore_space] == -1)
                highscore_index[highscore_space] = VALID_CHARS.size() - 1;
            highscore_name[highscore_space] = VALID_CHARS[highscore_index[highscore_space]];
            time_pressed = 0;
        }
    }

    if (released(Player::P1, Name::BOOST)) {
        time_pressed = 0;
        update_speed = 1;
    }

    if (pressed(Player::P1, Name::UP)) {
        Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
        highscore_space = (highscore_space + 1) % 3;
    }

    if (pressed(Player::P1, Name::DOWN)) {
        Mixer::play_sound(ASSET_SELECT, 1.0, 0.5);
        if (highscore_space == 0) highscore_space = 2;
        else highscore_space--;
    }

    if (pressed(Player::P1, Name::CONFIRM)) {
        Mixer::play_sound(ASSET_SELECT, 2.0, 0.5);
        highscore_space = 0;
        write_highscores(highscores, highscore_name, score);
        highscores = read_highscores();
        highscore_name = "AAA";
        game_over = false;
        dead = false;
        title_screen = true;
        initalize_enemies();

        truck.reset();
        currentTrashLevel = START_TRASH_LEVEL;
        goalTrashLevel = MIN_TRASH_LEVEL;
        groundLevel = currentTrashLevel + COLLISION_TRASH_LEVEL;

        // TODO(ed): Reset truck here.
        show_controls = 0;
        first_pass = true;
        reset_score();
        bullets.clear();
        stars.clear();
    }
}

void update_game(f32 delta) {
    // Update game elements
    show_controls += delta;
    truck.update(delta);
    update_bullets(delta);
    spawner.update(delta);
    update_enemies(delta);

    update_score();

    for (Enemy* enemy : enemies) {
        Physics::Body enemy_body = enemy->get_body();
        if (Physics::check_overlap(&enemy_body, &truck.body)) {
            if (truck.boost_to_kill && enemy->boost_killable) {
                // TODO(ed): More hp requires more speed!
                score_boost_kill_enemy();
                emit_boost_to_kill_particles(enemy->pos);
                truck.super_boost();
                enemy->hp = 0;
            } else {
                end_game();
            }
        }
    }

    // Check for bullet collisions
    for (Bullet& bullet : bullets) {
        for (Enemy* enemy : enemies) {
            Physics::Body enemy_body = enemy->get_body();
            if (check_overlap(&bullet.body, &enemy_body)) {
                bullet.hit_enemy = true;
                enemy->hp -= 1;
                score_hit_enemy();
                emit_hit_particles(bullet.body.position);
            }
        }
    }

    if (down(Player::P1, Name::BOOST)) {
        Renderer::global_camera.shake = random_unit_vec2() * 0.001;
    }

    camera_follow(truck.body.position, delta);

    if (currentTrashLevel >= MAX_TRASH_LEVEL) {
        end_game();
    } else if (currentTrashLevel < goalTrashLevel){
        currentTrashLevel += TRASH_VELOCITY;
    }
}

// Main logic
void update(f32 delta) {
    Renderer::global_camera.shake = V2(0, 0);
    updateClouds(delta);

    if (title_screen)
        update_title_screen();
    else if (game_over)
        update_game_over_screen(delta);
    else
        update_game(delta);

    // Mute logic
    if (pressed(Player::P1, Name::MUTE)) {
        if (music_muted) {
            music_id = play_music();
        } else {
            stop_sound(music_id);
        }
        music_muted = !music_muted;
    }

    // Fullscreen logic
    if (pressed(Player::P1, Name::FULLSCREEN)) {
        Renderer::toggle_fullscreen();
    }
}

// Main draw
void draw() {
    Renderer::global_camera.shake = V2(0, 0);
    Renderer::push_sprite(paralax(V2(0, 0), 1.0), V2(120, -67), 0,
                          ASSET_BACKGROUND, V2(0, 0), V2(120, 67));
    drawClouds();

    Renderer::push_sprite(paralax(V2(0, -0.5), CASTLE_DISTANCE), V2(43, -66), 0,
                          ASSET_CASTLE, V2(0, 0), V2(43, 66));

    // Draw trash mountain.
    Renderer::push_sprite(
        V2(60, currentTrashLevel),
        V2(120, -37), 0, ASSET_TRASH_MOUNTAIN, V2(0, 0), V2(120, 37));
    Renderer::push_sprite(
        V2(-60, currentTrashLevel),
        V2(120, -37), 0, ASSET_TRASH_MOUNTAIN, V2(0, 0), V2(120, 37));

    Vec2 cam = -Renderer::global_camera.position;
    stars.position.x = cam.x;
    if (game_over) {
        Vec2 dim;
        f32 size = 1.6;

        drawStars();

        if (highscores.empty() || score > highscores[0].score) {
            dim = messure_text(" NEW HIGHSCORE", size);
            draw_text(" NEW HIGHSCORE", cam - V2(dim.x / 2, -20), size, 0.7, 2.5);
        }

        size = 1.0;
        dim = messure_text("GAME OVER", size);
        draw_text("GAME OVER", cam - V2(dim.x / 2, -6.0),
                  size, sin(Logic::now() / 10) * 0.5);

        char *score_text = Util::format("SCORE: %d", score);
        size = 0.75;
        dim = messure_text(score_text, size);
        draw_text(score_text, cam - V2(dim.x / 2, 0.0),
                size, sin(Logic::now() / 10) * 0.5);

        char output[] = " \0";
        f32 spacing = 16 * PIXEL_TO_WORLD;
        f32 left = -spacing * 1.5;
        size = 1.0;
        for (u32 i = 0; i < highscore_name.size(); i++) {
            Vec2 position = cam + V2(left + i * spacing, -8.0);
            if (i == highscore_space) {
                output[0] = '<';
                draw_text(output, position + V2(0,  4.0), size);
                output[0] = '>';
                draw_text(output, position + V2(0, -4.0), size);
            }

            output[0] = highscore_name[i];
            draw_text(output, position, size);
        }
    } else if (title_screen) {
        Vec2 dim;
        f32 scale;

        scale = 1.0;
        dim = messure_text("FLIGHT", scale);
        draw_text("FLIGHT", cam - V2(dim.x / 2, -5.0), scale, 0.02);

        scale = 0.5;
        dim = messure_text("OF THE", scale);
        draw_text("OF THE", cam - V2(dim.x / 2, 0.0), scale, 0.02);

        scale = 1.0;
        dim = messure_text("TRUCK", scale);
        draw_text("TRUCK", cam - V2(dim.x / 2, 5.0), scale, 0.40, 2.5);

        if (MOD(Logic::now(), 1.3) > 0.5) {
            scale = 0.5;
            dim = messure_text("PRESS ENTER TO START", scale);
            draw_text("PRESS ENTER TO START", cam - V2(dim.x / 2, 13.0), scale, 0.02);
        }

        scale = 0.5;
        for (u32 i = 0; i < highscores.size() && i < 3; i++) {
            char *text = Util::format("%s %10d", highscores[i].name.c_str(), highscores[i].score);
            dim = messure_text(text, scale);
            draw_text(text, cam - V2(dim.x / 2 + sin(Logic::now() + i), 19 + 4 * i), scale, 0.5 / (i + 1));

        }

    } else {
        if (show_controls < SHOW_CONTROLS_FOR)
            Renderer::push_sprite(paralax(V2(0, -12.0), CASTLE_DISTANCE), V2(39, -16), 0,
                    ASSET_PARTICLE_SPRITESHEEP, V2(0, 35), V2(39, 16));

        truck.draw();
        draw_bullets();
        draw_enemies();

        f32 scale = 1.0;
        f32 wavey_ness = MIN(pow(get_multiplier(), 0.75) / 5.0, 2);
        char *score_text = Util::format("%10d %dX", get_score(), get_multiplier());
        Vec2 dim = messure_text(score_text, scale);
        draw_text(score_text, cam + V2(60 - dim.x, +27), scale, wavey_ness, wavey_ness);

    }
}

}  // namespace Game

// Tell the engine that this is loaded
#define FOG_GAME
#include <vector>

const u32 NO_ASSET = 1024;
const f32 WORLD_LEFT_EDGE  = -20;
const f32 WORLD_RIGHT_EDGE =  20;
const f32 PIXEL_TO_WORLD = 1.0 / 3.0;

namespace Game {

using namespace Input;
Physics::ShapeID square;

Vec2 get_truck_pos();

#include "entity.h"
#include "enemy.h"
#include "truck.h"
#include "truck.cpp"
#include "clouds.h"

Truck truck;

float MAX_TRASH_LEVEL = -15;
float MIN_TRASH_LEVEL = -43;

float CASTLE_DISTANCE = 5;
float TRASH_MOUNTAIN_DISTANCE = -0.5;

float CAMERA_MAX = 20;
float CAMERA_MIN = -20;

std::vector<Enemy*> enemies;
Spawner spawner(&enemies);

Vec2 get_truck_pos() {
    return truck.body.position;
}

void draw_entity(Entity* entity) {
    if (entity->image != NO_ASSET) {
        Image* img = Asset::fetch_image(entity->image);
        Renderer::push_sprite(entity->pos,
                -entity->dim,
                entity->rotation,
                entity->image,
                V2(0,0),
                V2(img->width, img->height));
    } else {
        Renderer::push_rectangle(entity->pos, entity->dim);
    }
}

void setup() {
    add(K(w), Player::P1, Name::UP);
    add(K(d), Player::P1, Name::UP);
    add(K(s), Player::P1, Name::DOWN);
    add(K(a), Player::P1, Name::DOWN);
    add(K(SPACE), Player::P1, Name::BOOST);
    add(K(m), Player::P1, Name::SHOOT);

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

	createCloudSystems();

    Renderer::global_camera.zoom = 3.335 / 200.0;

	Logic::add_callback(Logic::At::PRE_UPDATE, spawnCloud, 0, Logic::FOREVER, 2);
}

void camera_follow(Vec2 target, f32 delta) {
    target.x = CLAMP(CAMERA_MIN, CAMERA_MAX, -target.x);
    Vec2 curr = Renderer::global_camera.position;
    Renderer::global_camera.position.x = LERP(curr.x, 0.01, target.x);
}

Vec2 paralax(Vec2 position, f32 distance) {
    return position - Renderer::global_camera.position / distance;
}

// Main logic
void update(f32 delta) {

    // Update game elements
    truck.update(delta);
    update_bullets(delta);
    spawner.update(delta);
    for (Enemy* enemy : enemies) {
        enemy->update(delta);
    }

    camera_follow(truck.body.position, delta);

	// spawnCloud();
	updateClouds(delta);
}

// Main draw
void draw() {
    Renderer::push_sprite(paralax(V2(0, 0), 1.0), V2(120, -67), 0,
            ASSET_BACKGROUND, V2(0, 0), V2(120, 67));

	drawClouds();

    Renderer::push_sprite(paralax(V2(0, -0.5), CASTLE_DISTANCE), V2(43, -66), 0,
            ASSET_CASTLE, V2(0, 0), V2(43, 66));

    truck.draw();
    draw_bullets();
    for (Enemy* enemy : enemies) {
        draw_entity(enemy);
    }

    // Draw trash mountain.
    Renderer::push_sprite(paralax(V2( 60, -43), TRASH_MOUNTAIN_DISTANCE),
            V2(120, -37), 0, ASSET_TRASH_MOUNTAIN, V2(0, 0), V2(120, 37));
    Renderer::push_sprite(paralax(V2(-60, -43), TRASH_MOUNTAIN_DISTANCE),
            V2(120, -37), 0, ASSET_TRASH_MOUNTAIN, V2(0, 0), V2(120, 37));

}

}  // namespace Game

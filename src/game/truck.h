const Vec4 TRUCK_SPRITE = V4(1, 4, 15, 9);
const Vec4 BULLET_SPRITE = V4(10, 0, 6, 3);
const f32 BULLET_ALIVE_TIME = 5.0;

const f32 TRUCK_MAX_SPEED = 225.0;
const f32 TRUCK_BOOST_STRENGTH = 15.0;
const f32 TRUCK_ROTATION_SPEED = 5.0;
const f32 TRUCK_VELOCITY_DAMPING = 0.4;
const f32 TRUCK_VELOCITY_WEIGHT = 0.2;

struct Bullet {
    Physics::Body body;
    f32 spawn_time;
    f32 angle;
    f32 speed = 20.0;
    Vec2 dimension = V2(BULLET_SPRITE.z, BULLET_SPRITE.w) * PIXEL_TO_WORLD;
    
    void update(f32 delta);

    void draw();
};

struct Truck {
    Physics::Body body;
    Renderer::ParticleSystem boost_particles;
    Renderer::ParticleSystem smoke_particles;
    Vec2 dimension = V2(TRUCK_SPRITE.z, TRUCK_SPRITE.w) * PIXEL_TO_WORLD;
    Vec2 forward = V2(1, 0);

    void boost(f32 delta);

    void update(f32 delta);

    void draw();
};

std::vector<Bullet> bullets;
void initalize_bullets();

void create_bullet(Vec2 position, Vec2 forward);

void update_bullets(f32 delta);

void draw_bullets();

Truck create_truck();


#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include <float.h> // for the limit FLT_MAX

#define SCR_W 800
#define SCR_H 600

#define CLOTH_WIDTH 20
#define CLOTH_HEIGHT 20
#define SEP_W 25
#define SEP_H 25

typedef struct {
    Vector2 position;
    Vector2 previous_postion;
    Vector2 acceleration;
    bool fixed;
} Particle;

typedef struct {
    Particle *p1;
    Particle *p2;
    float initial_length;
    bool is_active;
} Constraint;

typedef struct constraint_list {
    Constraint *constraint;
    struct constraint_list *next;
} ConstraintList;

Particle create_particle(Vector2 position, Vector2 acceleration, bool fixed) {
    Particle p = {
            .position = position,
            .previous_postion = position,
            .acceleration = acceleration,
            .fixed = fixed
    };

    return p;
}

void apply_force(Particle *p, Vector2 force) {
    p->acceleration = Vector2Add(p->acceleration, force);
}

void update_particle(Particle *p, float delta_time) {

    if (!p->fixed) {

        Vector2 velocity = Vector2Subtract(p->position, p->previous_postion);
        p->previous_postion = p->position;

        p->position = Vector2Add(p->position,
                                 Vector2Add(velocity,
                                            Vector2Scale(p->acceleration, delta_time * delta_time)));
        p->acceleration = (Vector2) {0, 0}; // reset after update
    }

}

void constrain_to_bounds(Particle *p, Rectangle r) {
    if (p->position.x < r.x) p->position.x = r.x;
    if (p->position.x > r.x + r.width) p->position.x = r.x + r.width;

    if (p->position.y < r.y) p->position.y = r.y;
    if (p->position.y > r.y + r.height) p->position.y = r.y + r.height;
}

Constraint *create_constraint(Particle *p1, Particle *p2, float length) {
    Constraint *c = malloc(sizeof(Constraint));
    if (!c) {
        fprintf(stderr, "Unable to allocate new Constraint");
    }
    c->p1 = p1;
    c->p2 = p2;
    c->initial_length = length;
    c->is_active = true;

    return c;
}

void satisfy_constraint(Constraint *c) {
    if (!c->is_active) return;
    Vector2 delta = Vector2Subtract(c->p2->position, c->p1->position);
    float current_length = Vector2Length(delta);
    float difference = (current_length - c->initial_length ) / current_length;

    Vector2 correction = Vector2Scale(delta, 0.5 * difference);

    if (!c->p1->fixed) c->p1->position = Vector2Add(c->p1->position, correction);
    if (!c->p2->fixed) c->p2->position = Vector2Subtract(c->p2->position, correction);
}

void draw_points(Particle points[]);

void draw_constraints(ConstraintList *constraints);

void satisfy_constraints(ConstraintList *constraints);

Constraint *get_nearest_constraint(ConstraintList *constraints, Vector2 mousePosition);

float distance_from_point_to_line_segment(Vector2 point, Vector2 p1, Vector2 p2);

int main(void) {

    Particle points[CLOTH_WIDTH * CLOTH_HEIGHT];

    ConstraintList *constraints = NULL;
    ConstraintList *current_constraint_pointer = constraints;

    // determine padding
    float cloth_width = CLOTH_WIDTH * SEP_W;
    float cloth_height = CLOTH_HEIGHT * SEP_H;
    float padding_x = (SCR_W - cloth_width) / 2.0;
    float padding_y = (SCR_H - cloth_height) / 2.0;

    for (size_t y = 0; y < CLOTH_HEIGHT; y++) {
        for (size_t x = 0; x < CLOTH_WIDTH; x++) {
            points[y * CLOTH_WIDTH + x] = create_particle(
                    (Vector2) {x * SEP_W + padding_x, y * SEP_H + padding_y},
                    (Vector2) {0.0f, 0.0f},
                    (y == 0)
            );
        }
    }

    Particle *p1, *p2;
    Constraint *c;
    ConstraintList *cl = NULL;

    for (size_t y = 0; y < CLOTH_HEIGHT; y++) {
        for (size_t x = 0; x < CLOTH_WIDTH; x++) {
            if (x < CLOTH_WIDTH - 1) {
                p1 = &points[y * CLOTH_WIDTH + x];
                p2 = &points[y * CLOTH_WIDTH + x + 1];
                cl = malloc(sizeof(ConstraintList));
                if (!cl) {
                    fprintf(stderr, "Unable to allocate memory for constraint list node");
                    exit(1);
                }

                c = create_constraint(
                        p1,
                        p2,
                        Vector2Distance(p1->position, p2->position)
                );
                cl->constraint = c;
                if (current_constraint_pointer == NULL) {
                    constraints = cl;
                } else {
                    current_constraint_pointer->next = cl;
                }
                current_constraint_pointer = cl;
            }

            if (y < CLOTH_HEIGHT - 1) {
                p1 = &points[y * CLOTH_WIDTH + x];
                p2 = &points[(y + 1) * CLOTH_WIDTH + x];
                cl = malloc(sizeof(ConstraintList));
                if (!cl) {
                    fprintf(stderr, "Unable to allocate memory for constraint list node");
                    exit(1);
                }

                c = create_constraint(
                        p1,
                        p2,
                        Vector2Distance(p1->position, p2->position)
                );
                cl->constraint = c;

                if (current_constraint_pointer == NULL) {
                    constraints = cl;
                } else {
                    current_constraint_pointer->next = cl;
                }
                current_constraint_pointer = cl;
            }
        }
    }

    InitWindow(SCR_W, SCR_H, "Cloth Simulation");
    SetTargetFPS(30);
    while (!WindowShouldClose()) {

        float delta_time = GetFrameTime();

        if (IsKeyPressed(KEY_Q)) break;

        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
            Constraint * c = get_nearest_constraint(constraints, GetMousePosition());
            if (c != NULL) c->is_active = false;
        }


        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (size_t y = 0; y < CLOTH_HEIGHT; ++y) {
            for (size_t x = 0; x < CLOTH_WIDTH; ++x) {
                Particle *p = &points[y * CLOTH_WIDTH + x];
                apply_force(p, (Vector2) {0.0f, 10.0f});
                update_particle(p, delta_time * 4.0);
                constrain_to_bounds(p, (Rectangle) {0, 0, SCR_W, SCR_H});
            }
        }

        satisfy_constraints(constraints);

        draw_points(points);
        draw_constraints(constraints);
        EndDrawing();
    }
    CloseWindow();
}

void draw_points(Particle points[]) {
    for (size_t y = 0; y < CLOTH_HEIGHT; ++y) {
        for (size_t x = 0; x < CLOTH_WIDTH; ++x) {
            Particle p = points[y * CLOTH_WIDTH + x];
            DrawCircle(p.position.x,  p.position.y, 2, RED);
        }
    }
}

void draw_constraints(ConstraintList *constraints) {
    ConstraintList *iterator = constraints;
    while (iterator->constraint != NULL) {
        Constraint *c = iterator->constraint;
        if (c->is_active) {
            DrawLineEx(c->p1->position, c->p2->position, 1, BLUE);
        }
        if (!iterator->next) break;
        iterator = iterator->next;
    }
}

void satisfy_constraints(ConstraintList *constraints) {
    ConstraintList *iterator = constraints;
    while (iterator->constraint != NULL) {
        Constraint *c = iterator->constraint;
        satisfy_constraint(c);
        if (!iterator->next) break;
        iterator = iterator->next;
    }

}


Constraint *get_nearest_constraint(ConstraintList *constraints, Vector2 mousePosition){
    Constraint *c = NULL;
    // iterate over each constraint and determine the line between the points
    // then, determine if the mouse pointer is within specific distance of the line
    // if so, keep track of the contstraint.
    // return the constraint with the lowest distance
    float radius = 4.0;
    float distance = FLT_MAX;
    ConstraintList *iterator = constraints;
    while (iterator->constraint != NULL) {
        Constraint *c1 = iterator->constraint;
        if (c1->is_active) {
            float d = distance_from_point_to_line_segment(mousePosition, c1->p1->position, c1->p2->position);
            if (d < radius && d < distance) {
                distance = d;
                c = c1;
            }
        }
        if (!iterator->next) break;
        iterator = iterator->next;
    }

    return c;
}



/* https://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment */
float distance_from_point_to_line_segment(Vector2 point, Vector2 p1, Vector2 p2) {

    float A = point.x - p1.x;
    float B = point.y - p1.y;
    float C = p2.x - p1.x;
    float D = p2.y - p1.y;

    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    float param = -1.0;
    if (len_sq != 0) //in case of 0 length line
        param = dot / len_sq;

    float xx, yy;

    if (param < 0) {
        xx = p1.x;
        yy = p1.y;
    }
    else if (param > 1) {
        xx = p2.x;
        yy = p2.y;
    }
    else {
        xx = p1.x + param * C;
        yy = p1.y + param * D;
    }

    float dx = point.x - xx;
    float dy = point.y - yy;
    return sqrt(dx * dx + dy * dy);
}
#include <stdio.h>
#include <math.h>
#include "raylib.h"
#undef PI
#define PI 3.14159265358979323846
#define GRAVITY -9.80665
#define SMDEGTORAD (PI / 180.0)
#define MAXIMUM_SPHERES 8
#define SPHERE_ATTRIBUTES 11 // 3 pos + 3 vel + 3 acc + radius
#define X 0
#define Y 1
#define Z 2
#define PVX 3
#define PVY 4
#define PVZ 5
#define FCX 6
#define FCY 7
#define FCZ 8
#define MASS 9
#define RADIUS 10
#define DEBUG // printf("Debug: Spheres loaded = %d\n", sphCount);

typedef struct {
    double x;
    double y;
    double z;
} smVector3D;

void setUpStringSet(char* []);
void pullReq(char* , int);
void updateScene(double [][SPHERE_ATTRIBUTES], int, double*);
void updateSphere(double* , double*);
void render(double [][SPHERE_ATTRIBUTES], int);
double dblPow(double);
double smVectorDist(smVector3D, smVector3D);
smVector3D smVectorSubtract(smVector3D, smVector3D);
smVector3D smVectorNormalize(smVector3D);

int main (void) {
    char* ioStatements[11];
    double sphData[MAXIMUM_SPHERES][SPHERE_ATTRIBUTES] = {0}, dt;
    int spare, i1 = 0, sphCount, fps, cameraMode;
    int* ptrSpare = &spare;

    setUpStringSet(ioStatements);

    pullReq(ioStatements[0], 1);
    while (1) {
        // Positions
        pullReq(ioStatements[1], 0);
        scanf("%d", ptrSpare);
        sphData[i1][X] = (double) spare;
        pullReq(ioStatements[2], 0);
        scanf("%d", ptrSpare);
        sphData[i1][Y] = (double) spare;
        pullReq(ioStatements[3], 0);
        scanf("%d", ptrSpare);
        sphData[i1][Z] = (double) spare;

        // Radius
        pullReq(ioStatements[4], 0);
        scanf("%d", ptrSpare);
        sphData[i1][RADIUS] = (spare / 20.0);

        // Previous Positions
        // Might change these later
        sphData[i1][PVX] = sphData[i1][X] - 0.05; // Temporarily Done For Movement
        sphData[i1][PVY] = sphData[i1][Y];
        sphData[i1][PVZ] = sphData[i1][Z] + 0.05;

        // Force
        sphData[i1][FCX] = 0.0;
        sphData[i1][FCY] = 0.0;
        sphData[i1][FCZ] = 0.0;

        // Mass
        sphData[i1][MASS] = (4.0/3.0) * PI * (pow(sphData[i1][RADIUS], 3));

        i1++;

        if (i1 != MAXIMUM_SPHERES) {
            pullReq(ioStatements[5], 0);
            scanf("%d", ptrSpare);

            if (spare == 0) {
                break;
            }
        } else {
            pullReq(ioStatements[7], 1);
            break;
        }
    }
    sphCount = i1;

    // Request FPS
    pullReq(ioStatements[8], 0);
    scanf("%d", &fps);

    // Preprocessing
    dt = 1.0 / fps;

    // Engine Start
    InitWindow(100, 100, ioStatements[10]);

    int monitor = GetCurrentMonitor();
    int windowWidth = (0.8 * GetMonitorWidth(monitor));
    int windowHeight = (0.8 * GetMonitorHeight(monitor));

    SetWindowSize(windowWidth, windowHeight);
    SetWindowPosition((GetMonitorWidth(monitor) - windowWidth) / 2, (GetMonitorHeight(monitor) - windowHeight) / 2);
    DisableCursor();
    cameraMode = CAMERA_FREE;
    SetTargetFPS(fps);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 15.0f, 15.0f, 15.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    pullReq(ioStatements[6], 1);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F4)) {
            ToggleFullscreen();
        }

        UpdateCamera(&camera, cameraMode);

        updateScene(sphData, sphCount, &dt);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawGrid(20, 1.0f);

        render(sphData, sphCount);
        EndMode3D();

        DrawFPS(10, 10);
        EndDrawing();
    }

    DEBUG
    return 0;
}

void pullReq (char* str, int mode) {
    if (mode == 1) {
        printf("// %s\n", str);
    } else {
        printf("// %s", str);
    }

    return;
}

void setUpStringSet (char* a[]) {
    a[0] = "Welcome to the 3D sphere elastic collision chamber!";
    a[1] = "Enter position X: ";
    a[2] = "Enter position Y: ";
    a[3] = "Enter position Z: ";
    a[4] = "Enter radius: ";
    a[5] = "Would you like to add another sphere? (1/0) ";
    a[6] = "Running engine...";
    a[7] = "You can't add more spheres!";
    a[8] = "Target FPS: ";
    a[9] = "Engine shut down.";
    a[10] = "Simulation Chamber";
    return;
}

void updateScene(double data[][SPHERE_ATTRIBUTES], int count, double* dt) {
    // Update Position
    for (int i = 0; i < count; i++) {
        updateSphere(data[i], dt);
    }

    // Floor Collision
    for (int i = 0; i < count; i++) {
        double radius = data[i][RADIUS];

        if (data[i][Y] - radius < 0.0) {
            double penetration = radius - data[i][Y];
            double oldVelocityY = data[i][Y] - data[i][PVY];

            data[i][Y] = radius + penetration;
            data[i][PVY] = data[i][Y] + oldVelocityY;
        }
    }

    // Wall Collisions
    double roomLimit = 10.0;
    for (int i = 0; i < count; i++) {
        double radius = data[i][RADIUS];

        if (data[i][X] + radius > roomLimit) {
            double penetration = (data[i][X] + radius) - roomLimit;
            double oldVelocityX = data[i][X] - data[i][PVX];

            data[i][X] = roomLimit - radius - penetration;
            data[i][PVX] = data[i][X] + oldVelocityX;
        }
        else if (data[i][X] - radius < -roomLimit) {
            double penetration = -roomLimit - (data[i][X] - radius);
            double oldVelocityX = data[i][X] - data[i][PVX];

            data[i][X] = -roomLimit + radius + penetration;
            data[i][PVX] = data[i][X] + oldVelocityX;
        }

        if (data[i][Z] + radius > roomLimit) {
            double penetration = (data[i][Z] + radius) - roomLimit;
            double oldVelocityZ = data[i][Z] - data[i][PVZ];

            data[i][Z] = roomLimit - radius - penetration;
            data[i][PVZ] = data[i][Z] + oldVelocityZ;
        }
        else if (data[i][Z] - radius < -roomLimit) {
            double penetration = -roomLimit - (data[i][Z] - radius);
            double oldVelocityZ = data[i][Z] - data[i][PVZ];

            data[i][Z] = -roomLimit + radius + penetration;
            data[i][PVZ] = data[i][Z] + oldVelocityZ;
        }
    }

    // Sphere-To-Sphere Collisions
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            smVector3D posI = { data[i][X], data[i][Y], data[i][Z] };
            smVector3D posJ = { data[j][X], data[j][Y], data[j][Z] };

            double dist = smVectorDist(posI, posJ);
            double minDist = data[i][RADIUS] + data[j][RADIUS];

            if (dist < minDist) {
                if (dist == 0.0) continue;

                smVector3D normal = smVectorNormalize(smVectorSubtract(posJ, posI));
                double penetration = minDist - dist;

                double massI = data[i][MASS];
                double massJ = data[j][MASS];
                double totalMass = massI + massJ;

                double separationI = penetration * (massJ / totalMass);
                double separationJ = penetration * (massI / totalMass);

                data[i][X] -= normal.x * separationI;
                data[i][Y] -= normal.y * separationI;
                data[i][Z] -= normal.z * separationI;

                data[j][X] += normal.x * separationJ;
                data[j][Y] += normal.y * separationJ;
                data[j][Z] += normal.z * separationJ;

                smVector3D velI = { data[i][X] - data[i][PVX], data[i][Y] - data[i][PVY], data[i][Z] - data[i][PVZ] };
                smVector3D velJ = { data[j][X] - data[j][PVX], data[j][Y] - data[j][PVY], data[j][Z] - data[j][PVZ] };

                double relVelNormal = (velJ.x - velI.x) * normal.x + \
                                      (velJ.y - velI.y) * normal.y + \
                                      (velJ.z - velI.z) * normal.z;

                if (relVelNormal < 0) {
                    double impulse = (2.0 * relVelNormal) / totalMass;

                    data[i][PVX] -= impulse * massJ * normal.x;
                    data[i][PVY] -= impulse * massJ * normal.y;
                    data[i][PVZ] -= impulse * massJ * normal.z;

                    data[j][PVX] += impulse * massI * normal.x;
                    data[j][PVY] += impulse * massI * normal.y;
                    data[j][PVZ] += impulse * massI * normal.z;
                }
            }
        }
    }

    return;
}

void updateSphere(double* sph, double* dt) {
    sph[FCY] = GRAVITY * sph[MASS]; // Apply Gravity
    double tempX, tempY, tempZ;
    double dts = dblPow(*dt);
    tempX = sph[X], tempY = sph[Y], tempZ = sph[Z];

    sph[X] = (2.0 * sph[X]) - sph[PVX] + ((sph[FCX] / sph[MASS]) * dts);
    sph[Y] = (2.0 * sph[Y]) - sph[PVY] + ((sph[FCY] / sph[MASS]) * dts);
    sph[Z] = (2.0 * sph[Z]) - sph[PVZ] + ((sph[FCZ] / sph[MASS]) * dts);

    sph[PVX] = tempX;
    sph[PVY] = tempY;
    sph[PVZ] = tempZ;

    sph[FCX] = 0.0;
    sph[FCY] = 0.0;
    sph[FCZ] = 0.0;

    return;
}

void render (double data[][SPHERE_ATTRIBUTES], int count) {
    for (int i = 0; i < count; i++) {
        float x = (float)data[i][X];
        float y = (float)data[i][Y];
        float z = (float)data[i][Z];
        float radius = (float)data[i][RADIUS];

        Vector3 position = { x, y, z };

        DrawSphere(position, radius, MAROON);
        DrawSphereWires(position, radius, 8, 8, DARKGRAY);
    }

    return;
}

double dblPow (double val) {
    return (val * val);
}

double smVectorDist (smVector3D vtr1, smVector3D vtr2) {
    double dx = vtr2.x - vtr1.x;
    double dy = vtr2.y - vtr1.y;
    double dz = vtr2.z - vtr1.z;

    return sqrt(dblPow(dx) + dblPow(dy) + dblPow(dz));
}

smVector3D smVectorSubtract (smVector3D vtr1, smVector3D vtr2) {
    return (smVector3D) {vtr1.x - vtr2.x, vtr1.y - vtr2.y, vtr1.z - vtr2.z};
}

smVector3D smVectorNormalize (smVector3D vtr) {
    double length = sqrt(dblPow(vtr.x) + dblPow(vtr.y) + dblPow(vtr.z));

    if (length == 0) {
        return (smVector3D) {0, 0, 0};
    } else {
        return (smVector3D) {(vtr.x / length), (vtr.y / length), (vtr.z / length)};
    }
}

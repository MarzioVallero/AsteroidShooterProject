#include <random>
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Sound.h"
#include <GL/glew.h>
#include <gl/glut.h>
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include "Scene.h"
#include <IL/il.h>
#include <map>
#include <cmath>
#include "IL/ilu.h"

#define SW 1280
#define SH 720
#define MAX_NUMBER_LASERS 100
#define MAX_NUMBER_ASTEROIDS 6
#define MAX_VELOCITY 2
#define SPHERE_RADIUS 5
#define EXPLOSION_SPHERE_RADIUS 5
#define ONE_OVER_SQRT_TWO 0.85090352453
#define M_PI 3.14159265358979323846

enum State { mainmenu, pause, game, death, leaderboards, writeplayername, test };
enum Color { red, orange, yellow, lightgreen, darkgreen, cyan, blue, magenta, purple };
enum Difficulty { easy, normal, hard, insane };

struct Vector3 {
    float x, y, z;
} vector3;

struct ObjectStruct {
    float lx, ly, lz, ax, ay, az, tx, ty, tz;
    int enable = 0, hp = 0;
} objectStruct;

struct ExplosionStruct {
    float x, y, z, depth;
    int enable;
} explosionStruct;

struct Abilities {
    int qEnable, qCD, wEnable, wCD, eEnable, eCD, rEnable, rCD;
    float energy;
} abilitiesStruct;

Scene* spaceshipInterior = NULL;
Scene* spaceBackground = NULL;
Scene* writeplayernameBackground = NULL;
Scene* mainmenuBackground = NULL;
Scene* meteorite = NULL;
Scene* cannon = NULL;
static const std::string pathscene_1 = "../AsteroidShooterProject/Resources/spaceshipInterior.obj";
static const std::string pathscene_2 = "../AsteroidShooterProject/Resources/spaceBackground.obj";
static const std::string pathscene_3 = "../AsteroidShooterProject/Resources/writeplayernameBackground.obj";
static const std::string pathscene_4 = "../AsteroidShooterProject/Resources/mainmenuBackground.obj";
static const std::string pathscene_5 = "../AsteroidShooterProject/Resources/Meteorite.obj";
static const std::string pathscene_6 = "../AsteroidShooterProject/Resources/Cannon.obj";
GLuint scene_list = 0;
GLubyte lists[10];
const struct aiScene* scene = NULL;
std::map<std::string, GLuint*> textureIdMap;    // map image filenames to textureIds
GLuint* textureIds;                            // pointer to texture Array
GLfloat LightAmbient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat LightDiffuse[] = { 0.9f, 0.9f, 0.9f, 0.9f };
GLfloat LightPosition[] = { 0.0f, 0.0f, 10.0f, 1.0f };

ObjectStruct asteroids[MAX_NUMBER_ASTEROIDS];
ObjectStruct lasers[MAX_NUMBER_LASERS];
ObjectStruct cannons[2];
ExplosionStruct explosions[MAX_NUMBER_ASTEROIDS];
Abilities weapons;
Vector3 laserColor;
State GameState = writeplayername;
Color activeColor = lightgreen;
Difficulty difficulty = easy;
Sound sound;
int nextlaser, nextExplosion, killedAsteroids, framesDamage, asteroidInitialSpacing, fs;
float velocity, hullIntegrity, asteroidSpawnDistance, asteroidRampUpSpeed, spin, mouseX, mouseY, mouseZ, passiveEnergy, energyOnKill;
std::string difficultyName;
std::string colorName;
std::string playerName = "PlayerName";
std::string leaderboard[10][3];

void updateScene();
void resizeWindow(int width, int height);
void glInitialize(int w, int h);
void mouseClick(int button, int state, int x, int y);
void keyboardPress(unsigned char key, int x, int y);
void writeText(float x, float y, std::string str, int centered);
int Get3Dpos(int x, int y);
void initGameVariables(int firstinit);
void updateAsteroid(int i);
void drawMovingLaser(int i);
void resetMovingLaser(int i);
void getLaserAimDir();
void getAsteroidAimDir(int i);
void updateLaser(int i);
void drawMovingAsteroid(int i);
void resetMovingAsteroid(int i, int firstinit);
void checkLaserCollision(int i);
void colorSwitch();
void drawDamageFeedback();
void drawMenuLines(int death);
void fixEmissive();
void initializeExplosion(float x, float y, float z);
void drawExplosion(int i);
void resetExplosions(int i);
void difficultySwitch();
void readLeaderboard();
void updateLeaderboard();
void drawLeaderboard();
float bitmapCenterX(std::string text);
void laserSound();
void explosionSound();
void updateSpin();
void generateSceneLists();
void checkPlayerName();
void cursorPosition(int x, int y);
void drawCannons();
void getCannonAimDir();
void updateAbilities();
void useAbility(char key);
void drawExpandingShield();
void drawUI();

// Get the 3D position of the point clicked according to the current view
int Get3Dpos(int x, int y) {
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble object_x, object_y, object_z;
    int mouse_x = x;
    int mouse_y = y;
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    winX = (float)mouse_x;
    winY = (float)viewport[3] - (float)mouse_y - 1.0f;
    glReadBuffer(GL_BACK);
    glReadPixels(mouse_x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    gluUnProject((GLdouble)winX, (GLdouble)winY, (GLdouble)winZ, modelview, projection, viewport, &object_x, &object_y, &object_z);
    if (object_z > -5)
        return 1;
    lasers[nextlaser].tx = object_x;
    lasers[nextlaser].ty = object_y;
    lasers[nextlaser].tz = object_z;
    fprintf(stdout, "%f, %f, %f\n", object_x, object_y, object_z);

    return 0;
}

// Draws all the information provided by the in game HUD
// and the status bars of HullIntegrity, Energy and Abilities' cd
void drawUI() {
    glColor3f(laserColor.x, laserColor.y, laserColor.z);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    if (nextlaser % 2 == 0)
        writeText(1.4, -2.1, "Right cannon>: ready", 0);
    else
        writeText(1.4, -2.1, "Left cannon>: ready", 0);

    std::string string = "Hull integrity>: " + std::to_string(hullIntegrity).substr(0, std::to_string(hullIntegrity).find(".") + 3) + "%";
    writeText(0, -2.1, string, 1);
    string = "Asteroids destroyed>: " + std::to_string(killedAsteroids);
    writeText(-2.1, -2.1, string, 0);

    string = "Hull Repair>:";
    if (weapons.qCD == 0 && weapons.energy >= 20)
        string += " ready";
    else if (weapons.qCD == 0 && weapons.energy < 20)
        string += " missing energy";
    else
        string += " recharging " + std::to_string((float)weapons.qCD / 60).substr(0, std::to_string((float)weapons.qCD / 60).find(".") + 2) + " s";
    writeText(-2, -2.4, string, 1);

    string = "B.F. Lasers>:";
    if (weapons.wCD == 0 && weapons.energy >= 30)
        string += " ready";
    else if (weapons.wCD == 0 && weapons.energy < 30)
        string += " missing energy";
    else
        string += " recharging " + std::to_string((float)weapons.wCD / 60).substr(0, std::to_string((float)weapons.wCD / 60).find(".") + 2) + " s";
    writeText(-0.65, -2.4, string, 1);

    string = "Repulsor MK3>:";
    if (weapons.eCD == 0 && weapons.energy >= 50)
        string += " ready";
    else if (weapons.eCD == 0 && weapons.energy < 50)
        string += " missing energy";
    else
        string += " recharging " + std::to_string((float)weapons.eCD / 60).substr(0, std::to_string((float)weapons.eCD / 60).find(".") + 2) + " s";
    writeText(0.65, -2.4, string, 1);

    string = "NovaBlast KM>:";
    if (weapons.rCD == 0 && weapons.energy >= 75)
        string += " ready";
    else if (weapons.qCD == 0 && weapons.energy < 75)
        string += " missing energy";
    else
        string += " recharging " + std::to_string((float) weapons.rCD/60).substr(0, std::to_string((float)weapons.rCD / 60).find(".") + 2) + " s";
    writeText(2, -2.4, string, 1);

    string = "Energy at " + std::to_string(weapons.energy).substr(0, std::to_string(weapons.energy).find(".") + 3) + "%";
    writeText(0, -2.62, string, 1);

    // Draw HP graph
    float Graphoffset = (1.2f * hullIntegrity) / 100.0f;
    float ColorOffset = hullIntegrity / 100.0f;

    glPushMatrix();
        glColor3f(1.0, 0.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(-0.6, -2.13, 0);
            glVertex3f(-0.6, -2.22, 0);
            glColor3f(1.0 - ColorOffset, ColorOffset, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-0.6 + Graphoffset, -2.22, 0);
            glVertex3f(-0.6 + Graphoffset, -2.13, 0);
        glEnd();
    glPopMatrix();

    // Draw Energy graph
    Graphoffset = (1.2f * weapons.energy) / 100.0f;
    ColorOffset = weapons.energy / 100.0f;

    glPushMatrix();
        glColor3f(1.0, 0.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(-0.6, -2.65, 0);
            glVertex3f(-0.6, -2.74, 0);
            glColor3f(1.0 - ColorOffset, 0.0, ColorOffset);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-0.6 + Graphoffset, -2.74, 0);
            glVertex3f(-0.6 + Graphoffset, -2.65, 0);
        glEnd();
    glPopMatrix();

    // Draw qCD
    Graphoffset = (1.2f * weapons.qCD) / (3.0f * 60.0f);
    ColorOffset = weapons.qCD / (3.0f * 60.0f);

    glPushMatrix();
        glColor3f(0.0, 1.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(-2.6, -2.43, 0);
            glVertex3f(-2.6, -2.48, 0);
            glColor3f(0.0, 1.0 - ColorOffset, ColorOffset);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-2.6 + Graphoffset, -2.48, 0);
            glVertex3f(-2.6 + Graphoffset, -2.43, 0);
        glEnd();
    glPopMatrix();

    // Draw wCD
    Graphoffset = (1.2f * weapons.wCD) / (15.0f * 60.0f);
    ColorOffset = weapons.wCD / (15.0f * 60.0f);

    glPushMatrix();
        glColor3f(0.0, 1.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(-1.25, -2.43, 0);
            glVertex3f(-1.25, -2.48, 0);
            glColor3f(0.0, 1.0 - ColorOffset, ColorOffset);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-1.25 + Graphoffset, -2.48, 0);
            glVertex3f(-1.25 + Graphoffset, -2.43, 0);
        glEnd();
    glPopMatrix();

    // Draw eCD
    Graphoffset = (1.2f * weapons.eCD) / (30.0f * 60.0f);
    ColorOffset = weapons.eCD / (30.0f * 60.0f);

    glPushMatrix();
        glColor3f(0.0, 1.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(0.05, -2.43, 0);
            glVertex3f(0.05, -2.48, 0);
            glColor3f(0.0, 1.0 - ColorOffset, ColorOffset);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(0.05 + Graphoffset, -2.48, 0);
            glVertex3f(0.05 + Graphoffset, -2.43, 0);
        glEnd();
    glPopMatrix();

    // Draw rCD
    Graphoffset = (1.2f * weapons.rCD) / (30.0f * 60.0f);
    ColorOffset = weapons.rCD / (30.0f * 60.0f);

    glPushMatrix();
        glColor3f(0.0, 1.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(1.4, -2.43, 0);
            glVertex3f(1.4, -2.48, 0);
            glColor3f(0.0, 1.0 - ColorOffset, ColorOffset);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(1.4 + Graphoffset, -2.48, 0);
            glVertex3f(1.4 + Graphoffset, -2.43, 0);
        glEnd();
    glPopMatrix();
}

// Draws recursively between frames the expanding shield 
// that destroys all the asteroids on screen
void drawExpandingShield() {

    glLineWidth(0.001);
    glPushMatrix();
        glColor3f(0, 1, 0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glRotatef(100, 0, 1, 0);
        glutWireSphere(weapons.rEnable, 32, 32);
    glPopMatrix();
    glPushMatrix();
        glColor3f(0, 0, 1);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glRotatef(-100, 0, 1, 0);
        glutWireSphere(weapons.rEnable - 0.2, 32, 32);
    glPopMatrix();
    glPushMatrix();
        glColor3f(1, 0, 0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glRotatef(90, 1, 0, 0);
        glutWireSphere(weapons.rEnable - 0.4, 32, 32);
    glPopMatrix();

    for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i) {
        if (asteroids[i].lz >=  - 2 * weapons.rEnable) {
            initializeExplosion(asteroids[i].lx, asteroids[i].ly, asteroids[i].lz);
            explosionSound();
            resetMovingAsteroid(i, 0);
        }
    }        

    if (weapons.rEnable >= 201)
        weapons.rEnable = 0;
    else
        weapons.rEnable++;
}

// Updates the cooldown status of all the abilities
// and the energy value
void updateAbilities() {
    if (weapons.energy < 100)
        weapons.energy += passiveEnergy;

    if (weapons.qCD != 0)
        --weapons.qCD;
    if(weapons.qEnable != 0)
        --weapons.qEnable;

    if (weapons.wCD != 0)
        --weapons.wCD;
    if (weapons.wEnable != 0)
        --weapons.wEnable;

    if (weapons.eCD != 0)
        --weapons.eCD;
    if (weapons.eEnable != 0)
        --weapons.eEnable;

    if (weapons.rCD != 0)
        --weapons.rCD;
}

// Manages the behaviour of an attempted cast of an
// ability with respect to current cooldowns and
// energy levels
void useAbility(char key) {
    switch (key) {
        case 'q':
            if (!weapons.qEnable&& weapons.energy >= 20 && weapons.qCD == 0) {
                weapons.energy -= 20;
                weapons.qEnable = 1;
                weapons.qCD = 3 * 60;
                hullIntegrity = (hullIntegrity) + ((hullIntegrity <= 90) ? (10) : (100 - hullIntegrity));
            }
            break;
        case 'w':
            if (!weapons.wEnable && weapons.energy >= 30 && weapons.wCD == 0) {
                weapons.energy -= 30;
                weapons.wEnable = 10 * 60;
                weapons.wCD = 15 * 60;
            }
            break;
        case 'e':
            if (!weapons.eEnable && weapons.energy >= 50 && weapons.eCD == 0) {
                weapons.energy -= 50;
                weapons.eEnable = 1;
                weapons.eCD = 30 * 60;
                velocity /= 2;
                switch (difficulty) {
                case easy:
                    if(velocity <= 0.2)
                        velocity = 0.2;
                    break;
                case normal:
                    if (velocity <= 0.3)
                        velocity = 0.3;
                    break;
                case hard:
                    if (velocity <= 0.5)
                        velocity = 0.5;
                    break;
                case insane:
                    if (velocity <= 0.7)
                        velocity = 0.7;
                    break;
                default:
                    break;
                }
            }
            break;
        case 'r':
            if (!weapons.eEnable && weapons.energy >= 75 && weapons.rCD == 0) {
                weapons.energy -= 75;
                weapons.rEnable = 5;
                weapons.rCD = 30 * 60;
            }
            break;
    }
}

// Passive function that gets the position of the cursor
// on the screen in pixel coordinates
void cursorPosition(int x, int y) {
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble object_x, object_y, object_z;
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    winX = (float) x;
    winY = (float)viewport[3] - (float) y - 1.0f;
    glReadBuffer(GL_BACK);
    glReadPixels(x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
    gluUnProject((GLdouble)winX, (GLdouble)winY, (GLdouble)winZ, modelview, projection, viewport, &object_x, &object_y, &object_z);
    if (object_z < -5) {
        mouseX = object_x;
        mouseY = object_y;
        mouseZ = object_z;
    }
    //fprintf(stdout, "%d, %d, %f, %f, %f\n", x, y, mouseX, mouseY, mouseZ);
}

// Draws the cannons pointing towards the current 3D point
// over which the cursor is hovering, obtaining the rotation
// vector and angle from getCannonAimDir()
void drawCannons() {
    getCannonAimDir();

    for (int i = 0; i < 2; ++i) {
        glPushMatrix();
        glTranslatef((i == 0) ? (-7) : (7), -1.5, -5);
        glRotatef(cannons[i].enable, cannons[i].ax, cannons[i].ay, cannons[i].az);
        glTranslatef(0, 0, cannons[i].tz);
        glScalef(1.0, 1.0, 2.0);
        cannon->recursive_render(cannon->getRootNode(), 1.0);
        glPopMatrix();
        if(cannons[i].tz > 0)
            cannons[i].tz -= 0.1;
    }
}

// Computes the angle between the rotation vector (0,0,-1) of the cannons
// and the vector pointing towards the current point in 3D space over which
// the mouse is hovering
void getCannonAimDir() {
    Vector3 res;
    float norm;

    for (int i = 0; i < 2; ++i) {
        res.x = mouseX - cannons[i].lx;
        res.y = mouseY - cannons[i].ly;
        res.z = mouseZ - cannons[i].lz;
        norm = sqrtf(powf(res.x, 2) + powf(res.y, 2) + powf(res.z, 2));
        cannons[i].ax = res.x / norm;
        cannons[i].ay = res.y / norm;
        cannons[i].az = res.z / norm;
        cannons[i].enable = acosf(-cannons[i].az) * (180.0f / 3.14f);
        cannons[i].ax = res.y / norm;   
        cannons[i].ay = - res.x / norm;
        cannons[i].az = 0;
    }
}

// Resizes the window according to the user's input
void resizeWindow(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(60.0, 1.0, 0.0001, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

// Checks the player name and plays a soundtrack accordingly
void checkPlayerName() {
    if (playerName == "Gandalf")
        sound.Play(BRIDGEKHAZADDUM);
    else if (playerName == "DarthVader")
        sound.Play(IMPERIALMARCH);
    else if (playerName == "Indy")
        sound.Play(INDYTHEME);
    else if (playerName == "2B" || playerName == "A2" || playerName == "9S")
        sound.Play(BIPOLARNIGHTMARE);
    else
        sound.Play(SPECIALORDER939);
}

// Updates the global spin value for the asteroids
void updateSpin() {
    spin = (int) (spin + 1.0) % 36000;
}

// Plays a random audio clip with the sound effect of the blaster of a Tie-Fighter
void laserSound() {
    static int i = 0;
    switch (i) {
        case 0:
            sound.Play(LASER1);
            break;
        case 1:
            sound.Play(LASER2);
            break;
        case 2:
            sound.Play(LASER3);
            break;
        default:
            break;
    }
    i = (i + 1) % 3;
}

// Plays a random audio clip with the sound effect of an explosion
void explosionSound() {
    static int i = 0;
    switch (i) {
    case 0:
        sound.Play(EXP1);
        break;
    case 1:
        sound.Play(EXP2);
        break;
    case 2:
        sound.Play(EXP3);
        break;
    default:
        break;
    }
    i = (i + 1) % 3;
}

// Loads the formatted leaderboard file of the current difficulty level into memory
void readLeaderboard() {
    std::fstream filestream;
    std::string filename = "Leaderboards/" + difficultyName + "Leaderboard.txt";
    std::string line;

    filestream.open(filename, std::ios::in);

    if (filestream.is_open())
        fprintf(stdout, "Loading new leaderboard file!\n");
    else
        fprintf(stdout, "Error whilst opening file!\n");

    int i = 0;
    while (i < 10 && getline(filestream, line)) {
        std::vector <std::string> tokens;
        std::istringstream ss(line);
        std::string intermediate;

        while (getline(ss, intermediate, ' '))
            tokens.push_back(intermediate);

        leaderboard[i][0].clear();
        leaderboard[i][0].replace(0, tokens[0].length(), tokens[0]);
        leaderboard[i][1].clear();
        leaderboard[i][1].replace(0, tokens[1].length(), tokens[1]);
        leaderboard[i][2].clear();
        leaderboard[i][2].replace(0, tokens[2].length(), tokens[2]);

        i++;
    }
    filestream.close();
}

// Updates the leaderboard file of the current difficulty with the game's score,
// then reloads the leaderboard in memory
void updateLeaderboard() {
    std::fstream leaderboard;
    std::string line[10];
    std::string oldLine;
    std::string filename = "Leaderboards/" + difficultyName + "Leaderboard.txt";
    int insert = 0, translate = 0;

    leaderboard.open(filename, std::ios::in);

    if (leaderboard.is_open())
        fprintf(stdout, "Opened file!\n");
    else
        fprintf(stdout, "Error whilst opening the file!\n");

    int i = 0;
    while (i < 10 && getline(leaderboard, line[i])) {
        std::vector <std::string> tokens;
        std::istringstream ss(line[i]);
        std::string intermediate;
        
        while (getline(ss, intermediate, ' '))
            tokens.push_back(intermediate);
        
        if (killedAsteroids > stoi(tokens[2]) && insert == 0 && translate == 0)
            insert = 1;
        if (insert) {
            std::string tmp = std::to_string(i + 2) + " " + tokens[1] + " " + tokens[2];
            oldLine.clear();
            oldLine.replace(0, tmp.length(), tmp);
            std::string newLine = std::to_string(i + 1) + " " + playerName + " " + std::to_string(killedAsteroids);
            line[i].clear();
            line[i].replace(0, newLine.length(), newLine);
            translate = 1;
            insert = 0;
        }
        else if (translate) {
            std::string tmp = std::to_string(i + 2) + " " + tokens[1] + " " + tokens[2];
            line[i].clear();
            line[i].replace(0, oldLine.length(), oldLine);
            oldLine.clear();
            oldLine.replace(0, tmp.length(), tmp);
        }
        i++;
    }
    leaderboard.close();

    leaderboard.open(filename, std::ios::out | std::ios::trunc);
    for (int i = 0; i < 10; i++) {
        std::cout << line[i] << "\n";
        leaderboard << line[i] << std::endl;
    }
    leaderboard.close();

    readLeaderboard();
}

// Draws the current leaderboard stored in memory as a 10 by 3 table
void drawLeaderboard() {
    for (int i = 0; i < 10; i++) {
        writeText(-0.8, 0.9 - (i * 0.25), leaderboard[i][0], 0);
        writeText(0.0, 0.9 - (i * 0.25), leaderboard[i][1], 1);
        writeText(0.7, 0.9 - (i * 0.25), leaderboard[i][2], 0);
    }
}

// Returns the bitmap width of a string
float bitmapCenterX(std::string text) {
    float width = 0.0;
    int textLen = text.size();
    for (unsigned int i = 0; i < textLen; i++)
        width += glutBitmapWidth(GLUT_BITMAP_9_BY_15, text[i]);
    return width / (glutGet(GLUT_WINDOW_WIDTH)/2.8  );
}

// Switches the difficulty global variable and the difficultyName
void difficultySwitch() {
    switch (difficulty) {
    case easy:
        difficulty = normal;
        difficultyName = "Normal";
        break;
    case normal:
        difficulty = hard;
        difficultyName = "Hard";
        break;
    case hard:
        difficulty = insane;
        difficultyName = "Insane";
        break;
    case insane:
        difficulty = easy;
        difficultyName = "Easy";
        break;
    default:
        break;
    }
}

// Writes text onto screen with float precision positioning. 
// Features an option to center text orizontally with respect to
// the coordinates provided as argument
void writeText(float x, float y, std::string str, int centered)
{
    int len, i;

    if(centered)
        glRasterPos3f(x-bitmapCenterX(str), y, 0.0);
    else
        glRasterPos3f(x, y, 0.0);

    len = str.length();

    glPushMatrix();
    for (i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);

    glPopMatrix();

    glutPostRedisplay();
}

// Initializes all the variables needed to start a new game.
// Some variables are only initialized once per program execution,
// while others depend on the Difficulty setting
void initGameVariables(int firstinit) {
    nextlaser = 0;
    nextExplosion = 0;
    killedAsteroids = 0;
    framesDamage = 0;
    hullIntegrity = 100.0;
    spin = 45.0;

    weapons.energy = 0.0;
    weapons.qEnable = 0,
    weapons.qCD = 0;
    weapons.wEnable = 0,
    weapons.wCD = 0;
    weapons.eEnable = 0,
    weapons.eCD = 0;
    weapons.rEnable = 0,
    weapons.rCD = 0;

    switch (difficulty) {
    case easy:
        velocity = 0.2;
        asteroidRampUpSpeed = 0.01;
        asteroidInitialSpacing = 80;
        asteroidSpawnDistance = -250;
        passiveEnergy = 0.5;
        energyOnKill = 4;
        break;
    case normal:
        velocity = 0.3;
        asteroidRampUpSpeed = 0.01;
        asteroidInitialSpacing = 50;
        asteroidSpawnDistance = -250;
        passiveEnergy = 0.2;
        energyOnKill = 2;
        break;
    case hard:
        velocity = 0.5;
        asteroidRampUpSpeed = 0.02;
        asteroidInitialSpacing = 35;
        asteroidSpawnDistance = -200;
        passiveEnergy = 0.1;
        energyOnKill = 1;
        break;
    case insane:
        velocity = 0.8;
        asteroidRampUpSpeed = 0.04;
        asteroidInitialSpacing = 25;
        asteroidSpawnDistance = -150;
        passiveEnergy = 0.05;
        energyOnKill = 2;
        break;
    default:
        break;
    }
    
    if (firstinit) {
        for (int i = 0; i < 2; ++i) {
            cannons[i].lx = (i == 0) ? (7) : (-7);
            cannons[i].ly = -1.5;
            cannons[i].lz = -5;
        }

        laserColor.x = 0.317647058;
        laserColor.y = 0.980392156;
        laserColor.z = 0.098039215;
        colorName = "Light Green";
        difficultyName = "Easy";
    }

    readLeaderboard();

    for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i) {
        resetExplosions(i);
    }

    for (int i = 0; i < MAX_NUMBER_LASERS; ++i) {
        resetMovingLaser(i);
    }

    for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i) {
        asteroids[i].tx = 0;
        asteroids[i].ty = 0;
        asteroids[i].tz = 0;
        resetMovingAsteroid(i, 1);
        asteroids[i].enable = 1;
        /*
        fprintf(stdout, "lx %f, ly %f, lz %f,\nax %f, ay %f, az %f,\ntx %f, ty %f, tz %f,\nenable %d\n",
            asteroids[i].lx, asteroids[i].ly, asteroids[i].lz,
            asteroids[i].ax, asteroids[i].ay, asteroids[i].az,
            asteroids[i].tx, asteroids[i].ty, asteroids[i].tz,
            asteroids[i].enable);
        */
    }
}

// Switches the laserColor values between various combinations
void colorSwitch() {
    if (activeColor == red)
    {
        laserColor.x = 0.968627450;
        laserColor.y = 0.537254901;
        laserColor.z = 0.007843137;
        activeColor = orange;
        colorName = "Orange";
    }
    else if (activeColor == orange)
    {
        laserColor.x = 1.0;
        laserColor.y = 1.0;
        laserColor.z = 0.109803921;
        activeColor = yellow;
        colorName = "Yellow";
    }
    else if (activeColor == yellow)
    {
        laserColor.x = 0.317647058;
        laserColor.y = 0.980392156;
        laserColor.z = 0.098039215;
        activeColor = lightgreen;
        colorName = "Light Green";
    }
    else if (activeColor == lightgreen)
    {
        laserColor.x = 0.0;
        laserColor.y = 0.388235294;
        laserColor.z = 0.019607843;
        activeColor = darkgreen;
        colorName = "Dark Green";
    }
    else if (activeColor == darkgreen)
    {
        laserColor.x = 0.0;
        laserColor.y = 1.0;
        laserColor.z = 1.0;
        activeColor = cyan;
        colorName = "Cyan";
    }
    else if (activeColor == cyan)
    {
        laserColor.x = 0.0;
        laserColor.y = 0.101960784;
        laserColor.z = 0.878431372;
        activeColor = blue;
        colorName = "Blue";
    }
    else if (activeColor == blue)
    {
        laserColor.x = 0.921568627;
        laserColor.y = 0.0;
        laserColor.z = 0.921568627;
        activeColor = magenta;
        colorName = "Magenta";
    }
    else if (activeColor == magenta)
    {
        laserColor.x = 0.729411764;
        laserColor.y = 0.0;
        laserColor.z = 0.498039215;
        activeColor = purple;
        colorName = "Purple";
    }
    else if (activeColor == purple)
    {
        laserColor.x = 1.0;
        laserColor.y = 0.039215686;
        laserColor.z = 0.039215686;
        activeColor = red;
        colorName = "Red";
    }
}

// Draws a point with an emissive material outside of the viewport
// to fix a light popping bug with the emissive material of lasers
void fixEmissive() {
    glPushMatrix();
    glColor3f(laserColor.x, laserColor.y, laserColor.z);
    glColorMaterial(GL_FRONT, GL_AMBIENT);
    glColorMaterial(GL_FRONT, GL_DIFFUSE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    glBegin(GL_POINT);
    glVertex3f(-10.0, 0.0, 0.0);
    glEnd();
    glPopMatrix();
}

// Sets the values and enables a given explosion from the explosions array
void initializeExplosion(float x, float y, float z) {
    explosions[nextExplosion].x = x;
    explosions[nextExplosion].y = y;
    explosions[nextExplosion].z = z;
    explosions[nextExplosion].enable = 1;
    nextExplosion = (nextExplosion + 1) % MAX_NUMBER_ASTEROIDS;
}

// Draws the explosion upon asteroid collision with a laser
void drawExplosion(int i) {
    float x = explosions[i].x;
    float y = explosions[i].y;
    float z = explosions[i].z;
    float depth = explosions[i].depth;
    glPushMatrix();
        glColor3f(0.8, 0.8, 0.8);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glTranslatef(x, y, z); // Inner Sphere
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS/2, 32, 32);
        glColor3f(1.0, 1.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32); // Middle Sphere
        glColor3f(1.0, 0.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glTranslatef(0.0, depth, 0.0); // North
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef((ONE_OVER_SQRT_TWO * depth), -((1 - ONE_OVER_SQRT_TWO) * depth), 0.0); // North-East
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(((1 - ONE_OVER_SQRT_TWO) * depth), -(ONE_OVER_SQRT_TWO * depth), 0.0); // East
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(-((1 - ONE_OVER_SQRT_TWO) * depth), -(ONE_OVER_SQRT_TWO * depth), 0.0); // South-East
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(-(ONE_OVER_SQRT_TWO * depth), -((1 - ONE_OVER_SQRT_TWO) * depth), 0.0); // South
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(-(ONE_OVER_SQRT_TWO * depth), ((1 - ONE_OVER_SQRT_TWO) * depth), 0.0); // South-West
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(-((1 - ONE_OVER_SQRT_TWO) * depth), (ONE_OVER_SQRT_TWO * depth), 0.0); // West
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
        glTranslatef(((1 - ONE_OVER_SQRT_TWO) * depth), (ONE_OVER_SQRT_TWO * depth), 0.0); // North-West
        glutSolidSphere(EXPLOSION_SPHERE_RADIUS, 32, 32);
    glPopMatrix();
    if (depth <= 4)
        explosions[i].depth += 0.2;
    else
        resetExplosions(i);
}

// Resets all values of an element of the explosions array
void resetExplosions(int i) {
    explosions[i].x = 0;
    explosions[i].y = 0;
    explosions[i].z = 0;
    explosions[i].depth = 0;
    explosions[i].enable = 0;
}

// Draws the shaded gradient rectangle at the borders of the screen,
// displaying damage to the hull of the spaceship as a non-diegetic feedback
void drawDamageFeedback() {
    glPushMatrix();
        glColor3f(0.0, 0.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_POLYGON);
            glVertex3f(-3, -3, 0);
            glVertex3f(3, -3, 0);
            glColor3f(1.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(2.6, -2.5, 0);
            glVertex3f(-2.6, -2.5, 0);
        glEnd();
        glBegin(GL_POLYGON);
            glColor3f(0.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-3, 3, 0);
            glVertex3f(3, 3, 0);
            glColor3f(1.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(2.6, 2.5, 0);
            glVertex3f(-2.6, 2.5, 0);
        glEnd();
        glBegin(GL_POLYGON);
            glColor3f(0.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-3, 3, 0);
            glVertex3f(-3, -3, 0);
            glColor3f(1.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(-2.6, -2.5, 0);
            glVertex3f(-2.6, 2.5, 0); 
        glEnd();
        glBegin(GL_POLYGON);
            glColor3f(0.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(3, 3, 0);
            glVertex3f(3, -3, 0);
            glColor3f(1.0, 0.0, 0.0);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
            glVertex3f(2.6, -2.5, 0);
            glVertex3f(2.6, 2.5, 0);
        glEnd();
    glPopMatrix();
}

// Draws the lines that compose the outer square, middle horizontal
// line and title rectangle in the main, pause and death menus
void drawMenuLines(int death) { 
    glPushMatrix();
        glColor3f(0.0, 0.0, 0.0);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glBegin(GL_POLYGON);
            glVertex3f(-4, -4, -1.5);
            glVertex3f(4, -4, -1.5);
            glVertex3f(4, 4, -1.5);
            glVertex3f(-4, 4, -1.5);
        glEnd();
        glLineWidth(50);
        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_LINE_LOOP);
            glVertex3f(-2.87, -2.85, 0);
            glVertex3f(2.87, -2.85, 0);
            glVertex3f(2.87, 2.85, 0);
            glVertex3f(-2.87, 2.85, 0);
        glEnd();
        glLineWidth(4);
        glBegin(GL_LINES);
            glVertex3f(-1, 2.25, -1);
            glVertex3f(1, 2.25, -1);
            glVertex3f(-1, 1.5, -1);
            glVertex3f(1, 1.5, -1);
            glVertex3f(-1, 2.25, -1);
            glVertex3f(-1, 1.5, -1);
            glVertex3f(1, 2.25, -1);
            glVertex3f(1, 1.5, -1);
            if (!death) {
                glVertex3f(-1.5, -1.2, -1);
                glVertex3f(1.5, -1.2, -1);
            }
        glEnd();
    glPopMatrix();
}

// Updates the position of the asteroid according to its direction
// towards the player with a given velocity each frame
void updateAsteroid(int i) {
    asteroids[i].lx += velocity * asteroids[i].ax;
    asteroids[i].ly += velocity * asteroids[i].ay;
    asteroids[i].lz += velocity * asteroids[i].az;
}

// Computes random spawn values for an asteroid and calls the
// getAsteroidAimDir() function on that asteroid
void resetMovingAsteroid(int i, int firstinit) {
    int prob = (std::rand() % 100);
    if (prob < 50)
        asteroids[i].hp = 1;
    else if (prob < 80)
        asteroids[i].hp = 2;
    else
        asteroids[i].hp = 3;
    asteroids[i].lx = (std::rand() % 180) - 90;
    asteroids[i].ly = (std::rand() % 120) - 60;
    if(firstinit)
        asteroids[i].lz = -150 - (i * asteroidInitialSpacing);
    else
        asteroids[i].lz = (std::rand() % 100) - 400;
    getAsteroidAimDir(i);
}

// Draws an asteroid according to its world coordinates and
// then updates its position for the next frame
void drawMovingAsteroid(int i)
{
    glEnable(GL_COLOR_MATERIAL);

    glPushMatrix();
        //glLineWidth(1);
        //glColor3f(0.4, 0.368627450, 0.329411764);
        //glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
        //glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
        //glColorMaterial(GL_FRONT_AND_BACK, GL_SHININESS);
        glTranslatef(asteroids[i].lx, asteroids[i].ly, asteroids[i].lz);
        switch (asteroids[i].hp) {
        case 1:
            glRotatef(45, 0, 1, 0);
            glRotatef(45, 1, 0, 0);
            glRotatef(spin, 0, 0, -1);
            glColor3f(0.4, 0.368627450, 0.329411764);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glScalef(2.9, 2.9, 2.9);
            //glutSolidDodecahedron();
            meteorite->recursive_render(meteorite->getRootNode(), 1.0);
            break;
        case 2:
            glRotatef(45, 0, 1, 0);
            glRotatef(45, 1, 0, 0);
            glRotatef(spin * 1.5, -1, 0, -1);
            glColor3f(0.65, 0.368627450, 0.329411764);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glScalef(2.9, 2.9, 2.9);
            //glutSolidDodecahedron();
            meteorite->recursive_render(meteorite->getRootNode(), 1.0);
            break;
        case 3:
            glRotatef(45, 0, 1, 0);
            glRotatef(45, 1, 0, 0);
            glRotatef(spin * 2, 0, -2, -1);
            glColor3f(0.9, 0.368627450, 0.329411764);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glScalef(2.9, 2.9, 2.9);
            //glutSolidDodecahedron();
            meteorite->recursive_render(meteorite->getRootNode(), 1.0);
            break;
        default:
            break;
        }
    glPopMatrix();

    updateAsteroid(i);
}

// Updates the position of the laser according to its direction towards
// the point selected with get3Dpos() with a costant velocity each frame
void updateLaser(int i) {
    lasers[i].lx += 1 * lasers[i].ax;
    lasers[i].ly += 1 * lasers[i].ay;
    lasers[i].lz += 1 * lasers[i].az;
}

// Resets the spawn point of a laser onto the spaceship and
// disables it from being rendered
void resetMovingLaser(int i){
    lasers[i].enable = 0;
    lasers[i].lx = (i % 2 == 0) ? (7) : (-7);
    lasers[i].ly = -1.5;
    lasers[i].lz = -5;
    lasers[i].ax = 0;
    lasers[i].ay = 0;
    lasers[i].az = 0;
    lasers[i].tx = 0;
    lasers[i].ty = 0;
    lasers[i].tz = 0;
}

// Draws a laser according to its world coordinates
void drawMovingLaser(int i)
{
    //fprintf(stdout, "%f, %f, %f, %f, %f, %f\n", lasers[i].lx, lasers[i].ly, lasers[i].lz, lasers[i].ax, lasers[i].ay, lasers[i].az);
    glLineWidth(5);
    glPushMatrix();
        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        glBegin(GL_LINES);
        glVertex3f(lasers[i].lx, lasers[i].ly, lasers[i].lz);
        glVertex3f(lasers[i].lx + 15 * lasers[i].ax, lasers[i].ly + 15 * lasers[i].ay, lasers[i].lz + 15 * lasers[i].az);
        glEnd();
        if (lasers[i].hp == 1) {
            float angle = acosf(-lasers[i].az) * (180.0f / 3.14f);
            glPushMatrix();
                glTranslatef(lasers[i].lx + 12 * lasers[i].ax, lasers[i].ly + 12 * lasers[i].ay, lasers[i].lz + 12 * lasers[i].az);
                glRotatef(angle, lasers[i].ay, -lasers[i].ax, 0);
                glutSolidTorus(0.4, 0.5, 20, 20);
            glPopMatrix();
            glPushMatrix();
                glTranslatef(lasers[i].lx + 8 * lasers[i].ax, lasers[i].ly + 8 * lasers[i].ay, lasers[i].lz + 8 * lasers[i].az);
                glRotatef(angle, lasers[i].ay, -lasers[i].ax, 0);
                glutSolidTorus(0.4, 0.5, 20, 20);
            glPopMatrix();
            glPushMatrix();
                glTranslatef(lasers[i].lx + 4 * lasers[i].ax, lasers[i].ly + 4 * lasers[i].ay, lasers[i].lz + 4 * lasers[i].az);
                glRotatef(angle, lasers[i].ay, -lasers[i].ax, 0);
                glutSolidTorus(0.4, 0.5, 20, 20);
            glPopMatrix();
        }
    glPopMatrix();
}

// Checks if a laser has collided with any of the active asteroids
void checkLaserCollision(int i) {
    for (int j = 0; j < MAX_NUMBER_ASTEROIDS; ++j) {
        if (lasers[i].lx < asteroids[j].lx + SPHERE_RADIUS && lasers[i].lx > asteroids[j].lx - SPHERE_RADIUS && lasers[i].ly < asteroids[j].ly + SPHERE_RADIUS && lasers[i].ly > asteroids[j].ly - SPHERE_RADIUS && lasers[i].lz < asteroids[j].lz + 2 * SPHERE_RADIUS && lasers[i].lz > asteroids[j].lz - 3 *SPHERE_RADIUS) {
            fprintf(stdout, "Collision detected at:\nx %f, y %f, z %f\n", asteroids[i].lx, asteroids[i].ly, asteroids[i].lz);
            if (weapons.wEnable) {
                asteroids[j].hp = 0;
            }
            else {
                asteroids[j].hp--;
                resetMovingLaser(i);
            }
            initializeExplosion(asteroids[j].lx, asteroids[j].ly, asteroids[j].lz);
            explosionSound();
            if (asteroids[j].hp <= 0) {
                resetMovingAsteroid(j, 0);
                killedAsteroids++;
                weapons.energy += energyOnKill;
                if (weapons.energy > 100)
                    weapons.energy = 100.000;
                velocity += asteroidRampUpSpeed;
                fprintf(stdout, "Asteroid destroyed!\n");
            }
            else {
                fprintf(stdout, "Asteroid damaged!\n");
            } 
        }
    }
}

// Computes the direction vector of the next available laser from its spawn world coordinates
// towards the target world coordinates obtained in 3D space with get3Dpos()
void getLaserAimDir() {
    Vector3 res;
    float norm;

    res.x = lasers[nextlaser].tx - lasers[nextlaser].lx;
    res.y = lasers[nextlaser].ty - lasers[nextlaser].ly;
    res.z = lasers[nextlaser].tz - lasers[nextlaser].lz;
    norm = sqrtf(powf(res.x, 2) + powf(res.y, 2) + powf(res.z, 2));
    lasers[nextlaser].ax = res.x / norm;
    lasers[nextlaser].ay = res.y / norm;
    lasers[nextlaser].az = res.z / norm;
    
    lasers[nextlaser].lx += 7 * lasers[nextlaser].ax;
    lasers[nextlaser].ly += 7 * lasers[nextlaser].ay;
    lasers[nextlaser].lz += 7 * lasers[nextlaser].az;
}

// Computes the direction vector of an asteroid from its spawn world coordinates
// towards the player
void getAsteroidAimDir(int i) {
    Vector3 res;
    float norm;
    res.x = asteroids[i].tx - asteroids[i].lx;
    res.y = asteroids[i].ty - asteroids[i].ly;
    res.z = asteroids[i].tz - asteroids[i].lz;
    norm = sqrtf(powf(res.x, 2) + powf(res.y, 2) + powf(res.z, 2));
    asteroids[i].ax = res.x / norm;
    asteroids[i].ay = res.y / norm;
    asteroids[i].az = res.z / norm;
}

// If not yet initialized, it creates the scene list
void generateSceneLists() {
    if (scene_list == 0) {
        scene_list = glGenLists(1);
        glNewList(scene_list, GL_COMPILE);
        glPushMatrix();
        spaceshipInterior->recursive_render(spaceshipInterior->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        glNewList(scene_list + 1, GL_COMPILE);
        glPushMatrix();
        spaceBackground->recursive_render(spaceBackground->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        glNewList(scene_list + 2, GL_COMPILE);
        glPushMatrix();
        writeplayernameBackground->recursive_render(writeplayernameBackground->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        glNewList(scene_list + 3, GL_COMPILE);
        glPushMatrix();
        mainmenuBackground->recursive_render(mainmenuBackground->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        glNewList(scene_list + 4, GL_COMPILE);
        glPushMatrix();
        meteorite->recursive_render(meteorite->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        glNewList(scene_list + 5, GL_COMPILE);
        glPushMatrix();
        cannon->recursive_render(cannon->getRootNode(), 1.0);
        glPopMatrix();
        glEndList();

        lists[0] = 0;
        lists[1] = 1;
        lists[2] = 2;
        lists[3] = 3;
        lists[4] = 4;
        lists[5] = 5;
        glListBase(scene_list);

        glCallLists(6, GL_UNSIGNED_BYTE, lists);
    }

    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
}

// Draws the scene according to the GameState value, is called once for each frame
void updateScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0);

    //fixEmissive();

    generateSceneLists();
    
    if (GameState == test) {

    } 
    else if (GameState == mainmenu) 
    {
        // Draw the main menu
        glPushMatrix();
        glTranslatef(0.0, 1.0, -58.0);
        mainmenuBackground->recursive_render(mainmenuBackground->getRootNode(), 1.0);
        glPopMatrix();

        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);

        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        writeText(0, 0.75, "Press S to start a new game!", 1);
        writeText(0, 0.5, "During the game, press P to pause.", 1);
        std::string difficulty_str = "Press D to change the difficulty setting, currently set to " + difficultyName + ".";
        writeText(0, 0.25, difficulty_str, 1);
        std::string leaderboard_str = "Press L to check the " + difficultyName + " difficulty's leaderboard.";
        writeText(0, 0.0, leaderboard_str, 1);
        std::string color_str = "Press C to change laser and menus' color, currently set to " + colorName + ".";
        writeText(0, -0.25, color_str, 1);
        writeText(0, -0.5, "Press W to change the player's name.", 1);
        writeText(0, -0.75, "Press ESC anytime to close the application." ,1);
        writeText(0, -1.4, "GROUP 5", 1);
        writeText(0, -1.65, "Riccardo Brian Bergoef - Angela D'Antonio - Marzio Vallero", 1);
        writeText(0, -1.9, "Polytechnic University of Turin", 1);
        writeText(0, -2.15, "02BHIOV - Informatica Grafica - Academic Year 2020/2021", 1);
        writeText(0, -2.4, "Course's Main Lecturer: Professor F. Lamberti - Course's Assistant Lecturer: A. Cannavo'", 1);
        //drawMenuLines(0);
    }
    else if (GameState == pause) 
    {
        // Draw the pause menu
        glPushMatrix();
        glTranslatef(0.0, 1.0, -58.0);
        mainmenuBackground->recursive_render(mainmenuBackground->getRootNode(), 1.0);
        glPopMatrix();

        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);

        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        writeText(0, 0, "PAUSE", 1);
        writeText(0, -1.9, "Press P to resume or M to go back to the main menu.", 1);
        //drawMenuLines(0);
    }
    else if (GameState == game) 
    {   
        // Check the hullIntegrity for fatal damage
        if (hullIntegrity <= 0.0) {
            for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i)
                asteroids[i].enable = 0;
            hullIntegrity = 0.0;
            GameState = death;
            glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
            glDisable(GL_LIGHT1);
            updateLeaderboard();
        }

        updateSpin();
        updateAbilities();
        
        glPushMatrix();
            glTranslatef(0.0, -4, 5.0);
            spaceshipInterior->recursive_render(spaceshipInterior->getRootNode(), 1.0);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.0, 0.0, -450.0);
            spaceBackground->recursive_render(spaceBackground->getRootNode(), 3);
        glPopMatrix();

        drawCannons();

        // Calls the functions that manage the asteroids in the scene
        for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i) {
            if (asteroids[i].lz >= -15 && asteroids[i].enable) {
                resetMovingAsteroid(i, 0);
                hullIntegrity -= velocity * 40;
                framesDamage = 15; // A quarter of a second at 60 fps
            }
            else if (asteroids[i].enable) {
                drawMovingAsteroid(i);
            }
        }

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_COLOR_MATERIAL);

        if (weapons.rEnable) {
            drawExpandingShield();
        }

        // Calls the drawDamageFeedback() for the 15 frames subsequent
        // to an asteroid colliding with the spaceship
        if (framesDamage > 0) {
            drawDamageFeedback();
            framesDamage--;
        }

        // Calls the function that manages the explosions in the scene
        for (int i = 0; i < MAX_NUMBER_ASTEROIDS; ++i) {
            if (explosions[i].enable) {
                drawExplosion(i);
            }
        }

        // Calls the functions that manage the lasers in the scene
        for (int i = 0; i < MAX_NUMBER_LASERS; ++i) {
            if (lasers[i].lz <= -800 && lasers[i].enable)
                resetMovingLaser(i);
            else if (lasers[i].enable) {
                drawMovingLaser(i);
                updateLaser(i);
                // Asteroids only spawn with world-Z values between [-400, -300],
                // so after Z=-400 it makes no sense to check for collisisons
                if (lasers[i].lz >= -400 && lasers[i].lz < 0)
                    checkLaserCollision(i);
            }
        }

        // Calls the functions that draws the UI
        drawUI();
    }
    else if (GameState == death) {
        //Draw the death menu and game statistics
        glPushMatrix();
        glTranslatef(0.0, 1.0, -58.0);
        mainmenuBackground->recursive_render(mainmenuBackground->getRootNode(), 1.0);
        glPopMatrix();

        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);

        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        writeText(0, 1.6, "GAME OVER", 1);
        std::string text_row_3 = "You destroyed " + std::to_string(killedAsteroids) + " asteroids!";
        writeText(0, 1.3, text_row_3, 1);
        drawLeaderboard();
        writeText(0, -1.9, "Press S to start a new game or M to go back to the main menu!", 1);
        //drawMenuLines(1);
    }
    else if (GameState == leaderboards) {
        //Draw the leaderboards statistics
        glPushMatrix();
        glTranslatef(0.0, 1.0, -58.0);
        mainmenuBackground->recursive_render(mainmenuBackground->getRootNode(), 1.0);
        glPopMatrix();

        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);

        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);

        writeText(0, 1.6, "LEADERBOARDS", 1);
        std::string text_row_3 = difficultyName + " difficulty";
        writeText(0, 1.35, text_row_3, 1);
        drawLeaderboard();
        writeText(0, -1.9, "Press D to switch difficulty or M to go back to the main menu!", 1);
        //drawMenuLines(1);
    }
    else if (GameState == writeplayername) {
        //Draw the insert player name menu
        glPushMatrix();
            glTranslatef(-10.0, 0.0, -12.0);
            writeplayernameBackground->recursive_render(writeplayernameBackground->getRootNode(), 1.05);
        glPopMatrix();

        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);

        glColor3f(laserColor.x, laserColor.y, laserColor.z);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
        writeText(0, 0.25, "Write your nickname:", 1);
        writeText(0, -0.5, playerName, 1);
        writeText(0, -1.5, "Press Delete to remove characters.", 1);
        writeText(0, -1.75, "Press Enter to confirm.", 1);
        //drawMenuLines(1);
    }
    
    glutSwapBuffers();
    glutPostRedisplay();
}

// Enables anti-aliasing, enables depth test, initializes and enables lighting,
// enables the use of glColorMaterial(), sets projection matrix and aspect ratio of the viewport
void glInitialize(int w, int h)
{
    
    if (!spaceshipInterior->LoadGLTextures() || !spaceBackground->LoadGLTextures() || !writeplayernameBackground->LoadGLTextures() || !mainmenuBackground->LoadGLTextures() || !meteorite->LoadGLTextures() || !cannon->LoadGLTextures())
    {
        fprintf(stderr, "Error loading gl textures.\n");
        return;
    }

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);

    glEnable(GL_COLOR_MATERIAL);

    glClearColor(1.0, 1.0, 1.0, 0.0);
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.0001, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

// Manages mouse clicks during the game 
void mouseClick(int button, int state, int x, int y)
{
    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN) && (GameState == game) && (!weapons.rEnable))
    {
        if (Get3Dpos(x, y))
            return;
        printf("Fire button %d pressed at %d %d\n", button, x, y);
        laserSound();
        getLaserAimDir();
        /*
        fprintf(stdout, "nextlaser %d,\nlx %f, ly %f, lz %f,\nax %f, ay %f, az %f,\ntx %f, ty %f, tz %f,\nenable %d\n",
            nextlaser,
            lasers[nextlaser].lx, lasers[nextlaser].ly, lasers[nextlaser].lz,
            lasers[nextlaser].ax, lasers[nextlaser].ay, lasers[nextlaser].az,
            lasers[nextlaser].tx, lasers[nextlaser].ty, lasers[nextlaser].tz,
            lasers[nextlaser].enable);
        */
        cannons[(nextlaser + 1) % 2].tz = 1.5;
        lasers[nextlaser].enable = 1;
        if (weapons.wEnable)
            lasers[nextlaser].hp = 1;
        else
            lasers[nextlaser].hp = 0;
        nextlaser = (nextlaser + 1) % MAX_NUMBER_LASERS; 
    }
}

// Manages key presses according to the GameState
void keyboardPress(unsigned char key, int x, int y) {
    switch (GameState) {
        case mainmenu:
            switch (key) {
                case 's':
                    GameState = game;
                    fprintf(stdout, "velocity %f, asteroidRampUpSpeed %f, asteroidInitialSpacing %d, asteroidSpawnDistance %f\n", velocity, asteroidRampUpSpeed, asteroidInitialSpacing, asteroidSpawnDistance);
                    sound.Play(ALIENBEEP);
                    glEnable(GL_LIGHT1);
                    checkPlayerName();
                    glutSetCursor(GLUT_CURSOR_CROSSHAIR);
                    break;
                case 'c':
                    colorSwitch();
                    sound.Play(ALIENWRITE);
                    break;
                case 'd':
                    difficultySwitch();
                    initGameVariables(0);
                    sound.Play(ALIENWRITE);
                    break;
                case 'l':
                    GameState = leaderboards;
                    sound.Play(ALIENBEEP);
                    break;
                case 'w':
                    GameState = writeplayername;
                    sound.Play(ALIENBEEP);
                    glEnable(GL_LIGHT1);
                    break;
                case 27:  /*  Escape key  */
                    exit(0);
                    break;
                case 6:  /*   Ctrl + F    */
                    (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                    (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                    fs = !fs;
                    break;
                default:
                    break;
            }
            break;
        case pause:
            switch (key) {
            case 'p':
                GameState = game;
                sound.Play(ALIENBEEP);
                glEnable(GL_LIGHT1);
                break;
            case 'm':
                initGameVariables(0);
                GameState = mainmenu;
                sound.Play(ALIENBEEP);
                sound.Play(MOTHERCONSOLE);
                break;
            case 27:  /*  Escape key  */
                exit(0);
                break;
            case 6:  /*   Ctrl + F    */
                (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                fs = !fs;
                break;
            default:
                break;
            }
            break;
        case game:
            switch (key) {
            case 'p':
                GameState = pause;
                sound.Play(ALIENBEEP);
                glDisable(GL_LIGHT1);
                glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
                break;
            case 27:  /*  Escape key  */
                exit(0);
                break;
            case 6:  /*   Ctrl + F    */
                (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                fs = !fs;
                break;
            default:
                useAbility((char) key);
                break;
            }
            break;
        case death:
            switch (key) {
            case 's':
                initGameVariables(0);
                glEnable(GL_LIGHT1);
                GameState = game;
                sound.Play(ALIENBEEP);
                break;
            case 'm':
                initGameVariables(0);
                GameState = mainmenu;
                sound.Play(MOTHERCONSOLE);
                break;
            case 27:  /*  Escape key  */
                exit(0);
                break;
            case 6:  /*   Ctrl + F    */
                (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                fs = !fs;
                break;
            default:
                break;
            }
            break;
        case leaderboards:
            switch (key) {
            case 'm':
                initGameVariables(0);
                GameState = mainmenu;
                sound.Play(ALIENBEEP);
                break;
            case 'd':
                difficultySwitch();
                initGameVariables(0);
                sound.Play(ALIENWRITE);
                break;
            case 27:  /*  Escape key  */
                exit(0);
                break;
            case 6:  /*   Ctrl + F    */
                (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                fs = !fs;
                break;
            default:
                break;
            }
            break;
        case writeplayername:
            switch (key) {
            case 8:  /*  Delete key   */
                if(playerName.length() != 0)
                    playerName.erase(playerName.end() - 1);
                sound.Play(ALIENWRITE);
                break;
            case 13: /*  Enter key    */
                if (playerName.length() == 0)
                    playerName = "Player1";
                sound.Play(ALIENBEEP);
                initGameVariables(0);
                glDisable(GL_LIGHT1);
                GameState = mainmenu;
                break;
            case 27:  /*  Escape key  */
                exit(0);
                break;
            case 32:  /*  Spacebar    */
                break;
            case 6:  /*   Ctrl + F    */
                (!fs) ? (glutFullScreen()) : (glutReshapeWindow(SW, SH));
                (!fs) ? (glutFullScreen()) : (glutPositionWindow(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2));
                fs = !fs;
                break;
            default:
                playerName += (char)key;
                sound.Play(ALIENWRITE);
                break;
            }
            break;
        default:
            break;
    }
}

// The main function, calls the glutMainLoop()
int main(int argc, char** argv)
{
    struct aiLogStream stream;

    initGameVariables(1);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(SW, SH);
    glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 2 - SW / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - SH / 2);
    glutCreateWindow("Asteroid Shooter");
    glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
    glutWarpPointer(SW / 2, SH / 2);
    glutDisplayFunc(updateScene);
    glutReshapeFunc(resizeWindow);
    glutIdleFunc(updateScene);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(cursorPosition);
    glutKeyboardFunc(keyboardPress);

    // FMOD related
    sound.Load();
    sound.Play(MOTHERCONSOLE);
    
    // AssImp and DevIL related
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
    aiAttachLogStream(&stream);
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
    aiAttachLogStream(&stream);
    spaceshipInterior = new Scene(pathscene_1.c_str());
    spaceBackground = new Scene(pathscene_2.c_str());
    writeplayernameBackground = new Scene(pathscene_3.c_str());
    mainmenuBackground = new Scene(pathscene_4.c_str());
    meteorite = new Scene(pathscene_5.c_str());
    cannon = new Scene(pathscene_6.c_str());

    glInitialize(SW, SH);
    glutMainLoop();
    
    // Free memory
    spaceshipInterior->~Scene();
    spaceBackground->~Scene();
    writeplayernameBackground->~Scene();
    mainmenuBackground->~Scene();
    meteorite->~Scene();
    cannon->~Scene();
    aiDetachAllLogStreams();

    return 0;
}
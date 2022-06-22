
#pragma once



#define ENGINE
#define MODEL_SKINS //Expensive.

#define COLLLISION_OUTPUTX //This is for outputting static collisions asap. Disable it to do so at the end of frame. Note that this is out of date and may 'cause issues as cout was noted to 'cause eccessive lag. 

#define DEATHS

#define PARTITION //Removing this is not supported.
#define LESS_PRECISE_COLLISIONS //Removing this is not supported.

#define NO_OUTPUT_BUT_FPS //Removes everything but fps counter.
#define FILE_ONLY_OUTPUTg // will instead output the FPS into Log.txt, closing at end of FPS_REMAINDER. Works with NO_OUTPUT_BUT_FPS

#define INVINCIBLE_DYNAMICd
#define INVINCIBLE_STATICd

#define STATIC_COLLISIONS//Note: There seems to be a bug where a set of values are not the same size despite being in the same loop when one of these is toggled. I cannot see why this would be the case.
#define DYNAMIC_COLLISIONS //I blame the lack of a pool allocator. Or multithreading. Probably multithreading. Disabling this pauses the threads twice but it was taking excessive time to debug.


#define NO_STATIC_SPHERESd
#define NO_DYNAMIC_SPHERESd //Seems buggy. Not something that's really needed so it hasn't been looked into. 

#define THREE_DIMENSIONdS //Change first word. TWO and THREE are the only designed key words.

#define CAMERA_MOVEMENT

constexpr int START_THREAD = 0; //This is for the deconstructor removing threads.

constexpr int STRING_RESERVE_SIZE = 100000; //It will write into a file once this size is reached then clear the string. 
constexpr int FRAME_REMAINDER = 50; //If Filereading is active, it will close after writing to the file.

constexpr int MAX_HEALTH = 100;
constexpr int HEALTH_HIT_REDUCTION = 20;

constexpr int SPHERE_COUNT = 50000;

constexpr float SPHERE_RESERVATION_MOD = .5f;
constexpr int SPHERE_HALFPOINT = SPHERE_COUNT / 2;


//I kept changing between 0.05 and 5 to test debug mode and release mode respectively. This just made it easier to tranasition. 
constexpr float MAX_VELOCITY =5.05f;
constexpr float MIN_VELOCITY = -5.05f;

constexpr float MODEL_RADIUS_SCALING = 0.09f;
constexpr float MAX_RADIUS = 30.f;
constexpr float MIN_RADIUS = 15.0f;
constexpr float MAX_COMBINED_RADIUS = MAX_RADIUS * 2.0f;

constexpr float MIN_POSITION = -2000;
constexpr float MAX_POSITION = 2000;

constexpr float WALL_MAX_POSITION = MAX_POSITION * 1.05f;

//I hoped this would work. It doesn't. Please don't break it. 
#if MIN_POSITION > 0
constexpr float MAX_ABS_POSITION = MIN_POSITION + MAX_POSITION;
#else
constexpr float MAX_ABS_POSITION = -MIN_POSITION + MAX_POSITION;
#endif

constexpr float HALF_MAX_ABS_POSITION = MAX_ABS_POSITION / 2;

constexpr int EXPECTED_SPHERES_PER_PARTITION = 5;//1;
constexpr int PARTITION_ARRAY_SIZE =  ((MAX_ABS_POSITION / (MAX_RADIUS * 1.15f)) / EXPECTED_SPHERES_PER_PARTITION) + 1; //Can set custom but I've automated it to be max for max radius, mediated by the determined spheres per partition..

constexpr float CAMERA_SPEED = 500.0f;

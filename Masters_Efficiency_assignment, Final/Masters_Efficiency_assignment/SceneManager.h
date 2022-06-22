#pragma once




#include "GlobalConstants.h"
#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <cmath>
#include <thread>
#include <condition_variable>
#include <vector>

#include <iostream>
#include <fstream>
#include <list>
//Time
#include <time.h>
#ifndef ENGINE
#include <chrono>
#include <ctime>

using getFps = std::chrono::duration<double, std::ratio<1, 24>>;
#endif

using namespace tle;


//struct Vector3
//{
//	Vector3(int xx = 0, int yy = 0, int zz = 0)
//	{
//		x = xx;
//		y = yy;
//		z = zz;
//	};
//
//	inline void operator*=(int input)
//	{
//		x *= input;
//		y *= input;
//		z *= input;
//	}
//
//	inline void operator+=(Vector3 input)
//	{
//		x += input.x;
//		y += input.y;
//		z += input.z;
//	}
//
//	float x;
//	float y;
//	float z;
//};

template<typename val>
struct arr4
{
	val value[4];
};

template<typename val>
struct arr
{
	val value;
};

struct float3Index
{
	float Position[3];
	int IndexRef;
};




struct SphereCopy
{
	float Position[3];
	int index;
};

struct threadData
{
	std::thread thread;
	int isFrameDone = 1;

	std::mutex mutexLock;
	std::condition_variable workReady;
};

#ifdef StructTest
struct MobileSphereData : StaticSphereData
{
	MobileSphereData()
	{
		Name.reserve(NameCharLimit);
	}

	Vector3 Position;
	float Radius;

	Vector3 Colour;
	float Health;

	Vector3 Velocity;
	std::vector<char> Name; //Allocating NameCharLimit * 8, in bytes. Original: 8x8=64
};

struct StaticSphereData
{
	StaticSphereData()
	{
		Name.reserve(NameCharLimit);
	}

	int Health; //Could be short if 2 bytes are freed elsewhere.
	Vector3 Position;

	Vector3 Colour;
	float Radius;

	std::vector<char> Name;
};
#endif



template<typename dimensions>
class SceneManager
{
private:

	//I didn't have access to the lab while writing this and did a bit of research to double check my memory and the powerpoint.
	//Didn't notice any extra calls being made with this so I designed my approach on using static_cast.
	//Could make this a friend function called by a constructor but it isn't a noted requirement. 


	//Note, only needed to allow for code reusage. Nothing noted on optimizing memory usage. Knowing this I can just use float[3]'s and use these templates for specific dimensional calculations.
	inline void AddRandomizedPosition(float min, float max)
	{
		return static_cast<dimensions*>(this)->AddRandomizedPosition(min, max);
	}

	inline auto AddVelocity(int index)
	{
		return static_cast<dimensions*>(this)->AddVelocity(index);
	}

public:

	SceneManager()
	{

		Start();
		MainLoop();
	}

	void MainLoop();


	~SceneManager() {

#ifdef ENGINE
		EngineReference->Delete();
#endif
		for (uint32_t i = START_THREAD; i < mNumWorkers; ++i)
		{
			mSphereThreads[i].thread.detach();
		}

#if defined(FILE_ONLY_OUTPUT) || defined(FILE_OUTPUT)

		file << cmdOutput[MAX_WORKERS];
		cmdOutput[MAX_WORKERS].clear();
		file.close();
#endif
	}


protected:
	void Start();

	void ThreadBase(int threadID, int min, int max);
	inline float random(float min, float max);

	template<typename Val>
	void swap (Val* A, Val* B)
	{
		Val C = *A;
		*A = *B;
		*B = C;
	};

#if  defined(FILE_ONLY_OUTPUT) || defined(FILE_OUTPUT)
	fstream file;
#endif

	// A pool of worker threads
	static const uint32_t MAX_WORKERS = 31;
	threadData mSphereThreads[MAX_WORKERS];
	uint32_t mNumWorkers;  // Actual number of worker threads being used in array above

	std::string cmdOutput[MAX_WORKERS+1];

	

	//Timing and visual output
	
#ifndef ENGINE

	long long Frametime;
	long long timeSinceStart = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
	long long timeSinceEpoch;
#else
	vector<int> DeadSpheresThisFrame[MAX_WORKERS];
	std::vector<arr<float[3]>> DeadSpheres[MAX_WORKERS];
	float fps;
	float Frametime;

	
#endif
	unsigned int frameCount;
	IFont* Font;

	//Engine setup
	I3DEngine* EngineReference;
	ICamera* myCamera;
	IMesh* SphereMesh;


	// Sphere 'struct'

#ifdef ENGINE
	IModel* SphereModel[SPHERE_COUNT];

#endif

#if StructTest
	ItemList<MobileSphereData> MobileSpheres;
	ItemList<StaticSphereData> StaticSpheres;
#else


	std::vector<arr<float[3]>> sphPosition;
	std::vector<arr<float[3]>> sphVelocity;
	std::vector<arr<float[3]>> sphColour;
	std::vector<int> Health;
	std::vector<std::string> Name; 
	std::vector<float> Radius;
	//Effectively creating a pool with arrays.

	//Name and radius are both used when collisions occur but due to the size of SPHERE this is hard to optimize with only 2 dimensions.

	/*
if [2][SPHERE_COUNT] memory would be like:

{{SPHERE_COUNT}{SPHERE_COUNT}}

if [3][2][SPHERE_COUNT] memory would be like:

{{ {SPHERE}, {SPHERE} }, { {SPHERE}, {SPHERE} } { {SPHERE}, {SPHERE} }}

This would give me as many X (The [3] is a float3) as possible with the cache given.
Considering how Threads share memory, I want as much detail from the static spheres in one cycle as possible. Since threads should, in theory,
be within the same 'zone' of influence, assuming they have calculated everything at a similar speed.

If this was [2][3][SPHERECOUNT] it would effectively have the same result as 'SPHERE' is always going to be above 1000.

{{{SPHERE}, {SPHERE}, {SPHERE}} {{SPHERE}, {SPHERE}, {SPHERE}}}

[SPHERECOUNT][2][3] would be shown as:

{{ {3} {3} } ...} (Repeats)

Which, assuming the memory is grabbed through iteration, would get both the dynamic sphere and static spheres data,
allowing for faster collisions but grabs a lot of useless data if we're only sticking to a single array.
*/






	//(((Y_POSITION+HALF_MAXY) / MAXY) * ARRAY_SIZE) * 2.0f
	//(((-966+1000) / 2000) * 40) - .5
	//The size of the grid and array are controlled and thus can be calculated allowing for easy prediction. 
	//Using my knowledge of CPU->GPU I have assumed that writing data will be far faster than directly reading it, if I can organize
	//It in an interative manner. In theory a float[3] and an int will make 16 bytes, which typically matches the TL_Engine offset. 

	//Storing the index and position will allow me to calculate any changes at a reasonable pace and write it in realtime. 

	//Originally attempted as an array but 2,000,000 * 20 * 20 * 20 is too big but I don't care since this assignment only cares for speed.
	std::vector<arr4<int>> StaticIDStorage;
	arr4<int > StaticIDInput;
#ifdef PARTITION
	// {{},{},{}}
	// {{},{},{}}
	// {{},{},{}}

	std::vector<arr4<float>> StaticPosition[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
	std::vector<float> StaticRadius[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
	std::vector<arr4<float>> StaticColour[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
	std::vector<int> StaticHealth[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
	std::vector<std::string> StaticID[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];

#ifdef DYNAMIC_COLLISIONS

	//This is extremely slow but I'm running out of memory and needed to make space. It might be slow but it's faster than it'd be if you didn't have enough memory. 
	std::list<float3Index> DynamicPosition[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][MAX_WORKERS];
#endif

#ifdef ENGINE

	std::vector<int> StaticModel[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
	std::vector<int> DynamicModel[PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE][PARTITION_ARRAY_SIZE];
#endif

#endif




#endif


};

//Noted that 2d is required. Nothing really needs to change for accomodate this beyond initial values being set.
class TwoDimensions : SceneManager<TwoDimensions>
{
public:

	//Perhaps this only counts as a "Partial" attempt but 2D is just a simplified form of 3D. No values need to be changed as padding would, for the most part,
	//ruin that aspect unless they were part of a single array.
	//The spatial partitioning also doesn't need to be changed as, if you look in "AddRandomizedPosition" I've attempted to make it allocate memory one step at a time, albeit in a highly inefficient manner
	//Which is not particularly a problem as it does it in the 'setup'
	inline void AddRandomizedPosition(float min, float max)
	{
		arr<float[3]> DynamicInput;
		arr4<float> StaticInput;

		DynamicInput.value[0] = (MIN_POSITION + MAX_POSITION) * 0.5f;
		DynamicInput.value[1] = random(MIN_POSITION, MAX_POSITION);
		DynamicInput.value[2] = random(MIN_POSITION, MAX_POSITION);
		sphPosition.push_back(DynamicInput);


		//Set up position value
		StaticInput.value[0] = 0;
		StaticInput.value[1] = random(MIN_POSITION, MAX_POSITION);
		StaticInput.value[2] = random(MIN_POSITION, MAX_POSITION);

		//Calculate where it is axis-wise and translate that to array-space.
		StaticIDInput.value[0] = (int)((((StaticInput.value[0] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of X position
		StaticIDInput.value[1] = (int)((((StaticInput.value[1] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of Y position
		StaticIDInput.value[2] = (int)((((StaticInput.value[2] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of Z position
		StaticIDInput.value[3] = -1;

		//Could happen> Prepare.
		for (int l = 0; l < 3; ++l)
			if (StaticIDInput.value[l] >= PARTITION_ARRAY_SIZE)
				StaticIDInput.value[l] = PARTITION_ARRAY_SIZE - 1;
			else if (StaticIDInput.value[l] < 0)
				StaticIDInput.value[l] = 0;

		StaticIDInput.value[3] = StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].size(); //Get index of position

		//I'm curious to see if this doesn't allocate extra memory. If it takes longer this is fine since it's the startup but if I can optimize the static memory usage, that'd be better.
		StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].size() + 1);
		StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].emplace_back(StaticInput);
		StaticIDStorage.push_back(StaticIDInput);

		return;
	}

	inline auto AddVelocity(int index)
	{
		arr <float[3]> input;
		input.value[0] = 0;
		input.value[1] = random(MIN_VELOCITY * 100.0f, MAX_VELOCITY * 100.0f) * 0.01f;
		input.value[2] = random(MIN_VELOCITY * 100.0f, MAX_VELOCITY * 100.0f) * 0.01f;

		sphVelocity.push_back(input);
	}

};


//Noted that 2d is required. Nothing really needs to change for accomodate this beyond initial values being set.
class ThreeDimensions : SceneManager<ThreeDimensions>
{
public:


	inline void AddRandomizedPosition(float min, float max)
	{
		arr<float[3]> DynamicInput;
		arr4<float> StaticInput;

		DynamicInput.value[0] = random(MIN_POSITION, MAX_POSITION);
		DynamicInput.value[1] = random(MIN_POSITION, MAX_POSITION);
		DynamicInput.value[2] = random(MIN_POSITION, MAX_POSITION);
		sphPosition.push_back(DynamicInput);


		//Set up position value
		StaticInput.value[0] = random(MIN_POSITION, MAX_POSITION);
		StaticInput.value[1] = random(MIN_POSITION, MAX_POSITION);
		StaticInput.value[2] = random(MIN_POSITION, MAX_POSITION);

		//Calculate where it is axis-wise and translate that to array-space.
		StaticIDInput.value[0] = (int)((((StaticInput.value[0] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of X position
		StaticIDInput.value[1] = (int)((((StaticInput.value[1] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of Y position
		StaticIDInput.value[2] = (int)((((StaticInput.value[2] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f); //Get index of Z position
		StaticIDInput.value[3] = -1;

		//Could happen> Prepare.
		for (int l = 0; l < 3; ++l)
			if (StaticIDInput.value[l] >= PARTITION_ARRAY_SIZE)
				StaticIDInput.value[l] = PARTITION_ARRAY_SIZE - 1;
			else if (StaticIDInput.value[l] < 0)
				StaticIDInput.value[l] = 0;

		StaticIDInput.value[3] = StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].size(); //Get index of position

		//I'm curious to see if this doesn't allocate extra memory, I'm not fully sure how an uncontrolled vector would manage this.
		//If it takes longer this is fine since it's the startup but if I can optimize the static memory usage, that'd be better.
		int sizeReserve = StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].size() + 1;


		StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(sizeReserve);
		StaticRadius[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(sizeReserve);
		StaticColour[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(sizeReserve);
		StaticHealth[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(sizeReserve);
		    StaticID[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].reserve(sizeReserve);

		StaticPosition[StaticIDInput.value[0]][StaticIDInput.value[1]][StaticIDInput.value[2]].emplace_back(StaticInput);
		StaticIDStorage.push_back(StaticIDInput);

		return;
	}

	inline auto AddVelocity(int index)
	{
		arr <float[3]> input;
		input.value[0] = random(MIN_VELOCITY * 100.0f, MAX_VELOCITY * 100.0f) * 0.01f;
		input.value[1] = random(MIN_VELOCITY * 100.0f, MAX_VELOCITY * 100.0f) * 0.01f;
		input.value[2] = random(MIN_VELOCITY * 100.0f, MAX_VELOCITY * 100.0f) * 0.01f;

		sphVelocity.push_back(input);
	}

};


#include "SceneManager.h"
#include <random>
#ifdef ENGINE
static bool hasTouched[SPHERE_HALFPOINT];
#endif

template<typename dimensions>
inline void SceneManager<dimensions>::Start(){
#ifdef ENGINE
	EngineReference = New3DEngine(kTLX);;
	EngineReference->StartWindowed();
	EngineReference->AddMediaFolder("C:\\ProgramData\\TL-Engine\\Media");
	Font = EngineReference->LoadFont("Font1.bmp", 50);
	EngineReference->SetAmbientLight(0, 0, 0);
	SphereMesh = EngineReference->LoadMesh("Sphere.x");

	//SpatialParititionVect.reserve(SPHERE_COUNT);

	if (SphereMesh == nullptr)
	{
		EngineReference->Delete();
		return;
	}
#endif




	std::cout << "Start:\n";

	sphVelocity.reserve(SPHERE_HALFPOINT);
	sphPosition.reserve(SPHERE_HALFPOINT);
	sphColour.reserve(SPHERE_COUNT);
	Health.reserve(SPHERE_COUNT);
	Name.reserve(SPHERE_COUNT); //Name just has to be ID. Does not ened tro be strinnnng.
	Radius.reserve(SPHERE_COUNT);



	std::cout << "\n:Sphere Start, Time used for setup: ";

#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << Frametime << '\n';
#endif


#if	defined(FILE_ONLY_OUTPUT) || defined(FILE_OUTPUT)
	file.open("Log.txt", std::ios_base::out);

	if (file.fail())
		return;

	file.clear();
#endif
	//StaticSpatialPartition



	arr4<int> StaticIDInput;
	arr<float[3]> sphInput;
	arr4<float> StaticInput;

	cmdOutput[MAX_WORKERS].reserve(STRING_RESERVE_SIZE);
	
	for (int i = 0; i < PARTITION_ARRAY_SIZE; ++i)
	{
		for (int j = 0; j < PARTITION_ARRAY_SIZE; ++j)
		{
			for (int k = 0; k < PARTITION_ARRAY_SIZE; ++k)
			{
				//Originally added 60% to the estimated spheres per partition but most where not used, in a lot of cases. And, since this is static, I'm assuming this is the most memory efficient method./
				StaticPosition[i][j][k].reserve(0);
				StaticRadius[i][j][k].reserve(0);
				StaticColour[i][j][k].reserve(0);
				StaticHealth[i][j][k].reserve(0);
				StaticID[i][j][k].reserve(0);




#ifdef ENGINE
				StaticModel[i][j][k].reserve(EXPECTED_SPHERES_PER_PARTITION * SPHERE_RESERVATION_MOD);
				DynamicModel[i][j][k].reserve(EXPECTED_SPHERES_PER_PARTITION * SPHERE_RESERVATION_MOD);
#endif
			}
		}

	}

	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{
		AddRandomizedPosition(MIN_POSITION,MAX_POSITION);
	}

#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << "Positions done, time: " << Frametime << '\n';
#endif


	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{
		Radius.push_back(std::abs(random(MIN_RADIUS, MAX_RADIUS)));

		StaticRadius[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]].push_back(std::abs(random(MIN_RADIUS, MAX_RADIUS)));



	}



#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << "Radius done, time: " << Frametime << '\n';
#endif

	arr4<float> VectorInput;
	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{
		arr<float[3]> input;
		input.value[0] = std::abs(random(0, 100.0f) * 0.01f);
		input.value[1] = std::abs(random(0, 100.0f) * 0.01f);
		input.value[2] = std::abs(random(0, 100.0f) * 0.01f);
		//Seems to be room for error. Safety net. 

		sphColour.push_back(input);

		VectorInput.value[0] = std::abs(random(0, 100.0f) * 0.01f);
		VectorInput.value[1] = std::abs(random(0, 100.0f) * 0.01f);
		VectorInput.value[2] = std::abs(random(0, 100.0f) * 0.01f);
		StaticColour[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]].push_back(VectorInput);
	}

#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << "Colour done, time: " << Frametime << '\n';
#endif


	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{

		int input[4];
		input[0] = StaticIDStorage[i].value[0];
		input[1] = StaticIDStorage[i].value[1];
		input[2] = StaticIDStorage[i].value[2];
		input[3] = StaticIDStorage[i].value[3];

		StaticHealth[input[0]][input[1]][input[2]].emplace_back(MAX_HEALTH);
		StaticID[input[0]][input[1]][input[2]].emplace_back('S' + std::to_string(i));
		Health.push_back(MAX_HEALTH);
		Name.push_back('D' + std::to_string(i));

		if (StaticID[input[0]][input[1]][input[2]].size() != StaticHealth[input[0]][input[1]][input[2]].size())
			throw std::abort; //There's a bug.

	}

	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{

	}

#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << "Name and Health done, time: " << Frametime << '\n';
#endif

#ifdef ENGINE
	int modelCount = sphPosition.size();
	for (int i = 0; i < sphPosition.size(); ++i)
	{

		while (SphereModel[i] == nullptr)
			SphereModel[i] = SphereMesh->CreateModel(sphPosition[i].value[0], sphPosition[i].value[1], sphPosition[i].value[2]);

		SphereModel[i]->Scale(Radius[i] * MODEL_RADIUS_SCALING);

	}



	for (int i = 0; i < StaticIDStorage.size(); ++i)
	{
		while (SphereModel[modelCount] == nullptr)
			SphereModel[modelCount] = SphereMesh->CreateModel
			(
				StaticPosition[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]][StaticIDStorage[i].value[3]].value[0],
				StaticPosition[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]][StaticIDStorage[i].value[3]].value[1],
				StaticPosition[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]][StaticIDStorage[i].value[3]].value[2]
			);

		SphereModel[modelCount]->Scale(StaticRadius[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]][StaticIDStorage[i].value[3]] * MODEL_RADIUS_SCALING);

		StaticModel[StaticIDStorage[i].value[0]][StaticIDStorage[i].value[1]][StaticIDStorage[i].value[2]].push_back(modelCount);

		++modelCount;
	}

	Frametime = EngineReference->Timer();
	cout << "Model spawning done, time: " << Frametime << '\n';

#ifdef MODEL_SKINS

	modelCount = 0;
	for (int j = 0; j < 2; ++j)
	{
		if (j < 50)
		{
			SphereModel[j]->SetSkin("tiles1.jpg");
		}
		else
			SphereModel[j]->SetSkin("CueTip.jpg");
	}
	Frametime = EngineReference->Timer();
	cout << "Model skins done, time: " << Frametime << '\n';

#endif

#endif


	for (int i = 0; i < SPHERE_HALFPOINT; ++i)
	{
		AddVelocity(i);
	
	}

#ifdef ENGINE
	Frametime = EngineReference->Timer();
	cout << "Velocity done, time: " << Frametime << '\n';
#endif
	std::cout << "Spheres_Spawned\n";

	std::cout << "\n:LineSweep start:\n";


#ifdef ENGINE
	myCamera = EngineReference->CreateCamera(kManual, MIN_POSITION - abs(MAX_POSITION * 1.5f), MIN_POSITION - abs(MAX_POSITION * 1.5f), 0);

	myCamera->LookAt(SphereModel[1]->GetLocalX(), SphereModel[1]->GetLocalY(), SphereModel[1]->GetLocalZ());
#endif

#ifndef SINGLE_THREAD
	// Start worker threads
	mNumWorkers = std::thread::hardware_concurrency(); // Gives a hint about level of thread concurrency supported by system (0 means no hint given)
	if (mNumWorkers == 0)  mNumWorkers = 8;
	--mNumWorkers; // Decrease by one because this main thread is already running
	int SphereDivision = SPHERE_HALFPOINT / mNumWorkers;

	for (uint32_t i = START_THREAD; i < mNumWorkers; ++i)
	{
		//Max would be SPHERE_COUNT but that's very memory intensive. 
#ifdef ENGINE
		DeadSpheres[i].reserve(SphereDivision * 1.5f);

#endif





#ifdef DEBUG
		if (i == mNumWorkers - 1)
			cout << "\n\n\nActual Thread: " + to_string(i) + ": " + to_string(SphereDivision * i) + ", " + to_string(SphereDivision * (i + 1) + (SPHERE_HALFPOINT - SphereDivision * (i + 1))) + "\n\n\n";
		else
			cout << "\n\n\nActual Thread: " + to_string(i) + ": " + to_string(SphereDivision * i) + ", " + to_string(SphereDivision * (i + 1)) + "\n\n\n";
#endif

		//Not every value adds up correctly. Could have up to mNumWorkers-1 spare spheres.
		if (i == mNumWorkers - 1) {
			mSphereThreads[i].thread =
				std::thread(&SceneManager::ThreadBase, this, i, SphereDivision * i, SphereDivision * (i + 1) + (SPHERE_HALFPOINT - SphereDivision * (i + 1)));
		}
		else {
			mSphereThreads[i].thread = std::thread(&SceneManager::ThreadBase, this, i, SphereDivision * i, SphereDivision * (i + 1));
		}
	}

	//distance(0.0f, 10.0f);
#else
	mNumWorkers = 1;
	mBlockSpritesWorkers[0].thread =
		std::thread(&SceneManager::ThreadBase, this, 0, 0, SPHERE_HALFPOINT);
#endif

	std::cout << "\n\n:Simulation setup, FINISHED: \n\n";
}

template<typename dimensions>

inline void SceneManager<dimensions>::ThreadBase(int threadID, static int min, static int max)
{

	int radX, radY, radZ, finalXYZ, i, bmax, loop;
	float Velocity[3], RadSum;
	std::string CollOutput;
	arr<float[4]> pushVal;



#ifndef OVER_PRECISE_COLLISIONS
	int ID[3];
#else
	int ID[3];
#endif
	while (1)
	{
		//Collision end 
		{
			std::unique_lock<std::mutex> lock(mSphereThreads[threadID].mutexLock);

			//Doesn't seem to like it when I just put in the boolean so I threw it in a lambda 'cause lambda's are great. Would've put the thread creation in Lambda's too but it screwed up the inputs, 
			//since it created the thread and then called the function with a reference to the current values, which would've updated at that point. Could've removed the & but it was pointless.
			while (mSphereThreads[threadID].isFrameDone != 0)
				mSphereThreads[threadID].workReady.wait(lock); //Once this is equal to 0, progress.
		}

#ifdef DYNAMIC_COLLISIONS
		for (int j = 0; j < PARTITION_ARRAY_SIZE; ++j)
			for (int k = 0; k < PARTITION_ARRAY_SIZE; ++k)
				for (int l = 0; l < PARTITION_ARRAY_SIZE; ++l)
					DynamicPosition[j][k][l][threadID].clear();
#endif



		for (i = min; i < max; ++i)
		{

#ifndef INVINCIBLE_DYNAMIC
			if (Health[i] > 0)
#endif
			{
				for (loop = 0; loop < 3; ++loop) {
					Velocity[loop] = sphVelocity[i].value[loop];
					if (std::abs(sphPosition[i].value[loop] + Velocity[loop]) > WALL_MAX_POSITION)
					{
						sphVelocity[i].value[loop] *= -1.0f;
						Velocity[loop] = sphVelocity[i].value[loop];
					}
				}

				sphPosition[i].value[0] += Velocity[0];
				sphPosition[i].value[1] += Velocity[1];
				sphPosition[i].value[2] += Velocity[2];

				pushVal.value[0] = sphPosition[i].value[0];
				pushVal.value[1] = sphPosition[i].value[1];
				pushVal.value[2] = sphPosition[i].value[2];



				ID[0] = std::max(0.0f, std::min((((sphPosition[i].value[0] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f, PARTITION_ARRAY_SIZE - 1.0f));
				ID[1] = std::max(0.0f, std::min((((sphPosition[i].value[1] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f, PARTITION_ARRAY_SIZE - 1.0f));
				ID[2] = std::max(0.0f, std::min((((sphPosition[i].value[2] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f, PARTITION_ARRAY_SIZE - 1.0f));
				pushVal.value[3] = i;

#ifdef DYNAMIC_COLLISIONS

				int inputID[3];


				inputID[0] = (int)std::round(ID[0]);
				inputID[1] = (int)std::round(ID[1]);
				inputID[2] = (int)std::round(ID[2]);

				float3Index listInput;

				
				listInput.Position[0] = sphPosition[i].value[0];
				listInput.Position[1] = sphPosition[i].value[1];
				listInput.Position[2] = sphPosition[i].value[2];
				listInput.IndexRef = i;
				DynamicPosition[inputID[0]][inputID[1]][inputID[2]][threadID].push_back(listInput);


#endif




#ifdef ENGINE
				SphereModel[i]->SetPosition(pushVal.value[0], pushVal.value[1], pushVal.value[2]);

				if (!hasTouched[i])
				{
					hasTouched[i] = true;
				}
#endif
			}


#ifdef STATIC_COLLISIONS
			auto CheckStaticCollisionAt = [&](int x, int y, int z)
			{

#ifndef INVINCIBLE_DYNAMIC
				if (Health[i] <= 0)
					return;
#endif
				if (x < 0 || x >= PARTITION_ARRAY_SIZE || y < 0 || y >= PARTITION_ARRAY_SIZE || z < 0 || z >= PARTITION_ARRAY_SIZE)
					return;


				auto end = StaticPosition[x][y][z].size();
				if (end > 0) {

					int l = 0;

					//These have all been set up exactly the same way. There shouldn't be any possible mistakes unless effected by corrupted or something. I've noted that it can have issues though I can't detect the source.
					auto start = StaticPosition[x][y][z][l];
					auto radius = StaticRadius[x][y][z][l];

					//These two are in the same loop with the exact same values. I don't see any way of them not working. 
					auto health = &StaticHealth[x][y][z][l];
					auto name = StaticID[x][y][z][l];




					for (; l < end; ++l) //If it's less than 0 then it's a static collision. 
					{



						health = &StaticHealth[x][y][z][l];

#ifdef DEATHS
						/*
						"Death" doesn't specify removal from program. Just ignore them. Slower but much more stable.
						I have no idea how I'd do deaths safetly unless I had a copied Vertex to output the spheres
						with a health >0, which sounds expensive.

						I could easily remove the spheres but then that can potentially collide with other threads
						and, again, unless I completely separate their data and make the end result from this, there's
						no real way of avoiding any interaction, as "Popping back" a vector would effect every single
						ID within that vectors scope.
						*/
#ifndef INVINCIBLE_STATIC
						if (*health > 0)
#endif

#endif
						{

							start = StaticPosition[x][y][z][l];
							radius = StaticRadius[x][y][z][l];
							





							{

								//Caluclate squared distance. Doesn't matter if another dimension is lost/added.
								radX = -sphPosition[i].value[0];
								radY = -sphPosition[i].value[1];
								radZ = -sphPosition[i].value[2];

								radX += start.value[0];
								radY += start.value[1];
								radZ += start.value[2];


								finalXYZ = (radX * radX) + (radY * radY) + (radZ * radZ);

								//Collision if less than squared sum of radii
								RadSum = radius + Radius[i];

								if (finalXYZ < RadSum * RadSum)
								{
									name = StaticID[x][y][z][l];

									for (loop = 0; loop < 3; ++loop)
										sphVelocity[i].value[loop] *= -1.0f;

									Health[i] -= HEALTH_HIT_REDUCTION;

									*health -= HEALTH_HIT_REDUCTION;

#ifdef ENGINE
#ifndef INVINCIBLE_STATIC
					
									if (*health < 1)
									{

										/*In theory, to get the static spheres you can calculate : y + (x * PARTITION_ARRAY_SIZE) + (z * (PARTITION_ARRAY_SIZE * PARTITION_ARRAY_SIZE)) + l
										* to get where in the grid they would be. Problem with that is I'm using Vectors and checking every size before that point would be expensive.										 */
										DeadSpheresThisFrame[threadID].push_back(StaticModel[x][y][z][l]);
									}
#endif
#ifndef INVINCIBLE_DYNAMIC
									if (Health[i] < 1)
									{
										DeadSpheresThisFrame[threadID].push_back(i);
									}
#endif

#endif

#ifdef COLLLISION_OUTPUT
									std::cout << " | Collision true, Spheres: " << Name[i] << " & " << std::to_string(name) << ", distance:" << finalXYZ << " | ";
#else
#ifndef ENGINE


									if (cmdOutput[threadID].size() > STRING_RESERVE_SIZE * 0.65f)
									{
										int stringInt = 0;
										while (cmdOutput[stringInt].size() > 0) ++stringInt;

										if (stringInt < MAX_WORKERS) {
											cmdOutput[stringInt] = cmdOutput[threadID];
											cmdOutput[threadID].clear();
										}
									}


									CollOutput +=
#ifndef NO_OUTPUT_BUT_FPS
										"\nFrame," + std::to_string(frameCount) +
										" Time, " + std::to_string((timeSinceStart - Frametime) * 0.001f) +
										", DSph" + Name[i] + 
										'(' + std::to_string(Health[i]) + ") & SSph" + 
										name + ",(" + std::to_string(health) + 
										") dist:" + std::to_string(finalXYZ) + "\n";
#else
										"";
#endif
									if (cmdOutput[threadID].size() < STRING_RESERVE_SIZE * 0.6f)
									{
										cmdOutput[threadID] += CollOutput;

									}
									else
									{
#ifndef FILE_ONLY_OUTPUT
										std::cout << "String full. Early release.\n" << cmdOutput[threadID] << '\n';
										std::cout << CollOutput;
#else
										file << "String full. Early release.\n" << cmdOutput[threadID] << '\n';
										file << CollOutput;
#endif
										cmdOutput[threadID].clear();
									}


#else

									if (cmdOutput[threadID].size() < STRING_RESERVE_SIZE * 0.75)
									{
										cmdOutput[threadID] +=

#ifndef NO_OUTPUT_BUT_FPS
											"| Collision at:" + 
											std::to_string(Frametime) + ", Spheres: " + (Name[i]) + '(' + 
											std::to_string(Health[i]) + ") & StaticSphere" + (name)+",  (" + 
											std::to_string(*health) + "),distance :" + 
											std::to_string(finalXYZ) + " | ";
#else
											"";
#endif
									}
									else
									{
										std::cout << cmdOutput[threadID] << "\nEarlyRelease";
										cmdOutput[threadID].clear();
									}
#endif
#endif



								}
							}

						}
						//Basic single axis collision detection. Should be faster to iterate through. We can assume the X axis is within range.

					}
				}
			};




			ID[0] = std::round((((sphPosition[i].value[0] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);
			ID[1] = std::round((((sphPosition[i].value[1] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);
			ID[2] = std::round((((sphPosition[i].value[2] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);




			//Safety checks in code which return if not viable. 
			constexpr int collRange = 1;
			for (int id0 = -collRange; id0 <= collRange; ++id0)
				for (int id1 = -collRange; id1 <= collRange; ++id1)
					for (int id2 = -collRange; id2 <= collRange; ++id2)
						CheckStaticCollisionAt(
							ID[0] + id1,
							ID[1] + id2,
							ID[2] + id0);



#endif





		}

		//Used to escape static spheres when max X axis value is passed. 
	BigBreak:;








		//BlockSprites(thread->start[0], thread->end[0], particleSize, particleAnchor, thread->start[1], thread->end[1], blockerSize, blockerAnchor);



		//Collision Start

	{
		//Set this to 1 to notify the main thread. Once it reacts and sets the value to 2, iterate.

		std::unique_lock<std::mutex> lock(mSphereThreads[threadID].mutexLock);
		mSphereThreads[threadID].isFrameDone = 1;
		mSphereThreads[threadID].workReady.notify_one();


		//Once this is greater than 1, move on. Shouldn't reset until it's 0.

		while (mSphereThreads[threadID].isFrameDone == 1)
			mSphereThreads[threadID].workReady.wait(lock);

	}


#ifdef DYNAMIC_COLLISIONS

	//Dynamic collisions. 
	{



		auto DynamicCollision = [&](int curSph, int x, int y, int z)
		{
			if (x < 0 || x >= PARTITION_ARRAY_SIZE || y < 0 || y >= PARTITION_ARRAY_SIZE || z < 0 || z >= PARTITION_ARRAY_SIZE)
				return;

			if (DynamicPosition[x][y][z][0].size() > 0)
			{




				//auto ref = DynamicPosition[x][y][z][0].begin();
				for(const auto& ref : DynamicPosition[x][y][z][0])
			

			//	for (;  (ref != DynamicPosition[x][y][z][0].end()); ++ref)  //This one will be slow.
				{


					//There seems to be a bug where this can happen with larger amounts of spheres.
					if (&ref == nullptr)
					{
						return;
					}
#if  defined(DEATHS) 
#ifndef INVINCIBLE_DYNAMIC
					if (Health[curSph] <= 0) return;
#endif
#endif

					//This is read only? I don't understand why this might crash. Nothing will be writing to it.
					//TestVal = &DynamicPosition[x][y][z][threadData][index];



					if (!(curSph == ref.IndexRef
#if  defined(DEATHS) 
#ifndef INVINCIBLE_DYNAMIC
						|| Health[ref.IndexRef] <= 0
#endif
#endif
						))
					{


						radX = -sphPosition[curSph].value[0];
						radY = -sphPosition[curSph].value[1];
						radZ = -sphPosition[curSph].value[2];


						radX += ref.Position[0];
						radY += ref.Position[1];
						radZ += ref.Position[2];


						finalXYZ = (radX * radX) + (radY * radY) + (radZ * radZ);


						RadSum = Radius[ref.IndexRef] + Radius[curSph];

						if (finalXYZ < RadSum * RadSum)
						{
							//CollOutput += "col:" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + " / ";
							for (loop = 0; loop < 3; ++loop)
								sphVelocity[i].value[loop] *= -1.0f;

							Health[curSph] -= HEALTH_HIT_REDUCTION;
#if  defined(DEATHS) && defined(ENGINE) 
#ifndef INVINCIBLE_DYNAMIC
							if (Health[curSph] <= 0)
							{
								DeadSpheresThisFrame[threadID].push_back(curSph);
							}
#endif
#endif



							CollOutput +=
#ifndef NO_OUTPUT_BUT_FPS
								": Frame," + std::to_string(frameCount) +
#ifndef ENGINE
								" Time, " + std::to_string((timeSinceStart - Frametime) * 0.001f) +
#else
								" FPS, " + std::to_string(Frametime) +
#endif
								", " + (Name[i]) + '(' + std::to_string(Health[i]) + ") & " + (Name[ref->IndexRef]) + ",(" + std::to_string(Health[ref->IndexRef]) + ") dist:" + std::to_string(finalXYZ) + " :";
#else
								"";
#endif						
		

							if (cmdOutput[threadID].size() < STRING_RESERVE_SIZE * 0.95f)
							{
								cmdOutput[threadID] += CollOutput;

							}
							else
							{
#if !defined(FILE_ONLY_OUTPUT)
								std::cout << "String full. Early release.\n" << cmdOutput[threadID] << '\n';
								std::cout << CollOutput;
#else
								file << "String full. Early release.\n" << cmdOutput[threadID] << '\n';
								file << CollOutput;
#endif
								cmdOutput[threadID].clear();
							}

						}
					}
				}
			}
		};




		for (i = min; i < max; ++i)
		{
#ifndef INVINCIBLE_DYNAMIC							
			//Iterate as fast as possible through groups of 'dead' entities.
			while (Health[i] <= 0 && i < max - 1)
				++i;

			if (Health[i] > 0)
#endif		
			{


				ID[0] = std::round((((sphPosition[i].value[0] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);
				ID[1] = std::round((((sphPosition[i].value[1] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);
				ID[2] = std::round((((sphPosition[i].value[2] + HALF_MAX_ABS_POSITION) / MAX_ABS_POSITION) * (PARTITION_ARRAY_SIZE / 2)) * 2.0f);




				//Safety checks in code which return if not viable. 
				constexpr int collRange = 1;
				for (int id0 = -collRange; id0 <= collRange; ++id0)
					for (int id1 = -collRange; id1 <= collRange; ++id1)
						for (int id2 = -collRange; id2 <= collRange; ++id2)
							DynamicCollision(i,
								ID[0] + id1,
								ID[1] + id2,
								ID[2] + id0);



			}
		}
	};




#endif



	{

		std::unique_lock<std::mutex> lock(mSphereThreads[threadID].mutexLock);
		mSphereThreads[threadID].isFrameDone = 3;
		mSphereThreads[threadID].workReady.notify_one();


		//Doesn't seem to like it when I just put in the boolean so I threw it in a lambda 'cause lambda's are great. Would've put the thread creation in Lambda's too but it screwed up the inputs, 
		//since it created the thread and then called the function with a reference to the current values, which would've updated at that point. Could've removed the & but it was pointless.

		while (mSphereThreads[threadID].isFrameDone == 3)
			mSphereThreads[threadID].workReady.wait(lock);

	}



	}



}
template<typename dimensions>
inline float SceneManager<dimensions>::random(float min, float max) //range : [min, max]
{

	static bool isFirst = true;
	if (isFirst)
	{
		//Want each iteration to be unique. Seed based on time.
		srand(time(NULL));
		isFirst = false;
	}

	float output = remainder(min + rand(), ((max * 2.0f)));

	//Seems to be a big. Not massively important. 
	if (output < min) output = min;
	else
		if (output > max) output = max;

	return output;

}



template<typename dimensions>
inline void SceneManager<dimensions>::MainLoop()
{
#ifdef ENGINE
	Frametime = EngineReference->Timer();

	//myCamera->LookAt(StaticSpheres.Stored[SPHERE_HALFPOINT].Position.x, StaticSpheres.Stored[SPHERE_HALFPOINT].Position.y, StaticSpheres.Stored[SPHERE_HALFPOINT].Position.z);

	if (EngineReference != nullptr)
		while (EngineReference->IsRunning())


#else

	
	static auto timeSinceEpoch = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

	while (1)
#endif

	{
#if defined(CAMERA_MOVEMENT) && defined(ENGINE)
		auto CameraControls = [&]()
		{


			if (frameCount > 0 && frameCount % FRAME_REMAINDER == 0) {
				fps / FRAME_REMAINDER;
				fps = 1.0f / Frametime;

				for (int i = 1; i < mNumWorkers; ++i)
				{
					cmdOutput[0] += cmdOutput[i];
					cmdOutput[i] = "";
				}

				std::cout << cmdOutput[0] << "\n\n" << fps << " averaged over, " << FRAME_REMAINDER << ", frames." << '\n';
				fps = 0;

				cmdOutput[0] = "";
			}
			if (EngineReference->KeyHit(Key_Escape))
			{
				EngineReference->Stop();
			}

			if (EngineReference->KeyHeld(Key_Left))
			{
				myCamera->MoveLocalX(-CAMERA_SPEED * Frametime);
				myCamera->LookAt((MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2);
			}
			else if (EngineReference->KeyHeld(Key_Right))
			{
				myCamera->MoveLocalX(CAMERA_SPEED * Frametime);
				myCamera->LookAt((MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2);
			}
			else if (EngineReference->KeyHeld(Key_Up))
			{
				myCamera->MoveLocalY(CAMERA_SPEED * Frametime);
				myCamera->LookAt((MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2);
			}
			else
				if (EngineReference->KeyHeld(Key_Down))
				{

					myCamera->MoveLocalY(-CAMERA_SPEED * Frametime);
					myCamera->LookAt((MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2, (MAX_POSITION + MIN_POSITION) / 2);

				}

		};
#endif
		{



			//Activate threads
			for (int i = 0; i < mNumWorkers; ++i)
			{
				//Tell the thread to start by setting its frameDone to 0.
				{
					std::unique_lock<std::mutex> lock(mSphereThreads[i].mutexLock);
					mSphereThreads[i].isFrameDone = 0;
				}
				mSphereThreads[i].workReady.notify_one();
			}



			//MainThread();
			{
#ifdef ENGINE
#ifdef CAMERA_MOVEMENT
				CameraControls();
#endif
				Frametime = EngineReference->Timer();
				fps += Frametime;
#endif
			}




			//Wait for threads.
			for (int i = 0; i < mNumWorkers; ++i)
			{
				//The frameDone has been set to 1. Or at least should be. Loop until this is the case. Need a notify to tell the script that it has updated.
				std::unique_lock<std::mutex> lock(mSphereThreads[i].mutexLock);

				while (mSphereThreads[i].isFrameDone != 1)
					mSphereThreads[i].workReady.wait(lock);

			}

		}

#ifdef DYNAMIC_COLLISIONS
		/*
		With a partition size of 3 and a world-space area covering -100 to 100 then, in theory, it should be broken up into: -100 to -33, -33 to 34 and 41 to 101. Slightly skewed thanks to rounding, 
		so inefficient usage of memory, but it fits within the given parameters.

		*/
		for (int id0 = 0; id0 < PARTITION_ARRAY_SIZE; ++id0)
			for (int id1 = 0; id1 < PARTITION_ARRAY_SIZE; ++id1)
				for (int id2 = 0; id2 < PARTITION_ARRAY_SIZE; ++id2)
					for (int i = 1; i < mNumWorkers; ++i) {
						auto DynamicIterator = DynamicPosition[id0][id1][id2][0].begin();
						DynamicPosition[id0][id1][id2][0].splice(DynamicIterator, DynamicPosition[id0][id1][id2][i]);
					}





#endif
		{

			//Activate dynamic collision segment
			for (int i = 0; i < mNumWorkers; ++i)
			{
				{
					std::unique_lock<std::mutex> lock(mSphereThreads[i].mutexLock);
					mSphereThreads[i].isFrameDone = 2;
				}
				mSphereThreads[i].workReady.notify_one();
			}


		}

#ifdef ENGINE
#ifdef CAMERA_MOVEMENT
		CameraControls();
#endif
#ifdef MODEL_SKINS
		static bool doOnce;

		if (!doOnce)
		{
			doOnce = true;
			for (int i = 0; i < SPHERE_HALFPOINT; ++i) if (hasTouched[i])
				SphereModel[i]->SetSkin("Sun.jpg");

#ifdef NO_STATIC_SPHERES
			for (int i = SPHERE_HALFPOINT; i < SPHERE_COUNT; ++i)
				SphereModel[i]->Scale(0.0f);
#endif

#ifdef NO_DYNAMIC_SPHERES
			for (int i = 0; i < SPHERE_HALFPOINT; ++i)
				SphereModel[i]->Scale(0.0f);
#endif

		}
#endif
		//Single IF statement for any visual representation. 

		{
			//Timer

			EngineReference->DrawScene();
		}

#else
		//This is slow. Rework

		timeSinceStart = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - timeSinceEpoch;
		if (frameCount % FRAME_REMAINDER == 0)
		{
			if (frameCount > 0) {
				for (int i = 1; i < MAX_WORKERS && 0 < cmdOutput[i].size(); ++i)
				{
					cmdOutput[0] += "\n\nCurrent frame: " + std::to_string(frameCount) + ", Current time:" + std::to_string(timeSinceStart - timeSinceEpoch) + "\n\nFPS: " + std::to_string(FRAME_REMAINDER / ((timeSinceStart - Frametime) * 0.001f)) + ':' + cmdOutput[i];
					//	if (cmdOutput[0].size() > STRING_RESERVE_SIZE * 0.8f)
					{
#ifndef  FILE_ONLY_OUTPUT
						std::cout << cmdOutput[0];
						cmdOutput[0] = "";
#else
						cmdOutput[MAX_WORKERS] += "\n" + cmdOutput[0];
						cmdOutput[0] = "";
#endif
					}

				}

#ifndef  FILE_ONLY_OUTPUT
				std::cout << "\n\n" + cmdOutput[0] + "\n\n Time since start: " + std::to_string(timeSinceStart * .001f) + ", Current frame: " + std::to_string(frameCount) + ", FPS:" + std::to_string(FRAME_REMAINDER / ((timeSinceStart - Frametime) * 0.001f)) + "\n";
#endif

#if defined(FILE_ONLY_OUTPUT) || defined(FILE_OUTPUT)
				cmdOutput[MAX_WORKERS] += "\n" + cmdOutput[0] + "\n\n Time since start: " + std::to_string(timeSinceStart * .001f) + ". FPS:" + std::to_string(FRAME_REMAINDER / ((timeSinceStart - Frametime) * 0.001f));
				file << cmdOutput[MAX_WORKERS];
				cmdOutput[MAX_WORKERS].clear();

				return;
				if (cmdOutput[MAX_WORKERS].size() > STRING_RESERVE_SIZE * 0.8f)
				{
					file << cmdOutput[MAX_WORKERS];
					cmdOutput[MAX_WORKERS].clear();
				}
#endif

				Frametime = timeSinceStart;
			}
		}
#endif

#ifdef DEATHS
		//A bit slow but the visual output isn't the most important. Can't do this in threads because the engine appears to be single threaded, or atleast doesn't have any 
		//form of mutexes to stop multiple threads causing issues.

#ifdef ENGINE
		for (int i = 0; i < mNumWorkers; ++i)
		{
			for (int j = 0; j < DeadSpheresThisFrame[i].size(); ++j)
			{
				//If health is below 0 it should skip it so the model is the only thing that needs changing. Doesn't appear to like threads.
				SphereModel[DeadSpheresThisFrame[i][j]]->SetPosition(MIN_POSITION * 10.0f, MIN_POSITION * 10.0f, MIN_POSITION * 10.0f);
				SphereModel[DeadSpheresThisFrame[i][j]]->Scale(0.0f);
			}
			DeadSpheresThisFrame[i].clear();
		}


#endif
#endif

		++frameCount;

		for (int i = 0; i < mNumWorkers; ++i)
		{
			//The frameDone has been set to 1. Or at least should be. Loop until this is the case. Need a notify to tell the script that it has updated.
			std::unique_lock<std::mutex> lock(mSphereThreads[i].mutexLock);

			while (mSphereThreads[i].isFrameDone != 3)
				mSphereThreads[i].workReady.wait(lock);


		}
	}
}


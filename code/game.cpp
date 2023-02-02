#include "game.h"

#include "raylib.h"
#include "raymath.h"

#include "physics.h"
#include "shipPhysics.h"

#include "math.h"

struct CameraArm
{
	btQuaternion currentRotation = QuatIdentity;

	btVector3 baseDir = Vec3Forward;
	btVector3 baseUp = Vec3Up;

	r32 currentDistance = 0.0f; // somewhere between min and max depending on camera collision
	r32 minDistance = 0.0f;
	r32 maxDistance = 0.0f; 

	CameraArm(const btVector3 &cameraOffset)
	{
		baseDir = cameraOffset.normalized();
		baseUp = -baseDir.cross(Vec3Left); // Using left as its boat to the
		currentDistance = cameraOffset.length();
		currentRotation = QuatIdentity;//QuaternionFromVector3ToVector3(Vec3Back, baseDir);

		minDistance = 2.0f; // TODO: pass in minDistance?
		maxDistance = currentDistance;
	};
};

struct Camera
{
	enum class Projection
	{
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	btVector3 position = Vec3Zero;
	btVector3 target = Vec3Zero;
	btVector3 up = Vec3Up;
	r32 fovY = 0.0f;
	Projection projection = Projection::PERSPECTIVE;

	Camera(const btVector3 &position, const btVector3 target, const btVector3 up, r32 fovY, Projection projection)
		: position(position), target(target), up(up), fovY(fovY), projection(projection) {};
};

int ToRaylibCameraProjection(Camera::Projection projection)
{
	return (projection == Camera::Projection::PERSPECTIVE) ? Raylib::CAMERA_PERSPECTIVE : Raylib::CAMERA_ORTHOGRAPHIC;
}

Raylib::Vector3 ToRaylibVec3(const btVector3 &source)
{
	Raylib::Vector3 result = {source.getX(), source.getY(), source.getZ()};
	return result;
}

inline btVector3 RandomPointInRange(btVector3 min, btVector3 max)
{
	r32 x = (r32)Raylib::GetRandomValue((s32)min.getX(), (s32)max.getX());
	r32 y = (r32)Raylib::GetRandomValue((s32)min.getY(), (s32)max.getY());
	r32 z = (r32)Raylib::GetRandomValue((s32)min.getZ(), (s32)max.getZ());

	btVector3 result(x, y, z);
	return result;
}

Raylib::Model LoadModelWithTextures(const char *meshFile, const char *albedoFile, const char *normalMapFile)
{
	OPTICK_EVENT();

	Raylib::Model model = Raylib::LoadModel(meshFile);
	if(albedoFile)
	{
		Raylib::Texture albedo = Raylib::LoadTexture(albedoFile);
		SetMaterialTexture(&model.materials[0], Raylib::MATERIAL_MAP_ALBEDO, albedo);
	}
	if(normalMapFile)
	{
		Raylib::Texture normalMap = Raylib::LoadTexture(normalMapFile);
		SetMaterialTexture(&model.materials[0], Raylib::MATERIAL_MAP_NORMAL, normalMap);
	}
	return model;
};

std::vector<Raylib::Model> LoadAsteroidModels()
{
	OPTICK_EVENT();

	std::vector<Raylib::Model> asteroidModels;
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_1.obj", "asteroids/asteroid_small_1_color.png", "asteroids/asteroid_small_1_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_2.obj", "asteroids/asteroid_small_2_color.png", "asteroids/asteroid_small_2_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_3.obj", "asteroids/asteroid_small_3_color.png", "asteroids/asteroid_small_3_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_4.obj", "asteroids/asteroid_small_4_color.png", "asteroids/asteroid_small_4_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_5.obj", "asteroids/asteroid_small_5_color.png", "asteroids/asteroid_small_5_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_6.obj", "asteroids/asteroid_small_6_color.png", "asteroids/asteroid_small_6_nm.png"));
	return asteroidModels;
}

std::vector<btConvexShape *> CreateAsteroidCollisions(const std::vector<Raylib::Model> &asteroidModels, r32 scale)
{
	std::vector<btConvexShape *> asteroidCollisions;

	for(const Raylib::Model &model : asteroidModels)
	{
		btConvexShape * collisionShapes = CreateConvexCollision(model, scale);
		asteroidCollisions.push_back(collisionShapes);
	}
	Assert(asteroidCollisions.size() == asteroidModels.size());

	return asteroidCollisions;
}


static void CreatePlayer(Game &game)
{
	entt::registry &registry = game.GetRegistry();

	entt::entity entity = registry.create();

	Transform transform;
	registry.emplace<Transform>(entity, transform);

	Raylib::Model shipModel = LoadModelWithTextures("ship/pirate-ship-blender-v2.obj", "ship/pirate-ship-blender-v2.png", nullptr);
	shipModel.transform = Raylib::MatrixRotateY(180.0f * DEG2RAD);
	registry.emplace<Raylib::Model>(entity, shipModel);

	registry.emplace<ShipInput>(entity); // Reads from global input and maps to ship movement inputs

	ShipConfig shipConfig; // Using default initialised values
	registry.emplace<ShipConfig>(entity, shipConfig);

	PhysicsWorld * const physics = game.GetPhysics();

	r32 collisionRadius = 1.0f;
	r32 collisionLength = 2.5f;
	//btConvexShape *shipCollision = CreateCapsuleZAxisCollision(collisionRadius, collisionLength);
	btConvexShape *shipCollision = CreateCylinderZAxisCollision(collisionRadius, collisionLength);

	int filterGroup = 0;
	int filterMask = 0;

	btPairCachingGhostObject *ghostObject = physics->CreateGhostObject(shipCollision, filterGroup, filterMask);
	ShipPhysics &shipPhyics = registry.emplace<ShipPhysics>(entity, shipCollision, ghostObject, shipConfig);
	physics->AddAction((btActionInterface *)&shipPhyics);

	const r32 cameraFovY = 45.0f;
	Camera camera(Vec3Zero, Vec3Forward, Vec3Up, cameraFovY, Camera::Projection::PERSPECTIVE);

	registry.emplace<Camera>(entity, camera);
	game.SetCameraEntity(entity);

	btVector3 cameraArmOffsetVec(2.5f, 3.0f, 10.0f);
	CameraArm cameraArm(cameraArmOffsetVec);
	registry.emplace<CameraArm>(entity, cameraArm);
}

Game::Game(u32 windowWidth, u32 windowHeight)
	: m_windowWidth(windowWidth), m_windowHeight(windowHeight)
{
	OPTICK_EVENT();

	SeedRandom((u32)Raylib::GetTime());

	m_physics = new PhysicsWorld();

	std::vector<Raylib::Color> colors = {
		Raylib::LIGHTGRAY, Raylib::GRAY, Raylib::DARKGRAY, Raylib::YELLOW, Raylib::GOLD, 
		Raylib::ORANGE, Raylib::PINK, Raylib::RED, Raylib::MAROON, Raylib::GREEN, 
		Raylib::LIME, Raylib::DARKGREEN, Raylib::SKYBLUE, Raylib::BLUE, Raylib::DARKBLUE, 
		Raylib::PURPLE, Raylib::VIOLET, Raylib::DARKPURPLE, Raylib::BEIGE, Raylib::BROWN,
		Raylib::DARKBROWN, Raylib::BLACK
	};

	constexpr u32 totalEventDataMemory = Kilobytes(4);
	m_eventDataMemory = Raylib::MemAlloc(totalEventDataMemory);
	m_eventDataMemorySize = totalEventDataMemory;

	CreatePlayer(*this);

	std::vector<Raylib::Model> asteroidModels = LoadAsteroidModels();

	r32 asteroidScale = 10.0f;
	std::vector<btConvexShape *> asteroidCollisions = CreateAsteroidCollisions(asteroidModels, asteroidScale);


	btVector3 asteroidMinBounds(-200, -200, -200);
	btVector3 asteroidMaxBounds(200, 200, 200);

	constexpr u32 asteroidsToCreate = 200;
	for(u32 entityNum = 0; entityNum < asteroidsToCreate; ++entityNum)
	{
		entt::entity entity = m_registry.create();

		Transform &transform = m_registry.emplace<Transform>(entity);

		transform.translation = RandomPointInRange(asteroidMinBounds, asteroidMaxBounds);

		btVector3 randomAxis(RandomFloat(0.0, 100.0), RandomFloat(0.0, 100.0), RandomFloat(0.0, 100.0));
		randomAxis.normalize();

		r32 randomAngle = RandomFloat(0.0, 180.0);
		transform.rotation = btQuaternion(randomAxis, randomAngle);

		transform.scale = btVector3(asteroidScale, asteroidScale, asteroidScale);


		r32 mass = 10.0f;

		u32 randAsteroidNum = Raylib::GetRandomValue(0, (u32)asteroidModels.size() - 1);
		btConvexShape *collisionShape = asteroidCollisions.at(randAsteroidNum);

		RigidBody rigidBody = m_physics->CreateRigidBody(transform.translation, transform.rotation, mass, collisionShape);
		m_registry.emplace<RigidBody>(entity, rigidBody);

		m_registry.emplace<Raylib::Model>(entity, asteroidModels.at(randAsteroidNum));

		m_entities.push_back(entity);
	}

}

Game::~Game()
{
	for(const auto entity : m_entities)
	{
		m_registry.destroy(entity);
	};
	m_entities.clear();
}

struct CylinderMesh
{
	r32 radius;
	r32 height;
	u32 slices;
	Raylib::Color color;
};

struct SpawnLaserbeamEventData
{
	Transform spawnTransform;
	btVector3 startVelocity;
};

void Game::SpawnLaserbeam(void *data)
{
	SpawnLaserbeamEventData *eventData = (SpawnLaserbeamEventData *)data;

	Raylib::TraceLog(Raylib::LOG_INFO, "FIRE!");

	entt::entity entity = m_registry.create();

	Transform &transform = m_registry.emplace<Transform>(entity, eventData->spawnTransform);

	btConvexShape *collisionShape = CreateCapsuleZAxisCollision(0.2f, 0.5f);

	r32 mass = 1.0f;

	RigidBody rigidBody = m_physics->CreateRigidBody(transform.translation, transform.rotation, mass, collisionShape);

	r32 shipVelocity = eventData->startVelocity.length();

	btScalar speed = 10.0f;
	btVector3 beamVelocity = Vec3Forward * speed * shipVelocity;
	beamVelocity = quatRotate(transform.rotation, beamVelocity);

	rigidBody.body->setLinearVelocity(beamVelocity);

	m_registry.emplace<RigidBody>(entity, rigidBody);
	
	CylinderMesh cylinderMesh = {};
	cylinderMesh.radius = 0.05f;
	cylinderMesh.height = 1.0f;
	cylinderMesh.slices = 16;
	cylinderMesh.color = Raylib::ORANGE;

	m_registry.emplace<CylinderMesh>(entity, cylinderMesh);
}


void Game::UpdateAndDraw()
{
	OPTICK_EVENT();

	r64 frameTime = Raylib::GetTime();
	r32 dt = (r32)(frameTime - m_lastFrameTime);
	m_lastFrameTime = frameTime;

	if(Raylib::IsKeyPressed(Raylib::KEY_F1)) { debugFlags.drawCollision = !debugFlags.drawCollision; }

	// Run events
	{
		for(Event &evnt : m_events)
		{
			evnt.delegate(evnt.data);
			evnt.data = nullptr;
		}
		m_events.clear();
		m_eventDataMemoryUsed = 0;
	}

	// Read input and map to actions
	{
		OPTICK_EVENT("UpdateShipInput");

		auto view = m_registry.view<ShipInput>();
		view.each([](ShipInput &shipInput) {

			Raylib::Vector2 mouseDelta = Raylib::GetMouseDelta();

			constexpr r32 mouseSensitivity = 0.15f;
			mouseDelta = Raylib::Vector2Scale(mouseDelta, mouseSensitivity);

			shipInput.inputs[ShipInput::THRUST_Z] = 0.0f;
			if(Raylib::IsKeyDown(Raylib::KEY_W)) { shipInput.inputs[ShipInput::THRUST_Z] -= 1.0f; }
			if(Raylib::IsKeyDown(Raylib::KEY_S)) { shipInput.inputs[ShipInput::THRUST_Z] += 1.0f; }

			shipInput.inputs[ShipInput::THRUST_X] = 0.0f;
			if(Raylib::IsKeyDown(Raylib::KEY_A)) { shipInput.inputs[ShipInput::THRUST_X] -= 1.0f; }
			if(Raylib::IsKeyDown(Raylib::KEY_D)) { shipInput.inputs[ShipInput::THRUST_X] += 1.0f; }

			shipInput.inputs[ShipInput::THRUST_Y] = 0.0f;
			if(Raylib::IsKeyDown(Raylib::KEY_LEFT_SHIFT)) { shipInput.inputs[ShipInput::THRUST_Y] += 1.0f; }
			if(Raylib::IsKeyDown(Raylib::KEY_LEFT_CONTROL)) { shipInput.inputs[ShipInput::THRUST_Y] -= 1.0f; }

			shipInput.inputs[ShipInput::PITCH] = 0.0f;
			shipInput.inputs[ShipInput::YAW] = 0.0f;
			shipInput.inputs[ShipInput::ROLL] = 0.0f;

			shipInput.inputs[ShipInput::PITCH] = -mouseDelta.y;
			shipInput.inputs[ShipInput::YAW] = -mouseDelta.x;

			if(Raylib::IsKeyDown(Raylib::KEY_Q)) { shipInput.inputs[ShipInput::ROLL] += 1.0f; }
			if(Raylib::IsKeyDown(Raylib::KEY_E)) { shipInput.inputs[ShipInput::ROLL] -= 1.0f; }

			shipInput.inputs[ShipInput::FIRE] = Raylib::IsMouseButtonPressed(Raylib::MOUSE_BUTTON_LEFT) ? 1.0f : 0.0f;
			shipInput.inputs[ShipInput::ALT_FIRE] = Raylib::IsMouseButtonPressed(Raylib::MOUSE_BUTTON_RIGHT) ? 1.0f : 0.0f;
		});
	}

	// Update ship physics based of current ship input
	{
		OPTICK_EVENT("ApplyShipInput");
		auto view = m_registry.view<const ShipInput, const ShipConfig, ShipPhysics>();
		view.each([dt, this](const ShipInput &shipInput, const ShipConfig &shipConfig, ShipPhysics &shipPhysics) {
			shipPhysics.ApplyShipInput(shipInput);

			if(shipInput.inputs[ShipInput::FIRE])
			{
				Event newEvent;
				newEvent.delegate.connect<&Game::SpawnLaserbeam>(this);

				SpawnLaserbeamEventData *eventData = AllocEventData<SpawnLaserbeamEventData>(newEvent);
				eventData->spawnTransform = shipPhysics.GetTransform();
				eventData->startVelocity = shipPhysics.GetVelocity();
				m_events.push_back(newEvent);
			}
		});
	}

	m_physics->Step(dt);

	// Apply physics update to our transforms
	{
		OPTICK_EVENT("UpdateTransforms")
		auto view = m_registry.view<const RigidBody, Transform>();
		view.each([this](const RigidBody &rigidBody, Transform &transform) {
			btTransform bodyTransform = rigidBody.body->getWorldTransform();
			transform.translation = bodyTransform.getOrigin();
			bodyTransform.getBasis().getRotation(transform.rotation);
		});

		auto shipView = m_registry.view<const ShipPhysics, Transform>();
		shipView.each([this](const ShipPhysics &shipPhysics, Transform &transform) {
			transform = shipPhysics.GetTransform();
		});
	}

	// Update Camera by attached transform
	{
		OPTICK_EVENT("UpdateCamera");

		auto view = m_registry.view<const Transform, CameraArm, Camera>();
		view.each([](const Transform &transform, CameraArm &cameraArm, Camera &camera) {

			if(cameraArm.currentRotation != transform.rotation)
			{
				constexpr r32 cameraLerpAmount = 0.08f;
				cameraArm.currentRotation = cameraArm.currentRotation.slerp(transform.rotation, cameraLerpAmount);
			}

			btVector3 toCameraDir = quatRotate(cameraArm.currentRotation, cameraArm.baseDir);
			btVector3 toCameraVec = toCameraDir * cameraArm.currentDistance;
			camera.position = transform.translation + toCameraVec;
			camera.up = quatRotate(cameraArm.currentRotation, cameraArm.baseUp);

			btVector3 transformFaceDir = quatRotate(transform.rotation, Vec3Forward);
			constexpr r32 inFrontAmount = 20.0f;
			btVector3 inFrontOfBoat = transformFaceDir * inFrontAmount;
			camera.target = transform.translation + inFrontOfBoat;
		});
	}

	
	Raylib::BeginDrawing();

	Raylib::ClearBackground(Raylib::BLACK);

	const Camera &camera = m_registry.get<const Camera>(m_cameraEntity);
	Raylib::Camera raylibCamera =
	{
		ToRaylibVec3(camera.position),
		ToRaylibVec3(camera.target),
		ToRaylibVec3(camera.up),
		camera.fovY,
		ToRaylibCameraProjection(camera.projection)
	};

	Raylib::BeginMode3D(raylibCamera);
	//Raylib::DrawCylinderEx({0, 0, 0}, {0,0,1}, 1, 1, 16, Raylib::RED);

	// Draw asteroids
	{
		OPTICK_EVENT("Draw");
		{
			OPTICK_EVENT("DrawModels");

			auto modelView = m_registry.view<const Raylib::Model, const Transform>();
			modelView.each([](const Raylib::Model &model, const Transform &transform) {
				btVector3 rotationAxis = transform.rotation.getAxis();
				r32 rotationAngle = transform.rotation.getAngle();
				rotationAngle = RadToDeg(rotationAngle);
				Raylib::DrawModelEx(model, ToRaylibVec3(transform.translation), ToRaylibVec3(rotationAxis), 
					rotationAngle, ToRaylibVec3(transform.scale), Raylib::WHITE);
				//DrawModelWiresEx(model, transform.translation, rotationAxis, rotationAngle, transform.scale, Raylib::WHITE);
			});
		}

		{
			OPTICK_EVENT("DrawCylinders");

			auto cylinderView = m_registry.view<const CylinderMesh, const Transform>();
			cylinderView.each([](const CylinderMesh &cylinderMesh, const Transform &transform) {
				btVector3 endPos = transform.translation + quatRotate(transform.rotation, Vec3Forward);
				endPos *= cylinderMesh.height;

				DrawCylinderEx(ToRaylibVec3(transform.translation), ToRaylibVec3(endPos), 
					cylinderMesh.radius, cylinderMesh.radius, cylinderMesh.slices, cylinderMesh.color);
			});
		}
		


		if(debugFlags.drawCollision)
		{
			m_physics->DrawDebugInfo();
		}
	}
	Raylib::EndMode3D();

	Raylib::DrawFPS(10, 10);

	Raylib::EndDrawing();
}
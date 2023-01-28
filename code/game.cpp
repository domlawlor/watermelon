#include "game.h"

#include "raylib.h"
#include "raymath.h"

#include "physics.h"
#include "shipPhysics.h"

constexpr Vector3 Vec3Zero = {0.0f, 0.0f, 0.0f};
constexpr Vector3 Vec3Up = {0.0f, 1.0f, 0.0f};
constexpr Vector3 Vec3Down = {0.0f, -1.0f, 0.0f};
constexpr Vector3 Vec3Forward = {0.0f, 0.0f, -1.0f};
constexpr Vector3 Vec3Back = {0.0f, 0.0f, 1.0f};
constexpr Vector3 Vec3Left = {-1.0f, 0.0f, 0.0f};
constexpr Vector3 Vec3Right = {1.0f, 0.0f, 0.0f};

constexpr Quaternion QuatIdentity = {0.0f, 0.0f, 0.0f, 1.0f};

struct CameraArm
{
	Quaternion currentRotation;

	Vector3 baseDir;
	Vector3 baseUp;

	r32 currentDistance; // somewhere between min and max depending on camera collision
	r32 minDistance;
	r32 maxDistance; 

	CameraArm() = delete;
	CameraArm(Vector3 cameraOffset)
	{
		baseDir = Vector3Normalize(cameraOffset);
		baseUp = Vector3Negate(Vector3CrossProduct(baseDir, Vec3Left));
		currentDistance = Vector3Length(cameraOffset);
		currentRotation = QuatIdentity;//QuaternionFromVector3ToVector3(Vec3Back, baseDir);

		minDistance = 2.0f; // TODO: pass in minDistance?
		maxDistance = currentDistance;
	};
};




inline Vector3 RandomVec3InBoundingBox(BoundingBox bounds)
{
	Vector3 result = {};
	result.x = (r32)GetRandomValue((s32)bounds.min.x, (s32)bounds.max.x);
	result.y = (r32)GetRandomValue((s32)bounds.min.y, (s32)bounds.max.y);
	result.z = (r32)GetRandomValue((s32)bounds.min.z, (s32)bounds.max.z);
	return result;
}


Model LoadModelWithTextures(const char *meshFile, const char *albedoFile, const char *normalMapFile)
{
	OPTICK_EVENT();

	Model model = LoadModel(meshFile);
	if(albedoFile)
	{
		Texture albedo = LoadTexture(albedoFile);
		SetMaterialTexture(&model.materials[0], MATERIAL_MAP_ALBEDO, albedo);
	}
	if(normalMapFile)
	{
		Texture normalMap = LoadTexture(normalMapFile);
		SetMaterialTexture(&model.materials[0], MATERIAL_MAP_NORMAL, normalMap);
	}
	return model;
};

std::vector<Model> LoadAsteroidModels()
{
	OPTICK_EVENT();

	std::vector<Model> asteroidModels;
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_1.obj", "asteroids/asteroid_small_1_color.png", "asteroids/asteroid_small_1_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_2.obj", "asteroids/asteroid_small_2_color.png", "asteroids/asteroid_small_2_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_3.obj", "asteroids/asteroid_small_3_color.png", "asteroids/asteroid_small_3_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_4.obj", "asteroids/asteroid_small_4_color.png", "asteroids/asteroid_small_4_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_5.obj", "asteroids/asteroid_small_5_color.png", "asteroids/asteroid_small_5_nm.png"));
	asteroidModels.push_back(LoadModelWithTextures("asteroids/asteroid_small_6.obj", "asteroids/asteroid_small_6_color.png", "asteroids/asteroid_small_6_nm.png"));
	return asteroidModels;
}


std::vector<btConvexShape *> CreateAsteroidCollisions(const std::vector<Model> &asteroidModels, r32 scale)
{
	std::vector<btConvexShape *> asteroidCollisions;

	for(const Model &model : asteroidModels)
	{
		btConvexShape * collisionShapes = CreateConvexCollision(model, scale);
		asteroidCollisions.push_back(collisionShapes);
	}
	Assert(asteroidCollisions.size() == asteroidModels.size());

	return asteroidCollisions;
}

Game::Game(u32 windowWidth, u32 windowHeight)
	: m_windowWidth(windowWidth), m_windowHeight(windowHeight)
{
	OPTICK_EVENT();

	m_physics = new PhysicsWorld();

	std::vector<Color> colors = {
		LIGHTGRAY, GRAY, DARKGRAY, YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
		SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN, BLACK
	};

	// Setup player entity
	m_playerEntity = m_registry.create();

	{
		Transform transform;
		transform.translation = Vector3Zero();
		transform.rotation = QuaternionIdentity();
		transform.scale = Vector3One();

		m_registry.emplace<Transform>(m_playerEntity, transform);

		Model shipModel = LoadModelWithTextures("ship/pirate-ship-blender-v2.obj", "ship/pirate-ship-blender-v2.png", nullptr);
		shipModel.transform = MatrixRotateY(180.0f * DEG2RAD);
		m_registry.emplace<Model>(m_playerEntity, shipModel);
		
		m_registry.emplace<ShipInput>(m_playerEntity); // Reads from global input and maps to ship movement inputs

		ShipConfig shipConfig; // Using default initialised values
		m_registry.emplace<ShipConfig>(m_playerEntity, shipConfig);

		r32 collisionRadius = 1.0f;
		r32 collisionLength = 2.5f;
		//btConvexShape *shipCollision = CreateCapsuleZAxisCollision(collisionRadius, collisionLength);
		btConvexShape *shipCollision = CreateCylinderZAxisCollision(collisionRadius, collisionLength);
		btPairCachingGhostObject *ghostObject = m_physics->CreateGhostObject(shipCollision);
		ShipPhysics &shipPhyics = m_registry.emplace<ShipPhysics>(m_playerEntity, shipCollision, ghostObject, shipConfig);
		m_physics->AddAction((btActionInterface *)&shipPhyics);

		Camera camera;
		camera.position = {0.0f, 0.0f, 0.0f};
		camera.target = Vec3Forward;
		camera.up = {0.0f, 1.0f, 0.0f};
		camera.fovy = 45.0f;
		camera.projection = CAMERA_PERSPECTIVE;
		
		m_registry.emplace<Camera>(m_playerEntity, camera);

		Vector3 cameraArmOffsetVec = { 2.5f, 3.0f, 10.0f };
		CameraArm cameraArm(cameraArmOffsetVec);
		m_registry.emplace<CameraArm>(m_playerEntity, cameraArm);
	}

	std::vector<Model> asteroidModels = LoadAsteroidModels();

	r32 asteroidScale = 10.0f;
	std::vector<btConvexShape *> asteroidCollisions = CreateAsteroidCollisions(asteroidModels, asteroidScale);

	BoundingBox asteroidPositionBounds = {};
	asteroidPositionBounds.min.x = -200.0;
	asteroidPositionBounds.max.x = 200.0;
	asteroidPositionBounds.min.y = -200.0;
	asteroidPositionBounds.max.y = 200.0;
	asteroidPositionBounds.min.z = -200.0;
	asteroidPositionBounds.max.z = 200.0;

	constexpr u32 asteroidsToCreate = 200;
	for(u32 entityNum = 0; entityNum < asteroidsToCreate; ++entityNum)
	{
		entt::entity entity = m_registry.create();

		Vector3 translation = RandomVec3InBoundingBox(asteroidPositionBounds);

		Vector3 randomAxis = {(r32)GetRandomValue(0, 100), (r32)GetRandomValue(0, 100), (r32)GetRandomValue(0, 100)};
		randomAxis = Vector3Normalize(randomAxis);
		r32 randomAngle = (r32)GetRandomValue(0, 180);
		Quaternion rotation = QuaternionFromAxisAngle(randomAxis, randomAngle);

		Vector3 scale = {asteroidScale, asteroidScale, asteroidScale};

		m_registry.emplace<Transform>(entity, translation, rotation, scale);

		r32 mass = 10.0f;

		u32 randAsteroidNum = GetRandomValue(0, (u32)asteroidModels.size() - 1);
		btConvexShape *collisionShape = asteroidCollisions.at(randAsteroidNum);

		RigidBody rigidBody = m_physics->CreateRigidBody(translation, rotation, mass, collisionShape);
		m_registry.emplace<RigidBody>(entity, rigidBody);

		m_registry.emplace<Model>(entity, asteroidModels.at(randAsteroidNum));

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


void Game::UpdateAndDraw()
{
	OPTICK_EVENT();

	r64 frameTime = GetTime();
	r32 dt = (r32)(frameTime - m_lastFrameTime);
	m_lastFrameTime = frameTime;

	if(IsKeyPressed(KEY_F1)) { debugFlags.drawCollision = !debugFlags.drawCollision; }

	// Read input and map to actions
	{
		OPTICK_EVENT("UpdateShipInput");

		auto view = m_registry.view<ShipInput>();
		view.each([](ShipInput &shipInput) {

			Vector2 mouseDelta = GetMouseDelta();

			constexpr r32 mouseSensitivity = 0.15f;
			mouseDelta = Vector2Scale(mouseDelta, mouseSensitivity);

			shipInput.inputs[ShipInput::THRUST_Z] = 0.0f;
			if(IsKeyDown(KEY_W)) { shipInput.inputs[ShipInput::THRUST_Z] -= 1.0f; }
			if(IsKeyDown(KEY_S)) { shipInput.inputs[ShipInput::THRUST_Z] += 1.0f; }

			shipInput.inputs[ShipInput::THRUST_X] = 0.0f;
			if(IsKeyDown(KEY_A)) { shipInput.inputs[ShipInput::THRUST_X] -= 1.0f; }
			if(IsKeyDown(KEY_D)) { shipInput.inputs[ShipInput::THRUST_X] += 1.0f; }

			shipInput.inputs[ShipInput::THRUST_Y] = 0.0f;
			if(IsKeyDown(KEY_LEFT_SHIFT)) { shipInput.inputs[ShipInput::THRUST_Y] += 1.0f; }
			if(IsKeyDown(KEY_LEFT_CONTROL)) { shipInput.inputs[ShipInput::THRUST_Y] -= 1.0f; }

			shipInput.inputs[ShipInput::PITCH] = 0.0f;
			shipInput.inputs[ShipInput::YAW] = 0.0f;
			shipInput.inputs[ShipInput::ROLL] = 0.0f;

			shipInput.inputs[ShipInput::PITCH] = -mouseDelta.y;
			shipInput.inputs[ShipInput::YAW] = -mouseDelta.x;

			if(IsKeyDown(KEY_Q)) { shipInput.inputs[ShipInput::ROLL] += 1.0f; }
			if(IsKeyDown(KEY_E)) { shipInput.inputs[ShipInput::ROLL] -= 1.0f; }

			if(IsKeyPressed(MOUSE_BUTTON_LEFT)) { shipInput.inputs[ShipInput::FIRE] = 1.0f; }
			if(IsKeyPressed(MOUSE_BUTTON_RIGHT)) { shipInput.inputs[ShipInput::ALT_FIRE] = 1.0f; }
		});
	}

	// Update ship physics based of current ship input
	{
		OPTICK_EVENT("ApplyShipInput");
		auto view = m_registry.view<const ShipInput, const ShipConfig, ShipPhysics>();
		view.each([dt, this](const ShipInput &shipInput, const ShipConfig &shipConfig, ShipPhysics &shipPhysics) {
			shipPhysics.ApplyShipInput(shipInput);
		});
	}

	m_physics->Step(dt);

	// Apply physics update to our transforms
	{
		OPTICK_EVENT("UpdateTransforms")
		auto view = m_registry.view<const RigidBody, Transform>();
		view.each([this](const RigidBody &rigidBody, Transform &transform) {
			m_physics->UpdateTransform(rigidBody, transform);
		});

		auto shipView = m_registry.view<const ShipPhysics, Transform>();
		shipView.each([this](const ShipPhysics &shipPhysics, Transform &transform) {
			shipPhysics.FillTransform(transform);
		});
	}

	// Update Camera by attached transform
	{
		OPTICK_EVENT("UpdateCamera");

		auto view = m_registry.view<const Transform, CameraArm, Camera>();
		view.each([](const Transform &transform, CameraArm &cameraArm, Camera &camera) {

			if(!QuaternionEquals(cameraArm.currentRotation, transform.rotation))
			{
				constexpr r32 cameraLerpAmount = 0.08f;
				cameraArm.currentRotation = QuaternionLerp(cameraArm.currentRotation, transform.rotation, cameraLerpAmount);
				//cameraArm.currentRotation = QuaternionNlerp(cameraArm.currentRotation, transform.rotation, cameraLerpAmount);
			}

			Vector3 toCameraDir = Vector3RotateByQuaternion(cameraArm.baseDir, cameraArm.currentRotation);
			Vector3 toCameraVec = Vector3Scale(toCameraDir, cameraArm.currentDistance);
			camera.position = Vector3Add(transform.translation, toCameraVec);
			camera.up = Vector3RotateByQuaternion(cameraArm.baseUp, cameraArm.currentRotation);

			Vector3 transformFaceDir = Vector3RotateByQuaternion(Vec3Forward, transform.rotation);
			constexpr r32 inFrontAmount = 20.0f;
			Vector3 inFrontOfBoat = Vector3Scale(transformFaceDir, inFrontAmount);
			camera.target = Vector3Add(transform.translation, inFrontOfBoat);

		});
	}

	
	BeginDrawing();

	ClearBackground(BLACK);

	const Camera &camera = m_registry.get<const Camera>(m_playerEntity);

	BeginMode3D(camera);

	// Draw asteroids
	{
		OPTICK_EVENT("Draw");
		{
			OPTICK_EVENT("DrawModels");

			auto modelView = m_registry.view<const Model, const Transform>();
			modelView.each([](const Model &model, const Transform &transform) {
				Vector3 rotationAxis = {0.0f, 0.0f, 0.0f};
				r32 rotationAngle = 0.0f;
				QuaternionToAxisAngle(transform.rotation, &rotationAxis, &rotationAngle);
				rotationAngle *= RAD2DEG;
				DrawModelEx(model, transform.translation, rotationAxis, rotationAngle, transform.scale, WHITE);
				//DrawModelWiresEx(model, transform.translation, rotationAxis, rotationAngle, transform.scale, WHITE);
			});
		}

		if(debugFlags.drawCollision)
		{
			m_physics->DrawDebugInfo();
		}
	}
	EndMode3D();

	DrawFPS(10, 10);

	EndDrawing();
}
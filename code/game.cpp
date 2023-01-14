#include "game.h"

#include "raylib.h"
#include "raymath.h"


#include "optick/optick.h"

constexpr Vector3 Vec3Zero = {0.0f, 0.0f, 0.0f};
constexpr Vector3 Vec3Up = {0.0f, 1.0f, 0.0f};
constexpr Vector3 Vec3Down = {0.0f, -1.0f, 0.0f};
constexpr Vector3 Vec3Forward = {0.0f, 0.0f, 1.0f};
constexpr Vector3 Vec3Back = {0.0f, 0.0f, -1.0f};
constexpr Vector3 Vec3Left = {-1.0f, 0.0f, 0.0f};
constexpr Vector3 Vec3Right = {1.0f, 0.0f, 0.0f};

constexpr Quaternion QuatIdentity = {0.0f, 0.0f, 0.0f, 1.0f};


struct Physics
{
	Vector3 velocity = {0.0f,0.0f,0.0f};
};

struct CubeCollision
{
	BoundingBox bounds = {}; // TODO: Some default?
};

constexpr r32 cDefaultSphereRadius = 10.0f;
struct SphereCollision
{
	Vector3 offset = Vec3Zero;
	r32 radius = cDefaultSphereRadius;
};

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
		baseUp = Vector3Negate(Vector3CrossProduct(baseDir, Vec3Right));
		currentDistance = Vector3Length(cameraOffset);
		currentRotation = QuatIdentity;//QuaternionFromVector3ToVector3(Vec3Back, baseDir);

		minDistance = 2.0f; // TODO: pass in minDistance?
		maxDistance = currentDistance;
	};
};

struct ShipInput
{
	enum Action
	{
		THRUST,
		PITCH, // X Axis
		YAW, // Y Axis
		ROLL, // Z Axis

		FIRE,
		ALT_FIRE,

		ACTION_TOTAL
	};
	r32 inputs[ACTION_TOTAL] = {};
};

struct ShipConfig
{
	r32 thrustSpeed = 80.0f;
	r32 pitchRate = 25.0f;
	r32 yawRate = 25.0f;
	r32 rollRate = 30.0f;
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


Game::Game(u32 windowWidth, u32 windowHeight)
	: m_windowWidth(windowWidth), m_windowHeight(windowHeight)
{
	OPTICK_EVENT();

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

		SphereCollision sphereCollision;
		sphereCollision.radius = 1.75f;
		sphereCollision.offset.y = 1.0f;
		m_registry.emplace<SphereCollision>(m_playerEntity, sphereCollision);

		Model shipModel = LoadModelWithTextures("ship/pirate-ship-blender-v2.obj", "ship/pirate-ship-blender-v2.png", nullptr);
		m_registry.emplace<Model>(m_playerEntity, shipModel);

		m_registry.emplace<ShipInput>(m_playerEntity); // Reads from global input and maps to ship movement inputs

		ShipConfig shipConfig; // Using default initialised values
		m_registry.emplace<ShipConfig>(m_playerEntity, shipConfig);

		Camera camera;
		camera.position = {0.0f, 0.0f, 0.0f};
		camera.target = {0.0f, 0.0f, 1.0f};
		camera.up = {0.0f, 1.0f, 0.0f};
		camera.fovy = 45.0f;
		camera.projection = CAMERA_PERSPECTIVE;
		
		m_registry.emplace<Camera>(m_playerEntity, camera);

		Vector3 cameraArmOffsetVec = {-2.5, 3.0f, -10.0f};
		CameraArm cameraArm(cameraArmOffsetVec);
		m_registry.emplace<CameraArm>(m_playerEntity, cameraArm);
	}

	std::vector<Model> asteroidModels = LoadAsteroidModels();

	BoundingBox asteroidPositionBounds = {};
	asteroidPositionBounds.min.x = -500.0;
	asteroidPositionBounds.max.x = 500.0;
	asteroidPositionBounds.min.y = -500.0;
	asteroidPositionBounds.max.y = 500.0;
	asteroidPositionBounds.min.z = -500.0;
	asteroidPositionBounds.max.z = 500.0;

	r32 asteroidScale = 10.0f;
	r32 asteroidCollisionRadius = 7.5f;

	//constexpr u32 minSpeed = 10;
	//constexpr u32 maxSpeed = 100;

	constexpr u32 asteroidsToCreate = 300;
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

		//Vector3 direction = {};
		//direction.x = (r32)GetRandomValue(-1000, 1000);
		//direction.y = (r32)GetRandomValue(-1000, 1000);
		//direction = Vector3Normalize(direction);

		//r32 speed = (r32)GetRandomValue(minSpeed, maxSpeed);
		//Vector3 velocity = Vector3Scale(direction, speed);

		//m_registry.emplace<Physics>(entity, velocity);

		SphereCollision sphereCollision;
		sphereCollision.radius = asteroidCollisionRadius;
		m_registry.emplace<SphereCollision>(entity, sphereCollision);

		u32 randAsteroidNum = GetRandomValue(0, (u32)asteroidModels.size() - 1);
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

			shipInput.inputs[ShipInput::THRUST] = 0.0f;
			if(IsKeyDown(KEY_W)) { shipInput.inputs[ShipInput::THRUST] += 1.0f; }
			if(IsKeyDown(KEY_S)) { shipInput.inputs[ShipInput::THRUST] -= 1.0f; }

			shipInput.inputs[ShipInput::PITCH] = 0.0f;
			shipInput.inputs[ShipInput::YAW] = 0.0f;
			shipInput.inputs[ShipInput::ROLL] = 0.0f;

			//shipInput.inputs[ShipInput::PITCH] = mouseDelta.y;
			//shipInput.inputs[ShipInput::YAW] = -mouseDelta.x;

			if(IsKeyDown(KEY_UP)) { shipInput.inputs[ShipInput::PITCH] += 1.0f; }
			if(IsKeyDown(KEY_DOWN)) { shipInput.inputs[ShipInput::PITCH] -= 1.0f; }

			if(IsKeyDown(KEY_A)) { shipInput.inputs[ShipInput::YAW] += 1.0f; }
			if(IsKeyDown(KEY_D)) { shipInput.inputs[ShipInput::YAW] -= 1.0f; }

			if(IsKeyDown(KEY_LEFT)) { shipInput.inputs[ShipInput::ROLL] -= 1.0f; }
			if(IsKeyDown(KEY_RIGHT)) { shipInput.inputs[ShipInput::ROLL] += 1.0f; }

			if(IsKeyPressed(MOUSE_BUTTON_LEFT)) { shipInput.inputs[ShipInput::FIRE] = 1.0f; }
			if(IsKeyPressed(MOUSE_BUTTON_RIGHT)) { shipInput.inputs[ShipInput::ALT_FIRE] = 1.0f; }
		});
	}

	// Update ship transform based of current ship input
	{
		OPTICK_EVENT("UpdateShipTransform");

		auto view = m_registry.view<const ShipInput, const ShipConfig, Transform>();
		view.each([dt](const ShipInput &shipInput, const ShipConfig shipConfig, Transform &transform) {
			// Movement
			Vector3 faceDir = Vector3RotateByQuaternion(Vec3Forward, transform.rotation);

			r32 moveDelta = shipInput.inputs[ShipInput::THRUST] * shipConfig.thrustSpeed * dt;
			Vector3 moveVector = Vector3Scale(faceDir, moveDelta);
			transform.translation = Vector3Add(transform.translation, moveVector);

			// Rotation
			r32 pitchDegrees = shipInput.inputs[ShipInput::PITCH] * shipConfig.pitchRate * dt;
			r32 yawDegrees = shipInput.inputs[ShipInput::YAW] * shipConfig.yawRate * dt;
			r32 rollDegrees = shipInput.inputs[ShipInput::ROLL] * shipConfig.rollRate * dt;

			if(pitchDegrees != 0.0f || yawDegrees != 0.0f || rollDegrees != 0.0f)
			{
				Quaternion rotationDelta = QuaternionFromEuler(pitchDegrees * DEG2RAD, yawDegrees * DEG2RAD, rollDegrees * DEG2RAD);
				transform.rotation = QuaternionMultiply(transform.rotation, rotationDelta);
			}
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

	//// Move spheres around screen
	//{
	//	auto view = m_registry.view<Transform, Physics>();
	//	view.each([this, dt](Transform &transform, Physics &physics) {
	//		Vector3 &translation = transform.translation;
	//		translation = Vector3Add(translation, Vector3Scale(physics.velocity, dt));

	//		// After fully off screen, reset on other side it went off
	//		const Vector3 &scale = transform.scale;
	//		r32 resetXMin = -scale.x;
	//		r32 resetXMax = m_windowWidth + scale.x;
	//		r32 resetYMin = -scale.y;
	//		r32 resetYMax = m_windowHeight + scale.y;

	//		if(translation.x < resetXMin)
	//		{
	//			translation.x = resetXMax;
	//		}
	//		else if(translation.x > resetXMax)
	//		{
	//			translation.x = resetXMin;
	//		}

	//		if(translation.y < resetYMin)
	//		{
	//			translation.y = resetYMax;
	//		}
	//		else if(translation.y > resetYMax)
	//		{
	//			translation.y = resetYMin;
	//		}
	//	}); 
	//}

	BeginDrawing();

	ClearBackground(BLACK);

	const Camera &camera = m_registry.get<const Camera>(m_playerEntity);

	BeginMode3D(camera);
	// Draw asteroids
	{
		OPTICK_EVENT("Draw");
		{
			OPTICK_EVENT("DrawModels");

			auto modelView = m_registry.view<const Transform, const Model>();
			modelView.each([](const Transform &transform, const Model &model) {
				Vector3 rotationAxis = {0.0f, 0.0f, 0.0f};
				r32 rotationAngle = 0.0f;
				QuaternionToAxisAngle(transform.rotation, &rotationAxis, &rotationAngle);
				rotationAngle *= RAD2DEG;
				DrawModelEx(model, transform.translation, rotationAxis, rotationAngle, transform.scale, WHITE);
				//DrawModelWiresEx(model, transform.translation, rotationAxis, rotationAngle, transform.scale, WHITE);
			});
		}

		{
			if(debugFlags.drawCollision)
			{
				OPTICK_EVENT("DrawSphereCollision");
				auto debugCollisionView = m_registry.view<const SphereCollision, const Transform>();
				debugCollisionView.each([](const SphereCollision &sphereCollision, const Transform &transform)
				{
					constexpr u32 rings = 5;
					constexpr u32 slices = 12;
					Vector3 rotatedOffset = Vector3RotateByQuaternion(sphereCollision.offset, transform.rotation);
					Vector3 position = Vector3Add(transform.translation, rotatedOffset);
					DrawSphereWires(position, sphereCollision.radius, rings, slices, LIME);
				});
			}
		}
	}
	EndMode3D();


	DrawFPS(10, 10);

	EndDrawing();
}
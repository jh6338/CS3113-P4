#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <vector> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#define PLATFORM_COUNT 16
#define ENEMY_COUNT 3

/*
Joyce Huang 
March 29th, 2020
CS-UY 3113 Intro to Game Programming 

Project 4: Rise of the AI 
- 3 or more enemies 
- Each should have different behavior 
- Player must be able to get rid of enemies
- If player touches the enemy, gets shot, show text "Game Over" 
- If player gets rid of all the enemies, show text "You Win" 
- You can use whatever graphics/theme as long as you meet requirements
*/

struct GameState {
	Entity* player;
	Entity* platforms;
	Entity* enemies;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

bool success = false;
bool gameEnd = false;
bool start = true; 
int enemiesAlive = ENEMY_COUNT; 

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position);

GLuint fontTextureID;

GLuint LoadTexture(const char* filePath) {
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}


void Initialize() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("Project 4: Rise of the AI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);

	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	glUseProgram(program.programID);

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// Initialize Game Objects

	// Initialize Player
	state.player = new Entity();
	state.player->entityType = PLAYER;
	state.player->position = glm::vec3(-4, -1, 0);
	state.player->movement = glm::vec3(0);
	state.player->acceleration = glm::vec3(0, -9.81f, 0);
	state.player->speed = 1.75f;
	state.player->textureID = LoadTexture("george_0.png");

	state.player->animRight = new int[4]{ 3, 7, 11, 15 };
	state.player->animLeft = new int[4]{ 1, 5, 9, 13 };
	state.player->animUp = new int[4]{ 2, 6, 10, 14 };
	state.player->animDown = new int[4]{ 0, 4, 8, 12 };

	state.player->animIndices = state.player->animRight;
	state.player->animFrames = 4;
	state.player->animIndex = 0;
	state.player->animTime = 0;
	state.player->animCols = 4;
	state.player->animRows = 4;

	state.player->height = 0.8f;
	state.player->width = 0.8f;

	state.player->jumpPower = 6.0f;

	// platforms stuff 
	state.platforms = new Entity[PLATFORM_COUNT];
	GLuint platformTextureID = LoadTexture("platformPack_tile001.png");

	for (int i = 0; i < PLATFORM_COUNT; ++i) {
		state.platforms[i].entityType = PLATFORM;
		state.platforms[i].textureID = platformTextureID;
		if (i < 11) {
			state.platforms[i].position = glm::vec3(-5 + i, -3.25f, 0);
		}
		else if (i < 14) {
			state.platforms[i].position = glm::vec3((5 - i) + 11, -2.25f, 0);
		}
		else {
			state.platforms[i].position = glm::vec3((-5 + i) - 14, -2.25f, 0);
		}
	}

	for (int i = 0; i < PLATFORM_COUNT; ++i) {
		state.platforms[i].Update(0, NULL, NULL, 0);
	}

	// enemies 
	state.enemies = new Entity[ENEMY_COUNT];
	GLuint radishTextureID = LoadTexture("Radish_Run.png");

	state.enemies[0].entityType = ENEMY;
	state.enemies[0].textureID = radishTextureID;
	state.enemies[0].position = glm::vec3(2, -1, 0);
	state.enemies[0].acceleration = glm::vec3(0, -9.81f, 0);
	state.enemies[0].speed = 1;
	state.enemies[0].aiType = WALKER;
	state.enemies[0].aiState = WALKING;

	state.enemies[0].animIndices = new int[12]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	state.enemies[0].animIndex = 0;
	state.enemies[0].animFrames = 12;
	state.enemies[0].animTime = 0;
	state.enemies[0].animCols = 12;
	state.enemies[0].animRows = 1;

	// state.enemies[0].height = 1.0f; 
	// state.enemies[0].width = 1.0f; 

	GLuint shroomTextureID = LoadTexture("mushroom_run.png"); 

	state.enemies[1].entityType = ENEMY; 
	state.enemies[1].textureID = shroomTextureID; 
	state.enemies[1].position = glm::vec3(2, -1, 0); 
	state.enemies[1].acceleration = glm::vec3(0, -9.81f, 0); 
	state.enemies[1].speed = 1; 
	state.enemies[1].aiType = WAITANDGO; 
	state.enemies[1].aiState = IDLE; 

	state.enemies[1].animIndices = new int[16]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15 }; 
	state.enemies[1].animIndex = 0; 
	state.enemies[1].animFrames = 16; 
	state.enemies[1].animTime = 0; 
	state.enemies[1].animCols = 16; 
	state.enemies[1].animRows = 1; 

	GLuint birdTextureID = LoadTexture("FatBird_Idle.png"); 

	state.enemies[2].entityType = ENEMY;
	state.enemies[2].textureID = birdTextureID; 
	state.enemies[2].position = glm::vec3(4.25f, 2, 0); 
	state.enemies[2].speed = 0.1f; 
	state.enemies[2].aiType = UPANDDOWN; 

	state.enemies[2].animIndices = new int[8]{ 0, 1, 2, 3, 4, 5, 6, 7 }; 
	state.enemies[2].animIndex = 0; 
	state.enemies[2].animFrames = 8; 
	state.enemies[2].animTime = 0; 
	state.enemies[2].animCols = 8; 
	state.enemies[2].animRows = 1; 

	fontTextureID = LoadTexture("font1.png");

}

void ProcessInput() {

	state.player->movement = glm::vec3(0);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				// Move the player left
				break;

			case SDLK_RIGHT:
				// Move the player right
				break;

			case SDLK_SPACE:
				if (state.player->collidedBottom) {
					state.player->jump = true;
				}
				start = false; 
				break;
			}
			break; // SDL_KEYDOWN
		}
	}

	const Uint8* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_LEFT]) {
		state.player->movement.x = -1.0f;
		state.player->animIndices = state.player->animLeft;
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		state.player->movement.x = 1.0f;
		state.player->animIndices = state.player->animRight;
	}

	if (glm::length(state.player->movement) > 1.0f) {
		state.player->movement = glm::normalize(state.player->movement);
	}

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;
void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float deltaTime = ticks - lastTicks;
	lastTicks = ticks;

	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}

	while (deltaTime >= FIXED_TIMESTEP) {
		// Update. Notice it's FIXED_TIMESTEP. Not deltaTime
		// state.player->Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT);

		for (int i = 0; i < ENEMY_COUNT; ++i) {
			state.enemies[i].Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT);
		}

		state.player->Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT);

		deltaTime -= FIXED_TIMESTEP;
	}

	accumulator = deltaTime;

	state.player->CheckCollisionsX(state.enemies, ENEMY_COUNT);
	state.player->CheckCollisionsY(state.enemies, ENEMY_COUNT); 

	if (state.player->lastCollided == ENEMY) {
		if (state.player->collidedLeft || state.player->collidedRight || state.player->collidedTop) {
			state.player->isActive = false;
			gameEnd = true;
			success = false;
		} 
		else if (state.player->collidedBottom) {
			// enemy is no longer active 
			(state.player->lastCollidedEntity)->isActive = false; 
			enemiesAlive -= 1; 
		}
	}

	if (enemiesAlive == 0) {
		state.player->isActive = false; 
		gameEnd = true; 
		success = true; 
	}

}


void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	if (start) {
		DrawText(&program, fontTextureID, "Press Spacebar to jump", 0.5f, -0.25f, glm::vec3(-2.75f, 1.0f, 0)); 
		DrawText(&program, fontTextureID, "Jump on Enemies to defeat them", 0.5f, -0.25f, glm::vec3(-3.5f, 0.5f, 0)); 
	}

	for (int i = 0; i < PLATFORM_COUNT; ++i) {
		state.platforms[i].Render(&program);
	}

	for (int i = 0; i < ENEMY_COUNT; ++i) {
		state.enemies[i].Render(&program);
	}

	if (gameEnd) {
		if (success) {
			DrawText(&program, fontTextureID, "You Win!", 0.75f, -0.25f, glm::vec3(-2.0f, 0.5f, 0));
		}
		else {
			DrawText(&program, fontTextureID, "Game Over!", 0.75f, -0.25f, glm::vec3(-2.0f, 0.5f, 0));
		}
	}

	state.player->Render(&program);

	SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
	SDL_Quit();
}

void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position)
{
	float width = 1.0f / 16.0f;
	float height = 1.0f / 16.0f;

	std::vector<float> vertices;
	std::vector<float> texCoords;

	for (int i = 0; i < text.size(); i++) {

		int index = (int)text[i];
		float offset = (size + spacing) * i;
		float u = (float)(index % 16) / 16.0f;
		float v = (float)(index / 16) / 16.0f;
		vertices.insert(vertices.end(), {
		 offset + (-0.5f * size), 0.5f * size,
		 offset + (-0.5f * size), -0.5f * size,
		 offset + (0.5f * size), 0.5f * size,
		 offset + (0.5f * size), -0.5f * size,
		 offset + (0.5f * size), 0.5f * size,
		 offset + (-0.5f * size), -0.5f * size,
			});
		texCoords.insert(texCoords.end(), {
		u, v,
		u, v + height,
		u + width, v,
		u + width, v + height,
		u + width, v,
		u, v + height,
			});

	} // end of for loop

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program->SetModelMatrix(modelMatrix);

	glUseProgram(program->programID);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glBindTexture(GL_TEXTURE_2D, fontTextureID);
	glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}
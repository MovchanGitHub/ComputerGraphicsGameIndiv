#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>
#include <set>
#include "model.h"
#include "shader.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

const std::string tree_model_path = "data/12150_Christmas_Tree_V2_L2.obj";
const std::string tree_texture_path = "data/tree.jpg";

const std::string present_model_path = "data/giftbox_obj.obj";
const std::string present_texture_path = "data/teapot.png";

const std::string floor_model_path = "data/floor.obj";
const std::string floor_texture_path = "data/bus2.png";

const std::string airship_model_path = "data/15724_Steampunk_Vehicle_Dirigible_v1.obj";
const std::string airship_texture_path = "data/bus2.png";

const std::string target_model_path = "data/snowman.obj";
const std::string target_texture_path = "data/snowman.jpg";

enum class light_kind {PointLightSource, Spotlight, DirLightSource};
constexpr light_kind LIGHT_KIND = light_kind::PointLightSource;

enum class shader_kind {Phong, OrenNayar, Toon, ToonSpecular};
constexpr shader_kind SHADER_KIND = shader_kind::Phong;

Model tree_model;
Model floor_model;
Model airship_model;
Model present_model;
Model target_model;

struct Camera {
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
};

Camera free_camera { 
	glm::vec3(0.0f, 0.0f, 3.0f),
	glm::vec3(0.0f, 0.0f, -1.0f),
	glm::vec3(0.0f, 1.0f, 0.0f),
};

Camera airship_camera;

Camera* camera = &free_camera;

float yaw = -90.0f;
float pitch = 0.0f;

struct Light {
	glm::vec4 position;       // Позиция источника света
	glm::vec3 spotDirection;  // Направление прожектора
	float spotCosCutoff;      // Косинус угла отсечения
	float spotExponent;       // Коэффициент экспоненциального затухания
	glm::vec3 attenuation;    // Коэффициенты затухания (constant, linear, quadratic)
	glm::vec4 ambient;        // Фоновая составляющая
	glm::vec4 diffuse;        // Рассеянная составляющая
	glm::vec4 specular;       // Зеркальная составляющая
};

Light light = {
	glm::vec4(10.0f, 10.0f, 10.0f, 1.0f),  // Позиция прожектора (например, на высоте 5, по оси Y)
	glm::vec3(0.0f, -1.0f, 0.0f),  // Направление светового луча вниз по оси Y
	cos(glm::radians(40.0f)),  // Угол отсечения 30 градусов (косинус угла отсечения)
	1.0f,  // Коэффициент экспоненциального затухания (можно регулировать для более мягкого или резкого падения света)
	glm::vec3(.5f, 0.001f, 0.0001f),  // Коэффициенты затухания: (constant, linear, quadratic)
	glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),  // Фоновая составляющая (слабое освещение)
	glm::vec4(2.0f, 2.0f, 2.0f, 2.0f),  // Рассеянная составляющая (освещает объекты белым светом)
	glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // Зеркальная составляющая (белый свет для отражений)
};

// ID шейдерной программы
GLuint Program;

void InitShader() {
	if constexpr (SHADER_KIND == shader_kind::Phong) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/phong_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/phong_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/phong_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::OrenNayar) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/oren_nayar_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/oren_nayar_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/oren_nayar_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::Toon) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/toon_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/toon_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/toon_spot.frag");
	}
	else if constexpr (SHADER_KIND == shader_kind::ToonSpecular) {
		if constexpr (LIGHT_KIND == light_kind::PointLightSource)
			Program = load_shaders("shaders/phong_point.vert", "shaders/toon_spec_point.frag");
		if constexpr (LIGHT_KIND == light_kind::DirLightSource)
			Program = load_shaders("shaders/phong_dir.vert", "shaders/toon_spec_dir.frag");
		if constexpr (LIGHT_KIND == light_kind::Spotlight)
			Program = load_shaders("shaders/phong_spot.vert", "shaders/toon_spec_spot.frag");
	}
}

glm::vec3 airship_position = glm::vec3(0.0f, 3.0f, 0.0f);
bool dont_draw_airship = false;
bool airship_dir = +1;
glm::vec3 present_position;
std::vector<glm::vec3> targets;
bool present_exists = false;
float target_radius = 0.5f;
float present_radius = 0.5f;
constexpr int TARGETS_COUNT = 5;

void SpawnNewTarget() {
	static int border = 20;
	static std::random_device dev;
	static std::mt19937 rng(dev());
	static std::uniform_int_distribution<std::mt19937::result_type> dist(0, 2 * border);

	while (true) {
		bool sucess = true;
		int rval = dist(rng) - border;
		if (rval == 0) continue;
		for (auto& target : targets) {
			if (target.x == rval) {
				sucess = false;
				break;
			}
		}

		if (sucess) {
			targets.emplace_back(rval, 0, 0);
			return;
		}
	}
}

void Update() {
	static float time = 0;
	static float eps = 1e-4;
	static float airship_speed = 0.065;
	static float present_fall_speed = 0.065;
	constexpr static float delta_time_to_turn = 650;
	static float next_turn_time = delta_time_to_turn;
	++time;

	// update airship position
	if (abs((time + delta_time_to_turn / 2) - next_turn_time) < eps) {
		next_turn_time += delta_time_to_turn;
		airship_dir = !airship_dir;
		dont_draw_airship = true;
	}
	if (airship_dir)
		airship_position[0] += airship_speed;
	else
		airship_position[0] -= airship_speed;

	// update airship camera position
	if (camera == &airship_camera) {
		glm::vec3 new_camera_pos = airship_position;
		new_camera_pos.y += 5;
		new_camera_pos.x -= airship_dir ? 7 : -7;
		camera->cameraPos = new_camera_pos;
		camera->cameraFront = glm::vec3((airship_dir ? 1.f : -1.f), -1.f, .0f);
		camera->cameraUp = glm::vec3((airship_dir ? 1.f : -1.f), 1.f, .0f);
	}

	// update present position
	if (present_exists) {
		present_position.y -= present_fall_speed;
		if (present_position.y < 0)
			present_exists = false;
		else {
			for (auto it = targets.begin(); it != targets.end(); ++it) {
				if (glm::distance(present_position, *it) <
					target_radius * target_radius + present_radius * present_radius) {
					targets.erase(it);
					std::cout << "+1 point\n";
					SpawnNewTarget();
					break;
				}
			}
		}
	}
}

void InitScene() {
	for (int i = 0; i < TARGETS_COUNT; ++i) {
		SpawnNewTarget();
	}
}

void InitModels() {
	tree_model = Model(tree_model_path, tree_texture_path);
	floor_model = Model(floor_model_path, floor_texture_path);
	airship_model = Model(airship_model_path, airship_texture_path);
	present_model = Model(present_model_path, present_texture_path);
	target_model = Model(target_model_path, target_texture_path);
}

void Init() {
	// Шейдеры
	InitShader();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5, 0.5, 0.5, 0.0);
	InitModels();
	//glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
	//glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 0.0f));
	//glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	//glUseProgram(Program);
	//glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
	//glUniformMatrix4fv(glGetUniformLocation(Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	//glUniformMatrix4fv(glGetUniformLocation(Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	//glUseProgram(0);
}

float angleX = 0.0f;
float angleY = 0.0f;

float aspectRatio;

void DrawModel(const Model& object, const glm::mat4& model, GLuint Program) {
	const glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(Program, "transform.normal"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	object.display_model(Program);
}

void Draw() {
	glUseProgram(Program); // Устанавливаем шейдерную программу текущей
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	free_camera.cameraFront = glm::normalize(front);

	glm::mat4 model;

	glm::mat4 view = glm::lookAt(camera->cameraPos, camera->cameraPos + camera->cameraFront, camera->cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);


	//model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, -15.0f));
	//model = glm::translate(model, glm::vec3(0.0f, 5.5f, 3.5f));
	//model = glm::rotate(model, glm::radians(180.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	//DrawModel(model0, model, Program);
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, -3.0f, 3.0f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 1.0f));
	//model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	//DrawModel(model1, model, Program);
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f));
	//model = glm::scale(model, glm::vec3(0.16f, 0.16f, 0.16f));
	//DrawModel(model2, model, Program);
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, -0.1f));
	//model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
	//DrawModel(model2, model, Program);
	//
	model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	DrawModel(tree_model, model, Program);

	if (present_exists) {
		model = glm::translate(glm::mat4(1.0f), present_position);
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		DrawModel(present_model, model, Program);
	}
	
	model = glm::mat4(1.0f);
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
	DrawModel(floor_model, model, Program);

	if (dont_draw_airship)
		dont_draw_airship = false;
	else {
		model = glm::translate(glm::mat4(1.0f), airship_position);
		model = glm::rotate(model, glm::radians(airship_dir ? 180.0f : 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(.3f, .3f, .3f));
		DrawModel(airship_model, model, Program);
	}

	for (auto& target : targets) {
		model = glm::translate(glm::mat4(1.0f), target);
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		DrawModel(target_model, model, Program);
	}
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 3.5f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	//model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	//DrawModel(tree_model, model, Program);
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 3.5f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	//model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	//DrawModel(tree_model, model, Program);
	//
	//model = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 3.5f));
	//model = glm::translate(model, glm::vec3(-30.0f, 30.0f, 30.0f));
	//model = glm::rotate(model, glm::radians(165.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	//model = glm::rotate(model, glm::radians(65.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
	//DrawModel(model0, model, Program);

	glUniformMatrix4fv(glGetUniformLocation(Program, "transform.viewProjection"), 1, GL_FALSE, glm::value_ptr(projection * view));
	glUniform3fv(glGetUniformLocation(Program, "transform.viewPosition"), 1, glm::value_ptr(camera->cameraPos));

	glUniform4fv(glGetUniformLocation(Program, "light.position"), 1, glm::value_ptr(light.position));
	glUniform4fv(glGetUniformLocation(Program, "light.ambient"), 1, glm::value_ptr(light.ambient));
	glUniform4fv(glGetUniformLocation(Program, "light.diffuse"), 1, glm::value_ptr(light.diffuse));
	glUniform4fv(glGetUniformLocation(Program, "light.specular"), 1, glm::value_ptr(light.specular));
	if constexpr (LIGHT_KIND != light_kind::DirLightSource)
		glUniform3fv(glGetUniformLocation(Program, "light.attenuation"), 1, glm::value_ptr(light.attenuation));

	if constexpr (LIGHT_KIND == light_kind::Spotlight) {
		glUniform3fv(glGetUniformLocation(Program, "light.spotDirection"), 1, glm::value_ptr(light.spotDirection)); // Направление прожектора
		glUniform1f(glGetUniformLocation(Program, "light.spotCosCutoff"), light.spotCosCutoff); // Косинус угла отсечения
		glUniform1f(glGetUniformLocation(Program, "light.spotExponent"), light.spotExponent); // Экспоненциальное затухание
	}

	glUniform1i(glGetUniformLocation(Program, "material.texture"), 0);
	glUniform4f(glGetUniformLocation(Program, "material.ambient"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.diffuse"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.specular"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(glGetUniformLocation(Program, "material.emission"), 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(glGetUniformLocation(Program, "material.shininess"), 32.0f);

	if constexpr (SHADER_KIND == shader_kind::OrenNayar)
		glUniform1f(glGetUniformLocation(Program, "roughness"), 0.6f); // От 0 до 1

	glUseProgram(0); // Отключаем шейдерную программу
}


// Освобождение шейдеров
void ReleaseShader() {
	// Передавая ноль, мы отключаем шейдерную программу
	glUseProgram(0);
	// Удаляем шейдерную программу
	glDeleteProgram(Program);
}

void Release() {
	// Шейдеры
	ReleaseShader();
	// Вершинный буфер
	//model0.release();
}

void HandleKeyboardInput() {
	constexpr float cameraSpeed = 0.3f;
	float cameraShiftScale = 0.5f;
	float rotationSpeed = 0.75f;
	constexpr float lightSpeed = 0.2f;
	static int change_camera_cool_down = 0;

	if (change_camera_cool_down > 0)
		--change_camera_cool_down;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
		cameraShiftScale *= 2;
		rotationSpeed *= 2;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && !change_camera_cool_down) {
		change_camera_cool_down = 20;
		if (camera == &free_camera)
			camera = &airship_camera;
		else
			camera = &free_camera;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !present_exists) {
		present_exists = true;
		present_position = airship_position;
		present_position.y -= 0.2;
	}

	if (camera == &free_camera) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) free_camera.cameraPos += cameraSpeed * free_camera.cameraFront * cameraShiftScale;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) free_camera.cameraPos -= cameraSpeed * free_camera.cameraFront * cameraShiftScale;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) free_camera.cameraPos -= glm::normalize(glm::cross(free_camera.cameraFront, free_camera.cameraUp)) * cameraSpeed * cameraShiftScale;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) free_camera.cameraPos += glm::normalize(glm::cross(free_camera.cameraFront, free_camera.cameraUp)) * cameraSpeed * cameraShiftScale;
	

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) pitch += rotationSpeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) pitch -= rotationSpeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) yaw -= rotationSpeed;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) yaw += rotationSpeed;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) light.position[0] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) light.position[0] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) light.position[1] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) light.position[1] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) light.position[2] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) light.position[2] -= lightSpeed;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

int main() {
	sf::Window window(sf::VideoMode(900, 900), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
	window.setVerticalSyncEnabled(true);
	window.setActive(true);
	glewInit();
	Init();
	InitScene();

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) { window.close(); }
			else if (event.type == sf::Event::Resized) { glViewport(0, 0, event.size.width, event.size.height); }
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sf::Vector2u windowSize = window.getSize();
		aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		HandleKeyboardInput();
		Update();
		Draw();
		window.display();
	}
	Release();
	return 0;
}
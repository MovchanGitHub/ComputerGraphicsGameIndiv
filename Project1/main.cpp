﻿#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <random>
#include "model.h"
#include "shader.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Model model0;
Model model1;
Model model2;
Model tree_model;
Model floor_model;
Model airship_model;

const std::string model_path = "data/Cearadactylus.obj";
const std::string texture_path = "data/Cearadactylus.jpg";
const std::string model_path1 = "data/Elasmosaurus.obj";
const std::string texture_path1 = "data/Elasmosaurus.jpg";
const std::string model_path2 = "data/Triceratops.obj";
const std::string texture_path2 = "data/Triceratops.png";
const std::string model_path3 = "data/treeBirch.obj";
const std::string texture_path3 = "data/treeBirch.jpg";

const std::string tree_model_path = "data/12150_Christmas_Tree_V2_L2.obj";
const std::string tree_texture_path = "data/tree.jpg";

const std::string floor_model_path = "data/floor.obj";
const std::string floor_texture_path = "data/bus2.png";

const std::string airship_model_path = "data/15724_Steampunk_Vehicle_Dirigible_v1.obj";
const std::string airship_texture_path = "data/New Bitmap Image.jpg";

enum class light_kind {PointLightSource, Spotlight, DirLightSource};
constexpr light_kind LIGHT_KIND = light_kind::PointLightSource;

enum class shader_kind {Phong, OrenNayar, Toon, ToonSpecular};
constexpr shader_kind SHADER_KIND = shader_kind::Phong;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
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

glm::vec3 airship_position = glm::vec3(0.0f, 5.0f, 0.0f);
bool airship_dir = +1;
void Update() {
	static int time = 0;
	static float airship_speed = 0.1;
	constexpr static int delta_time_to_turn = 100;
	++time;
	if ((time + delta_time_to_turn / 2) % delta_time_to_turn == 0)
		airship_dir = !airship_dir;
	if (airship_dir)
		airship_position[0] += airship_speed;
	else
		airship_position[0] -= airship_speed;
}

void InitModels() {
	//model0 = Model(model_path, texture_path);
	//model1 = Model(model_path1, texture_path1);
	//model2 = Model(model_path2, texture_path2);
	tree_model = Model(tree_model_path, tree_texture_path);
	floor_model = Model(floor_model_path, floor_texture_path);
	airship_model = Model(airship_model_path, airship_texture_path);
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
	cameraFront = glm::normalize(front);

	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glUniformMatrix4fv(glGetUniformLocation(Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

	glm::mat4 model;

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
	
	model = glm::mat4(1.0f);
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
	DrawModel(floor_model, model, Program);

	model = glm::translate(glm::mat4(1.0f), airship_position);
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(.3f, .3f, .3f));
	DrawModel(airship_model, model, Program);
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
	glUniform3fv(glGetUniformLocation(Program, "transform.viewPosition"), 1, glm::value_ptr(cameraPos));

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
	model0.release();
}

void HandleKeyboardInput() {
	constexpr float cameraSpeed = 0.3f;
	float cameraShiftScale = 0.5f;
	float rotationSpeed = 0.75f;
	constexpr float lightSpeed = 0.2f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
		cameraShiftScale *= 2;
		rotationSpeed *= 2;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += cameraSpeed * cameraFront * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= cameraSpeed * cameraFront * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraShiftScale;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * cameraShiftScale;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) pitch += rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) pitch -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) yaw -= rotationSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) yaw += rotationSpeed;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) light.position[0] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::N)) light.position[0] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) light.position[1] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) light.position[1] -= lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) light.position[2] += lightSpeed;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::K)) light.position[2] -= lightSpeed;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

void setupProjection(int width, int height) {
	// Устанавливаем перспективную проекцию
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Устанавливаем frustum (перспективу)
	float nearVal = 0.1f; // Ближняя плоскость отсечения
	float farVal = 1.0f; // Дальняя плоскость отсечения
	float fov = 45.0f; // Угол обзора по вертикали

	float aspectRatio = (float)width / (float)height;
	float top = tan(fov * 0.5f * 3.14 / 180.0f) * nearVal;
	float bottom = -top;
	float right = top * aspectRatio;
	float left = -right;

	// Устанавливаем frustum с заданными параметрами
	glFrustum(left, right, bottom, top, nearVal, farVal);

	// Включаем режим модели просмотра
	glMatrixMode(GL_MODELVIEW);
}

int main() {
	sf::Window window(sf::VideoMode(900, 900), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
	window.setVerticalSyncEnabled(true);
	window.setActive(true);
	glewInit();
	Init();

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) { window.close(); }
			else if (event.type == sf::Event::Resized) { glViewport(0, 0, event.size.width, event.size.height); }
		}
		HandleKeyboardInput();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		sf::Vector2u windowSize = window.getSize();
		setupProjection(windowSize.x, windowSize.y);
		aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
		Draw();
		Update();
		window.display();
	}
	Release();
	return 0;
}
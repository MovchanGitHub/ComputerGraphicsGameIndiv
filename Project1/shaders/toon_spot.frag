#version 330 core

#define FRAG_OUTPUT0 0

layout (location = FRAG_OUTPUT0) out vec4 color;

uniform struct SpotLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 attenuation;
    vec3 spotDirection;
    float spotCosCutoff;
    float spotExponent;
} projector;

uniform struct DirLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} light;

uniform struct Material {
    sampler2D texture;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emission;
    float shininess;
} material;

in Vertex {
    vec2 texcoord;
    vec3 normal;
    vec3 projectorDir;
	vec3 lightDir;
    vec3 viewDir;
    float distance;
} Vert;

void main() {
    vec3 normal = normalize(Vert.normal);
    vec3 projectorDir = normalize(Vert.projectorDir);

    // Направление света от прожектора
    vec3 spotDir = normalize(projector.spotDirection);
    // Угол между направлением прожектора и направлением к точке
    float spotEffect = dot(spotDir, -projectorDir);
    // Ограничение зоны влияния прожектора
    spotEffect = float(spotEffect > projector.spotCosCutoff);
    // Экспоненциальное затухание
    spotEffect = max(pow(spotEffect, projector.spotExponent), 0.0);

    // Коэффициент затухания прожектора
    float attenuation = spotEffect * (1.0 / max(projector.attenuation[0] + 
        projector.attenuation[1] * Vert.distance + 
        projector.attenuation[2] * Vert.distance * Vert.distance, 0.0001));

    vec4 toonColor;
    float Ndot = max(dot(normal, projectorDir), 0.0);
    if (Ndot > 0.9)
        toonColor = material.diffuse * projector.diffuse;
    else if (Ndot > 0.6)
        toonColor = material.diffuse * projector.diffuse * 0.6;
    else
        toonColor = material.diffuse * projector.diffuse * 0.1;

    color = material.emission;
    color += material.ambient * projector.ambient * attenuation;
    color += toonColor * attenuation;
	
	
    vec3 lightDir = normalize(Vert.lightDir);

    Ndot = max(dot(normal, lightDir), 0.0);
    if (Ndot > 0.9)
        color += material.diffuse * light.diffuse;
    else if (Ndot > 0.6)
        color += material.diffuse * light.diffuse * 0.6;
    else
        color += material.diffuse * light.diffuse * 0.1;
	
	
    color *= texture(material.texture, Vert.texcoord);
}
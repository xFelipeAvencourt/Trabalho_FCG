#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Parâmetros adicionais para o cone de luz
uniform vec3 light_position;
uniform vec3 light_color;
uniform vec3 light_direction;

uniform float light_intensity;
uniform float light_cutoff_angle;
uniform float light_outer_cutoff;
uniform float light_range;

// Identificador que define qual objeto está sendo desenhado no momento
#define PLANE  1
#define WALL   2
#define TABLE  3
#define LAMP   4
#define DOOR   5
uniform int object_id;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{

    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;
    vec4 p = position_world;

    vec4 n = normalize(normal);

    vec3 p3 = p.xyz;
    vec3 n3 = normalize(n.xyz);

    vec3 l3 = normalize(light_position - p3);

    vec3 v3 = normalize((camera_position - p).xyz);

    float U = 0.0;
    float V = 0.0;

    vec3 Kd0 = vec3(0.8, 0.8, 0.8);
    vec3 Kd1 = vec3(0.0, 0.0, 0.0);

/////////////////////////////////////////////
//  TEXTURAS
/////////////////////////////////////////////
    
    if ( object_id == PLANE || object_id == WALL){
        float tiling = 4.0;
        U = texcoords.x * tiling;
        V = texcoords.y * tiling;
    }
    else if( object_id == TABLE || object_id == DOOR || object_id == LAMP){
        float tiling = 1.0;
        U = texcoords.x * tiling;
        V = texcoords.y * tiling;
    }
    if ( object_id == PLANE ){
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        Kd1 = vec3(0.0);
    }
    else if ( object_id == WALL ) {
        Kd0 = texture(TextureImage1, vec2(U,V)).rgb;
        Kd1 = vec3(0.0);
    }
    else if ( object_id == TABLE){
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
        Kd1 = vec3(0.0);
    }
    else if ( object_id == LAMP) {
        Kd0 = texture(TextureImage3, vec2(U,V)).rgb;
        Kd1 = vec3(0.0);
    }
    else if ( object_id == DOOR ) {
        Kd0 = texture(TextureImage4, vec2(U,V)).rgb;
        Kd1 = vec3(0.0);
    }
/////////////////////////////////////////////
//  LUZ
/////////////////////////////////////////////

    float cone_intensity = 1.0;
    float distance_attenuation = 1.0;

    if (object_id == TABLE || object_id == PLANE) {

        vec3 light_to_point = normalize(p3 - light_position);
        vec3 spot_dir = normalize(light_direction);
        float cosTheta = dot(light_to_point, spot_dir);
        float cosInner = cos(light_cutoff_angle);
        float cosOuter = cos(light_outer_cutoff);

        if (cosInner < cosOuter){
            float tmp = cosInner; cosInner = cosOuter; cosOuter = tmp;
        }

        float smoothFactor = max(0.0001, cosInner - cosOuter);
        cone_intensity = clamp((cosTheta - cosOuter) / smoothFactor, 0.0, 1.0);
        float distance = length(light_position - p3);
        distance_attenuation = 1.0 / (1.0 + 0.5 * distance + 0.01 * distance * distance);
        float range_attenuation = 1.0 - smoothstep(light_range * 0.8, light_range, distance);

        cone_intensity *= distance_attenuation * range_attenuation;

        if (object_id == PLANE)
            cone_intensity *= 0.3;
    }

    float lambert = max(0.0, dot(n3, l3));
    vec3 base = Kd0 * lambert + Kd1 * (1.0 - lambert);
    vec3 diffuse = base * light_color * light_intensity;
    vec3 ambient = Kd0 * 0.01;

    if (object_id == TABLE || object_id == PLANE)
        diffuse *= cone_intensity;

    if (object_id == WALL || object_id == DOOR) {
        float distance_wall = length(light_position - p3);
        float wall_atten = 1.0 / (1.0 + 10 * distance_wall + 0.1 * distance_wall * distance_wall);
        diffuse *= wall_atten;
    }

    if ((object_id == TABLE || object_id == PLANE) && cone_intensity < 0.1)
        color.rgb = ambient;
    else
        color.rgb = diffuse + ambient;
    color.a = 1;
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}
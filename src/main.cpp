#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "Personagem.h"
#include "Constantes.h"
#include "Collisions.h"
#include "SceneObject.h"

#define DADOS "INF01047 - 00342904 - Felipe Avencourt && Fernando Fink"

using namespace std;

// Estrutura que representa um modelo geométrico carregado a partir de um arquivo ".obj".
struct ObjModel
{
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);
        string fullpath(filename);
        string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        string warn;
        string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

enum class GameState {
    START_MENU,
    GAME_PLAY,
    GHOST_MODE,
    GAME_OVER
};
enum class DeathCause {
    NONE,
    DARDO,
    TIMEOUT
};


void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

///////////////////////////////////////////////////////////////////////
// DEFINIÇÃO DE FUNÇÕES RELACIONADAS AO CARREGAMENTO E RENDERIZAÇÃO DE OBJETOS 3D
///////////////////////////////////////////////////////////////////////

void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

////////////////////////////////////////////////////////////////////////
///////////////////// Funções auxiliares ///////////////////////////////
///////////////////////////////////////////////////////////////////////

void Dardos(bool &door, bool &exibirDardos);
void SalaPrincipal(bool &door, bool &exibirDardos);
void Alavanca(GLFWwindow* window, bool &door, bool &armadilhasDesativadas);
void DrawOBJ(int objectId,const std::vector<std::string>& parts,const glm::vec3& position,float size,const glm::vec3& rotation);
void Tempo(GLFWwindow* window);
void Porta(bool &door);
void Telas(GLFWwindow* window);
void allowPlayerMovement(int key, bool pressed);
///////////////////////////////////////////////////////////////////////
/////////////////////////Matrizes//////////////////////////////////////
///////////////////////////////////////////////////////////////////////

map<string, SceneObject> g_VirtualScene;
std::map<int, AABB> g_listaAABB;
// Pilha que guardará as matrizes de modelagem.
stack<glm::mat4>  g_MatrixStack;

///////////////////////////////////////////////////////////////////////
/////////////////////////CAMERA////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

double g_LastCursorPosX, g_LastCursorPosY;

// Variavéis Camera esféricas
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

Camera Player(glm::vec3(0.0f, 0.0f, 4.0f));
float deltaTime = 0.0f, lastFrameTime = 0.0f;

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;
bool  g_ShowInfoText = true;

bool  g_LeftMouseButtonPressed = false;
bool  g_RightMouseButtonPressed = false; 
bool  g_MiddleMouseButtonPressed = false;

///////////////////////////////////////////////////////////////////////
/////////////////////////MOVIMENTO/////////////////////////////////////
///////////////////////////////////////////////////////////////////////

bool  g_WPressed = false;
bool  g_SPressed = false;
bool  g_DPressed = false;
bool  g_APressed = false;
bool  g_ctrlPressed = false;
bool  g_SpacePressed = false;
bool  g_IsJumping = false;
float g_JumpVelocity = 0.0f;
bool  g_ghost = false;
bool  g_showLeverText = false;
bool  g_leverActivated = false;
float g_LeverAngle = 0.0f;
float g_DoorX = BASE_DOOR_X;
float g_DoorY = BASE_DOOR_Y;
float g_DoorZ = BASE_DOOR_Z;
float g_DoorAngle = 0.0f;
float g_LeverTargetAngle = 0.0f;
float g_LeverSpeed = glm::radians(90.0f);
float g_startGamePlayTime = 0.0f;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// LUZ
GLint g_light_direction_uniform;
GLint g_light_cutoff_angle_uniform;
GLint g_light_outer_cutoff_uniform;
GLint g_light_range_uniform;

GameState g_GameState = GameState::START_MENU;
GameState g_GameStateBeforeGhost = GameState::START_MENU;
DeathCause g_DeathCause = DeathCause::NONE;

// Tempo
float g_LeverAtivationTime = 0.0f;

GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[]) {
    // Inicialização da biblioteca GLFW, que gerencia a janela
    int success = glfwInit();
    if (!success){
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(WIDTH, HEIGHT, DADOS, NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, WIDTH, HEIGHT); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    LoadTextureImage("../../data/Textures/chao.png");
    LoadTextureImage("../../data/Textures/brick.png");
    LoadTextureImage("../../data/Textures/round_table.png");
    LoadTextureImage("../../data/Textures/lamp_grey.jpg");
    LoadTextureImage("../../data/Textures/door_txt.jpg");
    LoadTextureImage("../../data/Textures/teto.jpg");
    LoadTextureImage("../../data/Textures/mine.png");
    LoadTextureImage("../../data/Textures/Dardo.jpg");

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel ceilingmodel("../../data/ceiling.obj");
    ComputeNormals(&ceilingmodel);
    BuildTrianglesAndAddToVirtualScene(&ceilingmodel);

    ObjModel wallmodel("../../data/wall.obj");
    ComputeNormals(&wallmodel);
    BuildTrianglesAndAddToVirtualScene(&wallmodel);

    ObjModel tablemodel("../../data/table.obj");
    ComputeNormals(&tablemodel);
    BuildTrianglesAndAddToVirtualScene(&tablemodel);

    ObjModel doormodel("../../data/door.obj");
    ComputeNormals(&doormodel);
    BuildTrianglesAndAddToVirtualScene(&doormodel);

    ObjModel lampmodel("../../data/lamp.obj");
    ComputeNormals(&lampmodel);
    BuildTrianglesAndAddToVirtualScene(&lampmodel);

    ObjModel levermodel("../../data/lever.obj");
    ComputeNormals(&levermodel);
    BuildTrianglesAndAddToVirtualScene(&levermodel);

    /*
    ObjModel playermodel("../../data/personagem.obj");
    ComputeNormals(&playermodel);
    BuildTrianglesAndAddToVirtualScene(&playermodel);
    */

    ObjModel trapmodel("../../data/Dardo.obj");
    ComputeNormals(&trapmodel);
    BuildTrianglesAndAddToVirtualScene(&trapmodel);

    bool door = false;
    bool armadilhasDesativadas = false;
    bool exibirDardos = true;
   
    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while (!glfwWindowShouldClose(window))
    {
        #define PERSON 7
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        float lastTime = (float)glfwGetTime();
        deltaTime = lastTime - lastFrameTime;
        lastFrameTime = lastTime;

        
        if (g_WPressed)
            Player.ProcessKeyboard(FORWARD, deltaTime, g_ghost);
        if (g_SPressed)
            Player.ProcessKeyboard(BACKWARD, deltaTime, g_ghost);
        if (g_APressed)
            Player.ProcessKeyboard(LEFT, deltaTime, g_ghost);
        if (g_DPressed)
            Player.ProcessKeyboard(RIGHT, deltaTime, g_ghost);
        if (g_SpacePressed)
            Player.ProcessKeyboard(JUMP, deltaTime, g_ghost);
            
        Player.Update(deltaTime);

        PlayerWallCollision(Player, g_ghost, door);
        PlayerObjectCollision(Player, g_ghost, g_listaAABB);

    
        mat4 view = Player.GetViewMatrix();
        float field_of_view = PI / 3.0f;
        mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, NEARPLANE, FARPLANE);

        // Enviamos as matrizes "view" e "projection" para a GPU
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        /*
        // PERSONAGEM
        float yawRad = Player.Yaw * (PI / 180.0f);
        float pitchRad = Player.Pitch * (PI / 180.0f);
        glm::mat4 model = Matrix_Translate(Player.Position.x, Player.Position.y - 1.0f, Player.Position.z)
            * Matrix_Rotate_Y(PI_2.0f)
            * Matrix_Rotate_Y(-yawRad)
            * Matrix_Scale(PLAYER_HEIGHT, PLAYER_HEIGHT, PLAYER_HEIGHT);

        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PERSON);
        DrawVirtualObject("Group1");
        */

        SalaPrincipal(door, exibirDardos);
        TextRendering_ShowFramesPerSecond(window);
        Tempo(window);
        Alavanca(window, door, armadilhasDesativadas);
        Porta(door);
        Telas(window);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
    glfwTerminate();
    return 0;
}

void LoadTextureImage(const char* filename) {
    printf("Carregando imagem \"%s\"... ", filename);
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);
    if (!data) {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        exit(EXIT_FAILURE);
    }
    printf("OK (%dx%d).\n", width, height);

    GLuint texture_id, sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    std::string fname(filename);
    GLint wrap_mode = (fname.find("chao") != std::string::npos || fname.find("chao") != std::string::npos ||
                        fname.find("brick") != std::string::npos || fname.find("Brick") != std::string::npos) ||
                        fname.find("teto") != std::string::npos
                        ? GL_MIRRORED_REPEAT
                        : GL_CLAMP_TO_EDGE;

    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, wrap_mode);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint textureunit = g_NumLoadedTextures++;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);
}

void DrawVirtualObject(const char* object_name){
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );
    glBindVertexArray(0);
}

void LoadShadersFromFiles(){
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    g_GpuProgramID       = CreateGpuProgram(vertex_shader_id, fragment_shader_id);
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model");
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view");
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection");
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id");
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    
    g_light_direction_uniform = glGetUniformLocation(g_GpuProgramID, "light_direction");
    g_light_cutoff_angle_uniform = glGetUniformLocation(g_GpuProgramID, "light_cutoff_angle");
    g_light_outer_cutoff_uniform = glGetUniformLocation(g_GpuProgramID, "light_outer_cutoff");
    g_light_range_uniform = glGetUniformLocation(g_GpuProgramID, "light_range");

    glUseProgram(g_GpuProgramID);
    
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform3f(glGetUniformLocation(g_GpuProgramID, "light_position"), LIGHT_POSITION.x, LIGHT_POSITION.y, LIGHT_POSITION.z);
    glUniform3f(glGetUniformLocation(g_GpuProgramID, "light_color"), LIGHT_COLOR.x, LIGHT_COLOR.y, LIGHT_COLOR.z);
    glUniform1f(glGetUniformLocation(g_GpuProgramID, "light_intensity"), LIGHT_INTENSITY);
    
    glUniform3f(g_light_direction_uniform, 0.0f, -1.0f, 0.0f);
    glUniform1f(g_light_cutoff_angle_uniform, glm::radians(15.0f));
    glUniform1f(g_light_outer_cutoff_uniform, glm::radians(45.0f));
    glUniform1f(g_light_range_uniform, 10.0f);
    
    glUseProgram(0);
}

void PushMatrix(glm::mat4 M){
    g_MatrixStack.push(M);
}

void PopMatrix(glm::mat4& M){
    if ( g_MatrixStack.empty() ){
        M = Matrix_Identity();
    }
    else{
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;
    set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape){
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle){
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    for (const unsigned int & sgroup : sgroup_ids){
        vector<int> num_triangles_per_vertex(num_vertices, 0);
        vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        for (size_t shape = 0; shape < model->shapes.size(); ++shape){
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle){
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex){
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex){
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index){
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        for (size_t shape = 0; shape < model->shapes.size(); ++shape){
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle){
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex){
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }
    }
}

void BuildTrianglesAndAddToVirtualScene(ObjModel* model){
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    vector<GLuint> indices;
    vector<float>  model_coefficients;
    vector<float>  normal_coefficients;
    vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape){
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = numeric_limits<float>::min();
        const float maxval = numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle){
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                model_coefficients.push_back( vx );
                model_coefficients.push_back( vy );
                model_coefficients.push_back( vz );
                model_coefficients.push_back( 1.0f );
                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if ( idx.normal_index != -1 ){
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx );
                    normal_coefficients.push_back( ny );
                    normal_coefficients.push_back( nz );
                    normal_coefficients.push_back( 0.0f );
                }
                if ( idx.texcoord_index != -1 ){
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index;
        theobject.num_indices    = last_index - first_index + 1; 
        theobject.rendering_mode = GL_TRIANGLES;
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4;
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    glBindVertexArray(0);
}

GLuint LoadShader_Vertex(const char* filename){
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    LoadShader(filename, vertex_shader_id);
    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename){
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(filename, fragment_shader_id);
    return fragment_shader_id;
}

void LoadShader(const char* filename, GLuint shader_id){
    ifstream file;
    try {
        file.exceptions(ifstream::failbit);
        file.open(filename);
    } catch ( exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        exit(EXIT_FAILURE);
    }
    stringstream shader;
    shader << file.rdbuf();
    string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
    glCompileShader(shader_id);
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);
    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if ( log_length != 0 ){
        string  output;
        if ( !compiled_ok ){
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else{
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    delete [] log;
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id){
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);
    if ( linked_ok == GL_FALSE ){
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        GLchar* log = new GLchar[log_length];
        glGetProgramInfoLog(program_id, log_length, &log_length, log);
        string output;
        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";
        delete [] log;
        fprintf(stderr, "%s", output.c_str());
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        g_LeftMouseButtonPressed = false;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_RightMouseButtonPressed = false;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS){
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        g_MiddleMouseButtonPressed = false;

}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos){
    static bool firstMouse = true;
    static double lastX = WIDTH/2, lastY = HEIGHT/2;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    Player.ProcessMouseMovement(xoffset, yoffset);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    g_CameraDistance -= 0.1f*yoffset;
    const float verysmallnumber = numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Loop para correção autmática
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            exit(100 + i);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    bool pressed = (action != GLFW_RELEASE);
    float delta = PI / 16;

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;

    if (key == GLFW_KEY_G && action == GLFW_PRESS){
        if (!g_ghost) {
            g_GameStateBeforeGhost = g_GameState;
            g_GameState = GameState::GHOST_MODE;
        } else {
            g_GameState = g_GameStateBeforeGhost;
        }
        g_ghost = !g_ghost;
        Player.setGhostMode(g_ghost);
    }
    if ((key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) && action == GLFW_PRESS){
        g_ctrlPressed = !g_ctrlPressed;
        Player.setCtrlMode(g_ctrlPressed);
    }
    float distanceToLever = glm::length(Player.Position - LEVER_POSITION);
    if (key == GLFW_KEY_L && action == GLFW_PRESS && distanceToLever < 1.5f)
        g_leverActivated = !g_leverActivated;

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        g_ShowInfoText = !g_ShowInfoText;

    if (key == GLFW_KEY_R && action == GLFW_PRESS){
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Movimento do Jogador
    switch (g_GameState) {
        case GameState::START_MENU:
            if (key == GLFW_KEY_ENTER && action == GLFW_PRESS){
                g_GameState = GameState::GAME_PLAY;
                g_startGamePlayTime = (float)glfwGetTime();
            }
            break;
        case GameState::GAME_PLAY:
            allowPlayerMovement(key, pressed);
            break;
        case GameState::GHOST_MODE:
            allowPlayerMovement(key, pressed);
            break;
        case GameState::GAME_OVER:
            break;
        default:
            return;
    }
}

void allowPlayerMovement(int key, bool pressed) {
    switch (key) {
        case GLFW_KEY_W:
            g_WPressed = pressed;
            break;
        case GLFW_KEY_S:
            g_SPressed = pressed;
            break;
        case GLFW_KEY_A:
            g_APressed = pressed;
            break;
        case GLFW_KEY_D:
            g_DPressed = pressed;
            break;
        case GLFW_KEY_SPACE:
            g_SpacePressed = pressed;
            break;
        default:
            return;
    }
}

void ErrorCallback(int error, const char* description){
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;
    float seconds = (float)glfwGetTime();
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

void PrintObjModelInfo(ObjModel* model){
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    map<string, string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    map<string, string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void Dardos(bool &door, bool &exibirDardos){
    #define TRAP    9
    static double g_lastTrapTime = 0.0;
    static bool trapTimerStarted = false;
    
    if(!door)
        return;
    if(!exibirDardos)
        return;
    
    if(!trapTimerStarted) {
        g_lastTrapTime = glfwGetTime();
        trapTimerStarted = true;
    }
    
    double currentTime = glfwGetTime();
    if(currentTime - g_lastTrapTime > 50.0f) {
        exibirDardos = false;
        return;
    }
    
    glm::mat4 base = Matrix_Rotate_X(-PI_2) * Matrix_Scale(0.1f, 0.1f, 0.1f);
    glm::mat4 model;
    glUniform1i(g_object_id_uniform, TRAP);
    
    float pos = fmod((currentTime - g_lastTrapTime) * 1.0f, 2*SCALE_WALL);
    for(int i = -SCALE_FLOOR; i < SCALE_FLOOR; i++)
        for(int j = -SCALE_FLOOR; j < SCALE_FLOOR; j++) {
            model = Matrix_Translate(i+1.0f, SCALE_WALL - pos, j+1.0f) * base;
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            DrawVirtualObject("throwing_dart");
        }
}

void SalaPrincipal(bool &door, bool &exibirDardos){

    #define PLANE       1
    #define WALL        2
    #define CEILING     3
    #define TABLE       4
    #define LAMP        5
    #define DOOR        6
    #define LEVER       8
    glm::mat4 model = Matrix_Identity();
    vector<string> objeto;
    glm::vec3 posicao, rotacao;

    // Chão
    model = Matrix_Translate(0.0f,-1.1f,0.0f)
            * Matrix_Scale(SCALE_FLOOR,SCALE_FLOOR,SCALE_FLOOR);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, PLANE);
    DrawVirtualObject("the_plane");

    // Mesa
    objeto = {"Table_Circle.004","Leg_Cylinder.001","Support_Cube"};
    posicao = {0.0f, -1.1f, 0.0f};
    rotacao = {0.0f, PI/4, 0.0f};
    DrawOBJ(TABLE,objeto,posicao,0.3f,rotacao);

    // Alavanca
    objeto = {"Cobblestone"};
    posicao = {0.0f, -0.25f, 0.0f};
    rotacao = {0.0f, PI_2, 0.0f};
    DrawOBJ(LEVER,objeto,posicao,0.05f,rotacao);
    objeto = {"Lever"};
    posicao = {0.00f, -0.25f, 0.0f};
    rotacao = {g_LeverAngle, PI_2, 0.0f};
    DrawOBJ(LEVER,objeto,posicao,0.05f,rotacao);

    // Lampada de mesa
    objeto = {"Cone040","Helix039","LAMP","Object008","Rectangle043","Cylinder044","Rectangle047","Rectangle048","Cylinder059","Loft020"};
    posicao = {SCALE_WALL/2+0.1f, SCALE_WALL-0.75f, 0.0f};
    rotacao = {0.0f, 0.0f, 0.0f};
    DrawOBJ(LAMP,objeto,posicao,0.5f,rotacao);

    // Teto
    model = Matrix_Translate(0.0f,SCALE_WALL,0.0f)
            * Matrix_Rotate_X(PI)
            * Matrix_Scale(SCALE_FLOOR,SCALE_FLOOR,SCALE_FLOOR);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, CEILING);
    DrawVirtualObject("the_ceiling");

    // Parede frontal
    model = Matrix_Translate(0.0f,SCALE_WALL/4,-SCALE_FLOOR)
            * Matrix_Rotate_X(PI_2)
            * Matrix_Scale(SCALE_FLOOR,SCALE_WALL,SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    // Parede da porta - direita
    model = Matrix_Translate(-0.60f*SCALE_FLOOR, SCALE_WALL / 4, SCALE_FLOOR)
            * Matrix_Rotate_X(-PI_2)
            * Matrix_Scale(0.50f*SCALE_FLOOR, SCALE_WALL, SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    // Porta
    objeto  = {"10057_wooden_door_frame_v1"};
    posicao = {0.0f, 0.1f, SCALE_FLOOR};
    rotacao = {PI_2, 0.0f, PI};
    DrawOBJ(DOOR, objeto,posicao,SCALE_WALL/4,rotacao);
    objeto = {"10057_wooden_door_v1"};
    posicao = {g_DoorX, g_DoorY, g_DoorZ};
    rotacao = {PI_2, g_DoorAngle, 0.0f};
    DrawOBJ(DOOR, objeto, posicao, SCALE_WALL/4, rotacao);
   
    // Parede da porta - esquerda
    model = Matrix_Translate(0.60f*SCALE_FLOOR, SCALE_WALL / 4, SCALE_FLOOR)
            * Matrix_Rotate_X(-PI_2)
            * Matrix_Scale(0.50f*SCALE_FLOOR, SCALE_WALL, SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    // Parede da porta - cima
    model = Matrix_Translate(0.0f, 0.67f*SCALE_FLOOR, SCALE_FLOOR)
            * Matrix_Rotate_X(-PI_2)
            * Matrix_Scale(0.1f*SCALE_FLOOR, 0.01f*SCALE_FLOOR, SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    // Parede esquerda
    model = Matrix_Translate(-SCALE_FLOOR, SCALE_WALL / 4, 0.0f)
            * Matrix_Rotate_X(PI_2)
            * Matrix_Rotate_Z(-PI_2)
            * Matrix_Scale(SCALE_FLOOR, SCALE_WALL, SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    // Parede direita
    model = Matrix_Translate(SCALE_FLOOR, SCALE_WALL / 4, 0.0f)
            * Matrix_Rotate_X(PI_2)
            * Matrix_Rotate_Z(PI_2)
            * Matrix_Scale(SCALE_FLOOR, SCALE_WALL, SCALE_WALL);
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, WALL);
    DrawVirtualObject("the_wall");

    Dardos(door, exibirDardos);
}

// Função adaptada para desenhar objetos compostos por várias partes - gerado por I.A.
void DrawOBJ(int objectId,const std::vector<std::string>& parts,const glm::vec3& position,float size,const glm::vec3& rotation) {
    glm::vec3 bbox_min(std::numeric_limits<float>::max());
    glm::vec3 bbox_max(std::numeric_limits<float>::lowest());

    for (const auto& partName : parts){
        auto it = g_VirtualScene.find(partName);
        if (it != g_VirtualScene.end() && it->second.vertex_array_object_id != 0){            
            bbox_min.x = std::min(bbox_min.x, it->second.bbox_min.x);
            bbox_min.y = std::min(bbox_min.y, it->second.bbox_min.y);
            bbox_min.z = std::min(bbox_min.z, it->second.bbox_min.z);
            
            bbox_max.x = std::max(bbox_max.x, it->second.bbox_max.x);
            bbox_max.y = std::max(bbox_max.y, it->second.bbox_max.y);
            bbox_max.z = std::max(bbox_max.z, it->second.bbox_max.z);
        }
    }

    const glm::vec3 center = (bbox_min + bbox_max) * 0.5f;
    const float width = bbox_max.x - bbox_min.x;
    const float depth = bbox_max.z - bbox_min.z;
    const float desiredSize = size * SCALE_FLOOR;
    
    float scale = 1.0f;
    const float biggestDimension = std::max(width, depth);

    if (biggestDimension > std::numeric_limits<float>::epsilon())
        scale = desiredSize / biggestDimension;

    const float scaledMinY = (bbox_min.y - center.y) * scale;
    const float worldY = position.y - scaledMinY;

    if (objectId == TABLE) {
        float tableTopY = worldY + (bbox_max.y - center.y) * scale;
        g_listaAABB[objectId] = AABB{
            glm::vec3(bbox_min.x * scale + position.x, tableTopY,bbox_min.z * scale + position.z),
            glm::vec3(bbox_max.x * scale + position.x, SCALE_WALL,bbox_max.z * scale + position.z)
        };
    }
    else {
        g_listaAABB[objectId] = AABB{
            glm::vec3(bbox_min.x * scale + position.x, worldY,bbox_min.z * scale + position.z),
            glm::vec3(bbox_max.x * scale + position.x, (bbox_max.y - center.y) * scale + worldY,bbox_max.z * scale + position.z)
        };
    }

    const glm::mat4 rotationMatrix = 
        Matrix_Rotate_Z(rotation.z) *
        Matrix_Rotate_Y(rotation.y) *
        Matrix_Rotate_X(rotation.x);

    const glm::mat4 modelMatrix =
        Matrix_Translate(position.x, worldY, position.z) *
        rotationMatrix *
        Matrix_Scale(scale, scale, scale) *
        Matrix_Translate(-center.x, -center.y, -center.z);

    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform1i(g_object_id_uniform, objectId);

    for (const auto& partName : parts) {
        auto it = g_VirtualScene.find(partName);
        if (it != g_VirtualScene.end() && it->second.vertex_array_object_id != 0)
            DrawVirtualObject(partName.c_str());
    }
}

void Alavanca(GLFWwindow* window, bool &door, bool &armadilhasDesativadas){
    if (g_GameState != GameState::GAME_PLAY)
        return;

    float distanceToLever = glm::length(Player.Position - LEVER_POSITION);
    bool isCloseToLever = (distanceToLever < 1.5f && !g_leverActivated);
    bool isLookingAtLever = false;
    if (isCloseToLever) {
        glm::vec3 dirToLever = glm::normalize(LEVER_POSITION - Player.Position);
        float dot = glm::dot(Player.Front, dirToLever);
        const float viewAngleThreshold = cos(glm::radians(30.0f));
        isLookingAtLever = (dot > viewAngleThreshold);
    }
    g_showLeverText = isCloseToLever && isLookingAtLever;
    if (g_showLeverText)
        TextRendering_PrintString(window, "Pressione L para acionar a alavanca", 
            -0.3f - 0.5f * strlen("Pressione L para acionar a alavanca") * TextRendering_CharWidth(window), 
            -0.5f - 0.5f * TextRendering_LineHeight(window), FONT_HEIGHT);

    g_LeverTargetAngle = g_leverActivated ? -glm::radians(70.0f) : 0.0f;

    float diff = g_LeverTargetAngle - g_LeverAngle;
    float maxStep = g_LeverSpeed * deltaTime;
    if (fabs(diff) <= maxStep)
        g_LeverAngle = g_LeverTargetAngle;
    else
        g_LeverAngle += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;
    
    if (g_leverActivated)
        door = true;
    static double TimeOpen = 0.0;
    if (g_leverActivated && TimeOpen == 0.0)
        TimeOpen = glfwGetTime();
    if (TimeOpen > 0.0 && glfwGetTime() - TimeOpen >= 3.0) {
        if ( !CheckSafe(Player.Position) && !armadilhasDesativadas ) {
            g_GameState = GameState::GAME_OVER;
            g_DeathCause = DeathCause::DARDO;
            //glfwSetWindowShouldClose(window, GL_TRUE);
        }
        armadilhasDesativadas = true;
    }

}

void Porta(bool &door) {
    const float openedAngle = PI_2;
    const float closedAngle = 0.0f;

    float targetAngle = door ? openedAngle : closedAngle;

    const float doorSpeed = 1.0f;
    float diff = targetAngle - g_DoorAngle;
    float maxStep = doorSpeed * deltaTime;

    if (fabs(diff) <= maxStep)
        g_DoorAngle = targetAngle;
    else
        g_DoorAngle += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;

    g_DoorX = BASE_DOOR_X - DOOR_RADIUS * (1.0f - cos(g_DoorAngle));
    g_DoorZ = BASE_DOOR_Z - DOOR_RADIUS * sin(g_DoorAngle);
}

void Telas(GLFWwindow* window){

    if (g_GameState == GameState::START_MENU) {
            TextRendering_PrintString(window, "Pressione ENTER para iniciar o jogo", 
                -0.3f - 0.5f * strlen("Pressione ENTER para iniciar o jogo") * TextRendering_CharWidth(window), 
                0.0f - 0.5f * TextRendering_LineHeight(window), FONT_HEIGHT);
        } else if (g_GameState == GameState::GAME_OVER) {
            switch (g_DeathCause) {
                case DeathCause::NONE:
                    TextRendering_PrintString(window, "Parabéns, você conseguiu!", 
                        -0.4f - 0.5f * strlen("Parabéns, você conseguiu!") * TextRendering_CharWidth(window), 
                        0.0f - 0.5f * TextRendering_LineHeight(window), FONT_HEIGHT);
                    break;
                case DeathCause::DARDO:
                    TextRendering_PrintString(window, "Voce morreu! Cuidado com as armadilhas!", 
                        -0.4f - 0.5f * strlen("Voce morreu! Cuidado com as armadilhas!") * TextRendering_CharWidth(window), 
                        0.0f - 0.5f * TextRendering_LineHeight(window), FONT_HEIGHT);
                    break;
                case DeathCause::TIMEOUT:
                    TextRendering_PrintString(window, "Voce morreu! O tempo acabou!", 
                        -0.3f - 0.5f * strlen("Voce morreu! O tempo acabou!") * TextRendering_CharWidth(window), 
                        0.0f - 0.5f * TextRendering_LineHeight(window), FONT_HEIGHT);
                    break;
                
            }
        }
}

void Tempo(GLFWwindow* window){
    if (g_GameState != GameState::GAME_PLAY)
        return;
    
    double now = glfwGetTime();
    float elapsed = (float)(now - g_startGamePlayTime);
    float remaining = TOTAL_TIME - elapsed;
    
    if (remaining <= 0.0f){
        remaining = 0.0f;
        g_GameState = GameState::GAME_OVER;
        g_DeathCause = DeathCause::TIMEOUT;
    } else if (remaining > TOTAL_TIME)
        remaining = TOTAL_TIME;
    
    char time_text[50];
    sprintf(time_text, "Tempo restante: %.1f", remaining);
    
    float text_width = strlen(time_text) * TextRendering_CharWidth(window);
    float x_pos = -0.2f - 0.5f * text_width;
    float y_pos = 0.9f - TextRendering_LineHeight(window);
    
    TextRendering_PrintString(window, time_text, x_pos, y_pos, FONT_HEIGHT);
}
// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
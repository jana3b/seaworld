#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const * path);

void renderQuad();

void setShaderLights(Shader &shader);

// settings
const unsigned int SCR_WIDTH = 1200; //800
const unsigned int SCR_HEIGHT = 800; //600
float heightScale = 0.1;

int Width = SCR_WIDTH;
int Height = SCR_HEIGHT;


// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool blink = false;
int jellyfishColor = 2;

unsigned int quadVAO = 0;
unsigned int quadVBO;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    PointLight jellyfishPointLight;
    PointLight anglerfishPointLight;
    DirLight dirLight;
    SpotLight spotLight;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Seaworld", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


    //Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------

    // Depth testing
    glEnable(GL_DEPTH_TEST);

    // Face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // -------------------------
    Shader modelShader("resources/shaders/model.vs", "resources/shaders/model.fs");

    // load models
    // -----------
    Model submarineModel("resources/objects/submarine/scene.gltf");
    submarineModel.SetShaderTextureNamePrefix("material.");

    Model fishModel("resources/objects/fish/scene.gltf");
    fishModel.SetShaderTextureNamePrefix("material.");

    Model seashellModel("resources/objects/seashell/sea_shell.obj");
    seashellModel.SetShaderTextureNamePrefix("material.");

    Model fish2Model("resources/objects/fish2/scene.gltf");
    fish2Model.SetShaderTextureNamePrefix("material.");

    Model sharkModel("resources/objects/shark/scene.gltf");
    sharkModel.SetShaderTextureNamePrefix("material.");

    Model jellyfishModel("resources/objects/jellyfish/scene.gltf");
    jellyfishModel.SetShaderTextureNamePrefix("material.");

    Model anglerfishModel("resources/objects/anglerfish/scene.gltf");
    anglerfishModel.SetShaderTextureNamePrefix("material.");

    Model barrelsModel("resources/objects/barrels/scene.gltf");
    barrelsModel.SetShaderTextureNamePrefix("material.");

    // setting lights

    PointLight& jellyfishPointLight = programState->jellyfishPointLight;
    jellyfishPointLight.position = glm::vec3(-16.0f, 11.0f, -6.0f);
    jellyfishPointLight.ambient = glm::vec3(0.1, 0.1, 0.3);
    jellyfishPointLight.diffuse = glm::vec3(0.6, 0.6, 1.0);
    jellyfishPointLight.specular = glm::vec3(1.0, 1.0, 1.0);
    jellyfishPointLight.constant = 1.0f;
    jellyfishPointLight.linear = 0.027f;
    jellyfishPointLight.quadratic = 0.0028f;

    PointLight& anglerfishPointLight = programState->anglerfishPointLight;
    anglerfishPointLight.position = glm::vec3(0.0f, 3.0f, 51.0f);
    anglerfishPointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    anglerfishPointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    anglerfishPointLight.specular = glm::vec3(1.0, 1.0, 1.0);
    anglerfishPointLight.constant = 1.0f;
    anglerfishPointLight.linear = 0.027f;
    anglerfishPointLight.quadratic = 0.0028f;

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(0.0f, -1.0f, 0.15f);
    dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);


    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    spotLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09;
    spotLight.quadratic = 0.032;
    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));



    //********************************************************************************************************
    // METAL BOX


    Shader boxShader("resources/shaders/box.vs", "resources/shaders/box.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------

    float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, 0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, 0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, 0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, 0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, 0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, 0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };


    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normals attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    unsigned int boxDiffuseMap = loadTexture(FileSystem::getPath("resources/textures/metal/metal_diff.jpg").c_str());
    unsigned int boxSpecularMap = loadTexture(FileSystem::getPath("resources/textures/metal/metal_spec.jpg").c_str());

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------

    boxShader.use();
    boxShader.setInt("material.diffuse", 0);
    boxShader.setInt("material.specular", 1);


    stbi_set_flip_vertically_on_load(false);


    //********************************************************************************************************
    // SEAWEED

    Shader glassShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");

    float glassVertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f // top-left
    };

    unsigned int glassIndices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    unsigned int glassVBO, glassVAO, glassEBO;
    glGenVertexArrays(1, &glassVAO);
    glGenBuffers(1, &glassVBO);
    glGenBuffers(1, &glassEBO);

    glBindVertexArray(glassVAO);

    glBindBuffer(GL_ARRAY_BUFFER, glassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glassVertices), glassVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glassEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glassIndices), glassIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // loading glass texture

    unsigned int glassTexture = loadTexture(FileSystem::getPath("resources/textures/transparent/seaweed.png").c_str());

    glassShader.use();
    glassShader.setInt("texture1", 0);


    //********************************************************************************************************
    // SKYBOX


    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/aqua4_ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox/aqua4_bk.jpg"),
                    FileSystem::getPath("resources/textures/skybox/aqua4_up.jpg"),
                    FileSystem::getPath("resources/textures/skybox/aqua4_dn.jpg"),
                    FileSystem::getPath("resources/textures/skybox/aqua4_rt.jpg"),
                    FileSystem::getPath("resources/textures/skybox/aqua4_lf.jpg")
            };

    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);




    //********************************************************************************************************
    // QUAD - normal and parallax mapping


    Shader quadShader("resources/shaders/quad.vs", "resources/shaders/quad.fs");

    // load textures
    // -------------
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/iron/iron_diff.jpg").c_str());
    unsigned int normalMap  = loadTexture(FileSystem::getPath("resources/textures/iron/iron_nor.jpg").c_str());
    unsigned int heightMap  = loadTexture(FileSystem::getPath("resources/textures/iron/iron_disp.jpg").c_str());

    // shader configuration
    // --------------------
    quadShader.use();
    quadShader.setInt("diffuseMap", 0);
    quadShader.setInt("normalMap", 1);
    quadShader.setInt("depthMap", 2);




    //********************************************************************************************************
    // RENDER LOOP

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        if(blink){
            anglerfishPointLight.ambient = glm::vec3(0.1f + 0.0005f*cos(currentFrame));
            anglerfishPointLight.diffuse = glm::vec3(0.2f + 0.02f*tan(200*currentFrame));
            anglerfishPointLight.specular = glm::vec3(0.005f*(1/tan(100*currentFrame)));
        }else{
            anglerfishPointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
            anglerfishPointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
            anglerfishPointLight.specular = glm::vec3(1.0, 1.0, 1.0);
        }

        switch(jellyfishColor){
            case 0:
                jellyfishPointLight.ambient = glm::vec3(0.3, 0.1, 0.1);
                jellyfishPointLight.diffuse = glm::vec3(1.0, 0.6, 0.6);
                break;
            case 1:
                jellyfishPointLight.ambient = glm::vec3(0.1, 0.3, 0.1);
                jellyfishPointLight.diffuse = glm::vec3(0.6, 1.0, 0.6);
                break;
            case 2:
                jellyfishPointLight.ambient = glm::vec3(0.1, 0.1, 0.3);
                jellyfishPointLight.diffuse = glm::vec3(0.6, 0.6, 1.0);
                break;
        }


        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) Width / (float) Height, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();


        //light inside of jellyfish moves as jellyfish moves
        jellyfishPointLight.position = glm::vec3(-15.0f, 4.0f + 4*sin(0.5*currentFrame), -5.0f);


        // render metal box

        boxShader.use();
        setShaderLights(boxShader);

        glm::mat4 model  = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-10.0f, -50.0f, -10.0f));
        model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(40.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(10.0f));

        boxShader.setMat4("model", model);
        boxShader.setMat4("view", view);
        boxShader.setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, boxDiffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, boxSpecularMap);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        // render models

        modelShader.use();
        setShaderLights(modelShader);

        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);


        //render submarine

        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(2.0f));

        modelShader.setMat4("model", model);
        submarineModel.Draw(modelShader);


        //render fish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0f, 5.0f + 0.1*cos(currentFrame), 10.0f));
        model = glm::rotate(model, glm::radians( -90.0f - 2*cos(currentFrame)), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(- 2*sin(7*currentFrame)), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.7f));

        modelShader.setMat4("model", model);
        fishModel.Draw(modelShader);


        //render fish2

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(8.0f, 2.0f + 0.5*cos(currentFrame), 15.0f));
        model = glm::rotate(model, glm::radians(0.0f - 2*sin(currentFrame)), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-90.0f + 8*sin(5*currentFrame)), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(10.0f - 3*sin(currentFrame)), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.8f));

        modelShader.setMat4("model", model);
        fish2Model.Draw(modelShader);


        //render jellyfish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-15.0f, 4.0f + 4*sin(0.5*currentFrame), -5.0f));
        model = glm::rotate(model, glm::radians(-100.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.2f));

        modelShader.setMat4("model", model);
        jellyfishModel.Draw(modelShader);


        //render shark

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0f, 10.0f + 0.3*sin(0.2*currentFrame), 20.0f));
        model = glm::rotate(model, glm::radians(- 3*cos(2*currentFrame)), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));

        modelShader.setMat4("model", model);
        sharkModel.Draw(modelShader);


        //render anglerfish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f, -3.0f, 70.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1f));

        modelShader.setMat4("model", model);
        anglerfishModel.Draw(modelShader);


        //render seashell

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-4.0f, -48.0f, -7.0f));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(60.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.05f));

        modelShader.setMat4("model", model);
        seashellModel.Draw(modelShader);


        //render barrels

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-30.0f, -25.0f, -8.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0, 1.0, 0.0));

        modelShader.setMat4("model", model);
        barrelsModel.Draw(modelShader);



        //render quad

        glDisable(GL_CULL_FACE);

        quadShader.use();
        quadShader.setMat4("projection", projection);
        quadShader.setMat4("view", view);
        // render parallax-mapped quad
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -10.0f, -10.0f));
        model = glm::rotate(model, glm::radians(-60.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(3.0f));
        quadShader.setMat4("model", model);
        quadShader.setVec3("viewPos", programState->camera.Position);
        quadShader.setVec3("lightPos", jellyfishPointLight.position);
        quadShader.setVec3("lightColor", jellyfishPointLight.ambient);
        quadShader.setFloat("heightScale", heightScale);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, heightMap);

        renderQuad();


        glEnable(GL_CULL_FACE);


//        if (programState->ImGuiEnabled)
//            DrawImGui(programState);



        // render skybox

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        glm::mat4 skyboxProjection = glm::perspective(glm::radians(programState->camera.Zoom), (float)Width / (float)Height, 0.1f, 100.0f);
        skyboxShader.setMat4("view", skyboxView);
        skyboxShader.setMat4("projection", skyboxProjection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default



        // render seaweed

        glDisable(GL_CULL_FACE);

        glassShader.use();

        glassShader.setMat4("projection", projection);
        glassShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-20.0f, -9.5f, -10.0f));
        model = glm::rotate(model, glm::radians(200.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(4.0f));

        glassShader.setMat4("model", model);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glassTexture);

        glBindVertexArray(glassVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glEnable(GL_CULL_FACE);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glDeleteVertexArrays(1, &glassVAO);
    glDeleteBuffers(1, &glassVBO);
    glDeleteBuffers(1, &glassEBO);

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    glDeleteTextures(1, &boxDiffuseMap);
    glDeleteTextures(1,&boxSpecularMap);
    glDeleteTextures(1, &glassTexture);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &normalMap);
    glDeleteTextures(1, &heightMap);
    glDeleteTextures(1, &cubemapTexture);


    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    Width = width;
    Height = height;
    glViewport(0, 0, Width, Height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

//void DrawImGui(ProgramState *programState) {
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//
//
//    {
//        static float f = 0.0f;
//        ImGui::Begin("Hello window");
//        ImGui::Text("Hello text");
//        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
//        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
//        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
//        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);
//
//        ImGui::DragFloat("pointLight.constant", &programState->pointLight[0].constant, 0.05, 0.0, 1.0);
//        ImGui::DragFloat("pointLight.linear", &programState->pointLight[0].linear, 0.05, 0.0, 1.0);
//        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight[0].quadratic, 0.05, 0.0, 1.0);
//        ImGui::End();
//    }
//
//    {
//        ImGui::Begin("Camera info");
//        const Camera& c = programState->camera;
//        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
//        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
//        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
//        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
//        ImGui::End();
//    }
//
//    ImGui::Render();
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){
        blink = !blink;
    }
    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        jellyfishColor += 1;
        jellyfishColor %= 3;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   stbi_set_flip_vertically_on_load(true);

    return textureID;
}


unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


void renderQuad()
{
    if (quadVAO == 0)
    {


        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);


        float quadVertices[] = {
                // positions            // normal         // texcoords  // tangent                          // bitangent
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void setShaderLights(Shader &shader){
    shader.setVec3("pointLights[0].position", programState->jellyfishPointLight.position);
    shader.setVec3("pointLights[0].ambient", programState->jellyfishPointLight.ambient);
    shader.setVec3("pointLights[0].diffuse", programState->jellyfishPointLight.diffuse);
    shader.setVec3("pointLights[0].specular", programState->jellyfishPointLight.specular);
    shader.setFloat("pointLights[0].constant", programState->jellyfishPointLight.constant);
    shader.setFloat("pointLights[0].linear", programState->jellyfishPointLight.linear);
    shader.setFloat("pointLights[0].quadratic", programState->jellyfishPointLight.quadratic);


    shader.setVec3("pointLights[1].position", programState->anglerfishPointLight.position);
    shader.setVec3("pointLights[1].ambient", programState->anglerfishPointLight.ambient);
    shader.setVec3("pointLights[1].diffuse", programState->anglerfishPointLight.diffuse);
    shader.setVec3("pointLights[1].specular", programState->anglerfishPointLight.specular);
    shader.setFloat("pointLights[1].constant", programState->anglerfishPointLight.constant);
    shader.setFloat("pointLights[1].linear", programState->anglerfishPointLight.linear);
    shader.setFloat("pointLights[1].quadratic", programState->anglerfishPointLight.quadratic);

    shader.setVec3("viewPosition", programState->camera.Position);
    shader.setFloat("material.shininess", 32.0f);

    shader.setVec3("dirLight.direction", programState->dirLight.direction);
    shader.setVec3("dirLight.ambient", programState->dirLight.ambient);
    shader.setVec3("dirLight.diffuse", programState->dirLight.diffuse);
    shader.setVec3("dirLight.specular", programState->dirLight.specular);

    shader.setVec3("spotLight.position",programState->camera.Position);
    shader.setVec3("spotLight.direction", programState->camera.Front);
    shader.setVec3("spotLight.ambient", programState->spotLight.ambient);
    shader.setVec3("spotLight.diffuse", programState->spotLight.diffuse);
    shader.setVec3("spotLight.specular", programState->spotLight.specular);
    shader.setFloat("spotLight.constant", programState->spotLight.constant);
    shader.setFloat("spotLight.linear", programState->spotLight.linear);
    shader.setFloat("spotLight.quadratic", programState->spotLight.quadratic);
    shader.setFloat("spotLight.cutOff", programState->spotLight.cutOff);
    shader.setFloat("spotLight.outerCutOff", programState->spotLight.outerCutOff);

}
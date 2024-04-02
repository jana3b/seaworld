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

// settings
const unsigned int SCR_WIDTH = 1200; //800
const unsigned int SCR_HEIGHT = 800; //600

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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
    PointLight pointLight[2];
    DirLight dirLight;
    SpotLight spotLight;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

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
    glEnable(GL_DEPTH_TEST);

    // Enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    // build and compile shaders
    // -------------------------
    Shader modelShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");

    // load models
    // -----------
    Model submarineModel("resources/objects/submarine/scene.gltf");
    submarineModel.SetShaderTextureNamePrefix("material.");

    Model fishModel("resources/objects/fish/13009_Coral_Beauty_Angelfish_v1_l3.obj");
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
    jellyfishModel.SetShaderTextureNamePrefix("material.");


    // setting lights

    PointLight& jellyfishPointLight = programState->pointLight[0];
    jellyfishPointLight.position = glm::vec3(-16.0f, 11.0f, -6.0f);
    jellyfishPointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    jellyfishPointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    jellyfishPointLight.specular = glm::vec3(1.0, 1.0, 1.0);
    jellyfishPointLight.constant = 1.0f;
    jellyfishPointLight.linear = 0.027f;
    jellyfishPointLight.quadratic = 0.0028f;

    PointLight& anglerfishPointLight = programState->pointLight[1];
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
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, 0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, 0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

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


    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/metal/metal_plate_diff_4k.jpg").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/metal/metal_plate_spec_4k.jpg").c_str());

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------

    boxShader.use();
    boxShader.setInt("material.diffuse", 0);
    boxShader.setInt("material.specular", 1);


    stbi_set_flip_vertically_on_load(false);


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
                    FileSystem::getPath("resources/textures/skybox2/aqua4_ft.jpg"),
                    FileSystem::getPath("resources/textures/skybox2/aqua4_bk.jpg"),  //dobro
                    FileSystem::getPath("resources/textures/skybox2/aqua4_up.jpg"),  //dobro
                    FileSystem::getPath("resources/textures/skybox2/aqua4_dn.jpg"),  //dobro
                    FileSystem::getPath("resources/textures/skybox2/aqua4_rt.jpg"),  //dobro
                    FileSystem::getPath("resources/textures/skybox2/aqua4_lf.jpg")  //dobro
            };

    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


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


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //light inside of jellyfish moves as jellyfish moves
        jellyfishPointLight.position = glm::vec3(-15.0f, 4.0f + 4*sin(0.5*(float)glfwGetTime()), -5.0f);

        //anglerfishPointLight.position = glm::vec3(0.0f, 3.0f  + 3*sin(0.5*(float)glfwGetTime()), 51.0f);

        boxShader.use();

        boxShader.setVec3("pointLights[0].position", jellyfishPointLight.position);
        boxShader.setVec3("pointLights[0].ambient", jellyfishPointLight.ambient);
        boxShader.setVec3("pointLights[0].diffuse", jellyfishPointLight.diffuse);
        boxShader.setVec3("pointLights[0].specular", jellyfishPointLight.specular);
        boxShader.setFloat("pointLights[0].constant", jellyfishPointLight.constant);
        boxShader.setFloat("pointLights[0].linear", jellyfishPointLight.linear);
        boxShader.setFloat("pointLights[0].quadratic", jellyfishPointLight.quadratic);


        boxShader.setVec3("pointLights[1].position", anglerfishPointLight.position);
        boxShader.setVec3("pointLights[1].ambient", anglerfishPointLight.ambient);
        boxShader.setVec3("pointLights[1].diffuse", anglerfishPointLight.diffuse);
        boxShader.setVec3("pointLights[1].specular", anglerfishPointLight.specular);
        boxShader.setFloat("pointLights[1].constant", anglerfishPointLight.constant);
        boxShader.setFloat("pointLights[1].linear", anglerfishPointLight.linear);
        boxShader.setFloat("pointLights[1].quadratic", anglerfishPointLight.quadratic);

        boxShader.setVec3("viewPosition", programState->camera.Position);
        boxShader.setFloat("material.shininess", 32.0f);


        boxShader.setVec3("dirLight.direction", dirLight.direction);
        boxShader.setVec3("dirLight.ambient", dirLight.ambient);
        boxShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        boxShader.setVec3("dirLight.specular", dirLight.specular);


        boxShader.setVec3("spotLight.position",programState->camera.Position);
        boxShader.setVec3("spotLight.direction", programState->camera.Front);
        boxShader.setVec3("spotLight.ambient", spotLight.ambient);
        boxShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        boxShader.setVec3("spotLight.specular", spotLight.specular);
        boxShader.setFloat("spotLight.constant", spotLight.constant);
        boxShader.setFloat("spotLight.linear", spotLight.linear);
        boxShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        boxShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        boxShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

        glm::mat4 model  = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-10.0f, -50.0f, -10.0f));
        model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(40.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(10.0f));



        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        boxShader.setMat4("model", model);
        boxShader.setMat4("view", view);
        boxShader.setMat4("projection", projection);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);


        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);



        // don't forget to enable shader before setting uniforms
        modelShader.use();

        modelShader.setVec3("pointLights[0].position", jellyfishPointLight.position);
        modelShader.setVec3("pointLights[0].ambient", jellyfishPointLight.ambient);
        modelShader.setVec3("pointLights[0].diffuse", jellyfishPointLight.diffuse);
        modelShader.setVec3("pointLights[0].specular", jellyfishPointLight.specular);
        modelShader.setFloat("pointLights[0].constant", jellyfishPointLight.constant);
        modelShader.setFloat("pointLights[0].linear", jellyfishPointLight.linear);
        modelShader.setFloat("pointLights[0].quadratic", jellyfishPointLight.quadratic);


        modelShader.setVec3("pointLights[1].position", anglerfishPointLight.position);
        modelShader.setVec3("pointLights[1].ambient", anglerfishPointLight.ambient);
        modelShader.setVec3("pointLights[1].diffuse", anglerfishPointLight.diffuse);
        modelShader.setVec3("pointLights[1].specular", anglerfishPointLight.specular);
        modelShader.setFloat("pointLights[1].constant", anglerfishPointLight.constant);
        modelShader.setFloat("pointLights[1].linear", anglerfishPointLight.linear);
        modelShader.setFloat("pointLights[1].quadratic", anglerfishPointLight.quadratic);

        modelShader.setVec3("viewPosition", programState->camera.Position);
        modelShader.setFloat("material.shininess", 32.0f);


        modelShader.setVec3("dirLight.direction", dirLight.direction);
        modelShader.setVec3("dirLight.ambient", dirLight.ambient);
        modelShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        modelShader.setVec3("dirLight.specular", dirLight.specular);


        modelShader.setVec3("spotLight.position",programState->camera.Position);
        modelShader.setVec3("spotLight.direction", programState->camera.Front);
        modelShader.setVec3("spotLight.ambient", spotLight.ambient);
        modelShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        modelShader.setVec3("spotLight.specular", spotLight.specular);
        modelShader.setFloat("spotLight.constant", spotLight.constant);
        modelShader.setFloat("spotLight.linear", spotLight.linear);
        modelShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        modelShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        modelShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);


        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);



        // render loaded models



        //render fish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0f, 5.0f + 0.1*cos((float)glfwGetTime()), 10.0f));
        model = glm::rotate(model, glm::radians( -90.0f - 2*cos((float)glfwGetTime())), glm::vec3(1.0, 0.0, 0.0));
        //model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(- 2*sin(7*(float)glfwGetTime())), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(2.0f));

        modelShader.setMat4("model", model);
        fishModel.Draw(modelShader);


        //render submarine
        model = glm::mat4(1.0f);
        //model = glm::translate(model,glm::vec3());
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        //model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, -1.0, 0.0));
        //model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(2.0f));

        modelShader.setMat4("model", model);
        submarineModel.Draw(modelShader);


        //render fish2

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(8.0f, 2.0f + 0.5*cos((float)glfwGetTime()), 15.0f));
        model = glm::rotate(model, glm::radians(0.0f - 2*sin((float)glfwGetTime())), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-90.0f + 8*sin(5*(float)glfwGetTime())), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(10.0f - 3*sin((float)glfwGetTime())), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.8f));

        modelShader.setMat4("model", model);
        fish2Model.Draw(modelShader);


        //render jellyfish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-15.0f, 4.0f + 4*sin(0.5*(float)glfwGetTime()), -5.0f));
        model = glm::rotate(model, glm::radians(-100.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.2f));

        modelShader.setMat4("model", model);
        jellyfishModel.Draw(modelShader);


        //render shark

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(10.0f, 10.0f + 0.3*sin(0.2*(float)glfwGetTime()), 20.0f));
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(- 2*cos((float)glfwGetTime())), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.8f));

        modelShader.setMat4("model", model);
        sharkModel.Draw(modelShader);


        //render anglerfish

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f, -3.0f, 70.0f));
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0));
        //model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.1f));

        modelShader.setMat4("model", model);
        anglerfishModel.Draw(modelShader);



        //render seashell

        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-4.0f, -48.0f, -7.0f));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(60.0f), glm::vec3(0.0, 1.0, 0.0));
        //model = glm::rotate(model, glm::radians(40.0f), glm::vec3(0.0, 0.0, 1.0));
        model = glm::scale(model, glm::vec3(0.05f));

        modelShader.setMat4("model", model);
        seashellModel.Draw(modelShader);



        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // draw skybox as last

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        glm::mat4 skyboxProjection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        skyboxShader.setMat4("view", skyboxView);
        skyboxShader.setMat4("projection", skyboxProjection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);


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
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
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

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight[0].constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight[0].linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight[0].quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

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

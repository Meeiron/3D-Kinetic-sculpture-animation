// Kinetic Sculpture on top of LearnOpenGL "Multiple Lights" framework
// Uses the same shader names & uniforms so it drops into your existing setup.
//
// Files expected next to the executable or via FileSystem::getPath():
//   Shaders: 6.multiple_lights.vs, 6.multiple_lights.fs,
//            6.light_cube.vs,     6.light_cube.fs
//   Textures: resources/textures/container2.png
//             resources/textures/container2_specular.png
//
// Build: link glad, glfw, glm, stb_image (implementation in exactly ONE TU), OpenGL
//
// Controls:
//   W/A/S/D = move, Mouse = look, Scroll = zoom
//   P = pause/resume, R = reset animation params
//   ↑/↓ = wave speed ±, →/← = wave amplitude ±

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// --------------------------------------------------------------------------------------
// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 4.0f, 18.0f));
float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;
bool  firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Kinetic animation params
bool  paused = false;
float waveSpeed = 1.6f;
float waveAmp = 0.9f;
float tAccum = 0.0f;

// Grid (pendants)
const int   NX = 14;
const int   NZ = 14;
const float SPACING = 1.3f;

// Light positions (point lights at four corners above the grid)
glm::vec3 pointLightPositions[] = {
    {  7.0f,  7.5f,  7.0f },
    { -7.0f,  7.5f,  7.0f },
    {  7.0f,  7.5f, -7.0f },
    { -7.0f,  7.5f, -7.0f }
};

// --------------------------------------------------------------------------------------
// Function decls
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

// --------------------------------------------------------------------------------------
int main() {
    // GLFW init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Kinetic Sculpture - Multiple Lights", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Shaders (names match your request & the tutorial)
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    // Geometry: cube with positions, normals, texcoords
    float vertices[] = {
        // positions         // normals          // texcoords
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
         0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
        -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
        -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,

        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
         0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,

        -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    unsigned int VBO, cubeVAO, lightCubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(lightCubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Textures
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/container2.png").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/container2_specular.png").c_str());

    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        if (!paused) tAccum += deltaTime * waveSpeed;

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Lighting setup (dir + 4 point + spotlight)
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 32.0f);

        // Directional light
        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        lightingShader.setVec3("dirLight.diffuse", 0.35f, 0.35f, 0.35f);
        lightingShader.setVec3("dirLight.specular", 0.20f, 0.20f, 0.20f);

        // Point lights
        for (int i = 0; i < 4; ++i) {
            std::string base = "pointLights[" + std::to_string(i) + "].";
            lightingShader.setVec3(base + "position", pointLightPositions[i]);
            lightingShader.setVec3(base + "ambient", 0.05f, 0.05f, 0.05f);
            lightingShader.setVec3(base + "diffuse", 0.80f, 0.80f, 0.80f);
            lightingShader.setVec3(base + "specular", 1.00f, 1.00f, 1.00f);
            lightingShader.setFloat(base + "constant", 1.0f);
            lightingShader.setFloat(base + "linear", 0.09f);
            lightingShader.setFloat(base + "quadratic", 0.032f);
        }

        // Spotlight on camera
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // Camera matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // Base plate (thin scaled cube)
        glBindVertexArray(cubeVAO);
        {
            glm::mat4 model(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -0.05f, 0.0f));
            model = glm::scale(model, glm::vec3(NX * SPACING * 0.9f, 0.1f, NZ * SPACING * 0.9f));
            lightingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Animated pendants grid
        for (int ix = 0; ix < NX; ++ix) {
            for (int iz = 0; iz < NZ; ++iz) {
                float x = (ix - (NX - 1) * 0.5f) * SPACING;
                float z = (iz - (NZ - 1) * 0.5f) * SPACING;

                // Sine-wave vertical motion + slight rotations
                float phase = 0.35f * (ix + iz);
                float y = 1.8f + waveAmp * std::sinf(tAccum + phase);
                float yaw = 10.0f * std::sinf(0.6f * tAccum + 0.25f * ix);
                float tilt = 6.0f * std::sinf(0.7f * tAccum + 0.30f * iz);

                glm::mat4 model(1.0f);
                model = glm::translate(model, glm::vec3(x, y, z));
                model = glm::rotate(model, glm::radians(yaw), glm::vec3(0, 1, 0));
                model = glm::rotate(model, glm::radians(tilt), glm::vec3(1, 0, 0));
                model = glm::scale(model, glm::vec3(0.5f)); // pendant size
                lightingShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);

                // A thin "string" down to the base
                glm::mat4 s(1.0f);
                float strLen = y - 0.05f;
                s = glm::translate(s, glm::vec3(x, strLen * 0.5f, z));
                s = glm::scale(s, glm::vec3(0.06f, strLen, 0.06f));
                lightingShader.setMat4("model", s);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // Visualize point lights with small cubes
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(lightCubeVAO);
        for (int i = 0; i < 4; ++i) {
            glm::mat4 m(1.0f);
            m = glm::translate(m, pointLightPositions[i]);
            m = glm::scale(m, glm::vec3(0.25f));
            lightCubeShader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

// --------------------------------------------------------------------------------------
// Input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // Pause toggle
    static bool prevP = false;
    bool nowP = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (nowP && !prevP) paused = !paused;
    prevP = nowP;

    // Tuning animation params
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) waveSpeed += 0.8f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) waveSpeed = std::max(0.0f, waveSpeed - 0.8f * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) waveAmp += 0.8f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) waveAmp = std::max(0.0f, waveAmp - 0.8f * deltaTime);

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) { waveAmp = 0.9f; waveSpeed = 1.6f; tAccum = 0.0f; }
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y ranges bottom->top
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// --------------------------------------------------------------------------------------
// Texture loader (same as LearnOpenGL style)
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 1 ? GL_RED :
            (nrComponents == 3 ? GL_RGB : GL_RGBA));
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
            GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cerr << "Texture failed to load at: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

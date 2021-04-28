#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

int width, height;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = width  / 2.0f;
float lastY = height / 2.0f;
float fov = 45.0f;
bool firstMouse = true;

float deltaTime = 0.0f;	
float lastFrame = 0.0f;

bool disableMenu = true;

void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
}

void single_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        if (disableMenu) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            disableMenu = false;
            firstMouse = true;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            disableMenu = true;
        }
}

// Continous input that updates every frame
void processInput(GLFWwindow* window) {
    const float cameraSpeed = 3.5 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}       

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!disableMenu)
        return;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

std::vector<std::string> splitString(std::string input) {
    std::istringstream iss(input);
    std::vector<std::string> temp;
    do
    {
        std::string subs;
        iss >> subs;
        temp.push_back(subs);
    } while (iss);
    return temp;
}

void load_obj(std::string fileName, std::vector<glm::vec3> &vertices, std::vector<glm::vec3>&normals, std::vector<unsigned int> &indicies)
{
    using namespace std;

    ifstream in(fileName, ios::in);
    if (!in)
    {
        cerr << "Cannot open " << fileName << endl; exit(1);
    }

    string line;
    while (getline(in, line))
    {
        if (line.substr(0, 2) == "v ")
        {
            istringstream s(line.substr(2));
            glm::vec3 v;
            s >> v.x; s >> v.y; s >> v.z;
            vertices.push_back(v);
        }
        else if (line.substr(0, 2) == "f ")
        {
            istringstream s(line.substr(2));
            unsigned int a, b, c;
            s >> a; s >> b; s >> c;
            a--; b--; c--;
            indicies.push_back(a); indicies.push_back(b); indicies.push_back(c);
        }
    }

    normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
    for (int i = 0; i < indicies.size(); i += 3)
    {
        GLushort ia = indicies[i];
        GLushort ib = indicies[i + 1];
        GLushort ic = indicies[i + 2];
        glm::vec3 normal = glm::normalize(glm::cross(
            glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
            glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
        normals[ia] = normals[ib] = normals[ic] = normal;
    }
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    std::cout << std::endl;
}

int main(void)
{
    GLFWwindow* window;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 750, "Hello World", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to initialize glfw window" << std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize glad" << std::endl;
        return -1;
    }

    // ENALBE DISABLE DEBUG MODE
    // During init, enable debug output
    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback(MessageCallback, 0);

    float cubeVertices[] = {
        // location           
        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 

        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 

         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 

        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f 
    };

    glViewport(0, 0, 1000, 750);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, single_key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Setup Platform/Renderer backends
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::vector<glm::vec3> vertexes;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indexes;
    load_obj("cow.obj", vertexes, normals ,indexes);

    Shader cowShader( "shaders/vertexShader.shader", "shaders/fragmentShader.shader");
    Shader lightShader("shaders/lightvertexShader.shader", "shaders/lightfragmentShader.shader");
    
    // Vertex  array object
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertext buffer object, specifices vertex location, etc.
    unsigned int cowVBO, cowNormals;

    glGenBuffers(1, &cowVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(glm::vec3), &vertexes[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cowNormals);
    glBindBuffer(GL_ARRAY_BUFFER, cowNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // Elements for the indexes 
    unsigned int cowEBO;
    glGenBuffers(1, &cowEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), &indexes[0], GL_STATIC_DRAW);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);

    unsigned int cubeVBO;
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Load two textures
    unsigned int texture1;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
 
    // Load and generate texture
    int width, height, nrCHannels;
    unsigned char* data = stbi_load("Textures/cow.jpg", &width, &height, &nrCHannels, 0);
    if (data != NULL) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    glEnable(GL_DEPTH_TEST);

    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 cowRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    bool showLight = true;
    float specularStreght = 0.5f;
    int shinyness = 32;

    // Render loop until window closes
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        processInput(window);
        glfwPollEvents();

        glClearColor(0.4, 0.2, 0.3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if(disableMenu == false){
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Scene Controls");                          
            ImGui::SetNextWindowSize(ImVec2(0, 0));

            ImGui::Text("Enable or disable the light source");               
            ImGui::Checkbox("Light source", &showLight);
            
            ImGui::Text("Light source color");               
            ImGui::ColorEdit3("color", (float*)&lightColor);
            
            ImGui::SliderFloat3("Light position (x,y,z)", &lightPos.x, -5.0f, 5.0f);

            ImGui::SliderFloat("Specular strenght", &specularStreght, 0.0f, 2.0f);
            ImGui::SliderInt("Shinyness", &shinyness, 1, 256);

            ImGui::Text("Rotate the cow model (x,y,z)");               
            ImGui::SliderFloat3("Cow rotation (x,y,z)", &cowRotation[0], -180.0f, 180.0f);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            
            ImGui::End();
        }

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
     
        cowShader.use();
        cowShader.setInt("texture1", 0);
        showLight ? cowShader.setVec3("lightColor", lightColor) : cowShader.setVec3("lightColor", glm::vec3(0.0f, 0.0f, 0.0f));
        cowShader.setVec3("lightPos", lightPos);
        cowShader.setVec3("viewPos", cameraPos);
        cowShader.setFloat("strength", specularStreght);
        cowShader.setInt("shinyness", shinyness);

        // Translation and scalling 
        glm::mat4 projection = glm::perspective(glm::radians(fov), float(width) / float(height), 0.1f, 100.0f);
        cowShader.setMat4("projection", projection);

        // Model and projection matrix
        const float radius = 10.0f;
        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(cameraPos, cameraPos+cameraFront, cameraUp);
        cowShader.setMat4("view", view);
       
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(cowRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(cowRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(cowRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); 
        cowShader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_INT, 0);

        // Also draw the light source
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        lightShader.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
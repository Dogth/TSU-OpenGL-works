#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

//FIX: No mesh rendered
//FIX: Issues with VAO, VBO, EBO

//TODO: Refactor all this code
//TODO: Quaternion rotation
//TODO: Camera movement
//TODO: Error handling & logging
//TODO: Unit tests
//TODO: Documentation
GLFWwindow* window;
GLuint shaderProgram, VAO, VBO, EBO;

const glm::vec3 cameraPos = glm::vec3(3.0f, 3.0f, 3.0f);
const glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void setupRocket() {
    std::vector<GLfloat> vertices = {
        -0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,

        -0.5f, 0.0f, -0.5f,   0.0f, 0.707f, -0.707f,
         0.5f, 0.0f, -0.5f,   0.0f, 0.707f, -0.707f,
         0.0f, 1.0f,  0.0f,   0.0f, 0.707f, -0.707f,

         0.5f, 0.0f, -0.5f,   0.707f, 0.707f, 0.0f,
         0.5f, 0.0f,  0.5f,   0.707f, 0.707f, 0.0f,
         0.0f, 1.0f,  0.0f,   0.707f, 0.707f, 0.0f,

         0.5f, 0.0f,  0.5f,   0.0f, 0.707f, 0.707f,
        -0.5f, 0.0f,  0.5f,   0.0f, 0.707f, 0.707f,
         0.0f, 1.0f,  0.0f,   0.0f, 0.707f, 0.707f,

        -0.5f, 0.0f,  0.5f,  -0.707f, 0.707f, 0.0f,
        -0.5f, 0.0f, -0.5f,  -0.707f, 0.707f, 0.0f,
         0.0f, 1.0f,  0.0f,  -0.707f, 0.707f, 0.0f
    };

    std::vector<GLuint> indices = {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        7, 8, 9,
        10, 11, 12,
        13, 14, 15
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::string readShaderSource(const char* filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        exit(EXIT_FAILURE);
    }
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();
    return shaderStream.str();
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readShaderSource(vertexPath);
    std::string fragmentCode = readShaderSource(fragmentPath);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindAttribLocation(shaderProgram, 0, "aPos");
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void initOpenGL() {
    if (!glfwInit()) {
				//TODO: Error handling & logging
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "Rocket Trajectory", NULL, NULL);
    if (!window) {
				//TODO: Error handling & logging
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
				//TODO: Error handling & logging
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    shaderProgram = createShaderProgram("shader/vertex.glsl", "shader/fragment.glsl");

		setupRocket();
}

void render(const glm::mat4& projection, const glm::mat4& model) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(lightPosLoc, 1.2f, 1.0f, 2.0f);
    glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(objectColorLoc, 1.0f, 0.5f, 0.31f);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0); // Draw the pyramid
    glBindVertexArray(0);

    glfwSwapBuffers(window);
}

int main() {
    initOpenGL();

    glm::vec3 rocketPosition(0.0f, 0.0f, 0.0f);
    glm::quat rocketOrientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glClearColor(0.0f, 0.0f, 0.3f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        rocketPosition += glm::vec3(0.0f, 0.01f, 0.0f); // Example: move the rocket upwards
        rocketOrientation = glm::rotate(rocketOrientation, glm::radians(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Example: rotate the rocket

        glm::mat4 model = glm::translate(glm::mat4(1.0f), rocketPosition) * glm::mat4_cast(rocketOrientation);

        render(projection, model);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

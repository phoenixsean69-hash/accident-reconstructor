// Sovereign Accident Reconstructor — Phase 1
// OpenGL 4.6 Core. Renders a colored triangle.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>

// Forensic dark background
constexpr float BG_R = 0.08f;
constexpr float BG_G = 0.08f;
constexpr float BG_B = 0.09f;

// Vertex shader: positions + colors
const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

// Fragment shader: interpolate colors
const char* fragmentShaderSource = R"(
#version 460 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// Helper: compile shader, print errors
static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("[ERROR] Shader compilation failed:\n%s\n", infoLog);
    }
    return shader;
}

int main() {
    // ── 1. Init GLFW ─────────────────────────
    if (!glfwInit()) {
        printf("[FATAL] Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        1600, 900,
        "Sovereign Accident Reconstructor v0.1.0 — OpenGL 4.6",
        nullptr, nullptr
    );

    if (!window) {
        printf("[FATAL] Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // V-sync

    // ── 2. Load OpenGL via glad ──────────────
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("[FATAL] Failed to initialize glad\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    printf("[INFO] OpenGL %s\n", (const char*)glGetString(GL_VERSION));
    printf("[INFO] Renderer: %s\n", (const char*)glGetString(GL_RENDERER));

    // ── 3. Build shader program ──────────────
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint linkSuccess;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        printf("[ERROR] Program link failed:\n%s\n", infoLog);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    // ── 4. Upload geometry ───────────────────
    // Triangle: 3 vertices, each with position (xyz) + color (rgb)
    float vertices[] = {
        // Position              // Color (orange — evidence marker color)
         0.0f,  0.5f, 0.0f,     1.0f, 0.6f, 0.2f,   // Top
        -0.5f, -0.5f, 0.0f,     0.2f, 0.6f, 1.0f,   // Bottom left (blue)
         0.5f, -0.5f, 0.0f,     0.2f, 0.8f, 0.4f,   // Bottom right (green)
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ── 5. Main loop ─────────────────────────
    printf("[INFO] Engine running. Close window to exit.\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Clear to forensic dark
        glClearColor(BG_R, BG_G, BG_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw triangle
        glUseProgram(program);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    // ── 6. Cleanup ───────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();

    printf("[INFO] Engine shutdown complete.\n");
    return 0;
}

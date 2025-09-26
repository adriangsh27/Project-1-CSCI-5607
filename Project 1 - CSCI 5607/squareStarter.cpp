#define NOMINMAX
#include "glad/glad.h"  //Include order can matter here
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cstdio>
#include <ctime>   // For time()
#include <cstdlib> // For rand() and srand()
#include <iostream>
#include <fstream>
#include <string>
#include "pga.h"
#include <algorithm>
#include <vector>
#include <cmath> // for fmodf
#include <functional>

// Name of image texture
std::string textureName = "goldy.ppm";

// Screen size
int screen_width = 800;
int screen_height = 800;

// Background Color Variables
float bg_r = 0.62;
float bg_g = 0.0;
float bg_b = 1.0;

// Globals to store the state of the square (position, width, and angle)
Point2D rect_pos = Point2D(0, 0);
float rect_scale = 1;
float rect_angle = 0;

// Animation angle (rotates around square's center)
float anim_angle = 0.0f; // degrees
float anim_speed = 1.0f; // degrees per second (adjust to taste)

struct shape {
    uint32_t vertex_num;
    //  X     Y     R     G     B     U    V
    std::vector<float> vertices;
    std::vector<Point2D> initial_points;
    std::vector<Point2D> points;
};

std::vector<float> sq_vertices = {  // The function updateVertices() changes these values to match p1,p2,p3,p4
    //  X     Y     R     G     B     U    V
      -0.3f, -0.3f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom left
      0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.3f,  0.3f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
      0.3f,  0.3f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top right
};
std::vector<Point2D> sq_init_points = { Point2D(sq_vertices[0], sq_vertices[1]), Point2D(sq_vertices[7], sq_vertices[8]), Point2D(sq_vertices[21], sq_vertices[22]), Point2D(sq_vertices[14], sq_vertices[15]) };
shape square = { 4, sq_vertices, sq_init_points, sq_init_points };

std::vector<float> tri_vertices = {  // The function updateVertices() changes these values to match p1,p2,p3,p4
    //  X     Y     R     G     B     U    V
      0.3f, -0.3f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      -0.3f,  0.3f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
      0.3f,  0.3f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top right
};
std::vector<Point2D> tri_init_points = { Point2D(sq_vertices[0], sq_vertices[1]), Point2D(sq_vertices[7], sq_vertices[8]), Point2D(sq_vertices[14], sq_vertices[15]) };
shape triangle = { 3 , tri_vertices, tri_init_points, tri_init_points };


shape current_shape = square;

Point2D screen_origin = Point2D(0, 0);

// Helper variables for mouse interaction
Point2D clicked_pos;
Point2D clicked_mouse;
float clicked_angle, clicked_size;

void mouseClicked(float mx, float my); // Called whenever mouse is pressed down
void mouseDragged(float mx, float my); // Called every time mouse moves while button is down
void updateVertices();

bool do_translate = false;
bool do_rotate = false;
bool do_scale = false;

//TODO: Read from ASCII (P3) PPM files properly
//Note: Reference/output variables img_w and img_h used to return image width and height
unsigned char* loadImage(int& img_w, int& img_h) {
    // Open the texture image file
    std::ifstream ppmFile;
    ppmFile.open(textureName.c_str());
    if (!ppmFile) {
        printf("ERROR: Texture file '%s' not found.\n", textureName.c_str());
        exit(1);
    }

    // Check that this is an ASCII (P3) PPM file (i.e the first line is "P3")
    std::string PPM_style;
    ppmFile >> PPM_style; // Read the first line of the file/header
    if (PPM_style != "P3") {
        printf("ERROR: PPM Type number is %s. Not an ASCII (P3) PPM file!\n", PPM_style.c_str());
        exit(1);
    }

    // Read in the image dimensions (width, height)
    ppmFile >> img_w >> img_h;
    unsigned char* img_data = new unsigned char[4 * img_w * img_h];

    // Check that the 3rd line of the header is 255 (ie., this is an 8-bit per pixel image)
    int maximum;
    ppmFile >> maximum;
    if (maximum != 255) {
        printf("ERROR: Maximum size is (%d) not 255.\n", maximum);
        exit(1);
    }

    int r, g, b;

    for (int i = img_h - 1; i > -1; i--) {
        for (int j = img_w - 1; j > -1; j--) {
            ppmFile >> r >> g >> b;

            int max_safe = 255 - std::max({ r, g, b });
            int offset = std::min(80, max_safe);

            img_data[i * img_w * 4 + j * 4] = r + offset;  // Red 
            img_data[i * img_w * 4 + j * 4 + 1] = g + offset; // Green
            img_data[i * img_w * 4 + j * 4 + 2] = b + offset; // Blue
            img_data[i * img_w * 4 + j * 4 + 3] = 255; // Alpha
        }
    }
    return img_data;
}

bool pointInPolygon(Point2D p, const std::vector<Point2D>& vertices) {
    int initial_sign = sign(vee(p, vertices[0], vertices[1]));
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Point2D& t1 = vertices[i];
        const Point2D& t2 = vertices[(i + 1) % vertices.size()];
        int current_sign = sign(vee(p, t1, t2));
        if (current_sign != initial_sign) {
            return false;
        }
    }
    return true;
}

// Helper function to calculate the squared distance between two points
float distSq(Point2D p1, Point2D p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

// Function to calculate the distance from a point p to a line segment a-b
float pointLineSegmentDist(Point2D p, Point2D a, Point2D b) {
    // Calculate the squared length of the line segment
    float l2 = distSq(a, b);
    if (l2 == 0.0) { // a and b are the same point
        return std::sqrt(distSq(p, a));
    }

    float t = dot(vee(p, a), vee(b, a)) / l2;
    t = clamp(t, 0.0, 1.0);
    Point2D closest = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    return std::sqrt(distSq(p, closest));
}

bool pointOnPolygon(Point2D p, const std::vector<Point2D>& vertices) {
    float min_dist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < vertices.size(); ++i) {
        const Point2D& t1 = vertices[i];
        const Point2D& t2 = vertices[(i + 1) % vertices.size()];
        float d = pointLineSegmentDist(p, t1, t2);
        min_dist = std::min(min_dist, d);
    }

    // added a threshold because it's hard to click on the edge exactly.
    return min_dist < 0.05 && min_dist > 0 ? true : false;
}

bool pointEquality(Point2D p, const std::vector<Point2D>& vertices) {
    for (int i = 0; i < vertices.size(); i++) {
        Point2D vertex = vertices[i];
        // added a threshold because it's hard to click on the corner exactly.
        if (vertex.x < p.x + 0.1 && vertex.x > p.x - 0.1 && vertex.y < p.y + 0.1 && vertex.y > p.y - 0.1) return true;
    }
    return false;
}

float rect_drag_start_angle;
void mouseClicked(float m_x, float m_y) {
    // We may need to know where both the mouse and the rectangle were at the moment the mouse was clicked
    // so we store these values in global variables for later use
    clicked_mouse = Point2D(m_x, m_y);
    clicked_pos = rect_pos;
    clicked_angle = atan2(clicked_mouse.y - rect_pos.y,
        clicked_mouse.x - rect_pos.x);
    rect_drag_start_angle = rect_angle;
    clicked_size = rect_scale;


    // Helper global variables to determine if we are translating, rotating, or scaling
    // You will need to change this logic to choose between the 3 operations based on where the mouse was clicked
    do_translate = false;
    do_rotate = false;
    do_scale = false;

    if (pointEquality(clicked_mouse, current_shape.points)) {
        do_scale = true;
    }
    else if (pointOnPolygon(clicked_mouse, current_shape.points)) {
        do_rotate = true;
    }
    else if (pointInPolygon(clicked_mouse, current_shape.points)) {
        do_translate = true;
    }
}

void mouseDragged(float m_x, float m_y) {
    Point2D cur_mouse = Point2D(m_x, m_y);
    Dir2D disp = cur_mouse - clicked_mouse;

    if (do_translate) {
        // Compute the position, rect_pos, By first finding the displacement vector of the mouse since the initial click, then move the rectangle by the same amount
        rect_pos = clicked_pos + disp;
    }

    if (do_scale) {
        // Compute the new size, rect_scale, based on how far the mouse has moved around the rectangle since the initial click
        rect_scale = clicked_size + (disp.x * sign(clicked_mouse.x)) + (disp.y * sign(clicked_mouse.y));
    }

    if (do_rotate) {
        float current_angle = atan2(m_y - rect_pos.y, m_x - rect_pos.x);
        float delta_angle = current_angle - clicked_angle;

        rect_angle = rect_drag_start_angle - delta_angle;
    }

    // Note: we no longer update p1..p4 in this function; the per-frame update in main() will recompute positions
}

// Dirty implementation, not generalized
void updateVertices() {
    if (current_shape.vertex_num == 3) {
        for (uint32_t i = 0; i < current_shape.vertex_num; i++) {
            current_shape.vertices[i * 7] = current_shape.points[i].x;
            current_shape.vertices[(i * 7) + 1] = current_shape.points[i].y;
        }
    }
    else {
        current_shape.vertices[0] = current_shape.points[2].x; current_shape.vertices[1] = current_shape.points[2].y;     // Top right x & y
        current_shape.vertices[7] = current_shape.points[1].x; current_shape.vertices[8] = current_shape.points[1].y;     // Bottom right x & y
        current_shape.vertices[14] = current_shape.points[3].x; current_shape.vertices[15] = current_shape.points[3].y;   // Top left x & y
        current_shape.vertices[21] = current_shape.points[0].x; current_shape.vertices[22] = current_shape.points[0].y; // Bottom left x & y
    }
}

void r_keyPressed() {
    rect_pos = Point2D(0, 0);
    rect_scale = 1;
    rect_angle = 0;

    current_shape.points = current_shape.initial_points;

    updateVertices();
}

void b_keyPressed() {
    bg_r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    bg_g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    bg_b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void c_keyPressed(GLuint vbo) {
    if (current_shape.vertex_num == 4) current_shape = triangle;
    else if (current_shape.vertex_num == 3) current_shape = square;
    r_keyPressed();
}

// ----------------------- //
// Below this line is OpenGL specific code
// ----------------------- //

// Shader sources
const GLchar* vertexSource =
"#version 150 core\n" // Shader Version (150 --> OpenGL 3.2)
"in vec2 position; in vec3 inColor; in vec2 inTexcoord;" // Input vertex data (position, color, and texture coordinates)
"out vec3 Color; out vec2 texcoord;" // Output data to fragment shader (color and texture coordinates)
"void main() { Color = inColor; gl_Position = vec4(position, 0.0, 1.0); texcoord = inTexcoord; }"; // Position is a vec4 (x,y,z,w) so z=0 and w=1
const GLchar* fragmentSource =
"#version 150 core\n"
"uniform sampler2D tex0; in vec2 texcoord; out vec3 outColor;" // Input texture and texture coordinates from vertex shader, output is pixel color
"void main() { outColor = texture(tex0, texcoord).rgb; }"; // Set pixel color based on texture color at the texture coordinates

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(0)));
    //Initialize Graphics (for OpenGL)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    //Ask SDL to get a fairly recent version of OpenGL (3.2 or greater)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    //Create a window (title, width, height, flags)
    SDL_Window* window = SDL_CreateWindow("Adrian's Sophisticated Shape (CSCI 5607)", screen_width, screen_height, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    updateVertices();

    // Create an OpenGL context associated with the window (the context is like a "state" for OpenGL)
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // OpenGL functions are loaded from the driver using glad library
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("ERROR: Failed to initialize OpenGL context.\n");
        return -1;
    }
    printf("OpenGL loaded\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));

    // Allocate and assign a texture name (tex0) to an OpenGL texture object
    GLuint tex0; glGenTextures(1, &tex0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load texture data from a file
    int img_w, img_h;
    unsigned char* img_data = loadImage(img_w, img_h);
    printf("Loaded image %s (width=%d, height=%d)\n", textureName.c_str(), img_w, img_h);

    // Assign the image to the OpenGL texture object on the GPU's memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_w, img_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Allocate a Vertex Buffer Objects (VBOs) to store geometry (vertex data) on the GPU such as vertex position and color.
    GLuint vbo; glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, current_shape.vertices.size() * sizeof(float), current_shape.vertices.data(), GL_DYNAMIC_DRAW);

    // Load the vertex shader to the GPU
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
        printf("Vertex Shader Compile Error: %s\n", buffer);
    }

    // Load the fragment shader to the GPU
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
        printf("Fragment Shader Compile Error: %s\n", buffer);
    }

    // Link the vertex and fragment shaders together into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);

    glUseProgram(shaderProgram);

    // Tell OpenGL how to set fragment shader input variables (in vec3 Color) from the vertex data array.
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
    glEnableVertexAttribArray(posAttrib);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(colAttrib);

    GLint texAttrib = glGetAttribLocation(shaderProgram, "inTexcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(5 * sizeof(float)));

    // Status booleans that update based on user input
    bool done = false;
    bool mouse_dragging = false;
    bool fullscreen = false;

    // Timing for animation
    Uint64 last_ticks = SDL_GetTicks();

    // Event Loop
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                done = true;
                break;

            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_ESCAPE) done = true;
                if (event.key.key == SDLK_F) {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window, fullscreen);
                }
                if (event.key.key == SDLK_R) r_keyPressed();
                if (event.key.key == SDLK_B) b_keyPressed();
                if (event.key.key == SDLK_C) c_keyPressed(vbo);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    float mouse_x = 2 * event.button.x / (float)screen_width - 1;
                    float mouse_y = 1 - 2 * event.button.y / (float)screen_height;
                    mouseClicked(mouse_x, mouse_y);
                    mouse_dragging = true;
                    anim_speed = 0;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_dragging = false;
                    anim_speed = 1.0f;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                float mouse_x = 2 * event.motion.x / (float)screen_width - 1;
                float mouse_y = 1 - 2 * event.motion.y / (float)screen_height;
                if (mouse_dragging) {
                    mouseDragged(mouse_x, mouse_y);
                }
                break;
            }
        }

        // --- Animation timing ---
        Uint64 now_ticks = SDL_GetTicks();
        float dt_seconds = (now_ticks - last_ticks) / 1000.0f;
        last_ticks = now_ticks;

        //Advance animation angle (degrees)
        anim_angle += anim_speed * dt_seconds;
        // Keep anim_angle in a reasonable range
        anim_angle = fmodf(anim_angle, 360.0f);

        // Apply user rotation (around rect_pos) and translation -> movement motor as in your mouseDragged code
        Motor2D translate = Translator2D(rect_pos - screen_origin);
        Motor2D rotate = Rotator2D(rect_angle, rect_pos);
        Motor2D movement = rotate * translate;

        // Now apply global animation rotation around the origin (screen_origin)
        Motor2D anim_rot = Rotator2D(anim_angle, rect_pos);

        for (uint32_t i = 0; i < current_shape.vertex_num; i++) {
            current_shape.points[i] = transform(transform(current_shape.initial_points[i].scale(rect_scale), movement), anim_rot);
        }

        // Update vertex array to match transformed points
        updateVertices();

        // Add the vertices to the vertex buffer (VBO) each frame since they may change every frame
        glBufferData(GL_ARRAY_BUFFER, current_shape.vertices.size() * sizeof(float), current_shape.vertices.data(), GL_DYNAMIC_DRAW);

        // Clear the screen to background color
        glClearColor(bg_r, bg_g, bg_b, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, current_shape.vertex_num);

        SDL_GL_SwapWindow(window);
    }

    delete[] img_data;
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

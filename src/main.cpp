#include "tangram.h"
#include "platform.h"
#include "gl.h"
#include "glfontstash.h"
#include <cmath>
#include <random>

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds
const double scroll_multiplier = 0.05; // scaling for zoom

bool was_panning = false;
double last_mouse_up = -double_tap_time; // First click should never trigger a double tap
double last_x_down = 0.0;
double last_y_down = 0.0;

FONScontext* ftCtx;
fsuint textBuffer;
#define NB_TEXT 3000

int nextPowerOf2(int value) {
    return pow(2, ceil(log(value) / log(2)));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    if (was_panning) {
        was_panning = false;
        return; // Clicks with movement don't count as taps
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    double time = glfwGetTime();

    if (action == GLFW_PRESS) {
        last_x_down = x;
        last_y_down = y;
        return;
    }

    if (time - last_mouse_up < double_tap_time) {
        Tangram::handleDoubleTapGesture(x, y);
    } else {
        Tangram::handleTapGesture(x, y);
    }

    last_mouse_up = time;

}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);

    if (action == GLFW_PRESS) {

        if (was_panning) {
            Tangram::handlePanGesture(last_x_down, last_y_down, x, y);
        }

        was_panning = true;
        last_x_down = x;
        last_y_down = y;
    }

}

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (shoving) {
        Tangram::handleShoveGesture(scroll_multiplier * scrolly);
    } else if (rotating) {
        Tangram::handleRotateGesture(x, y, scroll_multiplier * scrolly);
    } else {
        Tangram::handlePinchGesture(x, y, 1.0 + scroll_multiplier * scrolly);
    }

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                Tangram::setDebugFlag(Tangram::DebugFlags::FREEZE_TILES, !Tangram::getDebugFlag(Tangram::DebugFlags::FREEZE_TILES));
                break;
            case GLFW_KEY_2:
                Tangram::setDebugFlag(Tangram::DebugFlags::PROXY_COLORS, !Tangram::getDebugFlag(Tangram::DebugFlags::PROXY_COLORS));
                break;
            case GLFW_KEY_3:
                Tangram::setDebugFlag(Tangram::DebugFlags::TILE_BOUNDS, !Tangram::getDebugFlag(Tangram::DebugFlags::TILE_BOUNDS));
                break;
            case GLFW_KEY_4:
                Tangram::setDebugFlag(Tangram::DebugFlags::TILE_INFOS, !Tangram::getDebugFlag(Tangram::DebugFlags::TILE_INFOS));
                break;
            default:
                break;
        }
    }
}


// Window handling
// ===============

void window_size_callback(GLFWwindow* window, int width, int height) {

    Tangram::resize(width, height);

}

// Main program
// ============

int main(void) {

    GLFWwindow* window;
    int width = 800;
    int height = 600;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_SAMPLES, 2);
    window = glfwCreateWindow(width, height, "Tangram ES - ASCII MAP", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    NSurlInit();

    Tangram::initialize();
    Tangram::resize(width, height);

    // init font context
    GLFONSparams params;
    params.useGLBackend = true; // if not set to true, you must provide your own gl backend
    ftCtx = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT, params, nullptr);
    fonsAddFont(ftCtx, "Arial", "/Library/Fonts/Arial.ttf");

    // set the screen size for font context transformations
    glfonsScreenSize(ftCtx, width, height);

    // create and bind buffer
    glfonsBufferCreate(ftCtx, nextPowerOf2(NB_TEXT), &textBuffer);

    // generate text ids for the currently bound text buffer

    struct Text {
        fsuint id;
        float x;
        float y;
    };

    Text texts[NB_TEXT];

    for (int i = 0; i < NB_TEXT; ++i) {
        glfonsGenText(ftCtx, 1, &texts[i].id);
    }

    fonsSetBlur(ftCtx, 2.5);
    fonsSetBlurType(ftCtx, FONS_EFFECT_DISTANCE_FIELD);
    fonsSetSize(ftCtx, 20.0);

    for (int i = 0; i < NB_TEXT; ++i) {
        glfonsRasterize(ftCtx, texts[i].id, "O");
    }

    // push transforms of currently bound buffer buffer to gpu
    glfonsUpdateTransforms(ftCtx);

    // upload rasterized data of currently bound buffer to gpu
    glfonsUpload(ftCtx);

    /* Work-around for a bug in GLFW on retina displays */
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    /* Set input callbacks */
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSwapInterval(1);

    double lastTime = glfwGetTime();

    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);
     
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTextureEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int pixelData[width * height];
    float data[width * height];
    //std::string chars = " .\'`^\",:;Il!i~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8\%B@$";

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {

        double t = glfwGetTime();
        double delta = t - lastTime;
        lastTime = t;

        glBindTexture(GL_TEXTURE_2D, renderedTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height); 

        /* Render here */
        Tangram::update(delta);
        Tangram::render();

        glBindTexture(GL_TEXTURE_2D, renderedTexture);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);

        for (int i = 0; i < width * height; ++i) {
            unsigned int pixel = pixelData[i];
            float b = pixel & 0xff;
            float g = pixel >> 8 & 0xff;
            float r = pixel >> 16 & 0xff;
            data[i] = ((r + g + b) / 3.0) / 255.0;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // push transforms to gpu
        glfonsUpdateTransforms(ftCtx);

        int i = 0;
        for (int x = 15; x < width - 15; x += 12) {
            for (int y = 15 * 1.8; y < height - 15; y += 12) {
                if (i < NB_TEXT) {
                    glfonsTransform(ftCtx, texts[i].id, x, y, 0.0, data[x + y * width]);
                }
                i++;
            }
        }

        // set rendering color for current buffer
        glfonsSetColor(ftCtx, 0x000000);

        // render the text
        glfonsDraw(ftCtx);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        if (isContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
    }

    Tangram::teardown();
    glfwTerminate();
    return 0;
}
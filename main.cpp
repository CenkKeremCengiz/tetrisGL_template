//
// Author: Ahmet Oguz Akyuz
//
// This is a sample code that draws a single block piece at the center
// of the window. It does many boilerplate work for you -- but no
// guarantees are given about the optimality and bug-freeness of the
// code. You can use it to get started or you can simply ignore its
// presence. You can modify it in any way that you like.
//
//


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

enum class View {
    Front,
    Left,
    Back,
    Right
};

GLuint gProgram[3];
int gWidth = 600, gHeight = 1000;
GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLuint gTex2D;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;
int gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];
GLint lightPosLoc[2];
GLint kdLoc[2];

// **********************************************
glm::vec3 cubeGroupPosition(0.0f, 7.0f, 0.0f);
float cameraAngle      = 0.0f;
float targetCameraAngle = 0.0f;
float cameraRadius     = 24.0f;
float rotationSpeed = 3.0f;

const float GAME_AREA_HALF_SIZE = 4.5f;

const float CUBE_GROUP_HALF_SIZE = 1.5f;

float lightYOffset = 5.0f;

std::string currentViewName = "Front";

bool gameOver = false;

bool cameraMoved;

// **********************************************


glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix = glm::mat4(1.f);
glm::vec3 eyePos = glm::vec3(0, 0, 24);
glm::vec3 lightPos = glm::vec3(0, 0, 7);

glm::vec3 kdGround(0.334, 0.288, 0.635);
glm::vec3 kdCubes(0.86, 0.11, 0.31);

float lastDropTime = 0.0f;     // Son düşme zamanı (başlangıçta sıfır)
float cubeDropInterval = 99999.0f; // Küplerin düşme aralığı (0.5 saniye)
bool gameRunning = true;       // Oyun başlangıçta çalışır durumda


int score = 0;

int activeProgramIndex = 0;

// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

glm::vec3 getCameraRightVector()
{
    float cameraAngleRad = glm::radians(cameraAngle);
    glm::vec3 right;
    right.x = sin(cameraAngleRad);
    right.y = 0.0f;
    right.z = -cos(cameraAngleRad);
    right = glm::normalize(right);
    return right;
}

// For reading GLSL files
bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

View getCurrentView(float angle) {
    angle = fmod(angle, 360.0f);
    if (angle < 0.0f) angle += 360.0f;

    if (angle >= 315.0f || angle < 45.0f)
        return View::Front;
    else if (angle >= 45.0f && angle < 135.0f)
        return View::Left;
    else if (angle >= 135.0f && angle < 225.0f)
        return View::Back;
    else
        return View::Right;
}

std::string viewToString(View view) {
    switch (view) {
        case View::Front:
            return "Front";
        case View::Left:
            return "Left";
        case View::Back:
            return "Back";
        case View::Right:
            return "Right";
        default:
            return "Unknown";
    }
}


GLuint createVS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

        return vs;
}

GLuint createFS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

        return fs;
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 0, &face))
    //if (FT_New_Face(ft, "/usr/share/fonts/truetype/gentium-basic/GenBkBasR.ttf", 0, &face)) // you can use different fonts
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint) face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initShaders()
{
        // Create the programs

    gProgram[0] = glCreateProgram();
        gProgram[1] = glCreateProgram();
        gProgram[2] = glCreateProgram();

        // Create the shaders for both programs

    GLuint vs1 = createVS("vert.glsl"); // for cube shading
    GLuint fs1 = createFS("frag.glsl");

        GLuint vs2 = createVS("vert2.glsl"); // for border shading
        GLuint fs2 = createFS("frag2.glsl");

        GLuint vs3 = createVS("vert_text.glsl");  // for text shading
        GLuint fs3 = createFS("frag_text.glsl");

        // Attach the shaders to the programs

        glAttachShader(gProgram[0], vs1);
        glAttachShader(gProgram[0], fs1);

        glAttachShader(gProgram[1], vs2);
        glAttachShader(gProgram[1], fs2);

        glAttachShader(gProgram[2], vs3);
        glAttachShader(gProgram[2], fs3);

        // Link the programs

    for (int i = 0; i < 3; ++i)
    {
        glLinkProgram(gProgram[i]);
        GLint status;
        glGetProgramiv(gProgram[i], GL_LINK_STATUS, &status);

        if (status != GL_TRUE)
        {
            cout << "Program link failed: " << i << endl;
            exit(-1);
        }
    }


        // Get the locations of the uniform variables from both programs

        for (int i = 0; i < 2; ++i)
        {
                modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
                viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
                projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
                eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
                lightPosLoc[i] = glGetUniformLocation(gProgram[i], "lightPos");
                kdLoc[i] = glGetUniformLocation(gProgram[i], "kd");

        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(modelingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
        glUniform3fv(eyePosLoc[i], 1, glm::value_ptr(eyePos));
        glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPos));
        glUniform3fv(kdLoc[i], 1, glm::value_ptr(kdCubes));
        }
}

// VBO setup for drawing a cube and its borders
void initVBO()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao > 0);
    glBindVertexArray(vao);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        assert(glGetError() == GL_NONE);

        glGenBuffers(1, &gVertexAttribBuffer);
        glGenBuffers(1, &gIndexBuffer);

        assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

        glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    GLuint indices[] = {
        0, 1, 2, // front
        3, 0, 2, // front
        4, 7, 6, // back
        5, 4, 6, // back
        0, 3, 4, // left
        3, 7, 4, // left
        2, 1, 5, // right
        6, 2, 5, // right
        3, 2, 7, // top
        2, 6, 7, // top
        0, 4, 1, // bottom
        4, 5, 1  // bottom
    };

    GLuint indicesLines[] = {
        7, 3, 2, 6, // top
        4, 5, 1, 0, // bottom
        2, 1, 5, 6, // right
        5, 4, 7, 6, // back
        0, 1, 2, 3, // front
        0, 3, 7, 4, // left
    };

    GLfloat vertexPos[] = {
        0, 0, 1, // 0: bottom-left-front
        1, 0, 1, // 1: bottom-right-front
        1, 1, 1, // 2: top-right-front
        0, 1, 1, // 3: top-left-front
        0, 0, 0, // 0: bottom-left-back
        1, 0, 0, // 1: bottom-right-back
        1, 1, 0, // 2: top-right-back
        0, 1, 0, // 3: top-left-back
    };

    GLfloat vertexNor[] = {
         1.0,  1.0,  1.0, // 0: unused
         0.0, -1.0,  0.0, // 1: bottom
         0.0,  0.0,  1.0, // 2: front
         1.0,  1.0,  1.0, // 3: unused
        -1.0,  0.0,  0.0, // 4: left
         1.0,  0.0,  0.0, // 5: right
         0.0,  0.0, -1.0, // 6: back
         0.0,  1.0,  0.0, // 7: top
    };

        gVertexDataSizeInBytes = sizeof(vertexPos);
        gNormalDataSizeInBytes = sizeof(vertexNor);
    gTriangleIndexDataSizeInBytes = sizeof(indices);
    gLineIndexDataSizeInBytes = sizeof(indicesLines);
    int allIndexSize = gTriangleIndexDataSizeInBytes + gLineIndexDataSizeInBytes;

        glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexPos);
        glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, vertexNor);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndexSize, 0, GL_STATIC_DRAW);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, gTriangleIndexDataSizeInBytes, indices);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes, indicesLines);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}
void initCamera()
{
    // cameraAngle üzerinden kamera konumunu hesapla
    float cameraAngleRad = glm::radians(cameraAngle);  // cameraAngle = 0 ise sin(0)=0, cos(0)=1
    float camX = cameraRadius * cos(cameraAngleRad);
    float camZ = cameraRadius * sin(cameraAngleRad);

    // Kameranın dünya koordinatındaki konumu
    eyePos = glm::vec3(camX, 0.0f, camZ);

    // LookAt matrisi
    viewingMatrix = glm::lookAt(
        eyePos,                // Kamera konumu
        glm::vec3(0, 0, 0),    // Hedef
        glm::vec3(0, 1, 0)     // Yukarı vektör (Dünya'da Y ekseni)
    );

    // Kamera ayarlarını shader programlarına aktar
    for (int i = 0; i < 2; ++i) {
        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(viewingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
        glUniform3fv(eyePosLoc[i], 1, glm::value_ptr(eyePos));
    }
}

void initLight()
{
    // Işık pozisyonunu yeniden hesapla
    glm::vec3 forward = glm::normalize(-glm::vec3(viewingMatrix[2])); // Kamera yönü
    lightPos = eyePos + forward * 7.0f + glm::vec3(0.0f, lightYOffset, 0.0f);

    // Işık pozisyonunu shader programlarına gönder
    for (int i = 0; i < 2; ++i) {
        glUseProgram(gProgram[i]);
        glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPos));
    }
}


void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // polygon offset is used to prevent z-fighting between the cube and its borders
    glPolygonOffset(0.5, 0.5);
    glEnable(GL_POLYGON_OFFSET_FILL);

    initShaders();
    initVBO();
    initFonts(gWidth, gHeight);
    initCamera();
    initLight();
}

void drawCube()
{
        glUseProgram(gProgram[0]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void drawCubeEdges()
{
    glLineWidth(3);

        glUseProgram(gProgram[1]);

    for (int i = 0; i < 6; ++i)
    {
            glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(gTriangleIndexDataSizeInBytes + i * 4 * sizeof(GLuint)));
    }
}


void drawFallingCube() {
    // 3×3×3 bloc
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            for(int k=0; k<3; k++){
                glm::vec3 offset(i - 1.5f, j - 1.5f, k - 1.5f);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), cubeGroupPosition + offset);

                // Dolgu
                glUseProgram(gProgram[0]);
                glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(model));
                glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdCubes));
                drawCube();

                // Kenar
                glUseProgram(gProgram[1]);
                glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(model));
                glm::vec3 edgeColor(1.0f, 1.0f, 1.0f);
                glUniform3fv(kdLoc[1], 1, glm::value_ptr(edgeColor));
                drawCubeEdges();
            }
        }
    }
}




void drawGround()
{
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, glm::vec3(i, -0.5f, j));

            model = glm::translate(model, glm::vec3(-4.5f, -6.5f,  -4.5f));

            model = glm::scale(model, glm::vec3(1.0f, 0.5f, 1.0f));

            glUseProgram(gProgram[0]);
            glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE,  glm::value_ptr(model));
            glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdGround));
            drawCube();

            glUseProgram(gProgram[1]);
            glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE,  glm::value_ptr(model));
            glm::vec3 edgeColor(1.0f, 1.0f, 1.0f);
            glUniform3fv(kdLoc[1], 1, glm::value_ptr(edgeColor));
            drawCubeEdges();
        }
    }
}

class BoundingBox {
public:
    glm::vec3 min; // Min köşe noktası
    glm::vec3 max; // Max köşe noktası

    BoundingBox(const glm::vec3& position, const glm::vec3& size) {
        min = position - size * 0.5f;
        max = position + size * 0.5f;
    }

    // Çarpışma kontrolü: İki kutunun kesişip kesişmediğini kontrol eder
    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    // Bir noktanın bu kutunun içinde olup olmadığını kontrol eder
    bool contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
               (point.y >= min.y && point.y <= max.y) &&
               (point.z >= min.z && point.z <= max.z);
    }
};

BoundingBox createGroundBoundingBox() {
    // Zemin ortalama Y değeri: -5.25 (üst: -5.0, alt: -5.5).
    // X ve Z boyutu 9×9, Y boyutu 0.5.
    glm::vec3 groundCenter(0.0f, -6.8f, 0.0f);
    glm::vec3 groundSize(9.0f, 0.5f, 9.0f);
    return BoundingBox(groundCenter, groundSize);
}

std::vector<BoundingBox> settledBlocks; // Sabitlenen blokların bounding box'ları

bool checkGroundCollision(const BoundingBox& cubeBox) {
    BoundingBox groundBox = createGroundBoundingBox();
    return cubeBox.intersects(groundBox);
}

bool checkBlockCollision(const BoundingBox& cubeBox) {
    for (const auto& block : settledBlocks) {
        if (cubeBox.intersects(block)) {
            return true;
        }
    }
    return false;
}


void checkLineCompletion() {
    std::map<float, std::vector<BoundingBox>> levelMap;

    for(const auto& block : settledBlocks) {
        levelMap[block.min.y].push_back(block);
    }

    for (auto it = levelMap.begin(); it != levelMap.end(); ++it) {
        float y = it->first;  // The key (y-level)
        const std::vector<BoundingBox>& blockList = it->second;  // The value (vector of blocks)

    }

    for(const auto& [level, levelBlocks] : levelMap) {
        if(levelBlocks.size() == 9) {
            // Remove completed level
            settledBlocks.erase(
                std::remove_if(settledBlocks.begin(), settledBlocks.end(),
                    [level](const BoundingBox& b) { return b.min.y == level; }
                ),
                settledBlocks.end()
            );

            // Move blocks down
            for(auto& block : settledBlocks) {
                if(block.min.y > level) {
                    block.min.y -= 3.0f;
                    block.max.y -= 3.0f;
                }
            }

            score += 243;
        }
    }
}

void updateCubePosition(float currentTime) {
    if (gameOver || !gameRunning) return; // Stop updates if the game is over

    // Drop the cube at regular intervals
    if (currentTime - lastDropTime >= cubeDropInterval) {
        glm::vec3 newPosition = cubeGroupPosition;
        newPosition.y -= 1.0f; // Move down 1 unit

        BoundingBox currentCube(newPosition, glm::vec3(2.99f, 2.99f, 2.99f));

        // Check for collision with ground or blocks
        if (checkGroundCollision(currentCube) || checkBlockCollision(currentCube)) {
            // Add the block to the settled list
            BoundingBox settledBlock(cubeGroupPosition, glm::vec3(2.99f, 2.99f, 2.99f));
            settledBlocks.push_back(settledBlock);

            // Update the grid
            // markGroundGrid(settledBlock);

            // // Check if the ground is full
            // if (isGroundFull()) {
            //     clearGroundAndMoveBlocksDown();
            // }

            checkLineCompletion();

            // Create a new block
            cubeGroupPosition = glm::vec3(0.0f, 7.0f, 0.0f);

            // Check for Game Over: If the new block overlaps with settled blocks
            BoundingBox newCube(cubeGroupPosition, glm::vec3(2.99f, 2.99f, 2.99f));
            if (checkBlockCollision(newCube)) {
                gameOver = true; // Set game over
            }

            lastDropTime = currentTime; // Reset timer
            return;
        }

        // No collision, update position
        cubeGroupPosition = newPosition;
        lastDropTime = currentTime;
    }
}


void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void drawSettledBlocks()
{
    // Her sabit kutu için
    for (const auto& blockBox : settledBlocks)
    {
        // blockBox.min ve blockBox.max var.
        // Ortasını bulmak için:
        glm::vec3 center = 0.5f * (blockBox.min + blockBox.max);
        // Bu kutunun boyutu 3x3x3 ise:
        // (Düşerken de 3×3 boyutlu demiştik.)
        // Asıl mantık: 3×3’lük küçük küp dizisini
        // blockBox’ın merkezine göre translate edip çizmek.

        // 3×3’lük çizim (küçük küpler) yapacaksanız:
        for(int i=0; i<3; i++){
            for(int j=0; j<3; j++){
                for(int k=0; k<3; k++){
                    glm::vec3 offset(i - 1.5f, j - 1.5f, k - 1.5f);
                    // blockBox’ın ortasına offset ekle
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), center + offset);

                    // Dolgu
                    glUseProgram(gProgram[0]);
                    glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(model));
                    glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdCubes));
                    drawCube();

                    // Kenar
                    glUseProgram(gProgram[1]);
                    glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(model));
                    glm::vec3 edgeColor(1.0f, 1.0f, 1.0f);
                    glUniform3fv(kdLoc[1], 1, glm::value_ptr(edgeColor));
                    drawCubeEdges();
                }
            }
        }
    }
}

void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    drawGround();
    drawFallingCube();
    drawSettledBlocks();

    // Render "Game Over" if the game has ended
    if (gameOver) {
        std::string gameOverText = "Game Over";
        // Calculate the position to center the text
        GLfloat textScale = 1.0f;
        GLfloat textWidth = gameOverText.length() * 25.0f * textScale; // Approximate width
        GLfloat x = (gWidth - textWidth) / 2.0f;
        GLfloat y = gHeight / 2.0f;

        renderText(gameOverText, x, y, textScale, glm::vec3(1.0f, 0.0f, 0.0f)); // Red color
    }

    assert(glGetError() == GL_NO_ERROR);
}


void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);

    float fovyRad = (float)(45.0 / 180.0) * M_PI;
    projectionMatrix = glm::perspective(fovyRad, gWidth / (float)gHeight, 1.0f, 100.0f);

    // Kamera matrisini bir kez daha güncelle
    initCamera();

    // Projeksiyon matrisini shader'a gönder
    for (int i = 0; i < 2; ++i)
    {
        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(projectionMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    }
}


void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (action == GLFW_PRESS && gameRunning && !gameOver) { // Only accept inputs if the game is running
        glm::vec3 right = getCameraRightVector();

        if (key == GLFW_KEY_H) {
            targetCameraAngle -= 90.0f;
        }
        else if (key == GLFW_KEY_K) {
            targetCameraAngle += 90.0f;
        }
        else if (key == GLFW_KEY_A && !cameraMoved) {
            glm::vec3 newPosition = cubeGroupPosition - right * 1.0f;
            BoundingBox currentCube(newPosition, glm::vec3(2.99f, 2.99f, 2.99f));
            if (!checkGroundCollision(currentCube) && !checkBlockCollision(currentCube)) {
                cubeGroupPosition = newPosition;
            }
        }
        else if (key == GLFW_KEY_D && !cameraMoved) {
            glm::vec3 newPosition = cubeGroupPosition + right * 1.0f;
            BoundingBox currentCube(newPosition, glm::vec3(2.99f, 2.99f, 2.99f));
            if (!checkGroundCollision(currentCube) && !checkBlockCollision(currentCube)) {
                cubeGroupPosition = newPosition;
            }
        }
        else if (key == GLFW_KEY_S) {
            // Increase the drop speed
            if(cubeDropInterval == 99999.0f){
                cubeDropInterval = 1.0f;
            }
            cubeDropInterval = glm::max(0.1f, cubeDropInterval - 0.1f); // Minimum 0.1 seconds

        }
        else if (key == GLFW_KEY_W) {
            // Decrease the drop speed
            if(cubeDropInterval == 1.0f){
                cubeDropInterval = 99999.0f;
            }
            else if(cubeDropInterval == 99999.0f){}
            else{
                cubeDropInterval = glm::min(1.0f, cubeDropInterval + 0.1f); // Maximum 2.0 seconds
            }
        }
    }
}




void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime(); // Current time
        updateCubePosition(currentTime);   // Update cube position

        // Update camera rotation
        if (fabs(targetCameraAngle - cameraAngle) > 0.1f) {

            cameraMoved = true;

            if (cameraAngle < targetCameraAngle) {
                cameraAngle += rotationSpeed;
                if (cameraAngle > targetCameraAngle) cameraAngle = targetCameraAngle;
            } else {
                cameraAngle -= rotationSpeed;
                if (cameraAngle < targetCameraAngle) cameraAngle = targetCameraAngle;
            }

            // Update camera position based on new angle
            float cameraAngleRad = glm::radians(cameraAngle);

            float camX = cameraRadius * cos(cameraAngleRad);
            float camZ = cameraRadius * sin(cameraAngleRad);
            eyePos = glm::vec3(camX, 0.0f, camZ);

            viewingMatrix = glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

            for (int i = 0; i < 2; ++i)
            {
                glUseProgram(gProgram[i]);
                glUniformMatrix4fv(viewingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
                glUniform3fv(eyePosLoc[i], 1, glm::value_ptr(eyePos));
            }

            glm::vec3 forward = glm::normalize(-glm::vec3(viewingMatrix[2]));
            lightPos = eyePos + forward * 7.0f + glm::vec3(0.0f, lightYOffset, 0.0f);

            cameraMoved = true;
        } else {
            // Rotation completed, update the view name
            View newView = getCurrentView(cameraAngle);
            std::string newViewName = viewToString(newView);
            if (newViewName != currentViewName) {
                currentViewName = newViewName;
            }
            cameraMoved = false;
        }

        if (cameraMoved) {
            for (int i = 0; i < 2; ++i)
            {
                glUseProgram(gProgram[i]);
                glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPos));
                initLight();
            }
        }

        // Clamp cube position within game area
        cubeGroupPosition.x = glm::clamp(cubeGroupPosition.x, -GAME_AREA_HALF_SIZE + CUBE_GROUP_HALF_SIZE, GAME_AREA_HALF_SIZE - CUBE_GROUP_HALF_SIZE);
        cubeGroupPosition.z = glm::clamp(cubeGroupPosition.z, -GAME_AREA_HALF_SIZE + CUBE_GROUP_HALF_SIZE, GAME_AREA_HALF_SIZE - CUBE_GROUP_HALF_SIZE);

        // Render the scene
        display();

        // Render the current view name
        // Position: (10, gHeight - 30) to place it near the top-left corner
        renderText(currentViewName, 10.0f, gHeight - 30.0f, 0.75f, glm::vec3(1.0f, 1.0f, 0.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}




int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(gWidth, gHeight, "tetrisGL", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

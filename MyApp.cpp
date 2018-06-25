#include "MyApp.hpp"
#include <vector>
#include <cmath>
#include <GLES2/gl2.h>
#include <EGL/egl.h>


WasmApp *CreateWasmAppInstance() {
    return new MyApp;
}


static GLuint program = 0;

static const char *vertex_shader_source =
    "attribute vec4 pos;\n"\
    "uniform mat4 W;"\
    "uniform mat4 V;"\
    "uniform mat4 P;"\
    "uniform mat4 Rx;"\
    "uniform mat4 Ry;"\
    "void main() {gl_Position = P*V*W*Ry*Rx*pos;}";

static const char *fragment_shader_source = "precision mediump float; void main() {gl_FragColor = vec4(1,1,1,1);}";

static const GLfloat cube_vertices[] = {
    // top
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f,  1.0f,
     1.0f, 1.0f,  1.0f,
     1.0f, 1.0f, -1.0f,

    // bottom
     1.0f,-1.0f, -1.0f,
     1.0f,-1.0f,  1.0f,
    -1.0f,-1.0f,  1.0f,
    -1.0f,-1.0f, -1.0f,

    // right
     1.0f, 1.0f,  1.0f,
     1.0f,-1.0f,  1.0f,
     1.0f,-1.0f, -1.0f,
     1.0f, 1.0f, -1.0f,

    // left
    -1.0f, 1.0f, -1.0f,
    -1.0f,-1.0f, -1.0f,
    -1.0f,-1.0f,  1.0f,
    -1.0f, 1.0f,  1.0f,

    // near side
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,

    // far side
     1.0f, 1.0f, -1.0f,
     1.0f,-1.0f, -1.0f,
    -1.0f,-1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f
};

static const GLuint cube_indices[] = {
    0,1,2, 0,2,3,
    4,5,6, 4,6,7,
    8,9,10, 8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

static GLuint vbo = 0;
static GLuint ibo = 0;

static int CheckShaderStatus(GLuint shader) {

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if(is_compiled == GL_TRUE) {
        return 0;
    } else {
        // Shader compilation failed.
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        // log_size == the long length INCLUDING the terminating null character
        std::vector<GLchar> error_log(log_length);
        glGetShaderInfoLog(shader, log_length, &log_length, &error_log[0]);

        printf("gl shader error: %s",&error_log[0]);

        glDeleteShader(shader);

        return -1;
    }
}

static int CheckProgramStatus(GLuint program) {

    GLint is_linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&is_linked);

    if(is_linked == GL_TRUE) {
        return 0;
    } else {
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        std::vector<GLchar> error_log(log_length);
        glGetProgramInfoLog(program, log_length, &log_length, &error_log[0]);

        printf("gl program error: %s",&error_log[0]);

        glDeleteProgram(program);

        return -1;
    }
}

int MyApp::Init() {

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    if(vertex_shader == 0) {
        return -1;
    }

    GLint len = strlen(vertex_shader_source);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, &len);

    glCompileShader(vertex_shader);

    if( CheckShaderStatus(vertex_shader) < 0 ) {
        return -1;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    if(fragment_shader == 0) {
        return -1;
    }

    len = strlen(fragment_shader_source);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, &len);

    glCompileShader(fragment_shader);

    if( CheckShaderStatus(fragment_shader) < 0 ) {
        glDeleteShader(vertex_shader);
        return -1;
    }

    program = glCreateProgram();

    glAttachShader(program,vertex_shader);
    glAttachShader(program,fragment_shader);

    glLinkProgram(program);

    if( CheckProgramStatus(program) < 0 ) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return -1;
    }

    float m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        0.0f, 0.0f,15.0f, 1.0f  // column 4
    };

    float view[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        0.0f, 0.0f, 5.0f, 1.0f  // column 4
    };

    float proj[16] = {
        1.81f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 2.41f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.001f, 1.0f, // column 3
        0.0f, 0.0f, -0.1001f, 0.0f  // column 4
    };

    GLint world_transform      = glGetUniformLocation(program,"W");
    GLint view_transform       = glGetUniformLocation(program,"V");
    GLint projection_transform = glGetUniformLocation(program,"P");
    console_log(std::string("W: ") + std::to_string(world_transform));
    console_log(std::string("V: ") + std::to_string(view_transform));
    console_log(std::string("P: ") + std::to_string(projection_transform));
    glUseProgram(program);
    glUniformMatrix4fv(world_transform, 1, GL_FALSE, m);
    glUniformMatrix4fv(view_transform, 1, GL_FALSE, view);
    glUniformMatrix4fv(projection_transform, 1, GL_FALSE, proj);

    // Set vertices
    glGenBuffers( 1, &vbo );
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    this->start_time = std::chrono::system_clock::now();

    return 0;
}

void MyApp::Render() {

    {
        static int count = 0;
        if(count < 5) {
            console_log("MyApp::Render");
            count += 1;
        }
    }
    //printf("");

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> t = now - this->start_time;

	const float PI = 3.141593;
    float a = (float)fmod(t.count(),PI*2.0);
    float c = cosf(a);
    float s = sinf(a);
    float rx[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, c,    s,    0.0f, // column 2
        0.0f, -s,   c,    0.0f, // column 3
        0.0f, 0.0f, 0.0f, 1.0f  // column 4
    };

    a = (float)fmod(t.count()*0.9,PI*2.0);
    c = cosf(a);
    s = sinf(a);
    float ry[16] = {
        c,    0.0f, -s,   0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    GLint rotation_x = glGetUniformLocation(program,"Rx");
    GLint rotation_y = glGetUniformLocation(program,"Ry");
    glUniformMatrix4fv(rotation_x, 1, GL_FALSE, rx);
    glUniformMatrix4fv(rotation_y, 1, GL_FALSE, ry);

    //unsigned int viewport_width = 1280;
    //unsigned int viewport_height = 720;
    //glViewport(0, 0, viewport_width, viewport_height);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);


//	glBindBuffer( GL_ARRAY_BUFFER, 0 );
//	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );

    GLuint vertex_attrib_index = 0;

    glEnableVertexAttribArray(vertex_attrib_index);

    GLboolean normalized = GL_FALSE;
    GLsizei stride = sizeof(float)*3;
    //glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, stride, cube_vertices );
    glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, stride, 0 );

    GLsizei num_elements_to_render = 36;
    //glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, cube_indices );
    glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, 0 );

    glDisableVertexAttribArray(vertex_attrib_index);
}


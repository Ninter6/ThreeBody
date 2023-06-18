//
//  main.cpp
//  ThreeBody
//
//  Created by Ninter6 on 2023/6/11.
//

#include <iostream>
#include <array>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "check_gl.h"

#define MATHPLS_DEFINITION
#include "mathpls.h"

constexpr uint32_t SC_WIDTH = 1200, SC_HEIGHT = 750;
const std::string SC_NAME= "Three Body";

GLFWwindow* win;

long double lastTime, deltaTime;

struct Star{
    mathpls::vec2 pos;
    mathpls::vec2 vel;
};

constexpr int nstars = 3;

using StarArray = std::array<Star, nstars>;
StarArray stars{{
    {{-0.5f, -0.5f}, {.5f, .1f}},
    {{0.5f, -0.5f}, {-.3, -.2f}},
    {{0.f, 0.5f}, {0, 0}}
}};

const float rect[] = {
    -1, -1,
     1,  -1,
     1,  1,
     1,  1,
    -1,  1,
    -1, -1
};

uint32_t VAO, VBO, shader, RVBO, cls;

const char* vertSource= R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 rPos;

uniform float prop;
uniform float size;

uniform vec3 uColor[16];
out vec3 color;
out vec2 fPos;

void main(){
    color = uColor[gl_InstanceID];
    fPos = rPos;
    gl_Position = vec4(aPos + vec2(rPos.x * size, rPos.y * size * prop), 0, 1);
}
)";
const char* fragSource= R"(
#version 330 core
in vec2 fPos;
in vec3 color;
out vec4 FragColor;
void main(){
    float n = length(fPos);
    if(n > 1) discard;
    n = 1 - pow(n, 16);
    FragColor = vec4(color, n);
}
)";
//const char* geomSource = R"(
//#version 330 core
//layout (points) in;
//layout (triangle_strip, max_vertices = 60) out;
//
//in vec3 color[];
//out vec3 fcolor;
//
//uniform float prop;
//uniform float size;
//
//void circle(vec2 pos){
//    for(float i = 0; i < 6.2831; i += 0.314159){
//        gl_Position = vec4(pos.x , pos.y, 0, 1);
//        EmitVertex();
//        gl_Position = vec4(pos.x + size * cos(i), pos.y + size*prop * sin(i), 0, 1);
//        EmitVertex();
//        gl_Position = vec4(pos.x + size * cos(i + 0.314159), pos.y + size*prop * sin(i + 0.314159), 0, 1);
//        EmitVertex();
//    }
//    EndPrimitive();
//}
//
//void main(){
//    fcolor = color[0];
//    circle(gl_in[0].gl_Position.xy);
//}
//)";

const char* rvs = R"(
#version 330 core
layout (location = 1) in vec2 aPos;
void main(){gl_Position = vec4(aPos, 0, 1);}
)";
const char* rfs = R"(
#version 330 core
out vec4 color;
void main(){color = vec4(0, 0, .1, .02);}
)";

void checkCompileErrors(unsigned int ID, bool isProgram){
    int success;
    char infoLog[512];
    
    if(!isProgram){
        glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
        if(!success){
            glGetShaderInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Compile shaders error:" << std::endl << infoLog << std::endl;
        }
    }else{
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success){
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Compile program errer:" << std::endl << infoLog << std::endl;
        }
    }
}

void init(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//    glfwWindowHint(GLFW_SAMPLES, 8);
    
    win = glfwCreateWindow(SC_WIDTH/2, SC_HEIGHT/2, SC_NAME.c_str(), NULL, NULL);
    if(win == NULL){
        throw std::runtime_error("Failed to open window!");
    }
    glfwMakeContextCurrent(win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD!");
    }
    glViewport(0, 0, SC_WIDTH, SC_HEIGHT);
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    CHECK_GL(glPointSize(24.f));
    CHECK_GL(glEnable(GL_BLEND));
    CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    CHECK_GL(glGenVertexArrays(1, &VAO));
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Star) * stars.size(), &stars[0], GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Star), (void*)0);
    glVertexAttribDivisor(0, 1);
    
    shader = glCreateProgram();
    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
//    auto geometry = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(vertex, 1, &vertSource, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, 0);
    glShaderSource(fragment, 1, &fragSource, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, 0);
//    glShaderSource(geometry, 1, &geomSource, NULL);
//    glCompileShader(geometry);
//    checkCompileErrors(geometry, 0);
    glAttachShader(shader, vertex);
    glAttachShader(shader, fragment);
//    glAttachShader(shader, geometry);
    glLinkProgram(shader);
    checkCompileErrors(shader, 1);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
//    glDeleteShader(geometry);
    
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "uColor[0]"), 1, 0, 0);
    glUniform3f(glGetUniformLocation(shader, "uColor[1]"), 0, 1, 0);
    glUniform3f(glGetUniformLocation(shader, "uColor[2]"), 0, 0, 1);
    glUniform3f(glGetUniformLocation(shader, "uColor[3]"), 1, 0, 1);
    
    glUniform1f(glGetUniformLocation(shader, "prop"), (float) SC_WIDTH / SC_HEIGHT);
    CHECK_GL(glUniform1f(glGetUniformLocation(shader, "size"), 0.01f));
    
    glGenBuffers(1, &RVBO);
    glBindBuffer(GL_ARRAY_BUFFER, RVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
    
    cls = glCreateProgram();
    vertex = glCreateShader(GL_VERTEX_SHADER);
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex, 1, &rvs, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, 0);
    glShaderSource(fragment, 1, &rfs, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, 0);
    glAttachShader(cls, vertex);
    glAttachShader(cls, fragment);
    glLinkProgram(cls);
    checkCompileErrors(shader, 1);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    lastTime = glfwGetTime();
}

void render(){
    glBindVertexArray(VAO);
    glUseProgram(cls);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glUseProgram(shader);
    CHECK_GL(glDrawArraysInstanced(GL_TRIANGLES, 0, 6, stars.size()));
}

constexpr float G = .2f;

void derivative(StarArray const &stars, StarArray &d_stars) {
    for (size_t i = 0; i < stars.size(); i++) {
        auto acc = mathpls::vec2(0, 0);
        for (size_t j = 0; j < stars.size(); j++) {
            if (i == j) continue;
            auto p1 = stars[i].pos;
            auto p2 = stars[j].pos;
            auto r12 = p2 - p1;
            auto rlen = std::max(0.01f, r12.length());
            auto rnorm = r12 / rlen;
            // 引力大小为：F = G*m1*m2/(|r|*|r|)，方向为 -r，其中 r 为两个质点之间距离，r = p1 - p2
            auto force = G / (rlen * rlen);
            // F = ma, a = F/m
            acc += rnorm * force;
        }
        d_stars[i].vel = acc;
        d_stars[i].pos = stars[i].vel;
    }
}

void integrate(StarArray &out_stars, StarArray const &stars, StarArray const &d_stars, long double dt) {
    for (size_t i = 0; i < stars.size(); i++) {
        out_stars[i].pos = stars[i].pos + d_stars[i].pos * dt;
        out_stars[i].vel = stars[i].vel + d_stars[i].vel * dt;
    }
}

void integrate(StarArray &stars, StarArray const &d_stars, long double dt) {
    for (size_t i = 0; i < stars.size(); i++) {
        stars[i].pos += d_stars[i].pos * dt;
        stars[i].vel += d_stars[i].vel * dt;
    }
}

void fixbounds(StarArray &stars) {
    for (size_t i = 0; i < stars.size(); i++) {
        if ((stars[i].pos.x < -1 && stars[i].vel.x < 0.0f)
            || (stars[i].pos.x > 1 && stars[i].vel.x > 0.0f)) {
            stars[i].vel.x = -stars[i].vel.x;
        }
        if ((stars[i].pos.y < -1.0f && stars[i].vel.y < 0.0f)
            || (stars[i].pos.y > 1.0f && stars[i].vel.y > 0.0f)) {
            stars[i].vel.y = -stars[i].vel.y;
        }
    }
}

void substep_rk4(long double dt) {
    StarArray tmp;
    StarArray k1, k2, k3, k4, k;
    derivative(stars, k1);                  // k1 = f(y)
    integrate(tmp, stars, k1, dt / 2.0f);   // tmp = y + k1/2 dt
    derivative(tmp, k2);                    // k2 = f(tmp)
    integrate(tmp, stars, k2, dt / 2.0f);   // tmp = y + k2/2 dt
    derivative(tmp, k3);                    // k3 = f(tmp)
    integrate(tmp, stars, k3, dt);          // tmp = y + k3 dt
    derivative(tmp, k4);                    // k4 = f(tmp)
    k = StarArray();                        // k = 0
    integrate(k, k1, 1.0f / 6.0f);          // k = k + k1/6
    integrate(k, k2, 1.0f / 3.0f);          // k = k + k2/3
    integrate(k, k3, 1.0f / 3.0f);          // k = k + k2/3
    integrate(k, k4, 1.0f / 6.0f);          // k = k + k2/6
    integrate(stars, k, dt);
    fixbounds(stars);
}

void update(){
    deltaTime = glfwGetTime() - lastTime;
    lastTime = glfwGetTime();
//
//    //update vel
//    constexpr float G = .5f;
//    for (int i = 0; i < stars.size(); i++) {
//        for (int j = 0; j < stars.size(); j++){
//            if (i == j) continue;
//            auto p1 = stars[i].pos;
//            auto p2 = stars[j].pos;
//            auto r = p2 - p1;
//            auto rlen = mathpls::max(r.length(), .1f);
//            auto F = r.normalize() * G / (rlen * rlen);
//            // m一律为1
//            stars[i].vel += F * deltaTime;
//        }
//    }
//
//    // update pos
//    for (auto& i : stars) {
//        i.pos += i.vel * deltaTime;
////        i.vel.y -= .5f * deltaTime;
//        if((i.pos.y < -1 && i.vel.y < 0) || (i.pos.y > 1 && i.vel.y > 0)) i.vel.y *= -0.707;
//        if((i.pos.x < -1 && i.vel.x < 0) || (i.pos.x > 1 && i.vel.x > 0)) i.vel.x *= -0.707;
//    }
    
    constexpr int n = 50;
    for (int i = 0; i < n; i++) {
        substep_rk4(deltaTime / n);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    CHECK_GL(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Star) * stars.size(), &stars[0]));
}

int main(int argc, const char * argv[]) {
    init();
    
    while (!glfwWindowShouldClose(win)) {
        render();
        
        update();
        
        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

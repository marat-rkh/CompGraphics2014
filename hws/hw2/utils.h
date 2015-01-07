#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <string>
#include "common.h"

class msg_exception : public std::exception {
    std::string what_;
public:
    msg_exception(std::string const& msg): what_(msg) {}

    const char* what() const throw() { return what_.c_str(); }
};

struct vertex_attr {
    const char* name;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride; // offset to next value
    GLvoid* pointer;
};

class utils {
public:
    static mat4 calculate_mvp_matrix(quat const& rotation) {
        float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
        float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);
        // строим матрицу проекции с aspect ratio (отношением сторон) таким же, как у окна
        mat4 const proj = perspective(45.0f, w / h, 0.1f, 100.0f);
        // преобразование из СК мира в СК камеры
        mat4 const view = lookAt(vec3(0, 0, 5), vec3(0, 0, 0), vec3(0, 1, 0));
        mat4 const modelview = view * mat4_cast(rotation);
        return proj * modelview;
    }

    static void set_vertex_attr_ptr(GLuint program, vertex_attr const& attr) {
        GLuint location = glGetAttribLocation(program, attr.name);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, attr.size, attr.type,
                              attr.normalized, attr.stride, attr.pointer);
    }
};

#endif // UTILS_H

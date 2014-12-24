#ifndef PROG_STATE_H
#define PROG_STATE_H

#include "common.h"
#include "model.h"

struct triangle {
    const vec2 v1, v2, v3;

    triangle(const vec2& v1, const vec2& v2, const vec2& v3)
        : v1(v1), v2(v2), v3(v3)
    {}
    vec2 centroid() const {
        return (v1 + v2 + v3) / 3.0f;
    }
};

enum ColorMode { NORMALS, FUNC };

class ProgState {
public:
    ProgState();
    ~ProgState();

    void draw() {
        static chrono::system_clock::time_point const start = chrono::system_clock::now();
        // вызов функции отрисовки с передачей ей времени от первого вызова
        draw_frame(chrono::duration<float>(chrono::system_clock::now() - start).count());
        // отрисовка GUI
        TwDraw();
        // смена front и back buffer'а (напоминаю, что у нас используется режим двойной буферизации)
        glutSwapBuffers();
    }

    void change_mode() {
        if (mode_ == NORMALS) {
            mode_ = FUNC;
        } else {
            mode_ = NORMALS;
        }
    }

private:
    void draw_frame(float time_from_start);
    void create_tw_bar();
    void init_buffer();
    void init(string const& vs_file, string const& fs_file);
    void refresh(mat4 m);

    bool   wireframe_;
    triangle triangle_;
    float  cell_size_;
    ColorMode mode_;

    GLuint vs_, fs_, program_;
    GLuint vx_buf_;
    quat   rotation_by_control_;

    vvec3 data_;
    Model model_;

    float v_, k_;
    vec3 center_;
    float max_;
};

#endif // PROG_STATE_H

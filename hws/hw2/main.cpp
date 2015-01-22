#include "common.h"
#include "shader.h"
#include "utils.h"
#include <FreeImage.h>
#include "model.h"

enum geom_obj { PLANE, CYLINDER, SPHERE };

struct program_state {
    quat rotation_by_control;

    program_state()
        : cur_figure(PLANE)
        , wireframe_mode(false)
    {
        init_data();
    }

    // this function must be called before main loop but after
    // gl libs init functions
    void init() {
        set_data_buffer();
        init_textures();
        set_draw_configs();
    }

    void on_display_event() {
        set_shaders();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw();
    }

    void next_figure() {
        if(cur_figure == PLANE) {
            cur_figure = CYLINDER;
//        } else if(cur_figure == CYLINDER) {
//            cur_figure = SPHERE;
        } else {
            cur_figure = PLANE;
        }
        set_data_buffer();
    }

    void switch_polygon_mode() {
        wireframe_mode = !wireframe_mode;
        set_draw_configs();
    }

    ~program_state() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteProgram(program);
        glDeleteShader(vx_shader);
        glDeleteShader(frag_shader);
        glDeleteBuffers(1, &vx_buffer);
    }

private:
    geom_obj cur_figure;
    bool wireframe_mode;

    GLuint vx_shader;
    GLuint frag_shader;
    GLuint program;

    GLuint vx_buffer;

    size_t DATA_PLANE_SIZE;
    vector<vec2> DATA_PLANE;

    const char* PLANE_GLSLVS_PATH = "..//shaders//plane.glslvs";
    const char* PLANE_GLSLFS_PATH = "..//shaders//plane.glslfs";
    const char* TEXTURE_PATH = "..//resources//wall.png";

    vertex_attr const PLANE_VERTEX_POS = { "vertex_pos", 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), 0 };
    vertex_attr const PLANE_VERTEX_TEXCOORDS = { "vertex_texcoords", 2, GL_FLOAT, GL_FALSE,
                                                 2 * sizeof(vec2), (void *)(sizeof(vec2)) };

    float const RADIUS = 0.5;
    size_t DATA_CYLINDER_SIZE;
    vector<vec3> DATA_CYLINDER;

    const char* CYLINDER_GLSLVS_PATH = "..//shaders//cylinder.glslvs";
    const char* CYLINDER_GLSLFS_PATH = "..//shaders//cylinder.glslfs";

    vertex_attr const CYLINDER_VERTEX_POS = { "vertex_pos", 3, GL_FLOAT, GL_FALSE,
                                              sizeof(vec3), 0 };
    vertex_attr const CYLINDER_VERTEX_TEXCOORDS = { "vertex_texcoords", 3, GL_FLOAT,
                                                    GL_FALSE, sizeof(vec3),
                                                    (void *)(sizeof(vec3) * DATA_CYLINDER.size() / 2) };

    GLsizeiptr cur_data_size() {
        switch (cur_figure) {
        case PLANE: return DATA_PLANE_SIZE;
        case CYLINDER: return DATA_CYLINDER_SIZE;
        case SPHERE: return 0;
        }
    }

    GLvoid const* cur_data() {
        switch (cur_figure) {
        case PLANE: return &(DATA_PLANE[0]);
        case CYLINDER: return &(DATA_CYLINDER[0]);
        case SPHERE: return 0;
        }
    }

    char const* cur_vertex_shader_path() {
        switch (cur_figure) {
        case PLANE: return PLANE_GLSLVS_PATH;
        case CYLINDER: return CYLINDER_GLSLVS_PATH;
        case SPHERE: return "";
        }
    }

    char const* cur_frag_shader_path() {
        switch (cur_figure) {
        case PLANE: return PLANE_GLSLFS_PATH;
        case CYLINDER: return CYLINDER_GLSLFS_PATH;
        case SPHERE: return "";
        }
    }

    void init_data() {
        switch (cur_figure) {
        case PLANE: init_plane(); break;
        case CYLINDER: init_cylinder(); break;
        case SPHERE: cout << "sphere init not implemented!!!!\n"; break;
        }
    }

    void init_plane() {
        DATA_PLANE = {
            vec2(-1.0f, 1.0f), vec2(0.0f, 0.0f),
            vec2(1.0f, 1.0f), vec2(1.0f, 0.0f),
            vec2(1.0f, -1.0f), vec2(1.0f, 1.0f),
            vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f)
        };
        DATA_PLANE_SIZE = DATA_PLANE.size() * sizeof(vec2);
    }

    void init_cylinder() {
        for(size_t fi = 0; fi <= 360; fi += 60) {
            float rad = fi * M_PI / 180.0f;
            float x = RADIUS * cos(rad);
            float y = RADIUS * sin(rad);
            DATA_CYLINDER.push_back(vec3(x, y, -0.5));
            DATA_CYLINDER.push_back(vec3(x, y, 0.5));
        }
        for(size_t fi = 0; fi <= 360; fi += 60) {
            DATA_CYLINDER.push_back(vec3(fi / 360.0f, 1.0f, 0.0f));
            DATA_CYLINDER.push_back(vec3(fi / 360.0f, 0.0f, 0.0f));
        }
        DATA_CYLINDER_SIZE = DATA_CYLINDER.size() * sizeof(vec3);
        cout << "Cylinder points num: " << DATA_CYLINDER.size() << endl;
    }

    void set_data_buffer() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glGenBuffers(1, &vx_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        glBufferData(GL_ARRAY_BUFFER, cur_data_size(), cur_data(), GL_STATIC_DRAW);
    }

    void init_textures() {
        texture_data tex_data = utils::load_texture(TEXTURE_PATH);
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_data.width, tex_data.height,
                     0, tex_data.format, GL_UNSIGNED_BYTE, tex_data.data_ptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    void set_draw_configs() {
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClearDepth(1);
        int mode = wireframe_mode ? GL_LINE : GL_FILL;
        glPolygonMode(GL_FRONT_AND_BACK, mode);
    }

    void set_shaders() {
        glDeleteProgram(program);
        glDeleteShader(vx_shader);
        glDeleteShader(frag_shader);

        vx_shader = create_shader(GL_VERTEX_SHADER, cur_vertex_shader_path());
        frag_shader = create_shader(GL_FRAGMENT_SHADER, cur_frag_shader_path());
        program = create_program(vx_shader, frag_shader);

        set_shader_attrs();
    }

    void set_shader_attrs() {
        glUseProgram(program);

        switch (cur_figure) {
        case PLANE: set_plane_shader_attrs(); break;
        case CYLINDER: set_cylinder_shader_attrs(); break;
        case SPHERE: set_sphere_shader_attrs(); break;
        }

        GLuint location = glGetUniformLocation(program, "mvp");
        mat4 mvp = utils::calculate_mvp_matrix(rotation_by_control);
        glUniformMatrix4fv(location, 1, GL_FALSE, &mvp[0][0]);
    }

    void set_plane_shader_attrs() {
        utils::set_vertex_attr_ptr(program, PLANE_VERTEX_POS);
        utils::set_vertex_attr_ptr(program, PLANE_VERTEX_TEXCOORDS);
    }

    void set_cylinder_shader_attrs() {
        utils::set_vertex_attr_ptr(program, CYLINDER_VERTEX_POS);
        utils::set_vertex_attr_ptr(program, CYLINDER_VERTEX_TEXCOORDS);
    }

    void set_sphere_shader_attrs() {
        cout << "set_sphere_shader_attrs not implemented !!! \n";
    }

    void draw() {
        switch (cur_figure) {
        case PLANE: glDrawArrays(GL_TRIANGLE_FAN, 0, DATA_PLANE.size() / 2); break;
        case CYLINDER: draw_cylinder(); break;
        case SPHERE: cout << "sphere draw func not set!!!!\n"; break;
        }
    }

    void draw_cylinder() {
        for(size_t i = 0; i < DATA_CYLINDER.size() / 2; i += 2) {
            glDrawArrays(GL_QUAD_STRIP, i, (i + 4) % DATA_CYLINDER.size());
        }
    }
};

// global, because display_func callback needs it
program_state prog_state;

void basic_init(int argc, char ** argv) {
    // Размеры окна по-умолчанию
    size_t const default_width  = 800;
    size_t const default_height = 800;

    glutInit               (&argc, argv);
    glutInitWindowSize     (default_width, default_height);
    // Указание формата буфера экрана:
    // - GLUT_DOUBLE - двойная буферизация
    // - GLUT_RGB - 3-ёх компонентный цвет
    // - GLUT_DEPTH - будет использоваться буфер глубины
    glutInitDisplayMode    (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    // Создаем контекст версии 3.2
    glutInitContextVersion (3, 0);
    // Контекст будет поддерживать отладку и "устаревшую" функциональность, которой, например, может пользоваться библиотека AntTweakBar
   // glutInitContextFlags   (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
    // Указание либо на core либо на compatibility профил
    //glutInitContextProfile (GLUT_COMPATIBILITY_PROFILE );
    glutCreateWindow("hw 2");

    // Инициализация указателей на функции OpenGL
    if (glewInit() != GLEW_OK) {
        throw msg_exception("GLEW init failed");
    }
    // Проверка созданности контекста той версии, какой мы запрашивали
    if (!GLEW_VERSION_3_0) {
       throw msg_exception("OpenGL 3.0 not supported");
    }
}

// == callbacks ==
// отрисовка кадра
void display_func() {
    prog_state.on_display_event();
    TwDraw();
    glutSwapBuffers();
}

// Переисовка кадра в отсутствии других сообщений
void idle_func() { glutPostRedisplay(); }

void keyboard_func(unsigned char button, int x, int y) {
   if (TwEventKeyboardGLUT(button, x, y))
      return;

   switch(button) {
   case 27:
      exit(0);
   }
}

// Отработка изменения размеров окна
void reshape_func(int width, int height) {
   if (width <= 0 || height <= 0)
      return;
   glViewport(0, 0, width, height);
   TwWindowSize(width, height);
}

// Очищаем все ресурсы, пока контекст ещё не удалён
void close_func() {  }

void register_callbacks() {
    // подписываемся на оконные события
    glutReshapeFunc(reshape_func);
    glutDisplayFunc(display_func);
    glutIdleFunc   (idle_func   );
    glutCloseFunc  (close_func  );
    glutKeyboardFunc(keyboard_func);

    // подписываемся на события для AntTweakBar'а
    glutMouseFunc        ((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
    glutMotionFunc       ((GLUTmousemotionfun)TwEventMouseMotionGLUT);
    glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
    glutSpecialFunc      ((GLUTspecialfun    )TwEventSpecialGLUT    );
    TwGLUTModifiersFunc  (glutGetModifiers);
}

void TW_CALL toggle_fullscreen_callback( void * ) { glutFullScreenToggle(); }

void next_figure_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->next_figure();
}

void switch_polygon_mode(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->switch_polygon_mode();
}

void create_controls(program_state& prog_state) {
    TwInit(TW_OPENGL, NULL);

    TwBar *bar = TwNewBar("Parameters");
    TwDefine("Parameters size='500 160' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                "label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &prog_state.rotation_by_control,
               "label='Object orientation' opened=true help='Change the object orientation.'");
    TwAddButton(bar, "Next figure", next_figure_callback, &prog_state,
                "label='Draw next figure' key=n");
    TwAddButton(bar, "Switch wireframe mode", switch_polygon_mode, &prog_state,
                "label='Switch wireframe mode' key=w");
}

void remove_controls() {
    TwDeleteAllBars();
    TwTerminate();
}

int main( int argc, char ** argv ) {
    try {
        basic_init(argc, argv);
        register_callbacks();
        create_controls(prog_state);
        prog_state.init();

        glutMainLoop();
    } catch(std::exception const & except) {
        cout << except.what() << endl;
        remove_controls();
        return 1;
    }
    remove_controls();
    return 0;
}

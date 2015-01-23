#include "common.h"
#include "shader.h"
#include "utils.h"
#include <FreeImage.h>

enum geom_obj { QUAD, CYLINDER, SPHERE };

struct draw_data {
    vector<GLfloat> vertices;
    vector<GLfloat> tex_mapping;

    size_t vertices_num() const { return vertices.size() / 3; }

    void* vertices_data() { return vertices.data(); }
    size_t vertices_data_size() const { return vertices.size() * sizeof(GLfloat); }

    void* tex_mapping_data() { return tex_mapping.data(); }
    size_t tex_mapping_data_size() const { return tex_mapping.size() * sizeof(GLfloat); }
};

struct program_state {
    quat rotation_by_control;

    program_state()
        : wireframe_mode(false)
        , cur_obj(QUAD)
    {}

    // this function must be called before main loop but after
    // gl libs init functions
    void init() {
        utils::read_obj_file(QUAD_MODEL_PATH, quad.vertices, quad.tex_mapping);
        utils::read_obj_file(CYLINDER_MODEL_PATH, cylinder.vertices, cylinder.tex_mapping);
        utils::read_obj_file(SPHERE_MODEL_PATH, sphere.vertices, sphere.tex_mapping);
        set_shaders();
        set_draw_configs();
        init_textures();
        set_data_buffer();
    }

    void on_display_event() {
        draw();
        TwDraw();
        glutSwapBuffers();
    }

    void next_figure() {
        if(cur_obj == QUAD) {
            cur_obj = CYLINDER;
        } else if(cur_obj == CYLINDER) {
            cur_obj = SPHERE;
        } else {
            cur_obj = QUAD;
        }
        set_data_buffer();
    }

    void switch_polygon_mode() {
        wireframe_mode = !wireframe_mode;
        set_draw_configs();
        on_display_event();
    }

    ~program_state() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteProgram(program);
        glDeleteShader(vx_shader);
        glDeleteShader(frag_shader);
        glDeleteBuffers(1, &vx_buffer);
    }

private:
    bool wireframe_mode;
    geom_obj cur_obj;

    GLuint vx_shader;
    GLuint frag_shader;
    GLuint program;

    GLuint vx_buffer;
    GLuint tex_buffer;

    GLuint sampler_id;
    GLuint texture_id;

    const char* QUAD_MODEL_PATH = "..//resources//quad.obj";
    draw_data quad;

    const char* CYLINDER_MODEL_PATH = "..//resources//cylinder.obj";
    draw_data cylinder;

    const char* SPHERE_MODEL_PATH = "..//resources//sphere.obj";
    draw_data sphere;

    const char* TEXTURE_PATH = "..//resources//wall.png";

    const char* VERTEX_SHADER_PATH = "..//shaders//0.glslvs";
    const char* FRAGMENT_SHADER_PATH = "..//shaders//0.glslfs";

    vertex_attr const VERTEX_POS = { "vertex_pos", 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0 };
    vertex_attr const VERTEX_TEXCOORDS = { "vertex_texcoords", 2, GL_FLOAT,
                                           GL_FALSE, 2 * sizeof(GLfloat), 0 };

    draw_data& cur_draw_data() {
        switch(cur_obj) {
        case QUAD: return quad;
        case CYLINDER: return cylinder;
        case SPHERE: return sphere;
        default: throw msg_exception("cur_obj is undefined");
        }
    }

    void set_shaders() {
        vx_shader = create_shader(GL_VERTEX_SHADER, VERTEX_SHADER_PATH);
        frag_shader = create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_PATH);
        program = create_program(vx_shader, frag_shader);
    }

    void set_draw_configs() {
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        int mode = wireframe_mode ? GL_LINE : GL_FILL;
        glPolygonMode(GL_FRONT_AND_BACK, mode);
    }

    void init_textures() {
        texture_data tex_data = utils::load_texture(TEXTURE_PATH);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_data.width, tex_data.height,
                     0, tex_data.format, GL_UNSIGNED_BYTE, tex_data.data_ptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        sampler_id  = glGetUniformLocation(program, "texture_sampler");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1i(sampler_id, 0);
    }

    void set_data_buffer() {
        draw_data& data = cur_draw_data();
        glGenBuffers(1, &vx_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.vertices_data_size(),
                     data.vertices_data(), GL_STATIC_DRAW);

        glGenBuffers(1, &tex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.tex_mapping_data_size(),
                     data.tex_mapping_data(), GL_STATIC_DRAW);
    }

    void draw() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        GLuint location = glGetUniformLocation(program, "mvp");
        mat4 mvp = utils::calculate_mvp_matrix(rotation_by_control);
        glUniformMatrix4fv(location, 1, GL_FALSE, &mvp[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        utils::set_vertex_attr_ptr(program, VERTEX_POS);

        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        utils::set_vertex_attr_ptr(program, VERTEX_TEXCOORDS);

        glDrawArrays(GL_TRIANGLES, 0, cur_draw_data().vertices_num());
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
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
        utils::debug("libs are initialized");
        register_callbacks();
        utils::debug("callbacks are registered");
        create_controls(prog_state);
        utils::debug("controls are created");
        prog_state.init();
        utils::debug("prog state is initiaized");

        glutMainLoop();
    } catch(std::exception const & except) {
        cout << except.what() << endl;
        remove_controls();
        return 1;
    }
    remove_controls();
    return 0;
}

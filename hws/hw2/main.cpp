#include "common.h"
#include "shader.h"
#include <SOIL.h>
#include "utils.h"

struct vertex_attr {
    const char* name;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride; // offset to next value
    GLvoid* pointer;
};

struct program_state {
    GLuint vx_shader;
    GLuint frag_shader;
    GLuint program;

    GLuint vx_buffer;

    quat rotation_by_control;

    static size_t const DATA_PLANE_SIZE = 8 * sizeof(vec2);
    vec2 const DATA_PLANE[DATA_PLANE_SIZE] = {
        vec2(-8.0f, 8.0f), vec2(0.0f, 0.0f),
        vec2(8.0f, 8.0f), vec2(1.0f, 0.0f),
        vec2(8.0f, -8.0f), vec2(1.0f, 1.0f),
        vec2(-8.0f, -8.0f), vec2(0.0f, 1.0f)
    };

    const char* VERTEX_SHADER_PATH = "..//shaders//0.glslvs";
    const char* FRAGMENT_SHADER_PATH = "..//shaders//0.glslfs";
    const char* TEXTURE_PATH = "..//resources//wall.png";

    vertex_attr const VERTEX_POS = { "vertex_pos", 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), 0 };
    vertex_attr const VX_TEX_COORDS = { "vx_tex_coords", 2, GL_FLOAT, GL_FALSE,
                                        2 * sizeof(vec2), (void *)(sizeof(vec2)) };
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

void init_shaders(program_state& prog_state) {
    prog_state.vx_shader = create_shader(GL_VERTEX_SHADER  , prog_state.VERTEX_SHADER_PATH);
    prog_state.frag_shader = create_shader(GL_FRAGMENT_SHADER, prog_state.FRAGMENT_SHADER_PATH);
    prog_state.program = create_program(prog_state.vx_shader, prog_state.frag_shader);
}

void init_data_buffer(program_state& prog_state) {
    glGenBuffers(1, &prog_state.vx_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, prog_state.vx_buffer);
    glBufferData(GL_ARRAY_BUFFER, program_state::DATA_PLANE_SIZE, prog_state.DATA_PLANE, GL_STATIC_DRAW);
}

void init_textures(program_state& prog_state) {
    int width, height;
    unsigned char* image = SOIL_load_image(prog_state.TEXTURE_PATH,
                                           &width,
                                           &height,
                                           0, 0);
    if(!image) { throw msg_exception(SOIL_last_result()); }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    SOIL_free_image_data(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void set_static_shaders_attrs(program_state const& prog_state) {
    glUseProgram(prog_state.program);

    vertex_attr attr = prog_state.VERTEX_POS;
    GLuint location = glGetAttribLocation(prog_state.program, attr.name);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);

    attr = prog_state.VX_TEX_COORDS;
    location = glGetAttribLocation(prog_state.program, attr.name);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
}

mat4 calculate_mvp_matrix(program_state const& prog_state) {
    float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
    float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);
    // строим матрицу проекции с aspect ratio (отношением сторон) таким же, как у окна
    mat4 const proj = perspective(45.0f, w / h, 0.1f, 100.0f);
    // преобразование из СК мира в СК камеры
    mat4 const view = lookAt(vec3(0, 0, 50), vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 const modelview = view * mat4_cast(prog_state.rotation_by_control);
    return proj * modelview;
}

void set_dynamic_shader_attrs(program_state const& prog_state) {
    glUseProgram(prog_state.program);

    GLuint location = glGetUniformLocation(prog_state.program, "mvp");
    mat4 mvp = calculate_mvp_matrix(prog_state);
    glUniformMatrix4fv(location, 1, GL_FALSE, &mvp[0][0]);
}

void set_draw_configs() {
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClearDepth(1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void remove_data(program_state const& prog_state) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteProgram(prog_state.program);
    glDeleteShader(prog_state.vx_shader);
    glDeleteShader(prog_state.frag_shader);
    glDeleteBuffers(1, &prog_state.vx_buffer);

    TwDeleteAllBars();
    TwTerminate();
}

// == callbacks ==
// отрисовка кадра
void display_func() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    set_dynamic_shader_attrs(prog_state);
    glDrawArrays(GL_QUADS, 0, 4);
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
void close_func() {}

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

void create_controls(program_state& prog_state) {
    TwInit(TW_OPENGL, NULL);

    TwBar *bar = TwNewBar("Parameters");
    TwDefine("Parameters size='500 160' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                "label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &prog_state.rotation_by_control,
               "label='Object orientation' opened=true help='Change the object orientation.'");
}

int main( int argc, char ** argv ) {
    try {
        basic_init(argc, argv);
        register_callbacks();
        create_controls(prog_state);

        init_shaders(prog_state);
        init_data_buffer(prog_state);
        init_textures(prog_state);
        set_static_shaders_attrs(prog_state);
        set_draw_configs();

        glutMainLoop();
    } catch(std::exception const & except) {
        cout << except.what() << endl;
        remove_data(prog_state);
        return 1;
    }
    remove_data(prog_state);
    return 0;
}

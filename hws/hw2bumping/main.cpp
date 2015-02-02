#include "common.h"
#include "shader.h"
#include "utils.h"

using namespace std;

enum geom_obj { QUAD, SPHERE, CYLINDER };
enum filtering_mode { NEAREST, LINEAR, MIPMAP };

class program_state {
public:
    quat rotation_by_control;

    float light_power;
    float specular_power;
    double light_src_direction[3];
    float light_color[3];
    float ambient[3];
    float specular[3];

    program_state()
        : wireframe_mode(false)
        , cur_obj(SPHERE)
        , tex_filtration(NEAREST)
        , light_power(1.0)
        , specular_power(5)
    {
        light_src_direction[0] = -13;
        light_src_direction[1] = -13;
        light_src_direction[2] = -8;
        light_color[0] = 1.0f;
        light_color[1] = 1.0f;
        light_color[2] = 1.0f;
        ambient[0] = 0.1f;
        ambient[1] = 0.1f;
        ambient[2] = 0.1f;
        specular[0] = 1.0f;
        specular[1] = 1.0f;
        specular[2] = 1.0f;
    }

    // this function must be called before main loop but after
    // gl libs init functions
    void init() {
        utils::read_obj_file(QUAD_MODEL_PATH, quad);
        utils::read_obj_file(CYLINDER_MODEL_PATH, cylinder);
        utils::read_obj_file(SPHERE_MODEL_PATH, sphere);

        utils::compute_tangent_basis(quad);
        utils::compute_tangent_basis(cylinder);
        utils::compute_tangent_basis(sphere);

        set_shaders();
        set_draw_configs();
        init_texture();
        set_data_buffer();
    }

    void on_display_event() {
        draw();
        TwDraw();
        glutSwapBuffers();
    }

    void next_figure() {
        switch(cur_obj) {
        case QUAD: cur_obj = CYLINDER; break;
        case CYLINDER: cur_obj = SPHERE; break;
        case SPHERE: cur_obj = QUAD; break;
        }
        set_data_buffer();
    }

    void switch_polygon_mode() {
        wireframe_mode = !wireframe_mode;
        set_draw_configs();
    }

    void next_filtering_mode() {
        switch(tex_filtration) {
        case NEAREST: tex_filtration = LINEAR; break;
        case LINEAR: tex_filtration = MIPMAP; break;
        case MIPMAP: tex_filtration = NEAREST; break;
        }
    }

    ~program_state() {
        glDeleteProgram(program);
        glDeleteShader(vx_shader);
        glDeleteShader(frag_shader);
        glDeleteBuffers(1, &vx_buffer);
        glDeleteBuffers(1, &norms_buffer);
        glDeleteBuffers(1, &tex_buffer);
        glDeleteBuffers(1, &tangent_buffer);
        glDeleteBuffers(1, &bitangent_buffer);
    }

  private:
    bool wireframe_mode;
    geom_obj cur_obj;
    filtering_mode tex_filtration;

    GLuint vx_shader;
    GLuint frag_shader;
    GLuint program;

    GLuint vx_buffer;
    GLuint tex_buffer;
    GLuint norms_buffer;
    GLuint tangent_buffer;
    GLuint bitangent_buffer;

    GLuint texture_id;
    GLuint texture_sampler;
    GLuint normals_map_id;
    GLuint normals_map_sampler;

    const char* QUAD_MODEL_PATH = "..//res//quad.obj";
    draw_data quad;

    const char* CYLINDER_MODEL_PATH = "..//res//cylinder.obj";
    draw_data cylinder;

    const char* SPHERE_MODEL_PATH = "..//res//sphere.obj";
    draw_data sphere;

    const char* TEXTURE_PATH = "..//res//wall.png";
    const char* NORMALS_MAP_PATH = "..//res//wall_normals.jpg";

    const char* VERTEX_SHADER_PATH = "..//shaders//main.vs";
    const char* FRAGMENT_SHADER_PATH = "..//shaders//main.fs";

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
        glClearDepth(1);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        int mode = wireframe_mode ? GL_LINE : GL_FILL;
        glPolygonMode(GL_FRONT_AND_BACK, mode);
    }

    void init_texture() {
        texture_data tex_data = utils::load_texture(TEXTURE_PATH);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_data.width, tex_data.height,
                     0, tex_data.format, GL_UNSIGNED_BYTE, tex_data.data_ptr);
        set_texture_filtration();

        texture_data norms_data = utils::load_texture(NORMALS_MAP_PATH);
        glGenTextures(1, &normals_map_id);
        glBindTexture(GL_TEXTURE_2D, normals_map_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, norms_data.width, norms_data.height,
                     0, norms_data.format, GL_UNSIGNED_BYTE, norms_data.data_ptr);
        set_texture_filtration();

        texture_sampler  = glGetUniformLocation(program, "texture_sampler");
        normals_map_sampler  = glGetUniformLocation(program, "normals_map_sampler");
    }

    void set_texture_filtration() {
        switch (tex_filtration) {
        case NEAREST:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        case LINEAR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        case MIPMAP:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
        }
    }

    void set_data_buffer() {
        draw_data& data = cur_draw_data();

        glDeleteBuffers(1, &vx_buffer);
        glGenBuffers(1, &vx_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.vertices_data_size(), data.vertices_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &tex_buffer);
        glGenBuffers(1, &tex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.tex_mapping_data_size(), data.tex_mapping_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &norms_buffer);
        glGenBuffers(1, &norms_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, norms_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.normals_data_size(), data.normals_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &tangent_buffer);
        glGenBuffers(1, &tangent_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.tangents_data_size(), data.tangents_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &bitangent_buffer);
        glGenBuffers(1, &bitangent_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, bitangent_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.bitangents_data_size(), data.bitangents_data(), GL_STATIC_DRAW);
    }

    void draw() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
        float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);

        mat4 const proj = perspective(45.0f, w / h, 0.1f, 100.0f);
        mat4 const view = lookAt(vec3(-2, 3, 6), vec3(0, 0, 0), vec3(0, 1, 0));
        mat4 const model = mat4_cast(rotation_by_control);
        mat4 modelview = view * model;
        mat3 modelview3x3 = mat3(modelview);
        mat4 const mvp = proj * modelview;

        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "proj"), 1, GL_FALSE, &proj[0][0]);
        glUniformMatrix3fv(glGetUniformLocation(program, "model_view3"), 1, GL_FALSE, &modelview3x3[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, &mvp[0][0]);
        glUniform3f(glGetUniformLocation(program, "light_pos"), light_src_direction[0], light_src_direction[1], light_src_direction[2]);
        glUniform3f(glGetUniformLocation(program, "light_color"), light_color[0], light_color[1], light_color[2]);
        glUniform3f(glGetUniformLocation(program, "ambient"), ambient[0], ambient[1], ambient[2]);
        glUniform3f(glGetUniformLocation(program, "specular"), specular[0], specular[1], specular[2]);
        glUniform1f(glGetUniformLocation(program, "power"), (GLfloat)light_power);
        glUniform1f(glGetUniformLocation(program, "specular_power"), (GLfloat)specular_power);

        pass_vertex_data();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        set_texture_filtration();
        glUniform1i(texture_sampler, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normals_map_id);
        set_texture_filtration();
        glUniform1i(normals_map_sampler, 1);

        glDrawArrays(GL_TRIANGLES, 0, cur_draw_data().vertices_num());
    }


    void pass_vertex_data() {
        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        GLuint location = glGetAttribLocation(program, "vert_pos");
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(location);

        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        location = glGetAttribLocation(program, "vert_uv");
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(location);

        glBindBuffer(GL_ARRAY_BUFFER, norms_buffer);
        location = glGetAttribLocation(program, "vert_normal");
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(location);

        glBindBuffer(GL_ARRAY_BUFFER, tangent_buffer);
        location = glGetAttribLocation(program, "vert_tangent");
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(location);

        glBindBuffer(GL_ARRAY_BUFFER, bitangent_buffer);
        location = glGetAttribLocation(program, "vert_bitangent");
        glVertexAttribPointer(location, 3 , GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(location);
    }
};

program_state prog_state;

// отрисовка кадра
void display_func() { prog_state.on_display_event(); }

// Переисовка кадра в отсутствии других сообщений
void idle_func() { glutPostRedisplay(); }

void keyboard_func( unsigned char button, int x, int y ) {
   if (TwEventKeyboardGLUT(button, x, y))
      return;

   switch(button) {
   case 27:
      exit(0);
   }
}

// Отработка изменения размеров окна
void reshape_func( int width, int height ) {
   if (width <= 0 || height <= 0)
      return;
   glViewport(0, 0, width, height);
   TwWindowSize(width, height);
}

// Очищаем все ресурсы, пока контекст ещё не удалён
void close_func() { }

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
    glutCreateWindow("OpenGL basic sample");

    // Инициализация указателей на функции OpenGL
    if (glewInit() != GLEW_OK) {
       cerr << "GLEW init failed" << endl;
       exit(1);
    }

    // Проверка созданности контекста той версии, какой мы запрашивали
    if (!GLEW_VERSION_3_0) {
       cerr << "OpenGL 3.0 not supported" << endl;
       exit(1);
    }
}

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

void next_filtration_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->next_filtering_mode();
}

void create_controls(program_state& prog_state) {
    TwInit(TW_OPENGL, NULL);

    TwBar* bar = TwNewBar("Parameters");
    TwDefine("Parameters size='400 550' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                "label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &prog_state.rotation_by_control,
               " label='Object orientation' opened=true help='Change the object orientation.' ");
    TwAddButton(bar, "Next figure", next_figure_callback, &prog_state,
                "label='Draw next figure' key=n");
    TwAddButton(bar, "Switch wireframe mode", switch_polygon_mode, &prog_state,
                "label='Switch wireframe mode' key=w");
    TwAddButton(bar, "Next filtration mode", next_filtration_callback, &prog_state,
                "label='Next filtration mode' key=t");
    TwAddVarRW(bar, "Light power", TW_TYPE_FLOAT, &prog_state.light_power, "min=1 max=5 step=0.1");
    TwAddVarRW(bar, "LightDir", TW_TYPE_DIR3D, &prog_state.light_src_direction, "");
    TwAddVarRW(bar, "Light Color", TW_TYPE_COLOR3F, &prog_state.light_color, " colormode=rgb ");
    TwAddVarRW(bar, "Ambient ", TW_TYPE_COLOR3F, &prog_state.ambient, " colormode=hls ");
    TwAddVarRW(bar, "Specular ", TW_TYPE_COLOR3F, &prog_state.specular, " colormode=rgb ");
    TwAddVarRW(bar, "SpecularPower ", TW_TYPE_FLOAT, &prog_state.specular_power, "min=1 max=100 step=0.1");
}

void remove_controls() {
    TwDeleteAllBars();
    TwTerminate();
}

int main(int argc, char ** argv) {
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


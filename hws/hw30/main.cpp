#include "common.h"
#include "shader.h"
#include "utils.h"
#include <FreeImage.h>

// Размеры окна по-умолчанию
size_t const DEFAULT_WINDOW_WIDTH  = 800;
size_t const DEFAULT_WINDOW_HEIGHT = 800;

enum geom_obj { QUAD, CYLINDER, SPHERE, BACK_QUAD };
enum tex_filtering_mode { NEAREST, LINEAR, MIPMAP };
enum filter { NO_FILTER = 0, BOX_BLUR, GAUSSIAN_BLUR, SOBEL_FILTER };

struct draw_data {
    vector<GLfloat> vertices;
    vector<GLfloat> tex_mapping;
    vector<GLfloat> normals;

    size_t vertices_num() const { return vertices.size() / 3; }

    void* vertices_data() { return vertices.data(); }
    size_t vertices_data_size() const { return vertices.size() * sizeof(GLfloat); }

    void* tex_mapping_data() { return tex_mapping.data(); }
    size_t tex_mapping_data_size() const { return tex_mapping.size() * sizeof(GLfloat); }

    void* normals_data() { return normals.data(); }
    size_t normals_data_size() const { return normals.size() * sizeof(GLfloat); }
};

struct program_state {
    quat rotation_by_control;

    float tex_coords_scale;
    vec3 light_color;
    float light_power;
    quat light_src_rotation;
    float ambient;
    float specular;

    int gaussian_kernel_radius;
    float gaussian_variance;

    float sobel_threshold;

    program_state()
        : wireframe_mode(false)
        , cur_obj(QUAD)
        , cur_tex_filtering(NEAREST)
        , tex_coords_scale(1)
        , light_color(1, 1, 1)
        , light_power(500)
        , ambient(0.1)
        , specular(0.5)
        , cur_filter(NO_FILTER)
        , gaussian_kernel_radius(4)
        , gaussian_variance(4)
        , sobel_threshold(0.25)
    {}

    // this function must be called before main loop but after
    // gl libs init functions
    void init() {
        utils::read_obj_file(QUAD_MODEL_PATH, quad.vertices, quad.tex_mapping, quad.normals);
        utils::read_obj_file(CYLINDER_MODEL_PATH, cylinder.vertices, cylinder.tex_mapping, cylinder.normals);
        utils::read_obj_file(SPHERE_MODEL_PATH, sphere.vertices, sphere.tex_mapping, sphere.normals);
        init_background_quad();
        init_framebuffer();
        set_shaders();
        set_draw_configs();
        init_textures();
        set_texture_filtration();
        set_data_buffer();
    }

    void on_display_event() {
        float const window_width = cur_window_width();
        float const window_height = cur_window_height();
        float const subwindow_width = window_width / 2 - 5;
        float const right_x = window_width / 2 + 5;

        glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
        glEnable(GL_SCISSOR_TEST);

        glViewport(0, 0, window_width, window_height);
        glScissor(0, 0, window_width, window_height);
        glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bind_offscreen_buffer();

        glScissor(0, 0, window_width, window_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture_id);
        render_scene(window_width, window_height);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures

        unbind_offscreen_buffer();

        geom_obj prev_obj = cur_obj;
        cur_obj = BACK_QUAD;
        set_data_buffer();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glViewport(0, 0, subwindow_width, window_height);
        glScissor(0, 0, subwindow_width, window_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        filter cur = cur_filter;
        cur_filter = NO_FILTER;

        glBindTexture(GL_TEXTURE_2D, fbo_texture); // Bind our frame buffer texture
        render_with_filter(subwindow_width, window_height);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures

        cur_filter = cur;

        glViewport(right_x, 0, subwindow_width, window_height);
        glScissor(right_x, 0, subwindow_width, window_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, fbo_texture); // Bind our frame buffer texture
        render_with_filter(subwindow_width, window_height);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures

        cur_obj = prev_obj;
        set_data_buffer();

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

    void switch_polygon_mode() { wireframe_mode = !wireframe_mode; }

    void next_filtering_mode() {
        switch(cur_tex_filtering) {
        case NEAREST: cur_tex_filtering = LINEAR; break;
        case LINEAR: cur_tex_filtering = MIPMAP; break;
        case MIPMAP: cur_tex_filtering = NEAREST; break;
        }
        glBindTexture(GL_TEXTURE_2D, texture_id);
        set_texture_filtration();
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void on_resize_event() {
        init_background_quad();
    }

    void on_apply_filter_event(filter f) {
        cur_filter = f;
    }

    ~program_state() {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteProgram(scene_program);
        glDeleteShader(scene_vx_shader);
        glDeleteShader(scene_frag_shader);
        glDeleteProgram(filtered_program);
        glDeleteShader(filtered_vx_shader);
        glDeleteShader(filtered_frag_shader);

        glDeleteBuffers(1, &vx_buffer);
        glDeleteBuffers(1, &tex_buffer);
        glDeleteBuffers(1, &norms_buffer);

        glDeleteBuffers(1, &fbo);
        glDeleteBuffers(1, &fbo_depth);
        glDeleteTextures(1, &fbo_texture);
    }

private:
    bool wireframe_mode;
    geom_obj cur_obj;
    tex_filtering_mode cur_tex_filtering;

    GLuint scene_vx_shader;
    GLuint scene_frag_shader;
    GLuint scene_program;
    GLuint filtered_vx_shader;
    GLuint filtered_frag_shader;
    GLuint filtered_program;

    GLuint vx_buffer;
    GLuint tex_buffer;
    GLuint norms_buffer;

    GLuint texture_sampler;
    GLuint texture_id;

    GLuint fbo; // The frame buffer object
    GLuint fbo_depth; // The depth buffer for the frame buffer object
    GLuint fbo_texture; // The texture object to write our frame buffer object

    const char* QUAD_MODEL_PATH = "..//resources//quad.obj";
    draw_data quad;

    const char* CYLINDER_MODEL_PATH = "..//resources//cylinder.obj";
    draw_data cylinder;

    const char* SPHERE_MODEL_PATH = "..//resources//sphere.obj";
    draw_data sphere;

    draw_data back_quad;

    filter cur_filter;

    const char* TEXTURE_PATH = "..//resources//wall3.jpg";

    const char* SCENE_VERTEX_SHADER_PATH = "..//shaders//for_scene.vs";
    const char* SCENE_FRAGMENT_SHADER_PATH = "..//shaders//for_scene.fs";
    const char* FILTERED_VERTEX_SHADER_PATH = "..//shaders//for_filtered.vs";
    const char* FILTERED_FRAGMENT_SHADER_PATH = "..//shaders//for_filtered.fs";

    vertex_attr const IN_POS = { "vert_pos_modelspace", 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0 };
    vertex_attr const VERTEX_UV = { "vert_uv", 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0 };
    vertex_attr const IN_NORM = { "vert_normal_modelspace", 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0 };

    draw_data& cur_draw_data() {
        switch(cur_obj) {
        case QUAD: return quad;
        case CYLINDER: return cylinder;
        case SPHERE: return sphere;
        case BACK_QUAD: return back_quad;
        default: throw msg_exception("cur_obj is undefined");
        }
    }

    void set_shaders() {
        scene_vx_shader = create_shader(GL_VERTEX_SHADER, SCENE_VERTEX_SHADER_PATH);
        scene_frag_shader = create_shader(GL_FRAGMENT_SHADER, SCENE_FRAGMENT_SHADER_PATH);
        scene_program = create_program(scene_vx_shader, scene_frag_shader);

        filtered_vx_shader = create_shader(GL_VERTEX_SHADER, FILTERED_VERTEX_SHADER_PATH);
        filtered_frag_shader = create_shader(GL_FRAGMENT_SHADER, FILTERED_FRAGMENT_SHADER_PATH);
        filtered_program = create_program(filtered_vx_shader, filtered_frag_shader);
    }

    void set_draw_configs() {
        glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    void init_textures() {
        texture_data tex_data = utils::load_texture(TEXTURE_PATH);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_data.width, tex_data.height,
                     0, tex_data.format, GL_UNSIGNED_BYTE, tex_data.data_ptr);
        set_texture_filtration();
        texture_sampler  = glGetUniformLocation(scene_program, "texture_sampler");
        glUniform1i(texture_sampler, 0);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void set_texture_filtration() {
        switch (cur_tex_filtering) {
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void set_data_buffer() {
        draw_data& data = cur_draw_data();
        glDeleteBuffers(1, &vx_buffer);
        glGenBuffers(1, &vx_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.vertices_data_size(),
                     data.vertices_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &tex_buffer);
        glGenBuffers(1, &tex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.tex_mapping_data_size(),
                     data.tex_mapping_data(), GL_STATIC_DRAW);

        glDeleteBuffers(1, &norms_buffer);
        glGenBuffers(1, &norms_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, norms_buffer);
        glBufferData(GL_ARRAY_BUFFER, data.normals_data_size(),
                     data.normals_data(), GL_STATIC_DRAW);
    }

    void render_scene(float window_width, float window_height) {
        glUseProgram(scene_program);

        mat4 const proj = perspective(45.0f, window_width / window_height, 0.1f, 100.0f);
        mat4 const model = mat4_cast(rotation_by_control);
        mat4 const view = lookAt(vec3(0, 0, 6), vec3(0, 0, 0), vec3(0, 1, 0));
        mat4 const modelview = view * model;
        mat4 const mvp = proj * modelview;

        GLuint location = glGetUniformLocation(scene_program, "mvp");
        glUniformMatrix4fv(location, 1, GL_FALSE, &mvp[0][0]);
        location = glGetUniformLocation(scene_program, "model");
        glUniformMatrix4fv(location, 1, GL_FALSE, &model[0][0]);
        location = glGetUniformLocation(scene_program, "view");
        glUniformMatrix4fv(location, 1, GL_FALSE, &view[0][0]);

        vec3 const lightPos = vec3(mat4_cast(light_src_rotation) * vec4(13, 13, 8, 1));
        glUniform3f(glGetUniformLocation(scene_program, "lightpos_worldspace"),
                    lightPos[0], lightPos[1], lightPos[2]);
        glUniform1f(glGetUniformLocation(scene_program, "tex_coords_scale"),
                    tex_coords_scale);
        glUniform3f(glGetUniformLocation(scene_program, "light_color"),
                    light_color[0], light_color[1], light_color[2]);
        glUniform1f(glGetUniformLocation(scene_program, "light_power"), light_power);
        glUniform3f(glGetUniformLocation(scene_program, "ambient"), ambient, ambient, ambient);
        glUniform3f(glGetUniformLocation(scene_program, "specular"), specular, specular, specular);

        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        utils::set_vertex_attr_ptr(scene_program, IN_POS);
        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        utils::set_vertex_attr_ptr(scene_program, VERTEX_UV);
        glBindBuffer(GL_ARRAY_BUFFER, norms_buffer);
        utils::set_vertex_attr_ptr(scene_program, IN_NORM);

        glDrawArrays(GL_TRIANGLES, 0, cur_draw_data().vertices_num());
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }

    float cur_window_width() { return glutGet(GLUT_WINDOW_WIDTH); }
    float cur_window_height() { return glutGet(GLUT_WINDOW_HEIGHT); }

    void init_framebuffer_depth_buf(void) {
        glGenRenderbuffersEXT(1, &fbo_depth); // Generate one render buffer and store the ID in fbo_depth
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo_depth); // Bind the fbo_depth render buffer

        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                                 cur_window_width(), cur_window_height()); // Set the render buffer storage to be a depth component, with a width and height of the window
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT, fbo_depth); // Set the render buffer of this buffer to the depth buffer

        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0); // Unbind the render buffer
    }

    void init_framebuffer_texture(void) {
        glGenTextures(1, &fbo_texture); // Generate one texture
        glBindTexture(GL_TEXTURE_2D, fbo_texture); // Bind the texture fbo_texture

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cur_window_width(),
                     cur_window_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        set_texture_filtration();

        // Unbind the texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void init_framebuffer(void) {
        init_framebuffer_depth_buf(); // Initialize our frame buffer depth buffer
        init_framebuffer_texture(); // Initialize our frame buffer texture

        glGenFramebuffersEXT(1, &fbo); // Generate one frame buffer and store the ID in fbo
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo); // Bind our frame buffer

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_2D, fbo_texture, 0); // Attach the texture fbo_texture to the color buffer in our frame buffer
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT, fbo_depth); // Attach the depth buffer fbo_depth to our frame buffer

        GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); // Check that status of our generated frame buffer
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT) { // If the frame buffer does not report back as complete
            throw msg_exception("frame buffer creation error"); // Output an error to the console
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // Unbind our frame buffer
    }

    void bind_offscreen_buffer() {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo); // Bind our frame buffer for rendering
        glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT); // Push our glEnable and glViewport states
        glViewport(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT); // Set the size of the frame buffer view port
    }

    void unbind_offscreen_buffer() {
        glPopAttrib(); // Restore our glEnable and glViewport states
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // Unbind our texture
    }

    void render_with_filter(float window_width, float window_height) {
        glUseProgram(filtered_program);

        mat4 const proj = perspective(45.0f, window_width / window_height, 0.1f, 100.0f);
        mat4 const model;
        mat4 const view = lookAt(vec3(0, 0, 3), vec3(0, 0, 0), vec3(0, 1, 0));
        mat4 const modelview = view * model;
        mat4 const mvp = proj * modelview;

        GLuint location = glGetUniformLocation(filtered_program, "mvp");
        glUniformMatrix4fv(location, 1, GL_FALSE, &mvp[0][0]);

        switch (cur_filter) {
        case BOX_BLUR:
            glUniform1i(glGetUniformLocation(filtered_program, "filter_type"), BOX_BLUR);
            break;
        case GAUSSIAN_BLUR:
            glUniform1i(glGetUniformLocation(filtered_program, "filter_type"), GAUSSIAN_BLUR);
            glUniform1i(glGetUniformLocation(filtered_program, "gaus_radius"), gaussian_kernel_radius);
            glUniform1f(glGetUniformLocation(filtered_program, "gaus_variance"), gaussian_variance);
            break;
        case SOBEL_FILTER:
            glUniform1i(glGetUniformLocation(filtered_program, "filter_type"), SOBEL_FILTER);
            glUniform1f(glGetUniformLocation(filtered_program, "sobel_threshold"), sobel_threshold);
            break;
        default:
            glUniform1i(glGetUniformLocation(filtered_program, "filter_type"), NO_FILTER);
            break;
        }

        glBindBuffer(GL_ARRAY_BUFFER, vx_buffer);
        utils::set_vertex_attr_ptr(scene_program, IN_POS);
        glBindBuffer(GL_ARRAY_BUFFER, tex_buffer);
        utils::set_vertex_attr_ptr(scene_program, VERTEX_UV);

        glDrawArrays(GL_QUADS, 0, cur_draw_data().vertices_num());
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    void init_background_quad() {
        float const cur_win_width = cur_window_width();
        float const cur_win_height = cur_window_height();
        float quad_half_width = 1;
        float quad_half_height = 1;
        if(cur_win_height > cur_win_width) {
            quad_half_height *= (cur_win_height / cur_win_width);
        } else {
            quad_half_width *= (cur_win_width / cur_win_height);
        }
        back_quad.vertices = {
            -quad_half_width, -quad_half_height, 0,
            quad_half_width,  -quad_half_height, 0,
            quad_half_width,   quad_half_height, 0,
            -quad_half_width,  quad_half_height, 0
        };
        back_quad.tex_mapping = {
            0, 0,
            1, 0,
            1, 1,
            0, 1
        };
    }
};

// global, because display_func callback needs it
program_state prog_state;

void basic_init(int argc, char ** argv) {
    glutInit               (&argc, argv);
    glutInitWindowSize     (DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
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
   prog_state.on_resize_event();
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

void next_filtration_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->next_filtering_mode();
}

void apply_no_filter_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->on_apply_filter_event(NO_FILTER);
}

void apply_box_filter_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->on_apply_filter_event(BOX_BLUR);
}

void apply_gaussian_filter_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->on_apply_filter_event(GAUSSIAN_BLUR);
}

void apply_sobel_filter_callback(void* prog_state_wrapper) {
    program_state* ps = static_cast<program_state*>(prog_state_wrapper);
    ps->on_apply_filter_event(SOBEL_FILTER);
}

void create_controls(program_state& prog_state) {
    TwInit(TW_OPENGL, NULL);

    TwBar *bar = TwNewBar("Parameters");
    TwDefine("Parameters size='400 470' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                "label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &prog_state.rotation_by_control,
               "label='Object orientation' opened=true help='Change the object orientation.'");
    TwAddButton(bar, "Next figure", next_figure_callback, &prog_state,
                "label='Draw next figure' key=n");
    TwAddButton(bar, "Switch wireframe mode", switch_polygon_mode, &prog_state,
                "label='Switch wireframe mode' key=w");
    TwAddButton(bar, "Next filtration mode", next_filtration_callback, &prog_state,
                "label='Next filtration mode' key=t");

    TwAddVarRW(bar, "Tex coords scale", TW_TYPE_FLOAT, &prog_state.tex_coords_scale,
               "min=0.1 max=2 step=0.1");
    TwAddVarRW(bar, "Light color R", TW_TYPE_FLOAT, &prog_state.light_color[0],
               "min=0 max=1 step=0.1");
    TwAddVarRW(bar, "Light color G", TW_TYPE_FLOAT, &prog_state.light_color[1],
               "min=0 max=1 step=0.1");
    TwAddVarRW(bar, "Light color B", TW_TYPE_FLOAT, &prog_state.light_color[2],
               "min=0 max=1 step=0.1");
    TwAddVarRW(bar, "Light power", TW_TYPE_FLOAT, &prog_state.light_power,
               "min=0 max=500 step=10");
    TwAddVarRW(bar, "Light src rotation", TW_TYPE_QUAT4F, &prog_state.light_src_rotation,
               "opened=true");
    TwAddVarRW(bar, "Ambient", TW_TYPE_FLOAT, &prog_state.ambient,
               "min=0 max=1 step=0.1");
    TwAddVarRW(bar, "Specular", TW_TYPE_FLOAT, &prog_state.specular,
               "min=0 max=1 step=0.1");

    TwAddButton(bar, "No filter", apply_no_filter_callback, &prog_state,
                "label='No filter' key=o");
    TwAddButton(bar, "Box blur", apply_box_filter_callback, &prog_state,
                "label='Box blur' key=b");
    TwAddButton(bar, "Gaussian blur", apply_gaussian_filter_callback, &prog_state,
                "label='Gaussian blur' key=g");
    TwAddVarRW(bar, "Gaussian kernel radius", TW_TYPE_UINT8, &prog_state.gaussian_kernel_radius,
               "min=1 max=10 step=1");
    TwAddVarRW(bar, "Gaussian variance", TW_TYPE_FLOAT, &prog_state.gaussian_variance,
               "min=0.1 max=5 step=0.1");
    TwAddButton(bar, "Sobel filter", apply_sobel_filter_callback, &prog_state,
                "label='Sobel filter' key=s");
    TwAddVarRW(bar, "Sobel post threshold", TW_TYPE_FLOAT, &prog_state.sobel_threshold,
               "min=0 max=1 step=0.05");
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

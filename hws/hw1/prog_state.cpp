#include "prog_state.h"
#include "shader.h"

static const string MODEL_FILE         = "..//input//model.obj";
static const string VERTEX_SHADER      = "..//shaders//0.glslvs";
static const string FRAGMENT_SHADER    = "..//shaders//0.glslfs";

void switch_colors_callback(void * sample) {
    static_cast<ProgState*>(sample)->change_mode();
}

void TW_CALL toggle_fullscreen_callback( void * ) {
    glutFullScreenToggle();
}

ProgState::ProgState()
    : wireframe_(false)
    , triangle_(vec2(0, 25), vec2(20, -15), vec2(-20, -15))
    , cell_size_(3.0f)
    , mode_(NORMALS)
    , v_(1)
    , k_(1)
    , center_(vec3(0, 0, 0))
    , max_(0)
{
    create_tw_bar();

    model_.load(MODEL_FILE);
    for (size_t i = 0; i < model_.vertices_count(); ++i) {
        data_.push_back(model_.vertices[i]);
        data_.push_back(model_.normals[i]);
    }

    init_shaders(VERTEX_SHADER, FRAGMENT_SHADER);
    init_buffer();
}

ProgState::~ProgState() {
    // Удаление ресурсов OpenGL
    glDeleteProgram(program_);
    glDeleteShader(vs_);
    glDeleteShader(fs_);
    glDeleteBuffers(1, &vx_buf_);

    TwDeleteAllBars();
    TwTerminate();
}

void ProgState::create_tw_bar() {
    TwInit(TW_OPENGL, NULL);

    // Определение "контролов" GUI
    TwBar *bar = TwNewBar("Parameters");
    TwDefine(" Parameters size='500 200' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddVarRW(bar, "v", TW_TYPE_FLOAT, &v_, " min=-100 max=100 step=1 label='V' keyincr=p keydecr=o");
    TwAddVarRW(bar, "k", TW_TYPE_FLOAT, &k_, " min=-100 max=100 step=1 label='K' keyincr=l keydecr=k");
    TwAddVarRW(bar, "Wireframe", TW_TYPE_BOOLCPP, &wireframe_, " true='ON' false='OFF' key=w");
    TwAddButton(bar, "SwitchColorMode", switch_colors_callback, this,
                " label = 'Switch color mode' key=g");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                " label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &rotation_by_control_,
               " label='Object orientation' opened=true help='Change the object orientation.' ");
}

void ProgState::init_shaders(string const& vs_file, string const& fs_file) {
    vs_ = create_shader(GL_VERTEX_SHADER  , vs_file.c_str());
    fs_ = create_shader(GL_FRAGMENT_SHADER, fs_file.c_str());

    program_ = create_program(vs_, fs_);
}

void ProgState::init_buffer() {
    // Создание пустого буфера
    glGenBuffers(1, &vx_buf_);
    // Делаем буфер активным
    glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);

    // Копируем данные для текущего буфера на GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * data_.size(), &data_[0], GL_STATIC_DRAW);

    // Сбрасываем текущий активный буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ProgState::draw_frame( float time_from_start ) {
    float const w = (float)glutGet(GLUT_WINDOW_WIDTH);
    float const h = (float)glutGet(GLUT_WINDOW_HEIGHT);

    mat4 const proj             = perspective(45.0f, w / h, 0.1f, 100.0f);
    mat4 const view             = lookAt(vec3(0, 0, 30), vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 const full_rotate      = mat4_cast(rotation_by_control_);

    mat4 const modelview        = view * full_rotate;
    mat4 const mvp              = proj * modelview;

    update_color_params(modelview); //or refresh(mat4(1.0f));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(program_);

    GLuint const mvp_location = glGetUniformLocation(program_, "mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

    GLuint const mv_location = glGetUniformLocation(program_, "mv");
    glUniformMatrix4fv(mv_location, 1, GL_FALSE, &modelview[0][0]);

    GLuint const is_wireframe_location = glGetUniformLocation(program_, "is_wireframe");
    glUniform1i(is_wireframe_location, false);

    GLuint const T_location = glGetUniformLocation(program_, "T");
    glUniform1f(T_location, time_from_start);

    GLuint const k_location = glGetUniformLocation(program_, "k");
    glUniform1f(k_location, k_);

    GLuint const v_location = glGetUniformLocation(program_, "v");
    glUniform1f(v_location, v_);

    GLuint const center_location = glGetUniformLocation(program_, "center");
    glUniform3f(center_location, center_[0], center_[1], center_[2]);

    GLuint const max_location = glGetUniformLocation(program_, "max");
    glUniform1f(max_location, max_);

    GLuint const func_mode_location = glGetUniformLocation(program_, "func_mode");
    if (mode_ == NORMALS) {
        glUniform1i(func_mode_location, false);
    } else {
        glUniform1i(func_mode_location, true);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);

    GLuint const pos_location = glGetAttribLocation(program_, "in_pos");
    glEnableVertexAttribArray(pos_location);
    glVertexAttribPointer(pos_location, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), 0);

    GLuint const color_location = glGetAttribLocation(program_, "in_color");
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), (GLvoid*)(sizeof(vec3)));

    glDrawArrays(GL_TRIANGLES, 0, data_.size() / 2);

    if (wireframe_) {
        glPolygonOffset(-1, -1);
        glEnable(GL_POLYGON_OFFSET_FILL);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        GLboolean const is_wireframe_location = glGetUniformLocation(program_, "is_wireframe");
        glUniform1i(is_wireframe_location, true);

        glDrawArrays(GL_TRIANGLES, 0, data_.size() / 2);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    glDisableVertexAttribArray(pos_location);
    glDisableVertexAttribArray(color_location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ProgState::update_color_params(mat4 m) {
    for (size_t i = 0; i < model_.vertices_count(); ++i) {
        center_ += model_.vertices[i];
    }
    center_ = center_ / float(model_.vertices_count());

    for (size_t i = 0; i < model_.vertices_count(); ++i) {
        if (glm::length(center_ - model_.vertices[i]) > max_) {
            max_ = glm::length(center_ - model_.vertices[i]);
        }
    }
    center_ = vec3(m * vec4(center_, 1));
}

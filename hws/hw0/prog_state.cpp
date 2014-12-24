#include "prog_state.h"
#include "shader.h"

prog_state::prog_state()
   : wireframe_(false)
   , task_num_(0)
{
    create_tw_bar();

    vs_ = create_shader(GL_VERTEX_SHADER  , "..//shaders//0.glslvs");
    fs0_ = create_shader(GL_FRAGMENT_SHADER, "..//shaders//0.glslfs");
    fs1_ = create_shader(GL_FRAGMENT_SHADER, "..//shaders//1.glslfs");
    fs2_ = create_shader(GL_FRAGMENT_SHADER, "..//shaders//2.glslfs");
    // Создание программы путём линковки шейдерова
    program0_ = create_program(vs_, fs0_);
    program1_ = create_program(vs_, fs1_);
    program2_ = create_program(vs_, fs2_);
    program_cur_ = program0_;
    // Создание буфера с вершинными данными
    init_buffer();
}

prog_state::~prog_state() {
    glDeleteProgram(program0_);
    glDeleteProgram(program1_);
    glDeleteProgram(program2_);
    glDeleteShader(vs_);
    glDeleteShader(fs0_);
    glDeleteShader(fs1_);
    glDeleteShader(fs2_);
    glDeleteBuffers(1, &vx_buf_);

    TwDeleteAllBars();
    TwTerminate();
}

void prog_state::init_buffer() {
    // Создание пустого буфера
    glGenBuffers(1, &vx_buf_);
    // Делаем буфер активным
    glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);

    // Данные для визуализации
    vec2 const data[3] = {
        vec2(5, 5)
      , vec2(-5, 5)
      , vec2(0, -10)
    };

    // Копируем данные для текущего буфера на GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 3, data, GL_STATIC_DRAW);

    // Сбрасываем текущий активный буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void prog_state::draw_triangle(float time_from_start) {
    float const rotation_angle = time_from_start * 90;

    float const w                = (float)glutGet(GLUT_WINDOW_WIDTH);
    float const h                = (float)glutGet(GLUT_WINDOW_HEIGHT);
    // строим матрицу проекции с aspect ratio (отношением сторон) таким же, как у окна
    mat4  const proj             = perspective(45.0f, w / h, 0.1f, 100.0f);
    // преобразование из СК мира в СК камеры
    mat4  const view             = lookAt(vec3(0, 0, 50), vec3(0, 0, 0), vec3(0, 1, 0));
    // анимация по времени
    quat  const rotation_by_time = quat(vec3(0, 0, radians(rotation_angle)));
    mat4  const modelview        = view * mat4_cast(rotation_by_control_ * rotation_by_time);
    mat4  const mvp              = proj * modelview;

    // выбор режима растеризации - только рёбра или рёбра + грани
    if (wireframe_)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // выключаем отсечение невидимых поверхностей
    glDisable(GL_CULL_FACE);
    // выключаем тест глубины
    glDisable(GL_DEPTH_TEST);
    // очистка буфера кадра
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // установка шейдеров для рисования
    glUseProgram(program_cur_);

    // установка uniform'ов
    GLuint const mvp_location = glGetUniformLocation(program_cur_, "mvp");
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp[0][0]);

    GLuint const time_location = glGetUniformLocation(program_cur_, "time");
    glUniform1f(time_location, time_from_start);

    glBindBuffer(GL_ARRAY_BUFFER, vx_buf_);

    // запрашиваем индек аттрибута у программы, созданные по входным шейдерам
    GLuint const pos_location = glGetAttribLocation(program_cur_, "in_pos");
    // устанавливаем формам данных для аттрибута "pos_location"
    // 2 float'а ненормализованных, шаг между вершиными равен sizeof(vec2), смещение от начала буфера равно 0
    glVertexAttribPointer(pos_location, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
    // "включаем" аттрибут "pos_location"
    glEnableVertexAttribArray(pos_location);

      // отрисовка
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(pos_location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TW_CALL toggle_fullscreen_callback( void * ) { glutFullScreenToggle(); }

void prog_state::create_tw_bar() {
    TwInit(TW_OPENGL, NULL);

    TwBar *bar = TwNewBar("Parameters");
    TwDefine(" Parameters size='500 160' color='70 100 120' valueswidth=220 iconpos=topleft");
    TwAddVarRW(bar, "Wireframe mode", TW_TYPE_BOOLCPP, &wireframe_, " true='ON' false='OFF' key=w");
    TwAddButton(bar, "Fullscreen toggle", toggle_fullscreen_callback, NULL,
                " label='Toggle fullscreen mode' key=f");
    TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &rotation_by_control_,
               " label='Object orientation' opened=true help='Change the object orientation.' ");
    TwAddButton(bar, "Next task result", prog_state::incr_task_num, this,
                " label='Next task result' key=n");

}

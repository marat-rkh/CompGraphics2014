#include "common.h"
#include "shader.h"

#include "prog_state.h"

#ifndef APIENTRY
   #define APIENTRY
#endif

unique_ptr<prog_state> g_sample;

// отрисовка кадра
void display_func() { g_sample->draw(); }

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
void close_func() { g_sample.reset(); }

// callback на различные сообщения от OpenGL
void APIENTRY gl_debug_proc(GLenum         //source
                          , GLenum         type
                          , GLuint         //id
                          , GLenum         //severity
                          , GLsizei        //length
                          , GLchar const * message
                          , GLvoid *       /*user_param*/)
{
   if (type == GL_DEBUG_TYPE_ERROR_ARB) {
      cerr << message << endl;
      exit(1);
   }
}

int main( int argc, char ** argv ) {
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
   int window_handle = glutCreateWindow("OpenGL basic sample");

   // Инициализация указателей на функции OpenGL
   if (glewInit() != GLEW_OK) {
      cerr << "GLEW init failed" << endl;
      return 1;
   }

   // Проверка созданности контекста той версии, какой мы запрашивали
   if (!GLEW_VERSION_3_0) {
      cerr << "OpenGL 3.0 not supported" << endl;
      return 1;
   }

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

   try {
      // Создание класса-примера
      g_sample.reset(new prog_state());
      // Вход в главный цикл приложения
      glutMainLoop();
   }
   catch( std::exception const & except ) {
      std::cout << except.what() << endl;
      return 1;
   }

   return 0;
}

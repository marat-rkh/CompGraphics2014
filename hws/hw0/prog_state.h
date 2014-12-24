#ifndef PROG_STATE_H
#define PROG_STATE_H

#include "common.h"

class prog_state {
public:
   prog_state();
   ~prog_state();

   void draw() {
       static chrono::system_clock::time_point const start = chrono::system_clock::now();
       switch (task_num_) {
       case 0:
           program_cur_ = program0_;
           draw_triangle(chrono::duration<float>(chrono::system_clock::now() - start).count());
           break;
       case 1:
           program_cur_ = program1_;
           draw_triangle(chrono::duration<float>(chrono::system_clock::now() - start).count());
           break;
       case 2:
           program_cur_ = program2_;
           draw_triangle(chrono::duration<float>(chrono::system_clock::now() - start).count());
           break;
       }
       TwDraw();
       glutSwapBuffers();
   }

private:
   void create_tw_bar();
   static void incr_task_num(void* prog_state_wrapper) {
       prog_state* ps = static_cast<prog_state*>(prog_state_wrapper);
       const size_t TASKS_NUM = 3;
       ps->task_num_ = (ps->task_num_ + 1) % TASKS_NUM;
   }
   void init_buffer();

   void draw_triangle(float time_from_start);

   bool wireframe_;
   quat rotation_by_control_;
   size_t task_num_;

   GLuint vs_, fs0_, fs1_, fs2_, program0_, program1_, program2_;
   GLuint program_cur_;

   GLuint vx_buf_;
};

#endif // PROG_STATE_H

/******************************************************************************\
| OpenGL 4 Example Code.                                                       |
| Accompanies written series "Anton's OpenGL 4 Tutorials"                      |
| Email: anton at antongerdelan dot net                                        |
| First version 27 Jan 2014                                                    |
| Dr Anton Gerdelan, Trinity College Dublin, Ireland.                          |
| See individual libraries for separate legal notices                          |
|******************************************************************************|
| Tessellation Shaders Example                                                 |
| Raise/Lower "inner" tessellation factor - Q,A keys                           |
| Raise/Lower "outer" tessellation factor - W,S keys                           |
\******************************************************************************/
#include "gl_utils.h"   // i put all the clutter and little functions here
#include "maths_funcs.h"
#include <GL/glew.h>    // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>


// keep track of window size for things like the viewport and the mouse cursor
int g_gl_width  = 640;
int g_gl_height = 480;

float inner_tess_fac = 3.0;
float outer_tess_fac = 2.0;

vec3 cam_pos( 0.0f, 0.0f, -5.0f );
vec3 target_pos( 0.0f, 0.0f, 0.0f );
vec3 up( 0, 1, 0 );

int main() {
  restart_gl_log();
  // start GL context and O/S window using the GLFW helper library
  gl_log( "starting GLFW %s", glfwGetVersionString() );

  glfwSetErrorCallback( glfw_error_callback );
  if ( !glfwInit() ) {
    gl_log_err( "ERROR: could not start GLFW3\n" );
    return 1;
  }

  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_SAMPLES, 4 );

  /*GLFWmonitor* mon = glfwGetPrimaryMonitor ();
  const GLFWvidmode* vmode = glfwGetVideoMode (mon);
  GLFWwindow* window = glfwCreateWindow (
          vmode->width, vmode->height, "Extended GL Init", mon, NULL
  );*/

  GLFWwindow* window = glfwCreateWindow( g_gl_width, g_gl_height, "Extended Init.", NULL, NULL );
  if ( !window ) {
    gl_log_err( "ERROR: could not open window with GLFW3\n" );
    glfwTerminate();
    return 1;
  }
  glfwSetFramebufferSizeCallback( window, glfw_framebuffer_size_callback );
  glfwMakeContextCurrent( window );


  // start GLEW extension handler
  glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte* renderer = glGetString( GL_RENDERER ); // get renderer string
  const GLubyte* version  = glGetString( GL_VERSION );  // version as a string
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );
  gl_log( "renderer: %s\nversion: %s\n", renderer, version );
  log_gl_params();

  // query hardware support for tessellation
  int max_patch_vertices = 0;
  glGetIntegerv( GL_MAX_PATCH_VERTICES, &max_patch_vertices );
  printf( "Max supported patch vertices %i\n", max_patch_vertices );

  // tell GL to only draw onto a pixel if the shape is closer to the viewer
  glEnable( GL_DEPTH_TEST ); // enable depth-testing
  glDepthFunc( GL_LESS );    // depth-testing interprets a smaller value as "closer"
  glClearColor( 0.7f, 0.6f, 0.5f, 1.0f );

  const float Verts[] = {
         0.000f,  0.000f,  1.000f,
         0.894f,  0.000f,  0.447f,
         0.276f,  0.851f,  0.447f,
        -0.724f,  0.526f,  0.447f,
        -0.724f, -0.526f,  0.447f,
         0.276f, -0.851f,  0.447f,
         0.724f,  0.526f, -0.447f,
        -0.276f,  0.851f, -0.447f,
        -0.894f,  0.000f, -0.447f,
        -0.276f, -0.851f, -0.447f,
         0.724f, -0.526f, -0.447f,
         0.000f,  0.000f, -1.000f };
    //GLfloat Verts[] = { 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f };


  const int Index[] = {
        2, 1, 0,
        3, 2, 0,
        4, 3, 0,
        5, 4, 0,
        1, 5, 0,

        11, 6,  7,
        11, 7,  8,
        11, 8,  9,
        11, 9,  10,
        11, 10, 6,

        1, 2, 6,
        2, 3, 7,
        3, 4, 8,
        4, 5, 9,
        5, 1, 10,

        2,  7, 6,
        3,  8, 7,
        4,  9, 8,
        5, 10, 9,
        1, 6, 10 };
 // const GLuint Index[] = {2, 1, 0};

  GLsizei IndexCount = sizeof(Index) / sizeof(Index[0]);

  GLuint points;
  glGenBuffers( 1, &points );
  glBindBuffer( GL_ARRAY_BUFFER, points );
  glBufferData( GL_ARRAY_BUFFER, 36 * sizeof( GLfloat ), Verts, GL_STATIC_DRAW );

  GLuint vao;
  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );
  glBindBuffer( GL_ARRAY_BUFFER, points );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  glEnableVertexAttribArray( 0 );

  GLuint indices;
  glGenBuffers(1, &indices);
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indices );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( Index ), Index, GL_STATIC_DRAW );

  char vertex_shader[4096];
  char tess_ctrl_shader[4096];
  char tess_eval_shader[4096];
  char geometry_shader[4096];
  char fragment_shader[4096];
  ( parse_file_into_str( "test_vs.glsl", vertex_shader, 4096 ) );
  ( parse_file_into_str( "test_tcs.glsl", tess_ctrl_shader, 4096 ) );
  ( parse_file_into_str( "test_tes.glsl", tess_eval_shader, 4096 ) );
  ( parse_file_into_str( "test_gs.glsl", geometry_shader, 4096 ) );
  ( parse_file_into_str( "test_fs.glsl", fragment_shader, 4096 ) );

  GLuint vs       = glCreateShader( GL_VERTEX_SHADER );
  const GLchar* p = (const GLchar*)vertex_shader;
  glShaderSource( vs, 1, &p, NULL );
  glCompileShader( vs );

  // check for compile errors
  int params = -1;
  glGetShaderiv( vs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: GL shader index %i did not compile\n", vs );
    _print_shader_info_log( vs );
    return 1; // or exit or something
  }

  // compile tessellation control shader
  GLuint tcs = glCreateShader( GL_TESS_CONTROL_SHADER );
  p          = (const GLchar*)tess_ctrl_shader;
  glShaderSource( tcs, 1, &p, NULL );
  glCompileShader( tcs );
  params = -1;
  glGetShaderiv( tcs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: GL shader index %i did not compile\n", tcs );
    _print_shader_info_log( tcs );
    return 1; // or exit or something
  }

  GLuint tes = glCreateShader( GL_TESS_EVALUATION_SHADER );
  p          = (const GLchar*)tess_eval_shader;
  glShaderSource( tes, 1, &p, NULL );
  glCompileShader( tes );
  params = -1;
  glGetShaderiv( tes, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: GL shader index %i did not compile\n", tes );
    _print_shader_info_log( tes );
    return 1; // or exit or something
  }

  GLuint gs = glCreateShader( GL_GEOMETRY_SHADER );
  p         = (const GLchar*)geometry_shader;
  glShaderSource( gs, 1, &p, NULL );
  glCompileShader( gs );
  // check for compile errors
  glGetShaderiv( gs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: GL shader index %i did not compile\n", gs );
    _print_shader_info_log( gs );
    return 1; // or exit or something
  }

  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  p         = (const GLchar*)fragment_shader;
  glShaderSource( fs, 1, &p, NULL );
  glCompileShader( fs );
  // check for compile errors
  glGetShaderiv( fs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: GL shader index %i did not compile\n", fs );
    _print_shader_info_log( fs );
    return 1; // or exit or something
  }

  GLuint shader_programme = glCreateProgram();
  glAttachShader( shader_programme, fs );
  glAttachShader( shader_programme, gs );
  // attach both tessellation
  glAttachShader( shader_programme, tes );
  glAttachShader( shader_programme, tcs );
  glAttachShader( shader_programme, vs );
  glLinkProgram( shader_programme );

  glGetProgramiv( shader_programme, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    gl_log_err( "ERROR: could not link shader programme GL index %i\n", shader_programme );
    _print_programme_info_log( shader_programme );
    return false;
  }
  ( is_valid( shader_programme ) );

  glUseProgram( shader_programme );
  GLuint Projection_loc = glGetUniformLocation(shader_programme, "Projection");
  GLuint Modelview_loc = glGetUniformLocation(shader_programme, "Modelview");
  GLuint NormalMatrix_loc = glGetUniformLocation(shader_programme, "NormalMatrix");
  GLuint LightPosition_loc = glGetUniformLocation(shader_programme, "LightPosition");
  GLuint AmbientMaterial_loc = glGetUniformLocation(shader_programme, "AmbientMaterial");
  GLuint DiffuseMaterial_loc = glGetUniformLocation(shader_programme, "DiffuseMaterial");
  GLuint TessLevelInner_loc = glGetUniformLocation(shader_programme, "TessLevelInner");
  GLuint TessLevelOuter_loc = glGetUniformLocation(shader_programme, "TessLevelOuter");

  #define ONE_DEG_IN_RAD ( 2.0 * M_PI ) / 360.0 // 0.017444444
  // input variables
  float near   = 0.1f;                                   // clipping plane
  float far    = 100.0f;                                 // clipping plane
  float fov    = 67.0f * ONE_DEG_IN_RAD;                 // convert 67 degrees to radians
  float aspect = (float)g_gl_width / (float)g_gl_height; // aspect ratio
  // matrix components
  float inverse_range = 1.0f / tan( fov * 0.5f );
  float Sx            = inverse_range / aspect;
  float Sy            = inverse_range;
  float Sz            = -( far + near ) / ( far - near );
  float Pz            = -( 2.0f * far * near ) / ( far - near );
  GLfloat proj_mat[]  = { Sx, 0.0f, 0.0f, 0.0f, 0.0f, Sy, 0.0f, 0.0f, 0.0f, 0.0f, Sz, -1.0f, 0.0f, 0.0f, Pz, 0.0f };

  /* create VIEW MATRIX */
  //float cam_pos[] = { 0.0f, 0.0f, 2.0f }; // don't start at zero, or we will be too close
  //float cam_yaw   = 0.0f;                 // y-rotation in degrees
  //mat4 T          = translate( identity_mat4(), vec3( -cam_pos[0], -cam_pos[1], -cam_pos[2] ) );
  //mat4 R          = rotate_y_deg( identity_mat4(), -cam_yaw );
  //mat4 view_mat   = R * T;
  mat4 view_mat = look_at(cam_pos, target_pos, up);
  mat3 normal_mat = mat3(view_mat.m[0], view_mat.m[1], view_mat.m[2], view_mat.m[4], view_mat.m[5], view_mat.m[6], view_mat.m[8], view_mat.m[9], view_mat.m[10]);

  float packed[9] = { normal_mat.m[0], normal_mat.m[1], normal_mat.m[2],
                        normal_mat.m[3], normal_mat.m[4], normal_mat.m[5],
                        normal_mat.m[6], normal_mat.m[7], normal_mat.m[8]};
  //float packed[9] = { normal_mat.m[0], normal_mat.m[3], normal_mat.m[6],
    //                    normal_mat.m[1], normal_mat.m[4], normal_mat.m[7],
      //                  normal_mat.m[2], normal_mat.m[5], normal_mat.m[8]};

  glUniformMatrix4fv(Projection_loc, 1, 0, proj_mat);
  glUniformMatrix4fv(Modelview_loc, 1, 0, view_mat.m);
  glUniformMatrix3fv(NormalMatrix_loc, 1, 0, packed);
  glUniform3f(LightPosition_loc, 0.25, 0.25, 1);
  glUniform3f(AmbientMaterial_loc, 0.04f, 0.04f, 0.04f);
  glUniform3f(DiffuseMaterial_loc, 0, 0.75, 0.75);
  glUniform1f(TessLevelInner_loc, inner_tess_fac);
  glUniform1f(TessLevelOuter_loc, outer_tess_fac);

  glEnable( GL_CULL_FACE ); // cull face
  //glCullFace( GL_BACK );    // cull back face
  //glFrontFace( GL_CW );     // GL_CCW for counter clock-wise
  // NB. front or back alone didn't work on OSX -- had to use F&B here
  //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  // i'm drawing a base mesh comprised of triangles (3 points per patch)
  glPatchParameteri( GL_PATCH_VERTICES, 3 );
  double previous = glfwGetTime() * 1000;
  while ( !glfwWindowShouldClose( window ) ) {
    double current = glfwGetTime() * 1000;
    const float RadiansPerMicrosecond = 0.05f;
    float Theta = (current - previous) * RadiansPerMicrosecond;
    mat4 R          = rotate_x_deg( identity_mat4(), Theta );
    previous = current;
    _update_fps_counter( window );
    // wipe the drawing surface clear
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glViewport( 0, 0, g_gl_width, g_gl_height );

    glUseProgram( shader_programme );
    view_mat = view_mat * R;
    normal_mat = mat3(view_mat.m[0], view_mat.m[1], view_mat.m[2], view_mat.m[4], view_mat.m[5], view_mat.m[6], view_mat.m[8], view_mat.m[9], view_mat.m[10]);

    float tmp[9] = { normal_mat.m[0], normal_mat.m[1], normal_mat.m[2],
                        normal_mat.m[3], normal_mat.m[4], normal_mat.m[5],
                        normal_mat.m[6], normal_mat.m[7], normal_mat.m[8]};
  //float packed[9] = { normal_mat.m[0], normal_mat.m[3], normal_mat.m[6],
    //                    normal_mat.m[1], normal_mat.m[4], normal_mat.m[7],
      //                  normal_mat.m[2], normal_mat.m[5], normal_mat.m[8]};

    glUniformMatrix4fv(Modelview_loc, 1, 0, view_mat.m);
    glUniformMatrix3fv(NormalMatrix_loc, 1, 0, tmp);
    glBindVertexArray( vao );
    // set patch parameters - I'm drawing a mesh comprising 2 triangles, so '6'
    //glDrawArrays( GL_TRIANGLES, 0, 3 );
    glDrawElements(GL_PATCHES, IndexCount, GL_UNSIGNED_INT, 0);
    // update other events like input handling
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( window, 1 ); }
    // handle key controls for controlling tessellation factors; q,a,w,s
    static bool q_was_down = false;
    static bool a_was_down = false;
    static bool w_was_down = false;
    static bool s_was_down = false;
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_Q ) ) {
      if ( !q_was_down ) {
        inner_tess_fac += 1.0f;
        printf( "inner tess. factor = %.1f\n", inner_tess_fac );
        q_was_down = true;
        glUniform1f( TessLevelInner_loc, inner_tess_fac );
      }
    } else {
      q_was_down = false;
    }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_A ) ) {
      if ( !a_was_down ) {
        inner_tess_fac -= 1.0f;
        printf( "inner tess. factor = %.1f\n", inner_tess_fac );
        a_was_down = true;
        glUniform1f( TessLevelInner_loc, inner_tess_fac );
      }
    } else {
      a_was_down = false;
    }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_W ) ) {
      if ( !w_was_down ) {
        outer_tess_fac += 1.0f;
        printf( "outer tess. factor = %.1f\n", outer_tess_fac );
        w_was_down = true;
        glUniform1f( TessLevelOuter_loc, outer_tess_fac );
      }
    } else {
      w_was_down = false;
    }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_S ) ) {
      if ( !s_was_down ) {
        outer_tess_fac -= 1.0f;
        printf( "outer tess. factor = %.1f\n", outer_tess_fac );
        s_was_down = true;
        glUniform1f( TessLevelOuter_loc, outer_tess_fac );
      }
    } else {
      s_was_down = false;
    }
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers( window );
  }

  // close GL context and any other GLFW resources
  glfwTerminate();

  return 0;
}

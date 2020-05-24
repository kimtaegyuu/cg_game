#include "cgmath.h"         // slee's simple math library
#include "cgut.h"         // slee's OpenGL utility
#include "irrKlang/irrKlang.h"
#pragma comment(lib, "irrKlang.lib")
using namespace irrklang;

//*******************************************************************
// global constants
static const char*   window_name = "cgbase - trackball";
static const char*   vert_shader_path = "../bin/shaders/trackball.vert";
static const char*   frag_shader_path = "../bin/shaders/trackball.frag";
static const char*   skyvert_shader_path = "../bin/shaders/skybox.vert";
static const char*   skyfrag_shader_path = "../bin/shaders/skybox.frag";
static const char*   ball_texture_path;
static const char*   tball_texture_path;
static const char*   map_texture_path;
static const char*   boost_texture_path = "../bin/boost.png";
static const char*   slow_texture_path = "../bin/slow.jpg";
static const char*   opposite_texture_path = "../bin/opposite.png";
static const char*   bgm_path;
static const char*   sound_path;
static const char*   gameover_path = "../bin/sound/game_over.wav";
static const char*   gameclear_path = "../bin/sound/clear.wav";
static const char*   jump_path = "../bin/sound/jump.wav";
static const uint   MAX_CUBES = 100;

//*******************************************************************
// include stb_image with the implementation preprocessor definition
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//*******************************************************************
// common structures
struct camera
{
	vec3   eye = vec3(0, -6, 4);      // position of camera
	vec3   at = vec3(0, 0, 1);      // position where the camera looks at
	vec3   up = vec3(0, 1, 0);
	mat4   view_matrix = mat4::look_at(eye, at, up);

	float   fovy = PI / 4.0f;            // must be in radian
	float   aspect_ratio;            // window_size.x / window_size.y
	float   dnear = 1.0f;
	float   dfar = 1000.0f;
	mat4   projection_matrix;
};

struct sphere
{
	vec3   center = vec3(0.0f, 0.0f, 0.0f);
	vec2   go = vec2(1.0f, 1.0f);
	float   radius = 0.2f;
	float   revolution_speed = 1.0f;
	float   rotation_speed = 1.0f;
	bool die = false;
	int movement_direct = 1;
};

struct cube
{
	vec3   center = vec3(0.0f, 0.0f, 0.0f);
	float   fall_t = 0.0f;
	float   radius = 1.0f;
};

struct item
{
	vec3   center = vec3(0.0f, 0.0f, 0.5f);
	bool exist = false;
	bool isboost = false;
	bool isslow = false;
	bool isopposite = false;
};

//*******************************************************************
// window objects
GLFWwindow*   window = nullptr;
//ivec2      window_size = ivec2(720, 480);   // initial window sizez
ivec2      window_size = ivec2(1440, 960);   // initial window sizez

//*******************************************************************
// OpenGL objects
GLuint   program = 0;   // ID holder for GPU program
GLuint   vertex_buffer_sphere = 0;
GLuint   index_buffer_sphere = 0;
GLuint   vertex_buffer_cube = 0;
GLuint   index_buffer_cube = 0;
GLuint    vertex_buffer_item = 0;
GLuint    index_buffer_item = 0;
GLuint    vao_sphere;
GLuint    vao_cube;
GLuint    vao_item;
// for skybox
GLuint    program_skybox;
GLuint    vao_skybox;
GLuint    vbo_skybox;
GLuint    idc_skybox;
// for texture
GLuint    skybox_texture;
GLuint    texture[6];

//*******************************************************************
// global variables

float sky_vertices[] = {
   -1.0f, 1.0f, -1.0f,		//0
   -1.0f, 1.0f, 1.0f,		//1
   1.0f, 1.0f, 1.0f,		//2
   1.0f, 1.0f, -1.0f,		//3
   -1.0f, -1.0f, -1.0f,		//4
   -1.0f, -1.0f, 1.0f,      //5
   1.0f, -1.0f, 1.0f,		//6
   1.0f, -1.0f, -1.0f,      //7
};

int sky_indices[] = {
   0, 2, 1,
   0, 3, 2,
   5, 6, 4,
   6, 7, 4,
   0, 4, 7,
   0, 7, 3,
   1, 6, 5,
   1, 2, 6,
   2, 7, 6,
   2, 3, 7,
   1, 5, 4,
   1, 4, 0
};
////////////////////////////////////////// texture variables
bool   show_texcoord = false;
static const char* skybox[6];

int    frame = 0;   // index of rendering frames
vec3   direction[3] = { vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f) };
int    path_dir[MAX_CUBES];
float	game_speed = 0.0f;

////////////////////////////////////////////// render() variable
float change_t = 0;
float t = 0;
float movement_x = 0;
float movement_y = 0;
float save_t = 0;
int key_board_press = 0;
int key_board_num = 1;
int ball_move = 0;
int ball_jump = 0;
float jump_t = 0;
int go_jump = 0;
int boost = 0;
int slow = 0;
int opposite = 0;
float boost_t = 0;
float slow_t = 0;
float opposite_t = 0;
mat4 ro;
int cur_pnum = 0;
int dis_pnum = 0;
int app_pnum = 0;

bool keyboard_can = true;  // keyboard() varible

////////////////////////////////////////////// update() variable
bool   turning = false;
bool   first_turn = false;
bool   go_next = false;
int      prev_dir = 1;
int      cur_dir = 1;
int      next_dir = 1;
int      cur_path = 0;
int      next_path = 1;
float   cam_speed = 1.0f;
float   theta = 0.0f;
float   rem = 0.0f;

/////////////////////////////////////////////// render() -> ball out of path
//*******************************************************************
// holder of vertices and indices
std::vector<vertex>   vertex_list_sphere;
std::vector<uint>   index_list_sphere;      // host-side indices
std::vector<vertex>   vertex_list_cube;
std::vector<uint>   index_list_cube;      // host-side indices
std::vector<vertex>   vertex_list_item;
std::vector<uint>   index_list_item;      // host-side indices

std::vector<float>   skyboxVertices;
std::vector<vec3>   skyBoxVertices;

//*******************************************************************
// irrKlang objects
irrklang::ISoundEngine* engine = nullptr;
irrklang::ISoundSource* sound_src = nullptr;
irrklang::ISoundEngine* bgm_engine = nullptr;
irrklang::ISoundSource* bgm_src = nullptr;
irrklang::ISoundEngine* gameover_engine = nullptr;
irrklang::ISoundSource* gameover_src = nullptr;
irrklang::ISoundEngine* gameclear_engine = nullptr;
irrklang::ISoundSource* gameclear_src = nullptr;
irrklang::ISoundEngine* jump_engine = nullptr;
irrklang::ISoundSource* jump_src = nullptr;

//*******************************************************************
// scene objects
camera      cam;
sphere      user;
cube      path[MAX_CUBES];
item      items[MAX_CUBES];

//*******************************************************************
// menu
bool stop_game = false;
int game_state = 1;
int home_menu = 1;
int game_pause_menu = 1;
int game_over_menu = 1;
int game_mode = 1;
bool isgameover = false;
bool isgameclear = false;

//*******************************************************************
// forward declarations for freetype text
void init_text();
void render_text(std::string text, GLint x, GLint y, GLfloat scale, vec4 color);
void init_game();

void change_mode()
{
	if (game_mode == 2) { // hard mode
		skybox[0] = "../bin/bloodvalley/right.png";
		skybox[1] = "../bin/bloodvalley/left.png";
		skybox[2] = "../bin/bloodvalley/back.png";
		skybox[3] = "../bin/bloodvalley/front.png";
		skybox[4] = "../bin/bloodvalley/top.png";
		skybox[5] = "../bin/bloodvalley/bottom.png";
		map_texture_path = "../bin/space.png";
		ball_texture_path = "../bin/wrecking.jpg";
		tball_texture_path = "../bin/twrecking.jpg";
		bgm_path = "../bin/sound/blacktooth.wav";
		sound_path = "../bin/sound/blacktooth.wav";
	}
	else { // easy mode
		skybox[0] = "../bin/deviltooth/right.png";
		skybox[1] = "../bin/deviltooth/left.png";
		skybox[2] = "../bin/deviltooth/back.png";
		skybox[3] = "../bin/deviltooth/front.png";
		skybox[4] = "../bin/deviltooth/top.png";
		skybox[5] = "../bin/deviltooth/bottom.png";
		map_texture_path = "../bin/beige.jpg";
		ball_texture_path = "../bin/BeachBallColor.jpg";
		tball_texture_path = "../bin/BeachBall.jpg";
		bgm_path = "../bin/sound/prelude.wav";
		sound_path = "../bin/sound/beach.wav";
	}
}

void update()
{
	// clear screen (with background color) and clear depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// notify GL that we use our own program and buffers
	glUseProgram(program);

	// update projection matrix
	cam.aspect_ratio = window_size.x / float(window_size.y);
	cam.projection_matrix = mat4::perspective(cam.fovy, cam.aspect_ratio, cam.dnear, cam.dfar);

	// switching view matrix during play
	vec3 z = vec3(0, 0, 1);
	vec3 n = cam.eye - cam.at;
	change_t *= 2.0f;
	cam_speed = 1.0f / change_t;
	float delta_theta = (PI / 2.0f)*(1 / cam_speed);

	if (turning)
	{
		if (cur_dir == 0 && next_dir == 1)
		{ // turn right
			if (first_turn) {
				theta = 3 * PI / 2.0f;
				first_turn = false;
			}
			theta -= delta_theta;
			cam.at.x -= change_t;
			if (cam.at.x <= path[cur_path].center.x)
			{
				rem = path[cur_path].center.x - cam.at.x;
				cam.at.x += rem;
				go_next = true;
			}
			cam.up.x = -cosf((3 * PI / 2.0f) - theta);
			cam.up.y = sinf((3 * PI / 2.0f) - theta);
			n = mat3(mat4::rotate(z, -delta_theta)) * n;
		}
		else if (cur_dir == 1 && next_dir == 0)
		{ // turn left
			if (first_turn) {
				theta = 0.0f;
				first_turn = false;
			}
			theta += delta_theta;
			cam.at.y += change_t;
			if (cam.at.y >= path[cur_path].center.y)
			{
				rem = cam.at.y - path[cur_path].center.y;
				cam.at.y -= rem;
				go_next = true;
			}
			cam.up.x = -sinf(theta);
			cam.up.y = cosf(theta);
			n = mat3(mat4::rotate(z, delta_theta)) * n;
		}
		else if (cur_dir == 1 && next_dir == 2)
		{ // turn right
			if (first_turn) {
				theta = PI;
				first_turn = false;
			}
			theta -= delta_theta;
			cam.at.y += change_t;
			if (cam.at.y >= path[cur_path].center.y)
			{
				rem = cam.at.y - path[cur_path].center.y;
				cam.at.y -= rem;
				go_next = true;
			}
			cam.up.x = sinf(PI - theta);
			cam.up.y = cosf(PI - theta);
			n = mat3(mat4::rotate(z, -delta_theta)) * n;
		}
		else if (cur_dir == 2 && next_dir == 1)
		{ // turn left
			if (first_turn) {
				theta = 3 * PI / 2.0f;
				first_turn = false;
			}
			theta += delta_theta;
			cam.at.x += change_t;
			if (cam.at.x >= path[cur_path].center.x)
			{
				rem = cam.at.x - path[cur_path].center.x;
				cam.at.x -= rem;
				go_next = true;
			}
			cam.up.x = cosf(theta - (3 * PI / 2.0f));
			cam.up.y = sinf(theta - (3 * PI / 2.0f));
			n = mat3(mat4::rotate(z, delta_theta)) * n;
		}

		if (go_next)
		{
			cur_path = (cur_path + 1) % MAX_CUBES;
			next_path = (next_path + 1) % MAX_CUBES;

			prev_dir = path_dir[cur_path - 1];
			cur_dir = path_dir[cur_path];
			next_dir = path_dir[(cur_path + 1) % MAX_CUBES];

			if (cur_dir != next_dir) first_turn = true;
			else turning = false;

			go_next = false;
		}
		if (cur_dir == 0) cam.at.x -= rem;
		else if (cur_dir == 1) cam.at.y += rem;
		else if (cur_dir == 2) cam.at.x += rem;
		rem = 0;
	}
	else
	{   // cur_path: destination
		if (cur_dir == 0) // go left (x--)
		{
			cam.at.x -= change_t;
			if (cam.at.x <= path[cur_path].center.x)
			{
				rem = path[cur_path].center.x - cam.at.x;
				cam.at.x += rem;
				go_next = true;
			}
		}
		else if (cur_dir == 1) // go forward (y++)
		{
			cam.at.y += change_t;
			if (cam.at.y >= path[cur_path].center.y)
			{
				rem = cam.at.y - path[cur_path].center.y;
				cam.at.y -= rem;
				go_next = true;
			}
		}
		else if (cur_dir == 2) // go right (x++)
		{
			cam.at.x += change_t;
			if (cam.at.x >= path[cur_path].center.x)
			{
				rem = cam.at.x - path[cur_path].center.x;
				cam.at.x -= rem;
				go_next = true;
			}
		}

		if (go_next)
		{
			cur_path = (cur_path + 1) % MAX_CUBES;
			next_path = (next_path + 1) % MAX_CUBES;

			prev_dir = path_dir[cur_path - 1];
			cur_dir = path_dir[cur_path];
			next_dir = path_dir[(cur_path + 1) % MAX_CUBES];

			if (cur_dir != next_dir)
			{
				turning = true;
				first_turn = true;
			}

			go_next = false;
		}

		if (cur_dir == 0) cam.at.x -= rem;
		else if (cur_dir == 1) cam.at.y += rem;
		else if (cur_dir == 2) cam.at.x += rem;
		rem = 0;
	}

	cam.eye = cam.at + n;
	cam.view_matrix = mat4::look_at(cam.eye, cam.at, cam.up);

	// update uniform variables in vertex/fragment shaders
	GLint uloc;
	uloc = glGetUniformLocation(program, "view_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.view_matrix);
	uloc = glGetUniformLocation(program, "projection_matrix");   if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);

}

void render_skybox()
{
	glDepthFunc(GL_LEQUAL);
	glUseProgram(program_skybox);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_skybox);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idc_skybox);

	mat4 view = cam.view_matrix;
	view[3] = 0;
	view[7] = 0;
	view[11] = 0;

	GLint uloc;
	uloc = glGetUniformLocation(program_skybox, "view");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, view);
	uloc = glGetUniformLocation(program_skybox, "projection");   if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, cam.projection_matrix);

	glBindVertexArray(vao_skybox);
	glActiveTexture(GL_TEXTURE0);                        // select the texture slot to bind
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glUniform1i(glGetUniformLocation(program_skybox, "skybox"), 0);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDepthFunc(GL_LESS);
}

void render()
{
	glBindVertexArray(vao_sphere);
	if (vertex_buffer_sphere)   glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_sphere);
	if (index_buffer_sphere)   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_sphere);

	// bind vertex attributes to your shader program
	const char*   vertex_attrib[] = { "position", "normal", "texcoord" };
	size_t      attrib_size[] = { sizeof(vertex().pos), sizeof(vertex().norm), sizeof(vertex().tex) };
	for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
	{
		GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
	}

	if (opposite == 1)
	{
		if (key_board_num == 0)			key_board_num = 2;
		else if (key_board_num == 2)	key_board_num = 0;
		opposite_t -= change_t;
		if (opposite_t <= 0)
		{
			opposite_t = 0;
			opposite = 0;
		}
	}
	if (boost == 1)
	{
		boost_t -= change_t;
		change_t *= 2;
		if (boost_t <= 0)
		{
			boost_t = 0;
			boost = 0;
		}
	}
	if (slow == 1)
	{
		slow_t -= change_t;
		change_t /= 10;
		if (slow_t <= 0)
		{
			slow_t = 0;
			slow = 0;
		}
	}

	t += 8.0f*change_t;
	vec3 center = user.center;
	float r = user.radius;

	mat4 model_matrix
	{
	   1, 0, 0, 0,
	   0, 1, 0, 0,
	   0, 0, 1, 0,
	   0, 0, 0, 1
	};
	//////////////////////////////////////*ball's rotation*
	mat4 trans =
	{
	   1,0,0,-center.at(0),
	   0,1,0,-center.at(1),
	   0,0,1,-center.at(2),
	   0,0,0,1
	};
	mat4 trans2 =
	{
	   1,0,0,center.at(0),
	   0,1,0,center.at(1),
	   0,0,1,center.at(2),
	   0,0,0,1
	};

	///////////////////////////////////////
	 ////ball's movement


	if (key_board_press == 0)
	{
		user.center[1] += 1.0f*change_t;
		ro =
		{
		 1,0,0,0,
		   0,cosf(t),sinf(t), 0,
		   0,-sinf(t),cosf(t), 0,
		   0, 0, 0, 1
		};
	}
	else if (key_board_press == 1)
	{
		if (user.movement_direct == 0)
		{
			if (key_board_num == 0)
			{
				ball_move = 1;
				user.movement_direct = 3;
				key_board_press = 2;
			}
			else if (key_board_num == 2)
			{
				ball_move = 2;
				user.movement_direct = 1;
				key_board_press = 2;
			}
		}
		else if (user.movement_direct == 1)
		{
			if (key_board_num == 0)
			{
				ball_move = 3;
				user.movement_direct = 0;
				key_board_press = 2;
			}
			else if (key_board_num == 2)
			{
				ball_move = 4;
				user.movement_direct = 2;
				key_board_press = 2;
			}
		}
		else if (user.movement_direct == 2)
		{
			if (key_board_num == 0)
			{
				ball_move = 5;
				user.movement_direct = 1;
				key_board_press = 2;
			}
			else if (key_board_num == 2)
			{
				ball_move = 6;
				user.movement_direct = 3;
				key_board_press = 2;
			}
		}
		else if (user.movement_direct == 3)
		{
			if (key_board_num == 0)
			{
				ball_move = 7;
				user.movement_direct = 2;
				key_board_press = 2;
			}
			else if (key_board_num == 2)
			{
				ball_move = 8;
				user.movement_direct = 0;
				key_board_press = 2;
			}
		}
	}
	else
	{
		if (ball_move == 1 || ball_move == 6)
		{
			user.center[1] -= 1.0f*change_t;
			ro =
			{
			 1,0,0,0,
			   0,cosf(t),-sinf(t), 0,
			   0,sinf(t),cosf(t), 0,
			   0, 0, 0, 1
			};
		}
		else if (ball_move == 2 || ball_move == 5)
		{
			user.center[1] += 1.0f*change_t;
			ro =
			{
			 1,0,0,0,
			   0,cosf(t),sinf(t), 0,
			   0,-sinf(t),cosf(t), 0,
			   0, 0, 0, 1
			};
		}
		else if (ball_move == 4 || ball_move == 7)
		{
			user.center[0] += 1.0f*change_t;
			ro =
			{
			 cosf(t),0,sinf(t),0,
			   0,1,0, 0,
			   -sinf(t),0,cosf(t), 0,
			   0, 0, 0, 1
			};
		}
		else if (ball_move == 3 || ball_move == 8)
		{
			user.center[0] -= 1.0f*change_t;
			ro =
			{
			 cosf(t),0,-sinf(t),0,
			   0,1,0, 0,
			   sinf(t),0,cosf(t), 0,
			   0, 0, 0, 1
			};

		}
	}

	if (ball_jump == 1 && go_jump == 0)
	{
		jump_t = t;
		go_jump = 1;
		ball_jump = 0;
	}
	if (go_jump == 1)
	{
		if (0.4f*(t - jump_t) - 0.04f*(t - jump_t)*(t - jump_t) >= 0)
		{
			user.center[2] = (0.4f*(t - jump_t) - 0.04f*(t - jump_t)*(t - jump_t));
			user.center[2] += user.radius + path[0].radius * (1 / (3.0f*float(sqrt(3))));
		}
		else
		{
			go_jump = 0;
			ball_jump = 0;
		}
	}

	/////////////////////////////////////////////////////////////circle out of path
	if (cam.at.x > path[cur_pnum].center.x + 0.5f || cam.at.x<path[cur_pnum].center.x - 0.5f || cam.at.y > path[cur_pnum].center.y + 0.5f || cam.at.y < path[cur_pnum].center.y - 0.5f)
	{ cur_pnum++; }
	if (cur_pnum - 1 >= 0) dis_pnum = cur_pnum - 1;
	app_pnum = cur_pnum + 3;
	int check_ball = 0;

	if (cur_pnum == MAX_CUBES - 1) {
		if (!isgameclear)
		{
			stop_game = true;
			game_state = 5;
			engine->setAllSoundsPaused();
			bgm_engine->setAllSoundsPaused();
			gameclear_engine->play2D(gameclear_src, false);
			isgameclear = true;
		}
	}

	if (user.die == false)
	{
		if (dis_pnum >= 0)
		{
			for (int i = dis_pnum; i <= app_pnum; i++)
			{
				if (user.center.x > path[i].center.x + 0.5f || user.center.x<path[i].center.x - 0.5f || user.center.y > path[i].center.y + 0.5f || user.center.y < path[i].center.y - 0.5f)
				{
					check_ball++;
				}
			}
			if (app_pnum - dis_pnum + 1 == check_ball)
			{
				user.die = true;
				keyboard_can = false;
			}
		}
		else
		{
			for (int i = 0; i <= app_pnum; i++)
			{
				if (user.center.x > path[i].center.x + 0.5f || user.center.x<path[i].center.x - 0.5f || user.center.y > path[i].center.y + 0.5f || user.center.y < path[i].center.y - 0.5f)
				{
					check_ball++;
				}
			}
			if (app_pnum + 1 == check_ball)
			{
				user.die = true;
				keyboard_can = false;
			}
		}
	}
	else
	{
		user.center[2] -= change_t;
		go_jump = 2;
	}

	if (user.center[2] <= -1.5f)
	{
		if (!isgameover)
		{
			bgm_engine->setAllSoundsPaused();
			gameover_engine->play2D(gameover_src, false);
			stop_game = true;
			game_state = 4;
			isgameover = true;
		}
	}

	///////////////////////////////////////////////////////////////
	mat4 rotate_matrix = trans2 * ro*trans;

	GLint uloc;
	uloc = glGetUniformLocation(program, "center");         if (uloc > -1) glUniform3fv(uloc, 1, center);
	uloc = glGetUniformLocation(program, "radius");         if (uloc > -1) glUniform1f(uloc, r);
	uloc = glGetUniformLocation(program, "rotate_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, rotate_matrix);
	uloc = glGetUniformLocation(program, "model_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

	glBindVertexArray(vao_sphere);
	glActiveTexture(GL_TEXTURE1);                        // select the texture slot to bind
	if (opposite) glBindTexture(GL_TEXTURE_2D, texture[2]);
	else glBindTexture(GL_TEXTURE_2D, texture[1]);
	glUniform1i(glGetUniformLocation(program, "ballTEX"), 1);
	glUniform1i(glGetUniformLocation(program, "isBall"), true);

	glDrawElements(GL_TRIANGLES, index_list_sphere.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindVertexArray(vao_cube);
	if (vertex_buffer_cube)   glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_cube);
	if (index_buffer_cube)   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_cube);

	// bind vertex attributes to your shader program
	for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
	{
		GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
	}

	for (uint k = 0; k < (uint)dis_pnum; k++)
	{
		path[k].center.z -= 2.0f*change_t;
		if (path[k].center.z <= -3.0f)
			continue;
		center = path[k].center;
		r = path[k].radius;

		mat4 rotate_matrix2
		{
		   1, 0, 0, 0,
		   0, 1, 0, 0,
		   0, 0, 1, 0,
		   0, 0, 0, 1
		};

		uloc = glGetUniformLocation(program, "center");         if (uloc > -1) glUniform3fv(uloc, 1, center);
		uloc = glGetUniformLocation(program, "radius");         if (uloc > -1) glUniform1f(uloc, r);
		uloc = glGetUniformLocation(program, "rotate_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, rotate_matrix2);
		uloc = glGetUniformLocation(program, "model_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

		glBindVertexArray(vao_cube);
		glActiveTexture(GL_TEXTURE0);                        // select the texture slot to bind
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glUniform1i(glGetUniformLocation(program, "TEX"), 0);    // GL_TEXTURE0
		glUniform1i(glGetUniformLocation(program, "isBall"), false);

		glDrawElements(GL_TRIANGLES, index_list_cube.size(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	for (uint k = dis_pnum; k < (uint)(app_pnum); k++)
	{
		center = path[k].center;
		r = path[k].radius;

		mat4 rotate_matrix2
		{
		   1, 0, 0, 0,
		   0, 1, 0, 0,
		   0, 0, 1, 0,
		   0, 0, 0, 1
		};

		glBindVertexArray(vao_cube);
		uloc = glGetUniformLocation(program, "center");         if (uloc > -1) glUniform3fv(uloc, 1, center);
		uloc = glGetUniformLocation(program, "radius");         if (uloc > -1) glUniform1f(uloc, r);
		uloc = glGetUniformLocation(program, "rotate_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, rotate_matrix2);
		uloc = glGetUniformLocation(program, "model_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

		glActiveTexture(GL_TEXTURE0);                        // select the texture slot to bind
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glUniform1i(glGetUniformLocation(program, "TEX"), 0);    // GL_TEXTURE0
		glUniform1i(glGetUniformLocation(program, "isBall"), false);

		glDrawElements(GL_TRIANGLES, index_list_cube.size(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glBindVertexArray(vao_item);
	if (vertex_buffer_item)   glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_item);
	if (index_buffer_item)   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_item);

	// bind vertex attributes to your shader program
	for (size_t k = 0, kn = std::extent<decltype(vertex_attrib)>::value, byte_offset = 0; k < kn; k++, byte_offset += attrib_size[k - 1])
	{
		GLuint loc = glGetAttribLocation(program, vertex_attrib[k]); if (loc >= kn) continue;
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, attrib_size[k] / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)byte_offset);
	}

	for (uint k = dis_pnum; k < (uint)(app_pnum); k++)
	{
		center = items[k].center;
		r = 0.5f;

		if (items[k].exist)
		{
			vec3 a = user.center;
			vec3 b = items[k].center;
			if ((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + (a.z - b.z)*(a.z - b.z) <= (121.0f / 900.0f))
			{
				if (items[k].isopposite)
				{
					opposite = 1;
					opposite_t = 3.0f;
					items[k].exist = false;
				}
				if (items[k].isboost)
				{
					boost = 1;
					boost_t = 0.5f;
					slow = 0;
					slow_t = 0;
					items[k].exist = false;
				}
				if (items[k].isslow)
				{
					slow = 1;
					slow_t = 0.5f;
					boost = 0;
					boost_t = 0;
					items[k].exist = false;
				}
			}
		}
		else
			continue;

		mat4 trans =
		{
		   1,0,0,-items[k].center.at(0),
		   0,1,0,-items[k].center.at(1),
		   0,0,1,-items[k].center.at(2),
		   0,0,0,1
		};
		mat4 trans2 =
		{
		   1,0,0,items[k].center.at(0),
		   0,1,0,items[k].center.at(1),
		   0,0,1,items[k].center.at(2),
		   0,0,0,1
		};
		mat4 ro
		{
			 cosf(t / 2),-sinf(t / 2),0, 0,
			 sinf(t / 2),cosf(t / 2),0, 0,
			 0,0,1, 0,
			 0, 0, 0, 1
		};
		mat4 rotate_matrix3 = trans2 * ro*trans;

		glBindVertexArray(vao_item);
		uloc = glGetUniformLocation(program, "center");         if (uloc > -1) glUniform3fv(uloc, 1, center);
		uloc = glGetUniformLocation(program, "radius");         if (uloc > -1) glUniform1f(uloc, r);
		uloc = glGetUniformLocation(program, "rotate_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, rotate_matrix3);
		uloc = glGetUniformLocation(program, "model_matrix");         if (uloc > -1) glUniformMatrix4fv(uloc, 1, GL_TRUE, model_matrix);

		if (items[k].exist) {
			glActiveTexture(GL_TEXTURE0);
			if (items[k].isboost) glBindTexture(GL_TEXTURE_2D, texture[3]);
			if (items[k].isslow) glBindTexture(GL_TEXTURE_2D, texture[4]);
			if (items[k].isopposite) glBindTexture(GL_TEXTURE_2D, texture[5]);
			glUniform1i(glGetUniformLocation(program, "TEX"), 0);    // GL_TEXTURE0
			glUniform1i(glGetUniformLocation(program, "isBall"), false);
			glDrawElements(GL_TRIANGLES, index_list_item.size(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
	//************************SKYBOX**************************//
	render_skybox();

	//************************TEXT**************************//
	GLuint x = GLuint(window_size.x / 2.0f);
	GLuint y = GLuint(window_size.y / 2.0f);
	float s = window_size.x / 1440.0f;
	float title_s = 2.0f * s;

	if (game_state == 1) {
		render_text("WELCOME", GLuint(x - 200 * s), GLuint(y - 100 * s), title_s, vec4(1, 1, 1, 1));
		if (home_menu == 1) {
			render_text("start EASY mode", GLuint(x - 220 * s), y, s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
			render_text("start HARD mode", GLuint(x - 220 * s), GLuint(y + 70 * s), s, vec4(1, 1, 1, 1));
		}
		else {
			render_text("start EASY mode", GLuint(x - 220 * s), y, s, vec4(1, 1, 1, 1));
			render_text("start HARD mode", GLuint(x - 220 * s), GLuint(y + 70 * s), s, vec4(234.0f / 255.0f, 103.0f / 255.0f, 118.0f / 255.0f, 1));
		}
		render_text("press 'ENTER' to start", GLuint(x + 100 * s), GLuint(2 * y - 100 * s), 0.8f*s, vec4(0, 0, 0, 1));
		render_text("press 'Q' to exit game", GLuint(x + 100 * s), GLuint(2 * y - 50 * s), 0.8f*s, vec4(0, 0, 0, 1));
	}
	else if (game_state == 2) {
		render_text("PAUSE", GLuint(x - 130 * s), GLuint(y - 100 * s), 2.0f, vec4(1, 1, 1, 1));
		if (game_pause_menu == 1) {
			render_text("resume", GLuint(x - 100 * s), y, s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
			render_text("restart", GLuint(x - 100 * s), GLuint(y + 70 * s), s, vec4(1, 1, 1, 1));
			render_text("home", GLuint(x - 100 * s), GLuint(y + 140 * s), s, vec4(1, 1, 1, 1));
		}
		else if (game_pause_menu == 2) {
			render_text("resume", GLuint(x - 100 * s), y, s, vec4(1, 1, 1, 1));
			render_text("restart", GLuint(x - 100 * s), GLuint(y + 70 * s), s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
			render_text("home", GLuint(x - 100 * s), GLuint(y + 140 * s), s, vec4(1, 1, 1, 1));
		}
		else {
			render_text("resume", GLuint(x - 100 * s), y, s, vec4(1, 1, 1, 1));
			render_text("restart", GLuint(x - 100 * s), GLuint(y + 70 * s), s, vec4(1, 1, 1, 1));
			render_text("home", GLuint(x - 100 * s), GLuint(y + 140 * s), s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
		}
	}
	else if (game_state == 4) {
		render_text("GAME OVER", GLuint(x - 250 * s), GLuint(y - s * 100), title_s, vec4(1, 1, 1, 1));
		if (game_over_menu == 1) {
			render_text("restart", GLuint(x - 100 * s), y, s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
			render_text("home", GLuint(x - 100 * s), GLuint(y + 70 * s), s, vec4(1, 1, 1, 1));
		}
		else if (game_over_menu == 2) {
			render_text("restart", GLuint(x - 100 * s), y, s, vec4(1, 1, 1, 1));
			render_text("home", GLuint(x - 100 * s), GLuint(y + 70 * s), s, vec4(112.0f / 255.0f, 207.0f / 255.0f, 1, 1));
		}
	}
	else if (game_state == 5) {
		render_text("GAME CLEAR", GLuint(x - 250 * s), y, title_s, vec4(1, 1, 1, 1));
		render_text("press 'ENTER' to go home", GLuint(x + 100 * s), GLuint(2 * y - 50 * s), 0.8f*s, vec4(0, 0, 0, 1));
	}

	// swap front and back buffers, and display to screen
	glfwSwapBuffers(window);
}

void reshape(GLFWwindow* window, int width, int height)
{
	// set current viewport in pixels (win_x, win_y, win_width, win_height)
	// viewport: the window area that are affected by rendering 
	window_size = ivec2(width, height);
	glViewport(0, 0, width, height);
}

void print_help()
{
	printf("[help]\n");
	printf("--------------------GAME INFO--------------------\n");
	printf("\tpress 'Q'        = TERMINATE the program\n");
	printf("\tpress 'ESC'      = PAUSE the game\n");
	printf("\tpress 'F1'/'h'   = see HELP\n");
	printf("\tpress '<-'('->') = turn the ball LEFT(RIGHT)\n");
	printf("\tpress 'SPACE'    = JUMP\n");
	printf("-------------------------------------------------\n\n");
	printf("--------------------ITEM INFO--------------------\n");
	printf("\tBLUE   = BOOST\n");
	printf("\tRED    = SLOW\n");
	printf("\tPURPLE = OPPOSITE DIRECTION\n");
	printf("-------------------------------------------------\n");
	printf("\n");
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	void update_vertex_buffer_sphere();   // forward declaration
	void update_circle_vertices_sphere();   // forward declaration
	void update_vertex_buffer_cube();   // forward declaration
	void update_circle_vertices_cube();   // forward declaration
	void update_vertex_buffer_item();   // forward declaration
	void update_circle_vertices_item();   // forward declaration

	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_Q)   glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_ESCAPE)
		{
			if (game_state == 3)
			{
				if (game_mode == 1) engine->setAllSoundsPaused();
				game_state = 2;
				stop_game = true;
			}
		}
		else if (key == GLFW_KEY_H || key == GLFW_KEY_F1)   print_help();
		else if (key == GLFW_KEY_LEFT && game_state == 3) {
			key_board_num = 0;
			if (keyboard_can)
				key_board_press = 1;
		}
		else if (key == GLFW_KEY_RIGHT && game_state == 3) {
			key_board_num = 2;
			if (keyboard_can)
				key_board_press = 1;
		}
		else if (key == GLFW_KEY_DOWN)
		{
			if (game_state == 1)
			{
				if (home_menu == 1) {
					home_menu = 2;
					game_mode = 2;
					init_game();
				}
				else {
					home_menu = 1;
					game_mode = 1;
					init_game();
				}
			}
			else if (game_state == 2)
			{
				if (game_pause_menu == 1) game_pause_menu = 2;
				else if (game_pause_menu == 2) game_pause_menu = 3;
				else game_pause_menu = 1;
			}
			else if (game_state == 4)
			{
				if (game_over_menu == 1) game_over_menu = 2;
				else game_over_menu = 1;
			}
		}
		else if (key == GLFW_KEY_UP)
		{
			if (game_state == 1) // home
			{
				if (home_menu == 1) {
					home_menu = 2;
					game_mode = 2;
					init_game();
				}
				else {
					home_menu = 1;
					game_mode = 1;
					init_game();
				}
			}
			else if (game_state == 2)
			{
				if (game_pause_menu == 1) game_pause_menu = 3;
				else if (game_pause_menu == 2) game_pause_menu = 1;
				else game_pause_menu = 2;
			}
			else if (game_state == 4)
			{
				if (game_over_menu == 1) game_over_menu = 2;
				else game_over_menu = 1;
			}
		}
		else if (key == GLFW_KEY_ENTER)
		{
			if (game_state == 1) //home
			{
				if (home_menu == 1)
				{
					game_speed = 1.7f;
					game_state = 3;
					bgm_engine->play2D(bgm_src, true);
				}
				else
				{
					engine->setAllSoundsPaused();
					engine->play2D(sound_src, true);
					game_speed = 2.5f;
					game_state = 3;
					home_menu = 1;
				}
			}
			else if (game_state == 2) //menu (esc)
			{
				if (game_mode == 1) engine->play2D(sound_src, true);
				if (game_pause_menu == 1)
				{ // resume
					stop_game = false;
					game_state = 3;
				}
				else if (game_pause_menu == 2)
				{ // restart
					bgm_engine->setAllSoundsPaused();
					float speed = game_speed;
					init_game();
					game_state = 3;
					game_speed = speed;
					game_pause_menu = 1;
					if (game_mode == 1) bgm_engine->play2D(bgm_src, true);
				}
				else
				{ // go home
					game_state = 1;
					game_pause_menu = 1;
					game_mode = 1;
					init_game();
				}
			}
			else if (game_state == 4) // dead
			{
				if (game_over_menu == 1)
				{
					float speed = game_speed;
					init_game();
					game_state = 3;
					game_speed = speed;
					if (game_mode == 1) bgm_engine->play2D(bgm_src, true);
				}
				else
				{
					game_state = 1;
					game_over_menu = 1;
					game_mode = 1;
					init_game();
				}
			}
			else if (game_state == 5) {
				game_state = 1;
				game_mode = 1;
				init_game();

			}
		}
		else if (key == GLFW_KEY_SPACE && game_state == 3)
		{
			ball_jump = 1;
			if (!go_jump)
				jump_engine->play2D(jump_src, false);
		}
	}
}

void mouse(GLFWwindow* window, int button, int action, int mods) {}
void motion(GLFWwindow* window, double x, double y) {}

void update_vertex_buffer_sphere()
{
	// clear and create new buffers
	if (vertex_buffer_sphere)   glDeleteBuffers(1, &vertex_buffer_sphere);   vertex_buffer_sphere = 0;
	if (index_buffer_sphere)   glDeleteBuffers(1, &index_buffer_sphere);   index_buffer_sphere = 0;

	// check exceptions
	if (vertex_list_sphere.empty()) { printf("[error] vertex_list_sphere is empty.\n"); return; }

	// create buffers
	index_list_sphere.clear();

	uint a = 0, b = 0, c = 0, d = 0;
	for (uint latitude = 0; latitude < 36; latitude++)
	{
		a = latitude * 73;
		b = a + 1;
		c = a + 73;
		d = c + 1;
		for (uint longitude = 0; longitude < 72; longitude++)
		{
			if (latitude > 0)
			{
				index_list_sphere.push_back(a);
				index_list_sphere.push_back(c);
				index_list_sphere.push_back(b);
			}
			if (latitude < 36)
			{
				index_list_sphere.push_back(b);
				index_list_sphere.push_back(c);
				index_list_sphere.push_back(d);
			}
			a++;
			b++;
			c++;
			d++;
		}
	}

	// generation of vertex buffer: use vertex_list_sphere as it is
	glGenVertexArrays(1, &vao_sphere);
	glGenBuffers(1, &vertex_buffer_sphere);
	glBindVertexArray(vao_sphere);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_sphere);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vertex_list_sphere.size(), &vertex_list_sphere[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &index_buffer_sphere);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_sphere);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*index_list_sphere.size(), &index_list_sphere[0], GL_STATIC_DRAW);
}

void update_vertex_buffer_cube()
{
	// clear and create new buffers
	if (vertex_buffer_cube)   glDeleteBuffers(1, &vertex_buffer_cube);   vertex_buffer_cube = 0;
	if (index_buffer_cube)   glDeleteBuffers(1, &index_buffer_cube);   index_buffer_cube = 0;

	// check exceptions
	if (vertex_list_cube.empty()) { printf("[error] vertex_list is empty.\n"); return; }

	// create buffers

	index_list_cube.clear();

	index_list_cube.push_back(0);
	index_list_cube.push_back(1);
	index_list_cube.push_back(2);

	index_list_cube.push_back(0);
	index_list_cube.push_back(2);
	index_list_cube.push_back(3);

	index_list_cube.push_back(5);
	index_list_cube.push_back(4);
	index_list_cube.push_back(6);

	index_list_cube.push_back(6);
	index_list_cube.push_back(4);
	index_list_cube.push_back(7);

	index_list_cube.push_back(0);
	index_list_cube.push_back(7);
	index_list_cube.push_back(4);

	index_list_cube.push_back(0);
	index_list_cube.push_back(3);
	index_list_cube.push_back(7);

	index_list_cube.push_back(1);
	index_list_cube.push_back(5);
	index_list_cube.push_back(6);

	index_list_cube.push_back(1);
	index_list_cube.push_back(6);
	index_list_cube.push_back(2);

	index_list_cube.push_back(2);
	index_list_cube.push_back(6);
	index_list_cube.push_back(7);

	index_list_cube.push_back(2);
	index_list_cube.push_back(7);
	index_list_cube.push_back(3);

	index_list_cube.push_back(1);
	index_list_cube.push_back(4);
	index_list_cube.push_back(5);

	index_list_cube.push_back(1);
	index_list_cube.push_back(0);
	index_list_cube.push_back(4);

	// generation of vertex buffer: use vertex_list as it is
	glGenVertexArrays(1, &vao_cube);
	glGenBuffers(1, &vertex_buffer_cube);
	glBindVertexArray(vao_cube);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_cube);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vertex_list_cube.size(), &vertex_list_cube[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &index_buffer_cube);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_cube);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*index_list_cube.size(), &index_list_cube[0], GL_STATIC_DRAW);
}

void update_vertex_buffer_item()
{
	// clear and create new buffers
	if (vertex_buffer_item)   glDeleteBuffers(1, &vertex_buffer_item);   vertex_buffer_item = 0;
	if (index_buffer_item)   glDeleteBuffers(1, &index_buffer_item);   index_buffer_item = 0;

	// check exceptions
	if (vertex_list_item.empty()) { printf("[error] vertex_list is empty.\n"); return; }

	// create buffers
	index_list_item.clear();

	index_list_item.push_back(0);
	index_list_item.push_back(1);
	index_list_item.push_back(2);

	index_list_item.push_back(0);
	index_list_item.push_back(2);
	index_list_item.push_back(3);

	index_list_item.push_back(5);
	index_list_item.push_back(4);
	index_list_item.push_back(6);

	index_list_item.push_back(6);
	index_list_item.push_back(4);
	index_list_item.push_back(7);

	index_list_item.push_back(0);
	index_list_item.push_back(7);
	index_list_item.push_back(4);

	index_list_item.push_back(0);
	index_list_item.push_back(3);
	index_list_item.push_back(7);

	index_list_item.push_back(1);
	index_list_item.push_back(5);
	index_list_item.push_back(6);

	index_list_item.push_back(1);
	index_list_item.push_back(6);
	index_list_item.push_back(2);

	index_list_item.push_back(2);
	index_list_item.push_back(6);
	index_list_item.push_back(7);

	index_list_item.push_back(2);
	index_list_item.push_back(7);
	index_list_item.push_back(3);

	index_list_item.push_back(1);
	index_list_item.push_back(4);
	index_list_item.push_back(5);

	index_list_item.push_back(1);
	index_list_item.push_back(0);
	index_list_item.push_back(4);

	// generation of vertex buffer: use vertex_list as it is
	glGenVertexArrays(1, &vao_item);
	glGenBuffers(1, &vertex_buffer_item);
	glBindVertexArray(vao_item);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_item);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*vertex_list_item.size(), &vertex_list_item[0], GL_STATIC_DRAW);

	// geneation of index buffer
	glGenBuffers(1, &index_buffer_item);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_item);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)*index_list_item.size(), &index_list_item[0], GL_STATIC_DRAW);
}

void update_vertex_buffer_skybox()
{
	glGenVertexArrays(1, &vao_skybox);
	glGenBuffers(1, &vbo_skybox);
	glGenBuffers(1, &idc_skybox);

	glBindVertexArray(vao_skybox);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_skybox);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sky_vertices), sky_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idc_skybox);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sky_indices), sky_indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void update_sphere_vertices()
{
	vertex_list_sphere.clear();

	// define the position of four corner vertices
	for (uint latitude = 0; latitude <= 36; latitude++)
	{
		for (uint longitude = 0; longitude <= 72; longitude++)
		{
			float phi = PI * 2.0f / 72 * float(longitude);
			float theta = PI * 1.0f / 36 * float(latitude);
			float sp = sin(phi), cp = cos(phi);
			float st = sin(theta), ct = cos(theta);
			vertex_list_sphere.push_back({ vec3(1.0f*st*cp, 1.0f*st*sp, 1.0f*ct), vec3(st*cp, st*sp, ct), vec2(phi / (PI*2.0f), 1 - theta / float(PI)) });
		}
	}
}

void update_cube_vertices()
{
	vertex_list_cube.clear();

	vertex_list_cube.push_back({ vec3(-1 / float(sqrt(3)), 1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))), vec3(-1 / float(sqrt(3)), 1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))),  vec2(0.0f, 1.0f) });
	vertex_list_cube.push_back({ vec3(-1 / float(sqrt(3)), 1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))), vec3(-1 / float(sqrt(3)), 1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))),  vec2(0.0f, 0.0f) });
	vertex_list_cube.push_back({ vec3(1 / float(sqrt(3)), 1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))), vec3(1 / float(sqrt(3)), 1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))),  vec2(1.0f, 0.0f) });
	vertex_list_cube.push_back({ vec3(1 / float(sqrt(3)), 1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))), vec3(1 / float(sqrt(3)), 1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_cube.push_back({ vec3(-1 / float(sqrt(3)), -1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))), vec3(-1 / float(sqrt(3)), -1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))),  vec2(0.0f, 0.0f) });
	vertex_list_cube.push_back({ vec3(-1 / float(sqrt(3)), -1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))), vec3(-1 / float(sqrt(3)), -1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))),  vec2(0.0f, 1.0f) });
	vertex_list_cube.push_back({ vec3(1 / float(sqrt(3)), -1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))), vec3(1 / float(sqrt(3)), -1 / float(sqrt(3)), 1 / (3.0f*float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_cube.push_back({ vec3(1 / float(sqrt(3)), -1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))), vec3(1 / float(sqrt(3)), -1 / float(sqrt(3)), -1 / (3.0f*float(sqrt(3)))),  vec2(1.0f, 0.0f) });
}

void update_item_vertices()
{
	vertex_list_item.clear();

	vertex_list_item.push_back({ vec3(-1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))), vec3(-1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(-1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))), vec3(-1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))), vec3(1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))), vec3(1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(-1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))), vec3(-1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(-1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))), vec3(-1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))), vec3(1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), 1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
	vertex_list_item.push_back({ vec3(1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))), vec3(1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3))), -1 / (3.0f* float(sqrt(3)))),  vec2(1.0f, 1.0f) });
}

float get_x(float limit) {
	return (rand() % 200 - 100) / (99.0f / float(limit));
}

float get_y(float x, float limit) {
	int s = (rand() % 2);
	if (s) return float(sqrt(limit*limit - (x*x)));
	else return -float(sqrt(limit*limit - (x*x)));
}

void init_path()
{
	srand(uint(time(NULL)));
	float plen = 1.0f;
	path[0].center = vec3(0.0f, 0.0f, 0.0f);
	path[1].center = vec3(0.0f, plen, 0.0f);
	path[2].center = vec3(0.0f, plen*2.0f, 0.0f);
	path[3].center = vec3(0.0f, plen*3.0f, 0.0f);
	path_dir[0] = path_dir[1] = path_dir[2] = path_dir[3] = 1;
	int prev_dir = 1;
	for (uint i = 3; i < MAX_CUBES - 1; i++)
	{
		vec3 prev_center = path[i].center;
		int cur_dir = rand() % 3;
		if ((prev_dir == 0 && cur_dir == 2) || (prev_dir == 2 && cur_dir == 0))
			cur_dir = 1;

		path[i + 1].center = prev_center + plen * direction[cur_dir];
		path_dir[i + 1] = cur_dir;
		prev_dir = cur_dir;
	}
	for (int i = 0; i < MAX_CUBES - 1; i++)
	{
		items[i].center.x = path[i].center.x;
		items[i].center.y = path[i].center.y;
	}
	int item_num[15] = { 5,14,21,30,42,49,56,61,67,71,78,83,88,91,95 };
	for (int i = 0; i < 15; i++)
	{
		items[item_num[i]].exist = true;
		items[item_num[i]].isboost = false;
		items[item_num[i]].isslow = false;
		items[item_num[i]].isopposite = false;
		int a = rand() % 3;
		if (a == 0) items[item_num[i]].isslow = true;
		else if (a == 1) items[item_num[i]].isopposite = true;
		else items[item_num[i]].isboost = true;
	}
}

void loadCubemap()
{
	//glGenTextures(1, &skybox_texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);

	int width, height, comp;
	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char *data = stbi_load(skybox[i], &width, &height, &comp, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void loadTexture(int idx, const char* path)
{
	// load and flip an image
	int width, height, comp = 3;
	unsigned char* image0 = stbi_load(path, &width, &height, &comp, 3); if (comp == 1) comp = 3; /* convert 1-channel to 3-channel image */
	int stride0 = width * comp, stride1 = (stride0 + 3)&(~3);   // 4-byte aligned stride
	unsigned char* image = (unsigned char*)malloc(sizeof(unsigned char)*stride1*height);
	for (int y = 0; y < height; y++) memcpy(image + (height - 1 - y)*stride1, image0 + y * stride0, stride0); // vertical flip

	glBindTexture(GL_TEXTURE_2D, texture[idx]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8 /* GL_RGB for legacy GL */, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	// allocate and create mipmap
	int mip_levels = miplevels(window_size.x, window_size.y);
	for (int k = 1, w = width >> 1, h = height >> 1; k < mip_levels; k++, w = max(1, w >> 1), h = max(1, h >> 1))
		glTexImage2D(GL_TEXTURE_2D, k, GL_RGB8 /* GL_RGB for legacy GL */, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glGenerateMipmap(GL_TEXTURE_2D);

	// configure texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// release the new image
	free(image);
}

bool user_init()
{
	// init GL states
	glLineWidth(1.0f);
	glClearColor(39 / 255.0f, 40 / 255.0f, 34 / 255.0f, 1.0f);   // set clear color
	glEnable(GL_CULL_FACE);                        // turn on backface culling
	glEnable(GL_DEPTH_TEST);                        // turn on depth tests
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	// define the position of four corner vertices
	update_sphere_vertices();
	update_cube_vertices();
	update_item_vertices();

	// create vertex buffer; called again when index buffering mode is toggled
	update_vertex_buffer_sphere();
	update_vertex_buffer_cube();
	update_vertex_buffer_item();

	user.center.at(2) = user.radius + path[0].radius * (1 / (3.0f*float(sqrt(3))));

	init_path();
	change_mode();

	// create engine
	if (!engine) engine = irrklang::createIrrKlangDevice();
	if (!bgm_engine) bgm_engine = irrklang::createIrrKlangDevice();
	if (!engine || !bgm_engine) return false;
	//add sound source from the sound file
	engine->removeAllSoundSources();
	bgm_engine->removeAllSoundSources();
	sound_src = engine->addSoundSourceFromFile(sound_path);
	bgm_src = bgm_engine->addSoundSourceFromFile(bgm_path);
	// play sound file
	engine->play2D(sound_src, true);

	// create textures
	glGenTextures(6, texture);
	loadTexture(0, map_texture_path);
	loadTexture(1, ball_texture_path);
	loadTexture(2, tball_texture_path);
	loadTexture(3, boost_texture_path);
	loadTexture(4, slow_texture_path);
	loadTexture(5, opposite_texture_path);

	update_vertex_buffer_skybox();
	glGenTextures(1, &skybox_texture);
	loadCubemap();
	glUseProgram(program_skybox);
	glUniform1i(glGetUniformLocation(program_skybox, "skybox"), 0);

	return true;
}

void init_game()
{
	cam.eye = vec3(0, -6, 4);      // position of camera
	cam.at = vec3(0, 0, 1);      // position where the camera looks at
	cam.up = vec3(0, 1, 0);

	user.center = vec3(0.0f, 0.0f, 0.0f);
	user.go = vec2(1.0f, 1.0f);
	user.die = false;
	user.movement_direct = 1;

	for (int i = 0; i < MAX_CUBES; i++)
	{
		path[i].center = vec3(0.0f, 0.0f, 0.0f);
		path[i].fall_t = 0.0f;
	}
	stop_game = false;

	frame = 0;   // index of rendering frames
	change_t = 0;
	t = 0;
	game_speed = 0;
	key_board_press = 0;
	key_board_num = 1;
	ball_move = 0;
	ball_jump = 0;
	jump_t = 0;
	go_jump = 0;
	boost = 0;
	slow = 0;
	opposite = 0;
	boost_t = 0;
	slow_t = 0;
	opposite_t = 0;
	////////////////////////////////////////////// render() variable
	keyboard_can = true;  // keyboard() varible
	turning = false;
	first_turn = false;
	go_next = false;
	prev_dir = 1;
	cur_dir = 1;
	next_dir = 1;
	cur_path = 0;
	next_path = 1;
	cam_speed = 1.0f;
	theta = 0.0f;
	rem = 0.0f;
	////////////////////////////////////////////// update() variable
	cur_pnum = 0;
	dis_pnum = 0;
	app_pnum = 0;

	isgameover = false;
	isgameclear = false;

	user_init();
}

void user_finalize()
{
	// close the engine
	engine->drop();
	bgm_engine->drop();
	gameover_engine->drop();
	gameclear_engine->drop();
	jump_engine->drop();
}

int main(int argc, char* argv[])
{
	// initialization
	if (!glfwInit()) { printf("[error] failed in glfwInit()\n"); return 1; }

	// create window and initialize OpenGL extensions
	if (!(window = cg_create_window(window_name, window_size.x, window_size.y))) { glfwTerminate(); return 1; }
	if (!cg_init_extensions(window)) { glfwTerminate(); return 1; }   // version and extensions

	// initializations and validations
	if (!(program = cg_create_program(vert_shader_path, frag_shader_path))) { glfwTerminate(); return 1; }   // create and compile shaders/program
	if (!(program_skybox = cg_create_program(skyvert_shader_path, skyfrag_shader_path))) { glfwTerminate(); return 1; }   // create and compile shaders/program
	if (!user_init()) { printf("Failed to user_init()\n"); glfwTerminate(); return 1; }               // user initialization

	// register event callbacks
	glfwSetWindowSizeCallback(window, reshape);   // callback for window resizing events
	glfwSetKeyCallback(window, keyboard);         // callback for keyboard events
	glfwSetMouseButtonCallback(window, mouse);   // callback for mouse click inputs
	glfwSetCursorPosCallback(window, motion);      // callback for mouse movement

	print_help();

	init_text();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gameover_engine = irrklang::createIrrKlangDevice();
	gameover_src = gameover_engine->addSoundSourceFromFile(gameover_path);
	gameclear_engine = irrklang::createIrrKlangDevice();
	gameclear_src = gameclear_engine->addSoundSourceFromFile(gameclear_path);
	jump_engine = irrklang::createIrrKlangDevice();
	jump_src = jump_engine->addSoundSourceFromFile(jump_path);

	// enters rendering/event loop
	float t1 = float(glfwGetTime())*0.4f;
	for (frame = 0; !glfwWindowShouldClose(window); frame++)
	{
		float t2 = float(glfwGetTime())*0.4f;
		change_t = t2 - t1;
		change_t *= game_speed;
		if (stop_game) change_t = 0;
		t1 = t2;
		glfwPollEvents();   // polling and processing of events
		update();         // per-frame update
		render();         // per-frame render
	}

	// normal termination
	user_finalize();
	cg_destroy_window(window);

	return 0;
}
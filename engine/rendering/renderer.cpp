#include "renderer.h"
#include "../../config.h"
#include "../../tinyutils.h"

#include <stdlib.h>
#include <time.h>

#include "../gui/imgui.h"

void draw_pixel(int x, int y, color_t color, unsigned char* pixels)
{
    pixels[(y * 300 + x) * 4 + 0] = color.r;
    pixels[(y * 300 + x) * 4 + 1] = color.g;
    pixels[(y * 300 + x) * 4 + 2] = color.b;
    pixels[(y * 300 + x) * 4 + 3] = color.a;
}

float vec2_t::dot(vec2_t v2)
{
     return x * v2.x + y * v2.y;
}

vec2_t vec2_t::normalize_vector()
{
    float length = sqrt(dot(*this));

    return { x / length, y / length};
}


vec2_t vec2_t::operator+(vec2_t &v1)
{
    vec2_t res = {this->x + v1.x, this->y + v1.y};
    return res;
}

vec2_t vec2_t::operator+=(vec2_t &v1)
{
    return {this->x + v1.x, this->y + v1.y};
}

vec2_t vec2_t::operator * (float scalar)
{
    return {this->x * scalar, this->y * scalar};
}

vec2_t vec2_t::operator-(vec2_t &v1)
{
    return {this->x - v1.x, this->y - v1.y};
}

void particle_t::initiate_particle(vec2_t p)
{
	for (int i = 0; i < instanciated_particles_number; i++)
	{
        offset_intsances[i] = {0.0f, i * 0.01f};
        particle_delay[i] = 0.4f * i;
        particle_gravity[i] = 0.002f;
    }
}

void particle_t::update_particle(vec2_t p, float delta_time)
{
    float g = 0.002f;
    for (int i = 0; i < instanciated_particles_number; i++)
	{
		offset_intsances[i].y += particle_gravity[i] * emission_speed;
		offset_intsances[i].x += (velocity.x + ((float)rand()/RAND_MAX)/100) * emission_speed;
        if (particle_delay[i] < 0.0f)
        {
            offset_intsances[i] = {p.x, p.y};
            particle_delay[i] = 0.4f * (instanciated_particles_number - i);
            particle_gravity[i] = 0.002f;
        }
        else
        {
            particle_delay[i] -= 0.1f;
        }
        particle_gravity[i] -= 0.0001f;
	}
}

shader_t shader(const char* path, shader_type type)
{
    shader_t shader_s;
    FILE *shader_file = fopen(path, "rb");  // stupid windows

    if (!shader_file)
    {
        printf("Shader file not found: %s\n", path);
        perror("Error");
        shader_s.shader_id = 0;
        return shader_s;
    }

    fseek(shader_file, 0, SEEK_END);
    long file_size = ftell(shader_file);
    char *shader_code = (char*)malloc(file_size + 1);
    fseek(shader_file, 0, SEEK_SET);
    fread(shader_code, sizeof(char), file_size, shader_file);
    shader_code[file_size] = '\0';

    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &shader_code, NULL);
    glCompileShader(shader);

    // Check shader compilation
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
    
    if(shaderCompiled != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        
        if(logLength > 0)
        {
            char *infoLog = (char*)malloc(logLength);
            glGetShaderInfoLog(shader, logLength, NULL, infoLog);
            printf("ERROR: Failed to compile shader: %s\n%s\n", path, infoLog);
            free(infoLog);
        }
        else
        {
            printf("ERROR: Failed to compile shader: %s (no error log available)\n", path);
        }
        exit(1);
    }

    free(shader_code);
    shader_s.shader_id = shader;
    fclose(shader_file);
    return shader_s;
}

void info_log_shader(GLuint id)
{
    GLint success = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
        
        if(logLength > 0)
        {
            char *infoLog = (char*)malloc(logLength);
            glGetProgramInfoLog(id, logLength, NULL, infoLog);
            printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
            free(infoLog);
        }
    }
}

window_canvas_t window_quad(const char* fragment, const char* vertex, unsigned char* data, int w, int h, vec2_t width, vec2_t height)
{
    // [MOUNIR]: Opengl by default works with ndc so playing on the range [-1, 1] would always be mapped to the window borders
    window_canvas_t canvas;

    canvas.opacity = 1;
 
    float x = (float)SCREEN_WIDTH/2 - (width.x + (width.y - width.x)/2);
    float y = (float)SCREEN_HEIGHT/2 - (height.x + (height.y - height.x)/2);

    if (width.x < SCREEN_WIDTH/2)
    {
        width.x += x;
        width.y += x;
    }
    else
    {
        width.x -= x;
        width.y -= x;
    }

    if (height.x < SCREEN_HEIGHT/2)
    {
        height.x += y;
        height.y += y;
    }
    else
    {
        height.x -= y;
        height.y -= y;
    }


    vec2_t  ndc_convert_width = {width.x/(float)(SCREEN_WIDTH/2) - 1, width.y/(float)(SCREEN_WIDTH/2) - 1};
    vec2_t  ndc_convert_height = {height.x/(float)(SCREEN_HEIGHT/2) - 1, height.y/(float)(SCREEN_HEIGHT/2) - 1};

    canvas.vertecies[0] = ndc_convert_width.y;
    canvas.vertecies[1] = ndc_convert_height.y;
    canvas.vertecies[2] = 0;    // z always 0

    canvas.vertecies[3] = ndc_convert_width.y;
    canvas.vertecies[4] = ndc_convert_height.x;
    canvas.vertecies[5] = 0;    // z always 0

    canvas.vertecies[6] = ndc_convert_width.x;
    canvas.vertecies[7] = ndc_convert_height.x;
    canvas.vertecies[8] = 0;    // z always 0

    canvas.vertecies[9]  = ndc_convert_width.x;
    canvas.vertecies[10] = ndc_convert_height.y;
    canvas.vertecies[11] = 0;    // z always 0

    // 
    canvas.uvs[0] = 1;
    canvas.uvs[1] = 0;

    canvas.uvs[2] = 1;
    canvas.uvs[3] = 1;

    canvas.uvs[4] = 0;
    canvas.uvs[5] = 1;

    canvas.uvs[6] = 0;
    canvas.uvs[7] = 0;

    canvas.indices[0] = 0;
    canvas.indices[1] = 1;
    canvas.indices[2] = 3;

    canvas.indices[3] = 1;
    canvas.indices[4] = 2;
    canvas.indices[5] = 3; 

    canvas.vertex_shader = shader(vertex, VERTEX).shader_id;
    canvas.fragment_shader = shader(fragment, FRAGEMENT).shader_id;

    canvas.stride = 3;

    canvas.shader_program = glCreateProgram();

    // vertecies and indecies drawing
    glGenVertexArrays(1, &canvas.vertex_array);
    glGenBuffers(1, &canvas.vertex_buffer);
    glGenBuffers(1, &canvas.indecies_buffer);

    glBindVertexArray(canvas.vertex_array);

    glBindBuffer(GL_ARRAY_BUFFER, canvas.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(canvas.vertecies), canvas.vertecies, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canvas.indecies_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(canvas.indices), canvas.indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // textures generation
    glGenTextures(1, &canvas.texture);  
    glBindTexture(GL_TEXTURE_2D, canvas.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenBuffers(1, &canvas.uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, canvas.uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(canvas.uvs), canvas.uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, canvas.uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // shaders linking
    glAttachShader(canvas.shader_program, canvas.vertex_shader);
    glAttachShader(canvas.shader_program, canvas.fragment_shader);
    glLinkProgram(canvas.shader_program);

    info_log_shader(canvas.shader_program);

    // clean up
    glDeleteShader(canvas.fragment_shader); 
    glDeleteShader(canvas.vertex_shader);    

    return canvas;
}


window_canvas_t window_quad_multipass(window_canvas_t canvas, const char* post_process)
{
    // [MOUNIR]: setuping the render pass here
    glGenFramebuffers(1, &canvas.frame_buffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, canvas.frame_buffer_id);
    glViewport(0, 0, canvas.width, canvas.height);
    glGenTextures(1, &canvas.frame_buffer_texture);  
    glBindTexture(GL_TEXTURE_2D, canvas.frame_buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.frame_buffer_texture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Error creating framebuffer\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    canvas.post_process_shader = shader(post_process, FRAGEMENT).shader_id;

    canvas.post_process_shader_program = glCreateProgram();
    glAttachShader(canvas.post_process_shader_program, canvas.vertex_shader);
    glAttachShader(canvas.post_process_shader_program, canvas.post_process_shader);
    glLinkProgram(canvas.post_process_shader_program);

    info_log_shader(canvas.post_process_shader_program);

    return canvas;
}

void render_quad_screen(window_canvas_t canvas_quad, bool instanced, int count, void* data)
{
    glUseProgram(canvas_quad.shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                            canvas_quad.texturee.texture_width, 
                            canvas_quad.texturee.texture_height, 0, 
                            GL_RGBA, GL_UNSIGNED_BYTE, canvas_quad.texturee.texture_data);
    GLint player_position = glGetUniformLocation(canvas_quad.shader_program, "player_position");
    GLint rotation_degree = glGetUniformLocation(canvas_quad.shader_program, "rotation_degree");
    GLint uv_side = glGetUniformLocation(canvas_quad.shader_program, "uv_side");
    GLint mouse_position = glGetUniformLocation(canvas_quad.shader_program, "mouse_position");
    GLint global_scale = glGetUniformLocation(canvas_quad.shader_program, "global_scale");
    GLint relative_scale = glGetUniformLocation(canvas_quad.shader_program, "relative_scale");
    GLint opacity = glGetUniformLocation(canvas_quad.shader_program, "opacity");
    GLint instanced_r = glGetUniformLocation(canvas_quad.shader_program, "instanced");
    GLint resolution = glGetUniformLocation(canvas_quad.shader_program, "resolution");

    glUniform2f(player_position, canvas_quad.position.x, canvas_quad.position.y);
    glUniform2f(uv_side, canvas_quad.uv_side.x, canvas_quad.uv_side.y);
    glUniform2f(mouse_position, (float)canvas_quad.mousex/((float)SCREEN_WIDTH/2),
                               (float)canvas_quad.mousey/((float)SCREEN_HEIGHT/2));
    glUniform1f(global_scale, canvas_quad.global_scale);
    glUniform1f(relative_scale, canvas_quad.scale);
    glUniform1f(rotation_degree, 0);
    glUniform1f(opacity, canvas_quad.opacity);
    glUniform1f(instanced_r, instanced);
    glUniform2f(resolution, SCREEN_WIDTH, SCREEN_HEIGHT);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(canvas_quad.vertex_array);    
	glBindBuffer(GL_ARRAY_BUFFER, canvas_quad.vertex_buffer);
    if (!instanced)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);    
	else
    {
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);    
    }
}

void render_quad_post_processing(window_canvas_t canvas_quad)
{
    glUseProgram(canvas_quad.post_process_shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.frame_buffer_texture);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


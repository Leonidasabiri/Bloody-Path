#include "renderer.h"
#include "../../config.h"
#include "../../tinyutils.h"

void draw_pixel(int x, int y, color_t color, unsigned char* pixels)
{
    pixels[(y * 300 + x) * 4 + 0] = color.r;
    pixels[(y * 300 + x) * 4 + 1] = color.g;
    pixels[(y * 300 + x) * 4 + 2] = color.b;
    pixels[(y * 300 + x) * 4 + 3] = color.a;
}

void renderMap(unsigned char* pixels, Map* map) {
    if (!map) return;

    int tileWidth =  SCREEN_WIDTH/map->width;
    int tileHeight =  SCREEN_WIDTH/map->height;

    for (int y = 0; y < map->height; ++y) {
        for (int x = 0; x < map->width; ++x) {
           
        }
    }
}

void renderPlayer(unsigned char* pixels, Player* player, unsigned char* frame) {
    if (!player) 
        return;
    vec2_t playerpos = { player->x, player->x + player->width} ;
    vec2_t player_dimension = {player->y, player->y + player->height };
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

    canvas.vertecies[0] = width.y/(float)(SCREEN_WIDTH/2) - 1;
    canvas.vertecies[1] = height.y/(float)(SCREEN_HEIGHT/2) - 1;
    canvas.vertecies[2] = 0;    // z always 0

    canvas.vertecies[3] = width.y/(float)(SCREEN_WIDTH/2) - 1;
    canvas.vertecies[4] = height.x/(float)(SCREEN_HEIGHT/2) - 1;
    canvas.vertecies[5] = 0;    // z always 0

    canvas.vertecies[6] = width.x/(float)(SCREEN_WIDTH/2) - 1;
    canvas.vertecies[7] = height.x/(float)(SCREEN_HEIGHT/2) - 1;
    canvas.vertecies[8] = 0;    // z always 0

    canvas.vertecies[9]  = width.x/(float)(SCREEN_WIDTH/2) - 1;
    canvas.vertecies[10] = height.y/(float)(SCREEN_HEIGHT/2) - 1;
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
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
    // glDeleteShader(canvas.fragment_shader);    

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

void render_quad_screen(window_canvas_t canvas_quad)
{
    glUseProgram(canvas_quad.shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                            canvas_quad.texturee.texture_width, 
                            canvas_quad.texturee.texture_height, 0, 
                            GL_RGBA, GL_UNSIGNED_BYTE, canvas_quad.texturee.texture_data);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void render_quad_post_processing(window_canvas_t canvas_quad)
{
    glUseProgram(canvas_quad.post_process_shader_program);
    glBindTexture(GL_TEXTURE_2D, canvas_quad.frame_buffer_texture);
    glBindVertexArray(canvas_quad.vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


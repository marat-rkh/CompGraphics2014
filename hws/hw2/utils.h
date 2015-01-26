#ifndef UTILS_H
#define UTILS_H

#include <exception>
#include <string>
#include "common.h"
#include <FreeImage.h>
#include "tiny_obj_loader.h"

class msg_exception : public std::exception {
    std::string what_;
public:
    msg_exception(std::string const& msg): what_(msg) {}

    const char* what() const throw() { return what_.c_str(); }
};

struct texture_data {
    BYTE* data_ptr;
    int width;
    int height;
    int format;
};

struct vertex_attr {
    const char* name;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride; // offset to next value
    GLvoid* pointer;
};

class utils {
public:
    static void debug(std::string const& msg) {
        cout << "->|DEBUG: " << msg << endl;
    }

    static texture_data load_texture(char const* img_path) {
        FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
        FIBITMAP* dib(0);
        // Check the file signature and deduce its format
        fif = FreeImage_GetFileType(img_path, 0);
        // If still unknown, try to guess the file format from the file extension
        if(fif == FIF_UNKNOWN) {
            fif = FreeImage_GetFIFFromFilename(img_path);
        }
        // If still unkown, return failure
        if(fif == FIF_UNKNOWN) {
            throw msg_exception("load_texture(): image format is unknown");
        }
        // Check if the plugin has reading capabilities and load the file
        if(FreeImage_FIFSupportsReading(fif)) {
            dib = FreeImage_Load(fif, img_path);
        }
        if(!dib) {
            throw msg_exception("load_texture(): no reading capabilities for file");
        }

        texture_data tex_data;
        tex_data.data_ptr = FreeImage_GetBits(dib);

        tex_data.width = FreeImage_GetWidth(dib);
        tex_data.height = FreeImage_GetHeight(dib);
        unsigned int pixel_size = FreeImage_GetBPP(dib);

        // If somehow one of these failed (they shouldn't), return failure
        if(tex_data.data_ptr == NULL || tex_data.width == 0 || tex_data.height == 0) {
            throw msg_exception("load_texture(): failed to load the texture");
        }

        tex_data.format = pixel_size == 24 ? GL_BGR : pixel_size == 8 ? GL_LUMINANCE : 0;
        //        int iInternalFormat = iBPP == 24 ? GL_RGB : GL_DEPTH_COMPONENT;
        return tex_data;
    }

    static void read_obj_file(char const* obj_file_path,
                              vector<GLfloat>& vertices,
                              vector<GLfloat>& tex_mapping,
                              vector<GLfloat>& normals)
    {
        vector<tinyobj::shape_t> shapes;
        vector<tinyobj::material_t> materials;
        string err = tinyobj::LoadObj(shapes, materials, obj_file_path);
        if (!err.empty()) {
            throw msg_exception("read_obj_file(): " + err);
        }
        if(shapes.size() < 1) {
            throw msg_exception("read_obj_file(): input file contains no shapes");
        }
        for (size_t i = 0; i < shapes[0].mesh.indices.size(); ++i) {
            int ind = shapes[0].mesh.indices[i];
            vertices.push_back(shapes[0].mesh.positions[3 * ind + 0]);
            vertices.push_back(shapes[0].mesh.positions[3 * ind + 1]);
            vertices.push_back(shapes[0].mesh.positions[3 * ind + 2]);

            tex_mapping.push_back(shapes[0].mesh.texcoords[2 * ind + 0]);
            tex_mapping.push_back(shapes[0].mesh.texcoords[2 * ind + 1]);

            normals.push_back(shapes[0].mesh.normals[3 * ind + 0]);
            normals.push_back(shapes[0].mesh.normals[3 * ind + 1]);
            normals.push_back(shapes[0].mesh.normals[3 * ind + 2]);
        }
    }

    static void set_vertex_attr_ptr(GLuint program, vertex_attr const& attr) {
        GLuint location = glGetAttribLocation(program, attr.name);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, attr.size, attr.type,
                              attr.normalized, attr.stride, attr.pointer);
    }
};

#endif // UTILS_H

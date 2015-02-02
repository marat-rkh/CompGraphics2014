#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <exception>
#include <FreeImage.h>
#include "libs/tiny_obj_loader.h"

class msg_exception : public std::exception {
    std::string what_;
public:
    msg_exception(std::string const& msg): what_(msg) {}

    const char* what() const throw() { return what_.c_str(); }
};

struct draw_data {
    vector<GLfloat> vertices;
    vector<GLfloat> tex_mapping;
    vector<GLfloat> normals;
    vector<GLfloat> tangents;
    vector<GLfloat> bitangents;

    size_t vertices_num() const { return vertices.size() / 3; }

    void* vertices_data() { return vertices.data(); }
    size_t vertices_data_size() const { return vertices.size() * sizeof(GLfloat); }

    void* tex_mapping_data() { return tex_mapping.data(); }
    size_t tex_mapping_data_size() const { return tex_mapping.size() * sizeof(GLfloat); }

    void* normals_data() { return normals.data(); }
    size_t normals_data_size() const { return normals.size() * sizeof(GLfloat); }

    void* tangents_data() { return tangents.data(); }
    size_t tangents_data_size() const { return tangents.size() * sizeof(GLfloat); }

    void* bitangents_data() { return bitangents.data(); }
    size_t bitangents_data_size() const { return bitangents.size() * sizeof(GLfloat); }
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

    static void read_obj_file(char const* obj_file_path, draw_data& out) {
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
            out.vertices.push_back(shapes[0].mesh.positions[3 * ind + 0]);
            out.vertices.push_back(shapes[0].mesh.positions[3 * ind + 1]);
            out.vertices.push_back(shapes[0].mesh.positions[3 * ind + 2]);

            out.tex_mapping.push_back(shapes[0].mesh.texcoords[2 * ind + 0]);
            out.tex_mapping.push_back(shapes[0].mesh.texcoords[2 * ind + 1]);

            out.normals.push_back(shapes[0].mesh.normals[3 * ind + 0]);
            out.normals.push_back(shapes[0].mesh.normals[3 * ind + 1]);
            out.normals.push_back(shapes[0].mesh.normals[3 * ind + 2]);
        }
    }

    static void compute_tangent_basis(draw_data& data) {
        vector<vec3> verts = to_vec3_vector(data.vertices);
        vector<vec2> tex = to_vec2_vector(data.tex_mapping);
        vector<vec3> norms = to_vec3_vector(data.normals);
        vector<vec3> tans;
        vector<vec3> bitans;
        compute_tangent_basis_impl(verts, tex, norms, tans, bitans);
        to_floats_vector(tans, data.tangents);
        to_floats_vector(bitans, data.bitangents);
    }

private:
    static vector<vec2> to_vec2_vector(vector<GLfloat>& in) {
        if(in.size() % 2 != 0) {
            throw msg_exception("to_vec2_vector() wrong input size");
        }
        vector<vec2> out;
        for(size_t i = 0; i != in.size(); ++i) {
            out.push_back(vec2(in[2 * i + 0], in[2 * i + 1]));
        }
        return out;
    }

    static vector<vec3> to_vec3_vector(vector<GLfloat>& in) {
        if(in.size() % 3 != 0) {
            throw msg_exception("to_vec3_vector() wrong input size: " + std::to_string(in.size()));
        }
        vector<vec3> out;
        for(size_t i = 0; i != in.size(); ++i) {
            out.push_back(vec3(in[3 * i + 0], in[3 * i + 1], in[3 * i + 2]));
        }
        return out;
    }

    static void to_floats_vector(vector<vec3>& in, vector<GLfloat>& out) {
        for(size_t i = 0; i != in.size(); ++i) {
            out.push_back(in[i][0]);
            out.push_back(in[i][1]);
            out.push_back(in[i][2]);
        }
    }

    static void compute_tangent_basis_impl(
            vector<vec3> & vertices,
            vector<vec2> & uvs,
            vector<vec3> & normals,
            vector<vec3> & tangents,
            vector<vec3> & bitangents)
    {
        for (unsigned int i = 0; i < vertices.size(); i += 3 ) {
            vec3 & v0 = vertices[i + 0];
            vec3 & v1 = vertices[i + 1];
            vec3 & v2 = vertices[i + 2];
            vec2 & uv0 = uvs[i + 0];
            vec2 & uv1 = uvs[i + 1];
            vec2 & uv2 = uvs[i + 2];

            vec3 deltaPos1 = v1 - v0;
            vec3 deltaPos2 = v2 - v0;
            vec2 deltaUV1 = uv1 - uv0;
            vec2 deltaUV2 = uv2 - uv0;
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y) * r;
            vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x) * r;

            tangents.push_back(tangent);
            tangents.push_back(tangent);
            tangents.push_back(tangent);

            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
        }
    }
};

#endif // UTILS_H

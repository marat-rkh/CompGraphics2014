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

    static void computeTangentBasisAdapter(
            std::vector<GLfloat> & vertices,
            std::vector<GLfloat> & uvs,
            std::vector<GLfloat> & normals,
            // outputs
            std::vector<GLfloat> & tangents,
            std::vector<GLfloat> & bitangents)
    {
        vector<vec3> verts = to_vec3_vector(vertices);
        vector<vec2> uvs_ = to_vec2_vector(uvs);
        vector<vec3> norms = to_vec3_vector(normals);
        vector<vec3> tans;
        vector<vec3> bitans;
        debug("computation started");
        computeTangentBasis(verts, uvs_, norms, tans, bitans);
        debug("computation finished");
        to_floats_vector(tans, tangents);
        to_floats_vector(bitans, bitangents);
    }

    static void computeTangentBasis(
            // inputs
            std::vector<glm::vec3> & vertices,
            std::vector<glm::vec2> & uvs,
            std::vector<glm::vec3> & normals,
            // outputs
            std::vector<glm::vec3> & tangents,
            std::vector<glm::vec3> & bitangents)
    {
        for (unsigned int i = 0; i < vertices.size(); i += 3 ) {
            // Shortcuts for vertices
            glm::vec3 & v0 = vertices[i + 0];
            glm::vec3 & v1 = vertices[i + 1];
            glm::vec3 & v2 = vertices[i + 2];
            // Shortcuts for UVs
            glm::vec2 & uv0 = uvs[i + 0];
            glm::vec2 & uv1 = uvs[i + 1];
            glm::vec2 & uv2 = uvs[i + 2];
            // Edges of the triangle : postion delta
            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;
            // UV delta
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;
            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x) * r;
            // Set the same tangent for all three vertices of the triangle.
            // They will be merged later, in vboindexer.cpp
            tangents.push_back(tangent);
            tangents.push_back(tangent);
            tangents.push_back(tangent);
            // Same thing for binormals
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
            bitangents.push_back(bitangent);
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

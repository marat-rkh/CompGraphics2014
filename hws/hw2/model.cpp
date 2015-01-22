#include "model.h"
#include <fstream>

using std::fstream;
using std::getline;

typedef vector<int> vint;

static string const VERTEX_HEADER  = "v";
static string const TEXTURE_HEADER = "vt";
static string const NORMAL_HEADER  = "vn";
static string const FACE_HEADER    = "f";

void Model::load(string const& path) {
    ifstream file(path.c_str());
    if (file.fail()) {
        std::cout << "fail to open file" << std::endl;
        return;
    }

    vvec3 temp_vertices;
    vvec3 temp_normals;
    vvec2 temp_textures;

    vint vertexIndices, textureIndices, normalIndices;

    while(!file.eof()) {
        string line;
        getline(file, line);
        if (line.length() == 0) {
            continue;
        }
        if (strncmp(line.c_str(), TEXTURE_HEADER.c_str(), TEXTURE_HEADER.length()) == 0) {
            stringstream in(line.substr(TEXTURE_HEADER.length()));
            temp_textures.push_back(read_vertex2(in));
        }
        else if (strncmp(line.c_str(), NORMAL_HEADER.c_str(), NORMAL_HEADER.length()) == 0) {
            stringstream in(line.substr(NORMAL_HEADER.length()));
            temp_normals.push_back(read_vertex3(in));
        }
        else if (strncmp(line.c_str(), VERTEX_HEADER.c_str(), VERTEX_HEADER.length()) == 0) {
            stringstream in(line.substr(VERTEX_HEADER.length()));
            temp_vertices.push_back(read_vertex3(in));
        }
        else if (strncmp(line.c_str(), FACE_HEADER.c_str(), FACE_HEADER.length()) == 0) {
            stringstream in(line.substr(FACE_HEADER.length()));
            for (int i = 0; i < 3; ++i) {
                vec3 f = read_vertex3(in);
                vertexIndices.push_back(int(f[0]));
                textureIndices.push_back(int(f[1]));
                normalIndices.push_back(int(f[2]));
            }
        }
    }

    for (size_t i = 0; i < vertexIndices.size(); ++i) {
        vertices.push_back(temp_vertices[--vertexIndices[i]]);
        textures.push_back(temp_textures[--textureIndices[i]]);
        normals.push_back(temp_normals[--normalIndices[i]]);
    }
}

size_t Model::vertices_count() const {
    return vertices.size();
}

vec2 Model::read_vertex2(stringstream &in) {
    float u, v;
    in >> u; in.get(); in >> v;
    return vec2(u, v);
}

vec3 Model::read_vertex3(stringstream &in) {
    float x, y, z;
    in >> x; in.get(); in >> y; in.get(); in >> z;
    return vec3(x, y, z);
}

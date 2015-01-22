#ifndef MODEL_H
#define MODEL_H

#include "common.h"
#include <sstream>

using std::stringstream;

typedef vector<vec2> vvec2;
typedef vector<vec3> vvec3;
typedef vector<vec4> vvec4;
typedef vector<mat3> vmat3;

struct Model {
    vvec3 vertices;
    vvec2 textures;
    vvec3 normals;

    Model() {}

    void load(string const& path);
    size_t vertices_count() const;

private:
    vec2 read_vertex2(stringstream & in);
    vec3 read_vertex3(stringstream & in);
};

#endif // MODEL_H

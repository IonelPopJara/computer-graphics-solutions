#include "embree.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void check(bool condition, const char* message)
{
    if(!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool close(float a, float b)
{
    return std::abs(a - b) < 0.0001f;
}
}

int main()
{
    using namespace glm;
    using namespace pathtracer;

    // Keep the CPU-only fixture alive for the scene's lifetime. Model's destructor
    // deletes OpenGL buffers, which requires a context unavailable in this test.
    auto* model = new labhelper::Model();
    model->m_name = "test triangles";
    model->m_positions = {vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
                          vec3(2, 0, 0), vec3(3, 0, 0), vec3(2, 1, 0)};
    model->m_normals.assign(6, vec3(0, 0, 1));
    model->m_texture_coordinates = {vec2(0), vec2(1, 0), vec2(0, 1),
                                    vec2(0), vec2(1, 0), vec2(0, 1)};
    model->m_materials.resize(2);
    model->m_meshes.resize(2);
    for(unsigned int index = 0; index < 2; ++index)
    {
        auto& mesh = model->m_meshes[index];
        mesh.m_start_index = index * 3;
        mesh.m_number_of_vertices = 3;
        mesh.m_material_idx = index;
    }

    reinitScene();
    addModel(model, translate(mat4(1), vec3(0, 0, 2)));
    buildBVH();
    Ray ray(vec3(2.25f, 0.25f, 0), vec3(0, 0, 1));
    check(intersect(ray), "Expected a triangle hit");
    check(close(ray.tfar, 2), "Incorrect hit distance");
    check(ray.primID == 0, "Incorrect primitive ID");
    check(close(ray.u, 0.25f) && close(ray.v, 0.25f), "Incorrect barycentrics");
    Intersection hit = getIntersection(ray);
    check(hit.material == &model->m_materials[1], "Incorrect mesh/material mapping");
    check(close(hit.position.z, 2), "Incorrect hit position");
    check(close(hit.uv.x, 0.25f) && close(hit.uv.y, 0.25f), "Incorrect UV interpolation");
    check(close(hit.geometry_normal.z, 1) && close(hit.shading_normal.z, 1),
          "Incorrect normals");

    // The normal used by the integrator's ray bias must point outside the
    // surface. An inward bias makes the triangle occlude its own illumination.
    Ray outward(hit.position + 0.0001f * hit.geometry_normal, vec3(0, 0, 1));
    check(!occluded(outward), "Surface incorrectly shadows itself after normal bias");
    check(!intersect(outward), "Secondary ray incorrectly re-enters its source surface");

    Ray shadow(vec3(0.25f, 0.25f, 0), vec3(0, 0, 1), 0, 3);
    check(occluded(shadow), "Expected shadow occlusion");
    shadow.tfar = 1;
    check(!occluded(shadow), "Occlusion exceeded the ray segment");
    check(!intersect(shadow), "Intersection exceeded the ray segment");
    shadow.tfar = 3;
    shadow.tnear = 2.5f;
    check(!intersect(shadow) && !occluded(shadow), "Ignored near bound");
    shadow.tnear = 0;
    shadow.mask = 0;
    check(!intersect(shadow) && !occluded(shadow), "Ignored ray mask");
    ray.o.x = 10;
    check(!intersect(ray) && ray.geomID == RTC_INVALID_GEOMETRY_ID,
          "Miss retained a previous hit");

    // Scene switches must discard old geometry and its model/material mappings.
    reinitScene();
    buildBVH();
    Ray emptyRay(vec3(0.25f, 0.25f, 0), vec3(0, 0, 1));
    check(!intersect(emptyRay) && !occluded(emptyRay), "Scene reset retained geometry");
    addModel(model, translate(mat4(1), vec3(0, 0, 4)));
    buildBVH();
    check(intersect(emptyRay) && close(emptyRay.tfar, 4), "Scene rebuild failed");
    check(getIntersection(emptyRay).material == &model->m_materials[0],
          "Scene rebuild retained old material mapping");
    std::cout << "Embree wrapper tests passed\n";
}

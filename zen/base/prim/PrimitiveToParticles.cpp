#include <zen/zen.h>
#include <zen/ParticlesObject.h>
#include <zen/PrimitiveObject.h>
#include <zen/NumericObject.h>
#include <zen/vec.h>
#include <cstring>
#include <cstdlib>
#include <cassert>

namespace zenbase {

struct PrimitiveToParticles : zen::INode{
    virtual void apply() override {
    auto prim = get_input("prim")->as<PrimitiveObject>();
    auto result = zen::IObject::make<ParticlesObject>();
    result->pos.resize(prim->size());
    result->vel.resize(prim->size());

    #pragma omp parallel for
    for(int i=0;i<prim->size();i++)
    {
        result->pos[i] = zen::vec_to_other<glm::vec3>(prim->attr<zen::vec3f>("pos")[i]);
        if (prim->has_attr("vel"))
            result->vel[i] = zen::vec_to_other<glm::vec3>(prim->attr<zen::vec3f>("vel")[i]);
    }
    
    set_output("pars", result);
  }
};

static int defPrimitiveToParticles = zen::defNodeClass<PrimitiveToParticles>("PrimitiveToParticles",
    { /* inputs: */ {
        "prim",
    }, /* outputs: */ {
        "pars",
    }, /* params: */ { 
    }, /* category: */ {
    "primitive",
    }});

}

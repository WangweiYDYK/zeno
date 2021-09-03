#include <zeno/core/Session.h>
#include <zeno/core/IObject.h>
#include <zeno/core/Scene.h>
#include <zeno/utils/safe_at.h>

namespace zeno {

ZENO_API Session::Session() = default;
ZENO_API Session::~Session() = default;

ZENO_API void Session::defNodeClass(std::string const &id, std::unique_ptr<INodeClass> &&cls) {
    nodeClasses[id] = std::move(cls);
}

ZENO_API void Session::defObjectMethod(
        std::string const &name,
        Session::ObjectMethod const &func,
        std::vector<std::string> const &types) {
    std::string key = name;
    for (auto const &type: types) {
        key += '@';
        key += type;
    }
    objectMethods[key] = func;
}

ZENO_API void Session::callObjectMethod(
        std::string const &name, UserData &ud,
        std::vector<IObject *> const &args) {
    std::string key = name;
    for (auto const &obj: args) {
        auto type = typeid(*obj).name();
        key += '@';
        key += type;
    }
    auto meth = safe_at(objectMethods, key, "object method");
    meth(ud, args);
}

ZENO_API INodeClass::INodeClass(Descriptor const &desc)
        : desc(std::make_unique<Descriptor>(desc)) {
}

ZENO_API INodeClass::~INodeClass() = default;

ZENO_API std::unique_ptr<Scene> Session::createScene() {
    auto scene = std::make_unique<Scene>();
    scene->sess = const_cast<Session *>(this);
    return scene;
}

ZENO_API Scene &Session::getDefaultScene() {
    if (!defaultScene)
        defaultScene = createScene();
    return *defaultScene;
}

ZENO_API std::string Session::dumpDescriptors() const {
  std::string res = "";
  for (auto const &[key, cls] : nodeClasses) {
    res += "DESC@" + key + "@" + cls->desc->serialize() + "\n";
  }
  return res;
}


ZENO_API Session &getSession() {
    static std::unique_ptr<Session> ptr;
    if (!ptr) {
        ptr = std::make_unique<Session>();
    }
    return *ptr;
}

}

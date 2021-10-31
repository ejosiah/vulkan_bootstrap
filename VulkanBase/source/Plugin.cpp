#include "Plugin.hpp"


void Plugin::set(PluginData data) {
    this->data = data;
}

void Plugin::set(Camera *camera) {
    _camera = camera;
}

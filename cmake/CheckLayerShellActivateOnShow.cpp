#include <LayerShellQt/Window>

int main()
{
    auto *window = LayerShellQt::Window::get(nullptr);
    window->setActivateOnShow(false);
    return 0;
}

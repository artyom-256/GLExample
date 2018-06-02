#include "glapp.h"
#include "glwindow.h"
#include "object.h"

int main()
{
    GLApp app;
    auto w = std::make_shared< GLWindow >(800, 600, "GLTest");
    app.registerWindow(w);
    app.run();
    return 0;
}

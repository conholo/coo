#include "core/application.h"

int main()
{
    try
    {
        Application app;
        app.Run();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
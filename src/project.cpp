#include "project.h"
//#include "storage.h"

Project project = Project();

// for use by menu
void save_project_settings() {
    project.save_project_settings();
}

#include "project.h"
//#include "storage.h"

Project *project = new Project();

// for use by menu
void save_project_settings() {
    project->save_project_settings();
}

volatile bool global_load_lock = false;
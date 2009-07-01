#include "pdsapp/config/Parameters.hh"
#include <string.h>

using namespace Pds_ConfigDb;

const char* Enums::Bool_Names[] = { "False", "True", NULL };
const char* Enums::Polarity_Names[] = { "Pos", "Neg", NULL };
const char* Enums::Enabled_Names[] = { "Enable", "Disable", NULL };

static bool _edit = false;

void Parameter::allowEdit(bool edit) { _edit = edit; }
bool Parameter::allowEdit() { return _edit; }


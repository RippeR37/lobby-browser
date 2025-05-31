#include "wx/app.h"

#include "application.h"

wxIMPLEMENT_APP(Application);

// Used if built as non-WIN32 executable
int main(int argc, char* argv[]) {
  return WinMain(GetModuleHandle(nullptr), nullptr, *argv, argc);
}

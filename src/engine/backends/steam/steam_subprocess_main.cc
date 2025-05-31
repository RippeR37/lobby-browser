#include "engine/backends/steam/steam_subprocess_auth_backend.h"

#include "base/init.h"
#include "base/message_loop/run_loop.h"

int main(int argc, char* argv[]) {
  base::Initialize(argc, argv, {});

  base::RunLoop run_loop;
  engine::backend::SteamSubprocessAuthBackend::RunInSubprocessAndDie(argc,
                                                                     argv);
  run_loop.Run();

  base::Deinitialize();
  return 0;
}

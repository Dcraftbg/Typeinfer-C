// nob.c
#define NOB_IMPLEMENTATION
#include "nob.h"
#define c_compiler(cmd)       nob_cmd_append(cmd, "gcc")
#define c_flags(cmd)          nob_cmd_append(cmd, "-Wall", "-Wextra", "-O2")
#define c_output(cmd, output) nob_cmd_append(cmd, "-o", output)
#define c_inputs(cmd, ...)    nob_cmd_append(cmd, __VA_ARGS__)
#define c_define(cmd, def)    nob_cmd_append(cmd, "-D", def)
#define c_object(cmd)         nob_cmd_append(cmd, "-c")
#define c_link(cmd, with)     nob_cmd_append(cmd, "-l"with)
#define c_include(cmd, dir)   nob_cmd_append(cmd, "-I", dir)

bool build_main(Nob_Cmd* cmd) {
    c_compiler(cmd);
    c_flags   (cmd);
    c_output  (cmd, "main");
    c_inputs  (cmd, "src/main.c");
    return nob_cmd_run_sync_and_reset(cmd);
}
int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    if(!build_main(&cmd))    return 1;
    return 0;
}

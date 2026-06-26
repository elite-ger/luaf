#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/luaf_runtime.h"

#ifdef _WIN32
    #include <windows.h>
    static double get_time(void) {
        static LARGE_INTEGER freq = {0};
        static int init = 0;
        if (!init) { QueryPerformanceFrequency(&freq); init = 1; }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (double)now.QuadPart / freq.QuadPart;
    }
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <sys/time.h>
    #include <unistd.h>
    static double get_time(void) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

static void print_banner(void) {
    printf("Luaf v0.1.0 (Lua 5.5.0 + wait/spawn/delay/http)\n");
    printf("Type 'exit' or Ctrl+C to quit.\n\n");
}

static int is_exit_command(const char* line) {
    return strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    LuafRuntime* rt = luaf_runtime_new();
    if (!rt) {
        fprintf(stderr, "Failed to create Luaf runtime\n");
        return 1;
    }
    
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                printf("Luaf v0.1.0\n");
                luaf_runtime_free(rt);
                return 0;
            }
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                printf("Usage: luaf [options] [script.luaf ...]\n");
                printf("Options:\n");
                printf("  -v, --version  Show version\n");
                printf("  -h, --help     Show this help\n");
                printf("  -e <code>      Execute code\n");
                luaf_runtime_free(rt);
                return 0;
            }
            if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
                luaf_runtime_add_string(rt, argv[i + 1]);
                i++;
                continue;
            }
            
            printf("Loading: %s\n", argv[i]);
            luaf_runtime_add_file(rt, argv[i]);
        }
        
        double start = get_time();
        double sim_time = 0.0;
        
        while (luaf_runtime_has_tasks(rt) && sim_time < 30.0) {
            double real_elapsed = get_time() - start;
            
            if (sim_time < real_elapsed) {
                luaf_runtime_tick(rt, 0.016);
                sim_time += 0.016;
            } else {
                double wait_sec = sim_time - real_elapsed;
                if (wait_sec > 0.05) wait_sec = 0.05;
                sleep_ms((int)(wait_sec * 1000));
            }
        }
        
    } else {
        print_banner();
        
        char line[4096];
        int line_count = 0;
        
        while (1) {
            printf("luaf> ");
            fflush(stdout);
            
            if (!fgets(line, sizeof(line), stdin)) {
                printf("\n");
                break;
            }
            
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            
            if (is_exit_command(line)) {
                break;
            }
            
            if (line[0] == '\0') continue;
            
            luaf_runtime_add_string(rt, line);
            line_count++;
            
            double start = get_time();
            double sim_time = 0.0;
            int max_ticks = 600;
            int tick_count = 0;
            
            while (luaf_runtime_has_tasks(rt) && tick_count < max_ticks) {
                double real_elapsed = get_time() - start;
                
                if (sim_time < real_elapsed) {
                    luaf_runtime_tick(rt, 0.016);
                    sim_time += 0.016;
                    tick_count++;
                } else {
                    double wait_sec = sim_time - real_elapsed;
                    if (wait_sec > 0.05) wait_sec = 0.05;
                    sleep_ms((int)(wait_sec * 1000));
                }
            }
            
            if (tick_count >= max_ticks) {
                printf("(timeout)\n");
            }
        }
    }
    
    luaf_runtime_free(rt);
    return 0;
}
/* test_requirements.c - Automated tests for MySh
 * Compile:  make test_requirements
 * Run:      make test
 *
 */

#include "mysh.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Test framework                                                      */
/* ------------------------------------------------------------------ */

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name, expr) do {                                        \
    int _ok = (int)(expr);                                           \
    tests_run++;                                                     \
    if (_ok) tests_passed++;                                         \
    printf("[%s] %s\n", _ok ? "PASS" : "FAIL", (name));             \
} while (0)

/* ------------------------------------------------------------------ */
/* Pipe capture helpers                                                */
/* ------------------------------------------------------------------ */

static void capture_start(int target_fd, int *saved, int fds[2]) {
    fflush(stdout);
    fflush(stderr);
    pipe(fds);
    *saved = dup(target_fd);
    dup2(fds[1], target_fd);
    close(fds[1]);
}

static ssize_t capture_end(int target_fd, int *saved, int fds[2],
                            char *buf, size_t n) {
    fflush(target_fd == STDOUT_FILENO ? stdout : stderr);
    dup2(*saved, target_fd);   /* restores original fd, closes pipe write side */
    close(*saved);
    fcntl(fds[0], F_SETFL, O_NONBLOCK);
    ssize_t r = read(fds[0], buf, n - 1);
    close(fds[0]);
    if (r > 0)      buf[r] = '\0';
    else            buf[0] = '\0';
    return r;
}

/* ------------------------------------------------------------------ */
/* R1 - Interactive prompt                                             */
/* ------------------------------------------------------------------ */

static void test_r1_prompt(void) {
    int fds[2], saved;
    char buf[256] = {0};

    capture_start(STDOUT_FILENO, &saved, fds);
    show_prompt();
    capture_end(STDOUT_FILENO, &saved, fds, buf, sizeof(buf));

    TEST("R1: prompt produces output",   buf[0] != '\0');
    TEST("R1: prompt contains '$'",      strchr(buf, '$') != NULL);
}

/* ------------------------------------------------------------------ */
/* R2 - Built-in exit command                                         */
/* ------------------------------------------------------------------ */

static void test_r2_exit_command(void) {
    int stdin_pipe[2];
    pipe(stdin_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
        execl("./mysh", "./mysh", (char *)NULL);
        _exit(127);
    }
    close(stdin_pipe[0]);
    write(stdin_pipe[1], "exit\n", 5);
    close(stdin_pipe[1]);

    /* poll up to 3 seconds in 10ms steps */
    int status = 0, exited = 0;
    for (int i = 0; i < 300; i++) {
        if (waitpid(pid, &status, WNOHANG) > 0) { exited = 1; break; }
        struct timespec ts = {0, 10000000L};
        nanosleep(&ts, NULL);
    }
    if (!exited) kill(pid, SIGKILL);

    TEST("R2: 'exit' terminates shell",           exited);
    TEST("R2: shell exits with status 0",
         exited && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

/* ------------------------------------------------------------------ */
/* R3 - Execution via PATH                                            */
/* ------------------------------------------------------------------ */

static void test_r3_path_execution(void) {
    int fds[2], saved;
    char buf[64] = {0};

    capture_start(STDOUT_FILENO, &saved, fds);
    char *args[] = {"echo", "ping_path", NULL};
    int ret = execute_command(args);
    capture_end(STDOUT_FILENO, &saved, fds, buf, sizeof(buf));

    TEST("R3: PATH execution returns 0",     ret == 0);
    TEST("R3: command output via PATH",      strstr(buf, "ping_path") != NULL);
}

/* ------------------------------------------------------------------ */
/* R4 - Execution via absolute and relative path                      */
/* ------------------------------------------------------------------ */

static void test_r4_paths(void) {
    /* Absolute path */
    fflush(stdout); fflush(stderr);
    char *abs_args[] = {"/bin/true", NULL};
    int ret_abs = execute_command(abs_args);
    TEST("R4: absolute path (/bin/true) returns 0", ret_abs == 0);

    /* Relative path: create a temp script and run ./script */
    char tmpdir[] = "/tmp/mysh_test_XXXXXX";
    if (mkdtemp(tmpdir) == NULL) {
        TEST("R4: relative path temp setup failed", 0);
        return;
    }

    char script[320];
    snprintf(script, sizeof(script), "%s/hello.sh", tmpdir);
    FILE *f = fopen(script, "w");
    if (f) { fputs("#!/bin/sh\nexit 0\n", f); fclose(f); }
    chmod(script, 0755);

    char cwd[MAX_CWD];
    getcwd(cwd, sizeof(cwd));
    chdir(tmpdir);

    fflush(stdout); fflush(stderr);
    char *rel_args[] = {"./hello.sh", NULL};
    int ret_rel = execute_command(rel_args);

    chdir(cwd);
    remove(script);
    rmdir(tmpdir);

    TEST("R4: relative path (./script) returns 0", ret_rel == 0);
}

/* ------------------------------------------------------------------ */
/* R5 - Argument management                                           */
/* ------------------------------------------------------------------ */

static void test_r5_arguments(void) {
    char *args[MAX_ARGS];
    int count;

    char input1[] = "ls -l /tmp\n";
    count = parse_input(input1, args);
    TEST("R5: correct token count (3)",             count == 3);
    TEST("R5: args[0] is command name",             strcmp(args[0], "ls") == 0);
    TEST("R5: args[1] is first argument",           strcmp(args[1], "-l") == 0);
    TEST("R5: args[2] is second argument",          strcmp(args[2], "/tmp") == 0);
    TEST("R5: args[count] is NULL",                 args[count] == NULL);

    char input2[] = "   \t  \n";
    count = parse_input(input2, args);
    TEST("R5: whitespace-only input returns 0",     count == 0);

    char input3[] = "echo   a   b\n";
    count = parse_input(input3, args);
    TEST("R5: multiple spaces collapsed (count=3)", count == 3);

    char input4[] = "\n";
    count = parse_input(input4, args);
    TEST("R5: bare newline returns 0",              count == 0);
}

/* ------------------------------------------------------------------ */
/* R6 - Error handling                                                */
/* ------------------------------------------------------------------ */

static void test_r6_errors(void) {
    int fds[2], saved;
    char buf[256] = {0};

    capture_start(STDERR_FILENO, &saved, fds);
    char *args1[] = {"nonexistent_cmd_xyz123", NULL};
    int ret1 = execute_command(args1);
    capture_end(STDERR_FILENO, &saved, fds, buf, sizeof(buf));

    TEST("R6: non-existent command returns non-zero", ret1 != 0);
    TEST("R6: error message printed to stderr",       buf[0] != '\0');

    fflush(stdout); fflush(stderr);
    char *args2[] = {"/nonexistent/path/prog", NULL};
    int ret2 = execute_command(args2);
    TEST("R6: invalid absolute path returns non-zero", ret2 != 0);
}

/* ------------------------------------------------------------------ */
/* R7 - No system() call in source files                             */
/* ------------------------------------------------------------------ */

static void test_r7_no_system(void) {
    FILE *fp = popen(
        "grep -n 'system(' main.c parser.c executor.c signals.c 2>/dev/null",
        "r");
    char line[256] = {0};
    int found = 0;
    if (fp) {
        if (fgets(line, sizeof(line), fp) != NULL) found = 1;
        pclose(fp);
    }
    TEST("R7: no system() call in source files", !found);
}

/* ------------------------------------------------------------------ */
/* R8 - No zombie processes                                           */
/* ------------------------------------------------------------------ */

static void test_r8_no_zombies(void) {
    setup_signals();

    for (int i = 0; i < 10; i++) {
        pid_t child = fork();
        if (child == 0) _exit(0);
    }

    /* give SIGCHLD handler time to reap all children */
    struct timespec ts = {0, 200000000L};
    nanosleep(&ts, NULL);

    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "ps --ppid %d -o stat= 2>/dev/null | grep -c Z", (int)getpid());
    FILE *fp = popen(cmd, "r");
    int zombies = -1;
    if (fp) { fscanf(fp, "%d", &zombies); pclose(fp); }

    TEST("R8: SIGCHLD handler prevents zombie processes", zombies == 0);
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("=== MySh Requirements Tests ===\n\n");

    test_r1_prompt();
    printf("\n");
    test_r2_exit_command();
    printf("\n");
    test_r3_path_execution();
    printf("\n");
    test_r4_paths();
    printf("\n");
    test_r5_arguments();
    printf("\n");
    test_r6_errors();
    printf("\n");
    test_r7_no_system();
    printf("\n");
    test_r8_no_zombies();

    printf("\n--- Results: %d/%d passed ---\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}

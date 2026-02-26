#include "emp_lexer.h"
#include "emp_parser.h"
#include "emp_json.h"
#include "emp_defer.h"
#include "emp_semantic.h"
#include "emp_typecheck.h"
#include "emp_borrow.h"
#include "emp_drop.h"
#include "emp_codegen_llvm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifndef _WIN32
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define EMP_PATH_SEP '\\'
#else
#define EMP_PATH_SEP '/'
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#endif

static char *read_entire_file(const char *path, size_t *out_len);
static void strip_markdown_fence_in_place(char *buf, size_t *len_io);
static char *path_join2(const char *a, const char *b);
static char *abs_path_dup(const char *path);
static char *path_dirname_dup(const char *path);
static bool ensure_dir(const char *path);

static bool slice_eq(EmpSlice a, EmpSlice b) {
    if (a.len != b.len) return false;
    if (a.ptr == b.ptr) return true;
    if (!a.ptr || !b.ptr) return false;
    return memcmp(a.ptr, b.ptr, a.len) == 0;
}

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static bool file_exists(const char *path) {
    if (!path) return false;
#ifdef _WIN32
    return _access(path, 0) == 0;
#else
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
#endif
}

static bool dir_exists(const char *path) {
    if (!path) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

static int write_text_file(const char *path, const char *content) {
    if (!path || !content) return 0;
    FILE *f = NULL;
#ifdef _WIN32
    if (fopen_s(&f, path, "wb") != 0) f = NULL;
#else
    f = fopen(path, "wb");
#endif
    if (!f) return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}

static char *path_parent_dir_dup(const char *path) {
    if (!path) return NULL;
    char *abs = abs_path_dup(path);
    if (!abs) return NULL;
    char *dir = path_dirname_dup(abs);
    free(abs);
    return dir;
}

static int cmd_new(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s new <project-name>\n", argv[0]);
        return 2;
    }

    const char *name = argv[2];
    if (!name || !name[0]) {
        fprintf(stderr, "Invalid project name\n");
        return 2;
    }

    if (dir_exists(name) || file_exists(name)) {
        fprintf(stderr, "Refusing to overwrite existing path: %s\n", name);
        return 1;
    }

    if (!ensure_dir(name)) {
        fprintf(stderr, "Failed to create project directory: %s\n", name);
        return 1;
    }
    char *src_dir = path_join2(name, "src");
    char *emp_mods_dir = path_join2(name, "emp_mods");
    if (!src_dir || !emp_mods_dir || !ensure_dir(src_dir) || !ensure_dir(emp_mods_dir)) {
        fprintf(stderr, "Failed to create project subdirectories\n");
        free(src_dir);
        free(emp_mods_dir);
        return 1;
    }

    char *toml = path_join2(name, "emp.toml");
    char *readme = path_join2(name, "README.md");
    char *main_em = path_join2(src_dir, "main.em");
    char *gitignore = path_join2(name, ".gitignore");

    // Build emp.toml in a tiny buffer.
    char toml_buf[1024];
    snprintf(toml_buf, sizeof(toml_buf), "name = \"%s\"\nversion = \"0.1.0\"\n", name);

    const char *readme_txt =
        "# EMP Project\n\n"
        "This project was scaffolded by `emp new`.\n\n"
        "## Build + run (native exe)\n\n"
        "From this folder:\n\n"
#ifdef _WIN32
        "- Build: `emp.exe src/main.em`\n"
#else
        "- Build: `emp src/main.em`\n"
#endif
        "- Run: `./out/bin/main.exe`\n\n"
        "## Emit LLVM IR only\n\n"
#ifdef _WIN32
        "- `emp.exe --nobin --out out/main.ll src/main.em`\n";
#else
        "- `emp --nobin --out out/main.ll src/main.em`\n";
#endif

    const char *gitignore_txt =
        "out/\n";

    const char *main_txt =
        "use {println} from std.console;\n\n"
        "// Tiny neural net demo: XOR using a 2-layer perceptron network.\n"
        "//\n"
        "// Implemented with integer math (scaled thresholds) so we don't need floats yet.\n\n"
        "fn step_gt(v: i32, thr: i32) -> i32 {\n"
        "  if v > thr { return 1; }\n"
        "  else { return 0; }\n"
        "}\n\n"
        "fn xor_nn(x1: i32, x2: i32) -> i32 {\n"
        "  // Hidden: h1 = OR, h2 = AND (scaled by 2 to avoid 0.5).\n"
        "  i32 sum2 = 2 * (x1 + x2);\n"
        "  i32 h1 = step_gt(sum2, 1);\n"
        "  i32 h2 = step_gt(sum2, 3);\n\n"
        "  // Output: y = h1 - 2*h2 - 0.5  (scaled: 2*h1 - 4*h2 > 1)\n"
        "  i32 v = 2 * h1 - 4 * h2;\n"
        "  return step_gt(v, 1);\n"
        "}\n\n"
        "fn check(x1: i32, x2: i32, expected: i32, label: *u8) {\n"
        "  i32 got = xor_nn(x1, x2);\n"
        "  if got == expected {\n"
        "    println(label);\n"
        "  } else {\n"
        "    println(\"FAILED\");\n"
        "  }\n"
        "}\n\n"
        "fn main() -> i32 {\n"
        "  println(`EMP XOR neural net demo\n(no floats yet; using integer thresholds)\n`);\n"
        "  check(0, 0, 0, \"case 0,0 OK\");\n"
        "  check(1, 0, 1, \"case 1,0 OK\");\n"
        "  check(0, 1, 1, \"case 0,1 OK\");\n"
        "  check(1, 1, 0, \"case 1,1 OK\");\n"
        "  return 0;\n"
        "}\n";

    int ok = 1;
    ok = ok && write_text_file(toml, toml_buf);
    ok = ok && write_text_file(readme, readme_txt);
    ok = ok && write_text_file(gitignore, gitignore_txt);
    ok = ok && write_text_file(main_em, main_txt);

    free(src_dir);
    free(emp_mods_dir);
    free(toml);
    free(readme);
    free(main_em);
    free(gitignore);

    if (!ok) {
        fprintf(stderr, "Failed to write one or more project files\n");
        return 1;
    }

    printf("Created EMP project: %s\n", name);
    printf("Next:\n");
    printf("  cd %s\n", name);
#ifdef _WIN32
    printf("  emp.exe src/main.em\n");
#else
    printf("  emp src/main.em\n");
#endif
    printf("  ./out/bin/main.exe\n");
    return 0;
}

static bool ensure_dir(const char *path) {
    if (!path || !path[0]) return false;
    if (dir_exists(path)) return true;
#ifdef _WIN32
    if (CreateDirectoryA(path, NULL)) return true;
    DWORD err = GetLastError();
    return err == ERROR_ALREADY_EXISTS;
#else
    if (mkdir(path, 0777) == 0) return true;
    return errno == EEXIST;
#endif
}

static bool ensure_dir_recursive(const char *path) {
    if (!path || !path[0]) return false;
    char *tmp = xstrdup(path);
    if (!tmp) return false;

    // Handle drive prefixes like C:\\ (or C:/)
    size_t i = 0;
    if (tmp[0] && tmp[1] == ':') {
        i = 2;
        while (tmp[i] == '\\' || tmp[i] == '/') i++;
    }

    for (; tmp[i]; i++) {
        if (tmp[i] != '\\' && tmp[i] != '/') continue;
        char ch = tmp[i];
        tmp[i] = '\0';
        if (tmp[0] && !dir_exists(tmp)) {
            (void)ensure_dir(tmp);
        }
        tmp[i] = ch;
        while (tmp[i + 1] == '\\' || tmp[i + 1] == '/') i++;
    }
    if (tmp[0] && !dir_exists(tmp)) {
        (void)ensure_dir(tmp);
    }
    free(tmp);
    return true;
}

static char *get_exe_dir_abs(void) {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, (DWORD)sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return NULL;
    return path_dirname_dup(buf);
#else
    return NULL;
#endif
}

static bool slice_starts_with_cstr(EmpSlice s, const char *prefix) {
    if (!s.ptr || !prefix) return false;
    size_t n = strlen(prefix);
    if (s.len < n) return false;
    return memcmp(s.ptr, prefix, n) == 0;
}

static bool module_path_is_std(EmpSlice module_path) {
    if (!slice_starts_with_cstr(module_path, "std")) return false;
    if (module_path.len == 3) return true;
    char ch = module_path.ptr[3];
    return ch == '.' || ch == ':';
}

static bool path_starts_with_ci_sep(const char *path, const char *prefix) {
    if (!path || !prefix) return false;
    size_t np = strlen(prefix);
    if (np == 0) return false;
    if (strlen(path) < np) return false;

    // Compare prefix case-insensitively, accepting either slash kind.
    for (size_t i = 0; i < np; i++) {
        char a = path[i];
        char b = prefix[i];
        if (a == '/') a = '\\';
        if (b == '/') b = '\\';
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

static bool copy_text_file_if_missing(const char *src_abs, const char *dst_abs) {
    if (!src_abs || !dst_abs) return false;
    if (file_exists(dst_abs)) return true;

    char *dst_dir = path_dirname_dup(dst_abs);
    if (!dst_dir) return false;
    (void)ensure_dir_recursive(dst_dir);
    free(dst_dir);

    size_t len = 0;
    char *src = read_entire_file(src_abs, &len);
    if (!src) return false;
    // Preserve exact bytes; do not add NULs beyond what read_entire_file provides.
    FILE *f = NULL;
    if (fopen_s(&f, dst_abs, "wb") != 0) f = NULL;
    if (!f) {
        free(src);
        return false;
    }
    fwrite(src, 1, len, f);
    fclose(f);
    free(src);
    return true;
}

static char *vendor_bundled_module_to_project(const char *resolved_abs, const char *bundled_emp_mods_abs, const char *project_emp_mods_abs) {
    if (!resolved_abs || !bundled_emp_mods_abs || !project_emp_mods_abs) return NULL;
    if (!path_starts_with_ci_sep(resolved_abs, bundled_emp_mods_abs)) return NULL;

    const char *p = resolved_abs + strlen(bundled_emp_mods_abs);
    while (*p == '\\' || *p == '/') p++;

    char *dst_abs = path_join2(project_emp_mods_abs, p);
    if (!dst_abs) return NULL;
    if (!copy_text_file_if_missing(resolved_abs, dst_abs)) {
        free(dst_abs);
        return NULL;
    }
    char *out = abs_path_dup(dst_abs);
    free(dst_abs);
    return out;
}

static char *path_basename_noext(const char *path) {
    if (!path) return xstrdup("emp");
    const char *base = path;
    for (const char *p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    const char *dot = NULL;
    for (const char *p = base; *p; p++) {
        if (*p == '.') dot = p;
    }
    size_t n = dot ? (size_t)(dot - base) : strlen(base);
    char *out = (char *)malloc(n + 1);
    if (!out) return xstrdup("emp");
    memcpy(out, base, n);
    out[n] = '\0';
    return out;
}

static char *find_windows_kits_um_x64(void) {
#ifdef _WIN32
    // Best-effort discovery of Windows SDK import libs (kernel32.lib).
    // Typical path: C:\Program Files (x86)\Windows Kits\10\Lib\<ver>\um\x64
    const char *root = "C:\\Program Files (x86)\\Windows Kits\\10\\Lib";
    if (!dir_exists(root)) return NULL;

    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", root);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return NULL;

    char best_ver[MAX_PATH];
    best_ver[0] = '\0';

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

        char cand[MAX_PATH];
        snprintf(cand, sizeof(cand), "%s\\%s\\um\\x64", root, fd.cFileName);
        char k32[MAX_PATH];
        snprintf(k32, sizeof(k32), "%s\\kernel32.lib", cand);
        if (!file_exists(k32)) continue;

        // Pick lexicographically highest version directory.
        if (best_ver[0] == '\0' || strcmp(fd.cFileName, best_ver) > 0) {
            snprintf(best_ver, sizeof(best_ver), "%s", fd.cFileName);
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    if (best_ver[0] == '\0') return NULL;

    char out[MAX_PATH];
    snprintf(out, sizeof(out), "%s\\%s\\um\\x64", root, best_ver);
    return xstrdup(out);
#else
    return NULL;
#endif
}

#ifndef _WIN32
static bool find_in_path(const char *name, char *out, size_t out_cap) {
    if (!name || !name[0] || !out || out_cap == 0) return false;
    const char *path = getenv("PATH");
    if (!path || !path[0]) return false;

    size_t nn = strlen(name);
    const char *cur = path;
    while (*cur) {
        const char *sep = strchr(cur, ':');
        size_t dn = sep ? (size_t)(sep - cur) : strlen(cur);
        if (dn > 0) {
            size_t need = dn + 1 + nn + 1;
            if (need <= out_cap) {
                memcpy(out, cur, dn);
                out[dn] = '/';
                memcpy(out + dn + 1, name, nn);
                out[dn + 1 + nn] = '\0';
                if (access(out, X_OK) == 0) return true;
            }
        }
        if (!sep) break;
        cur = sep + 1;
    }
    return false;
}
#endif

static bool arg_needs_quotes(const char *s) {
    if (!s) return false;
    for (const char *p = s; *p; p++) {
        if (*p == ' ' || *p == '\t') return true;
    }
    return false;
}

static void cmd_append(char *dst, size_t dst_cap, const char *s) {
    if (!dst || dst_cap == 0 || !s) return;
    size_t cur = strlen(dst);
    size_t n = strlen(s);
    if (cur + n + 1 >= dst_cap) return;
    memcpy(dst + cur, s, n + 1);
}

static intptr_t spawn_wait(const char *exe, const char *const *args) {
    if (!exe || !args) return -1;

#ifdef _WIN32

    // Build a command line: "exe" arg1 arg2 ...
    // This is a minimal quoting strategy (sufficient for file paths with spaces).
    char cmd[32768];
    cmd[0] = '\0';

    for (size_t i = 0; args[i]; i++) {
        if (i) cmd_append(cmd, sizeof(cmd), " ");
        if (arg_needs_quotes(args[i])) {
            cmd_append(cmd, sizeof(cmd), "\"");
            cmd_append(cmd, sizeof(cmd), args[i]);
            cmd_append(cmd, sizeof(cmd), "\"");
        } else {
            cmd_append(cmd, sizeof(cmd), args[i]);
        }
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    // CreateProcess may modify the command line buffer.
    char cmd_mut[32768];
    snprintf(cmd_mut, sizeof(cmd_mut), "%s", cmd);

    BOOL ok = CreateProcessA(
        exe,
        cmd_mut,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi);

    if (!ok) {
        return (intptr_t)(-2);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (intptr_t)code;
#else
    pid_t pid = fork();
    if (pid < 0) return (intptr_t)(-2);
    if (pid == 0) {
        execvp(exe, (char *const *)args);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return (intptr_t)(-3);
    if (WIFEXITED(status)) return (intptr_t)WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return (intptr_t)(128 + WTERMSIG(status));
    return (intptr_t)(-4);
#endif
}
typedef struct StrVec {
    char **items;
    size_t len;
    size_t cap;
} StrVec;

static int str_cmpi(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
#ifdef _WIN32
    return _stricmp(a, b);
#else
    return strcmp(a, b);
#endif
}

static int qsort_cstr_cmpi(const void *pa, const void *pb) {
    const char *a = *(const char *const *)pa;
    const char *b = *(const char *const *)pb;
    return str_cmpi(a, b);
}

static void strvec_free(StrVec *v) {
    if (!v) return;
    for (size_t i = 0; i < v->len; i++) free(v->items[i]);
    free(v->items);
    memset(v, 0, sizeof(*v));
}

static bool strvec_push(StrVec *v, char *s) {
    if (v->len + 1 > v->cap) {
        size_t nc = v->cap ? v->cap * 2 : 16;
        char **p = (char **)realloc(v->items, nc * sizeof(char *));
        if (!p) return false;
        v->items = p;
        v->cap = nc;
    }
    v->items[v->len++] = s;
    return true;
}

static StrVec list_em_files_in_dir(const char *dir_abs) {
    StrVec out;
    memset(&out, 0, sizeof(out));

#ifdef _WIN32
    char *pattern = path_join2(dir_abs, "*.em");
    if (!pattern) return out;

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE) return out;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char *full = path_join2(dir_abs, ffd.cFileName);
        if (!full) continue;
        char *abs = abs_path_dup(full);
        free(full);
        if (!abs) continue;
        (void)strvec_push(&out, abs);
    } while (FindNextFileA(h, &ffd));

    FindClose(h);

    // Ensure deterministic iteration order for wildcard package imports.
    if (out.len > 1) {
        qsort(out.items, out.len, sizeof(char *), qsort_cstr_cmpi);
    }
#endif

    return out;
}

static char *vendor_bundled_package_dir_to_project(const char *resolved_dir_abs, const char *bundled_emp_mods_abs, const char *project_emp_mods_abs) {
    if (!resolved_dir_abs || !bundled_emp_mods_abs || !project_emp_mods_abs) return NULL;
    if (!path_starts_with_ci_sep(resolved_dir_abs, bundled_emp_mods_abs)) return NULL;

    const char *p = resolved_dir_abs + strlen(bundled_emp_mods_abs);
    while (*p == '\\' || *p == '/') p++;

    char *dst_dir = path_join2(project_emp_mods_abs, p);
    if (!dst_dir) return NULL;
    (void)ensure_dir_recursive(dst_dir);

    StrVec files = list_em_files_in_dir(resolved_dir_abs);
    for (size_t i = 0; i < files.len; i++) {
        char *vend = vendor_bundled_module_to_project(files.items[i], bundled_emp_mods_abs, project_emp_mods_abs);
        free(vend);
    }
    strvec_free(&files);

    char *out = abs_path_dup(dst_dir);
    free(dst_dir);
    return out;
}

static char *path_dirname_dup(const char *path) {
    if (!path) return xstrdup(".");
    const char *slash1 = strrchr(path, '/');
    const char *slash2 = strrchr(path, '\\');
    const char *slash = slash1;
    if (!slash || (slash2 && slash2 > slash)) slash = slash2;
    if (!slash) return xstrdup(".");
    size_t n = (size_t)(slash - path);
    if (n == 0) n = 1;
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, path, n);
    out[n] = '\0';
    return out;
}

static char *path_join2(const char *a, const char *b) {
    if (!a) return xstrdup(b);
    if (!b) return xstrdup(a);
    size_t na = strlen(a);
    size_t nb = strlen(b);
    int need_sep = (na > 0 && a[na - 1] != '/' && a[na - 1] != '\\');
    char *out = (char *)malloc(na + (need_sep ? 1 : 0) + nb + 1);
    if (!out) return NULL;
    memcpy(out, a, na);
    size_t pos = na;
    if (need_sep) out[pos++] = EMP_PATH_SEP;
    memcpy(out + pos, b, nb);
    out[pos + nb] = '\0';
    return out;
}

static char *abs_path_dup(const char *path) {
    if (!path) return NULL;
#ifdef _WIN32
    char buf[4096];
    if (_fullpath(buf, path, sizeof(buf)) == NULL) {
        return xstrdup(path);
    }
    return xstrdup(buf);
#else
    return xstrdup(path);
#endif
}

static char *module_slice_to_rel_path(EmpSlice module_path) {
    if (!module_path.ptr || module_path.len == 0) return NULL;
    char *out = (char *)malloc(module_path.len + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < module_path.len; i++) {
        char ch = module_path.ptr[i];
        if (ch == '.') {
            out[j++] = EMP_PATH_SEP;
            continue;
        }
        if (ch == ':' && i + 1 < module_path.len && module_path.ptr[i + 1] == ':') {
            out[j++] = EMP_PATH_SEP;
            i++;
            continue;
        }
        out[j++] = ch;
    }
    out[j] = '\0';
    return out;
}

static char *resolve_module_file(const char *base_dir, EmpSlice module_path) {
    char *rel = module_slice_to_rel_path(module_path);
    if (!rel) return NULL;

    // Try <base>/<rel>.em
    char *rel_em = (char *)malloc(strlen(rel) + 3 + 1);
    if (!rel_em) {
        free(rel);
        return NULL;
    }
    snprintf(rel_em, strlen(rel) + 4, "%s.em", rel);
    char *cand1 = path_join2(base_dir, rel_em);
    free(rel_em);

    if (cand1 && file_exists(cand1)) {
        char *abs = abs_path_dup(cand1);
        free(cand1);
        free(rel);
        return abs;
    }
    free(cand1);

    // Try <base>/<rel>/mod.em
    char *cand2_dir = path_join2(base_dir, rel);
    char *cand2 = cand2_dir ? path_join2(cand2_dir, "mod.em") : NULL;
    free(cand2_dir);
    free(rel);
    if (cand2 && file_exists(cand2)) {
        char *abs = abs_path_dup(cand2);
        free(cand2);
        return abs;
    }
    free(cand2);
    return NULL;
}

static char *resolve_module_dir(const char *base_dir, EmpSlice module_path) {
    char *rel = module_slice_to_rel_path(module_path);
    if (!rel) return NULL;
    char *cand = path_join2(base_dir, rel);
    free(rel);
    if (!cand) return NULL;
    if (dir_exists(cand)) {
        char *abs = abs_path_dup(cand);
        free(cand);
        return abs;
    }
    free(cand);
    return NULL;
}

typedef struct EmpModule {
    char *path_abs;
    char *dir_abs;
    char *src_owned;
    size_t src_len;
    EmpParseResult pr;
} EmpModule;

typedef struct EmpModules {
    EmpModule *items;
    size_t len;
    size_t cap;
} EmpModules;

static void modules_init(EmpModules *m) { memset(m, 0, sizeof(*m)); }

static void modules_free(EmpModules *m) {
    if (!m) return;
    for (size_t i = 0; i < m->len; i++) {
        EmpModule *mm = &m->items[i];
        emp_parse_result_free(&mm->pr);
        free(mm->src_owned);
        free(mm->path_abs);
        free(mm->dir_abs);
    }
    free(m->items);
    memset(m, 0, sizeof(*m));
}

static EmpModule *modules_find(EmpModules *m, const char *path_abs) {
    for (size_t i = 0; i < m->len; i++) {
    if (!m->items[i].path_abs || !path_abs) continue;
#ifdef _WIN32
    if (_stricmp(m->items[i].path_abs, path_abs) == 0) return &m->items[i];
#else
    if (strcmp(m->items[i].path_abs, path_abs) == 0) return &m->items[i];
#endif
    }
    return NULL;
}

static EmpModule *modules_push(EmpModules *m, EmpModule mod) {
    if (m->len + 1 > m->cap) {
        size_t new_cap = m->cap ? m->cap * 2 : 16;
        EmpModule *p = (EmpModule *)realloc(m->items, new_cap * sizeof(EmpModule));
        if (!p) return NULL;
        m->items = p;
        m->cap = new_cap;
    }
    m->items[m->len] = mod;
    return &m->items[m->len++];
}

static EmpModule *load_module(EmpModules *mods, const char *path_abs) {
    EmpModule *existing = modules_find(mods, path_abs);
    if (existing) return existing;

    size_t len = 0;
    char *src = read_entire_file(path_abs, &len);
    if (!src) return NULL;
    strip_markdown_fence_in_place(src, &len);

    EmpParseResult pr = emp_parse(src, len);

    EmpModule mod;
    memset(&mod, 0, sizeof(mod));
    mod.path_abs = xstrdup(path_abs);
    mod.dir_abs = path_dirname_dup(path_abs);
    mod.src_owned = src;
    mod.src_len = len;
    mod.pr = pr;

    return modules_push(mods, mod);
}

static void diagf_owned(EmpArena *arena, EmpDiags *diags, EmpSpan span, const char *prefix, const char *msg) {
    size_t np = prefix ? strlen(prefix) : 0;
    size_t nm = msg ? strlen(msg) : 0;
    char *p = (char *)emp_arena_alloc(arena, np + nm + 1, 1);
    if (!p) return;
    if (np) memcpy(p, prefix, np);
    if (nm) memcpy(p + np, msg, nm);
    p[np + nm] = '\0';
    EmpDiag d;
    d.span = span;
    d.message = p;
    (void)emp_diags_push(diags, d);
}

static void diagf_tmp(EmpArena *arena, EmpDiags *diags, EmpSpan span, const char *prefix, const char *fmt, ...) {
    if (!arena || !diags || !fmt) return;
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    diagf_owned(arena, diags, span, prefix, buf);
}

typedef struct EmpResolveBase {
    const char *label;
    const char *dir_abs;
} EmpResolveBase;

static bool bases_contains(const EmpResolveBase *bases, size_t bases_len, const char *dir_abs) {
    if (!dir_abs) return true;
    for (size_t i = 0; i < bases_len; i++) {
        if (bases[i].dir_abs && str_cmpi(bases[i].dir_abs, dir_abs) == 0) return true;
    }
    return false;
}

static bool strvec_contains(const StrVec *v, const char *s) {
    if (!v || !s) return false;
    for (size_t i = 0; i < v->len; i++) {
        if (!v->items[i]) continue;
        if (str_cmpi(v->items[i], s) == 0) return true;
    }
    return false;
}

static void resolve_add_file_candidates(StrVec *abs_out, StrVec *desc_out, const EmpResolveBase *base, const char *rel) {
    if (!abs_out || !desc_out || !base || !base->dir_abs || !rel) return;

    // Candidate 1: <base>/<rel>.em
    char *rel_em = (char *)malloc(strlen(rel) + 3 + 1);
    if (rel_em) {
        snprintf(rel_em, strlen(rel) + 4, "%s.em", rel);
        char *cand1 = path_join2(base->dir_abs, rel_em);
        free(rel_em);
        if (cand1 && file_exists(cand1)) {
            char *abs = abs_path_dup(cand1);
            if (abs && !strvec_contains(abs_out, abs)) {
                char tmp[1024];
                snprintf(tmp, sizeof(tmp), "%s:%s.em", base->label ? base->label : "<base>", rel);
                (void)strvec_push(abs_out, abs);
                (void)strvec_push(desc_out, xstrdup(tmp));
            } else {
                free(abs);
            }
        }
        free(cand1);
    }

    // Candidate 2: <base>/<rel>/mod.em
    char *cand2_dir = path_join2(base->dir_abs, rel);
    char *cand2 = cand2_dir ? path_join2(cand2_dir, "mod.em") : NULL;
    free(cand2_dir);
    if (cand2 && file_exists(cand2)) {
        char *abs = abs_path_dup(cand2);
        if (abs && !strvec_contains(abs_out, abs)) {
            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s:%s\\mod.em", base->label ? base->label : "<base>", rel);
            (void)strvec_push(abs_out, abs);
            (void)strvec_push(desc_out, xstrdup(tmp));
        } else {
            free(abs);
        }
    }
    free(cand2);
}

static void resolve_add_dir_candidates(StrVec *abs_out, StrVec *desc_out, const EmpResolveBase *base, const char *rel) {
    if (!abs_out || !desc_out || !base || !base->dir_abs || !rel) return;
    char *cand = path_join2(base->dir_abs, rel);
    if (cand && dir_exists(cand)) {
        char *abs = abs_path_dup(cand);
        if (abs && !strvec_contains(abs_out, abs)) {
            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s:%s", base->label ? base->label : "<base>", rel);
            (void)strvec_push(abs_out, abs);
            (void)strvec_push(desc_out, xstrdup(tmp));
        } else {
            free(abs);
        }
    }
    free(cand);
}

static char *resolve_use_module_file(
    EmpArena *arena,
    EmpDiags *diags,
    EmpSpan span,
    const EmpResolveBase *bases,
    size_t bases_len,
    EmpSlice module_path,
    const char *bundled_emp_mods_abs,
    const char *project_emp_mods_abs,
    bool emit_diags,
    bool *out_had_diag
) {
    if (out_had_diag) *out_had_diag = false;
    char *rel = module_slice_to_rel_path(module_path);
    if (!rel) {
        if (emit_diags) {
            if (out_had_diag) *out_had_diag = true;
            diagf_owned(arena, diags, span, "module: ", "invalid module path in use statement");
        }
        return NULL;
    }

    StrVec cands_abs;
    StrVec cands_desc;
    memset(&cands_abs, 0, sizeof(cands_abs));
    memset(&cands_desc, 0, sizeof(cands_desc));

    for (size_t i = 0; i < bases_len; i++) {
        resolve_add_file_candidates(&cands_abs, &cands_desc, &bases[i], rel);
    }

    free(rel);

    // Bundled stdlib is a lowest-precedence fallback: if we found any non-bundled
    // candidates, ignore bundled ones.
    size_t nonbundled_count = 0;
    size_t nonbundled_idx = 0;
    for (size_t i = 0; i < cands_abs.len; i++) {
        const char *p = cands_abs.items[i];
        if (!p) continue;
        bool is_bundled = bundled_emp_mods_abs && path_starts_with_ci_sep(p, bundled_emp_mods_abs);
        if (!is_bundled) {
            nonbundled_idx = i;
            nonbundled_count++;
        }
    }

    if (nonbundled_count == 1) {
        char *out = xstrdup(cands_abs.items[nonbundled_idx]);
        strvec_free(&cands_abs);
        strvec_free(&cands_desc);
        return out;
    }

    if (nonbundled_count == 0 && cands_abs.len == 1) {
        char *out = xstrdup(cands_abs.items[0]);
        if (out && bundled_emp_mods_abs && project_emp_mods_abs && path_starts_with_ci_sep(out, bundled_emp_mods_abs)) {
            char *vend = vendor_bundled_module_to_project(out, bundled_emp_mods_abs, project_emp_mods_abs);
            if (vend) {
                free(out);
                out = vend;
            }
        }
        strvec_free(&cands_abs);
        strvec_free(&cands_desc);
        return out;
    }

    if (out_had_diag) *out_had_diag = true;

    if (emit_diags) {
        if (cands_abs.len == 0) {
            diagf_tmp(arena, diags, span, "module: ",
                      "failed to resolve module '%.*s' (searched: module_dir, entry_dir, emp_mods, entry_root, bundled)",
                      (int)module_path.len,
                      module_path.ptr ? module_path.ptr : "");
        } else {
            char buf[4096];
            size_t pos = 0;
            size_t printed = 0;
            pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos,
                                    "ambiguous module '%.*s' (candidates: ",
                                    (int)module_path.len,
                                    module_path.ptr ? module_path.ptr : "");
            for (size_t i = 0; i < cands_desc.len; i++) {
                if (!cands_desc.items[i]) continue;
                if (nonbundled_count > 0) {
                    const char *p = cands_abs.items[i];
                    bool is_bundled = bundled_emp_mods_abs && p && path_starts_with_ci_sep(p, bundled_emp_mods_abs);
                    if (is_bundled) continue;
                }
                if (pos + 4 >= sizeof(buf)) break;
                if (printed) pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos, "; ");
                pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos, "%s", cands_desc.items[i]);
                printed++;
            }
            (void)snprintf(buf + pos, sizeof(buf) - pos, ")");
            diagf_owned(arena, diags, span, "module: ", buf);
        }
    }

    strvec_free(&cands_abs);
    strvec_free(&cands_desc);
    return NULL;
}

static char *resolve_use_package_dir(
    EmpArena *arena,
    EmpDiags *diags,
    EmpSpan span,
    const EmpResolveBase *bases,
    size_t bases_len,
    EmpSlice module_path,
    const char *bundled_emp_mods_abs,
    const char *project_emp_mods_abs,
    bool emit_diags,
    bool *out_had_diag
) {
    if (out_had_diag) *out_had_diag = false;
    char *rel = module_slice_to_rel_path(module_path);
    if (!rel) return NULL;

    StrVec cands_abs;
    StrVec cands_desc;
    memset(&cands_abs, 0, sizeof(cands_abs));
    memset(&cands_desc, 0, sizeof(cands_desc));

    for (size_t i = 0; i < bases_len; i++) {
        resolve_add_dir_candidates(&cands_abs, &cands_desc, &bases[i], rel);
    }

    free(rel);

    size_t nonbundled_count = 0;
    size_t nonbundled_idx = 0;
    for (size_t i = 0; i < cands_abs.len; i++) {
        const char *p = cands_abs.items[i];
        if (!p) continue;
        bool is_bundled = bundled_emp_mods_abs && path_starts_with_ci_sep(p, bundled_emp_mods_abs);
        if (!is_bundled) {
            nonbundled_idx = i;
            nonbundled_count++;
        }
    }

    if (nonbundled_count == 1) {
        char *out = xstrdup(cands_abs.items[nonbundled_idx]);
        strvec_free(&cands_abs);
        strvec_free(&cands_desc);
        return out;
    }

    if (nonbundled_count == 0 && cands_abs.len == 1) {
        char *out = xstrdup(cands_abs.items[0]);
        if (out && bundled_emp_mods_abs && project_emp_mods_abs && path_starts_with_ci_sep(out, bundled_emp_mods_abs)) {
            char *vend = vendor_bundled_package_dir_to_project(out, bundled_emp_mods_abs, project_emp_mods_abs);
            if (vend) {
                free(out);
                out = vend;
            }
        }
        strvec_free(&cands_abs);
        strvec_free(&cands_desc);
        return out;
    }

    if (cands_abs.len > 1 && out_had_diag) *out_had_diag = true;

    if (emit_diags && cands_abs.len > 1) {
        char buf[4096];
        size_t pos = 0;
        size_t printed = 0;
        pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos,
                                "ambiguous package '%.*s' (directories: ",
                                (int)module_path.len,
                                module_path.ptr ? module_path.ptr : "");
        for (size_t i = 0; i < cands_desc.len; i++) {
            if (!cands_desc.items[i]) continue;
            if (nonbundled_count > 0) {
                const char *p = cands_abs.items[i];
                bool is_bundled = bundled_emp_mods_abs && p && path_starts_with_ci_sep(p, bundled_emp_mods_abs);
                if (is_bundled) continue;
            }
            if (pos + 4 >= sizeof(buf)) break;
            if (printed) pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos, "; ");
            pos += (size_t)snprintf(buf + pos, sizeof(buf) - pos, "%s", cands_desc.items[i]);
            printed++;
        }
        (void)snprintf(buf + pos, sizeof(buf) - pos, ")");
        diagf_owned(arena, diags, span, "module: ", buf);
    }

    strvec_free(&cands_abs);
    strvec_free(&cands_desc);
    return NULL;
}

static bool name_in_list(EmpSlice name, const EmpSlice *names, size_t names_len) {
    for (size_t i = 0; i < names_len; i++) {
        if (slice_eq(name, names[i])) return true;
    }
    return false;
}

static EmpItem *arena_make_fn_decl(EmpArena *a, const EmpItemFn *src_fn) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_FN;
    it->span = src_fn->span;
    it->as.fn.name = src_fn->name;
    it->as.fn.span = src_fn->span;
    it->as.fn.is_exported = false;
    it->as.fn.is_extern = src_fn->is_extern;
    it->as.fn.is_unsafe = src_fn->is_unsafe;
    it->as.fn.is_mm_only = src_fn->is_mm_only;
    // Preserve the signature for decls (used by typechecking and tooling).
    it->as.fn.ret_ty = src_fn->ret_ty;
    it->as.fn.body = NULL;
    emp_vec_init(&it->as.fn.params);

    for (size_t i = 0; i < src_fn->params.len; i++) {
        void *p = src_fn->params.items[i];
        if (!p) continue;
        (void)emp_vec_push(&it->as.fn.params, p);
    }
    return it;
}

static EmpItem *arena_make_class_decl(EmpArena *a, const EmpItemClass *src_cls) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_CLASS;
    it->span = src_cls->span;
    it->as.class_decl.name = src_cls->name;
    it->as.class_decl.span = src_cls->span;
    it->as.class_decl.is_exported = false;
    it->as.class_decl.base_name = src_cls->base_name;
    // Preserve fields + method signatures for decls, but drop method bodies.
    emp_vec_init(&it->as.class_decl.fields);
    for (size_t i = 0; i < src_cls->fields.len; i++) {
        void *f = src_cls->fields.items[i];
        if (!f) continue;
        (void)emp_vec_push(&it->as.class_decl.fields, f);
    }

    emp_vec_init(&it->as.class_decl.methods);
    for (size_t i = 0; i < src_cls->methods.len; i++) {
        const EmpClassMethod *m = (const EmpClassMethod *)src_cls->methods.items[i];
        if (!m) continue;
        EmpClassMethod *md = (EmpClassMethod *)emp_arena_alloc(a, sizeof(EmpClassMethod), sizeof(void *));
        if (!md) continue;
        memset(md, 0, sizeof(*md));
        md->name = m->name;
        md->is_init = m->is_init;
        md->is_exported = false;
        md->is_unsafe = m->is_unsafe;
        md->is_virtual = m->is_virtual;
        md->ret_ty = m->ret_ty;
        md->body = NULL;
        md->span = m->span;
        emp_vec_init(&md->params);
        for (size_t j = 0; j < m->params.len; j++) {
            void *p = m->params.items[j];
            if (!p) continue;
            (void)emp_vec_push(&md->params, p);
        }
        (void)emp_vec_push(&it->as.class_decl.methods, md);
    }
    return it;
}

static EmpItem *arena_make_trait_decl(EmpArena *a, const EmpItemTrait *src_tr) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_TRAIT;
    it->span = src_tr->span;
    it->as.trait_decl.name = src_tr->name;
    it->as.trait_decl.span = src_tr->span;
    it->as.trait_decl.is_exported = false;
    // Preserve method signatures for decls, but drop bodies.
    emp_vec_init(&it->as.trait_decl.methods);
    for (size_t i = 0; i < src_tr->methods.len; i++) {
        const EmpTraitMethod *m = (const EmpTraitMethod *)src_tr->methods.items[i];
        if (!m) continue;
        EmpTraitMethod *md = (EmpTraitMethod *)emp_arena_alloc(a, sizeof(EmpTraitMethod), sizeof(void *));
        if (!md) continue;
        memset(md, 0, sizeof(*md));
        md->name = m->name;
        md->ret_ty = m->ret_ty;
        md->body = NULL;
        md->span = m->span;
        emp_vec_init(&md->params);
        for (size_t j = 0; j < m->params.len; j++) {
            void *p = m->params.items[j];
            if (!p) continue;
            (void)emp_vec_push(&md->params, p);
        }
        (void)emp_vec_push(&it->as.trait_decl.methods, md);
    }
    return it;
}

static EmpItem *arena_make_const_decl(EmpArena *a, const EmpItemConst *src_c) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_CONST;
    it->span = src_c->span;
    it->as.const_decl.name = src_c->name;
    it->as.const_decl.span = src_c->span;
    it->as.const_decl.is_exported = false;
    it->as.const_decl.ty = src_c->ty;
    // Leave initializer empty for decl.
    it->as.const_decl.init = NULL;
    return it;
}

static EmpSlice item_decl_name(const EmpItem *it) {
    EmpSlice none;
    none.ptr = NULL;
    none.len = 0;
    if (!it) return none;
    if (it->kind == EMP_ITEM_FN) return it->as.fn.name;
    if (it->kind == EMP_ITEM_CLASS) return it->as.class_decl.name;
    if (it->kind == EMP_ITEM_TRAIT) return it->as.trait_decl.name;
    if (it->kind == EMP_ITEM_CONST) return it->as.const_decl.name;
    if (it->kind == EMP_ITEM_STRUCT) return it->as.struct_decl.name;
    if (it->kind == EMP_ITEM_ENUM) return it->as.enum_decl.name;
    return none;
}

static bool item_is_exported(const EmpItem *it) {
    if (!it) return false;
    if (it->kind == EMP_ITEM_FN) return it->as.fn.is_exported;
    if (it->kind == EMP_ITEM_CLASS) return it->as.class_decl.is_exported;
    if (it->kind == EMP_ITEM_TRAIT) return it->as.trait_decl.is_exported;
    if (it->kind == EMP_ITEM_CONST) return it->as.const_decl.is_exported;
    if (it->kind == EMP_ITEM_STRUCT) return it->as.struct_decl.is_exported;
    if (it->kind == EMP_ITEM_ENUM) return it->as.enum_decl.is_exported;
    return false;
}

static EmpItem *arena_make_struct_decl(EmpArena *a, const EmpItemStruct *src_st) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_STRUCT;
    it->span = src_st->span;
    it->as.struct_decl.name = src_st->name;
    it->as.struct_decl.span = src_st->span;
    it->as.struct_decl.is_exported = false;
    emp_vec_init(&it->as.struct_decl.fields);
    for (size_t i = 0; i < src_st->fields.len; i++) {
        void *f = src_st->fields.items[i];
        if (!f) continue;
        (void)emp_vec_push(&it->as.struct_decl.fields, f);
    }
    return it;
}

static EmpItem *arena_make_enum_decl(EmpArena *a, const EmpItemEnum *src_en) {
    EmpItem *it = (EmpItem *)emp_arena_alloc(a, sizeof(EmpItem), sizeof(void *));
    if (!it) return NULL;
    memset(it, 0, sizeof(*it));
    it->kind = EMP_ITEM_ENUM;
    it->span = src_en->span;
    it->as.enum_decl.name = src_en->name;
    it->as.enum_decl.span = src_en->span;
    it->as.enum_decl.is_exported = false;
    emp_vec_init(&it->as.enum_decl.variants);
    for (size_t i = 0; i < src_en->variants.len; i++) {
        void *v = src_en->variants.items[i];
        if (!v) continue;
        (void)emp_vec_push(&it->as.enum_decl.variants, v);
    }
    return it;
}

static EmpItem *arena_make_import_decl(EmpArena *a, const EmpItem *src_item) {
    if (!src_item) return NULL;
    if (src_item->kind == EMP_ITEM_FN) return arena_make_fn_decl(a, &src_item->as.fn);
    if (src_item->kind == EMP_ITEM_CLASS) return arena_make_class_decl(a, &src_item->as.class_decl);
    if (src_item->kind == EMP_ITEM_TRAIT) return arena_make_trait_decl(a, &src_item->as.trait_decl);
    if (src_item->kind == EMP_ITEM_CONST) return arena_make_const_decl(a, &src_item->as.const_decl);
    if (src_item->kind == EMP_ITEM_STRUCT) return arena_make_struct_decl(a, &src_item->as.struct_decl);
    if (src_item->kind == EMP_ITEM_ENUM) return arena_make_enum_decl(a, &src_item->as.enum_decl);
    return NULL;
}

static EmpSlice slice_from_cstr_in_arena(EmpArena *a, const char *s) {
    EmpSlice out;
    out.ptr = NULL;
    out.len = 0;
    if (!s) return out;
    size_t n = strlen(s);
    char *p = (char *)emp_arena_alloc(a, n, 1);
    if (!p) return out;
    memcpy(p, s, n);
    out.ptr = p;
    out.len = n;
    return out;
}

static void build_module_view_and_run(
    EmpModules *mods,
    EmpModule *m,
    const char *entry_dir_abs,
    const char *entry_root_abs,
    const char *bundled_emp_mods_abs,
    const char *project_emp_mods_abs
) {
    if (!m || !m->pr.program) return;

    const char *trace = getenv("EMP_TRACE");
    if (trace && trace[0]) {
        fprintf(stderr, "[trace] build_view: %s\n", m->path_abs ? m->path_abs : "<module>");
        fflush(stderr);
    }

    EmpProgram view;
    emp_vec_init(&view.items);

    const char *entry_emp_mods_abs = project_emp_mods_abs;

    // Deterministic module resolution bases.
    EmpResolveBase bases[4];
    memset(bases, 0, sizeof(bases));
    size_t bases_len = 0;
    if (m->dir_abs) bases[bases_len++] = (EmpResolveBase){"module_dir", m->dir_abs};
    if (entry_dir_abs && !bases_contains(bases, bases_len, entry_dir_abs)) bases[bases_len++] = (EmpResolveBase){"entry_dir", entry_dir_abs};
    if (entry_emp_mods_abs && !bases_contains(bases, bases_len, entry_emp_mods_abs)) bases[bases_len++] = (EmpResolveBase){"emp_mods", entry_emp_mods_abs};
    if (entry_root_abs && !bases_contains(bases, bases_len, entry_root_abs)) bases[bases_len++] = (EmpResolveBase){"entry_root", entry_root_abs};

    // Bundled stdlib modules next to emp.exe (lowest precedence).
    EmpResolveBase bases2[5];
    memset(bases2, 0, sizeof(bases2));
    memcpy(bases2, bases, bases_len * sizeof(EmpResolveBase));
    size_t bases2_len = bases_len;
    if (bundled_emp_mods_abs && dir_exists(bundled_emp_mods_abs) && !bases_contains(bases2, bases2_len, bundled_emp_mods_abs)) {
        bases2[bases2_len++] = (EmpResolveBase){"bundled", bundled_emp_mods_abs};
    }

    // Track names in scope to catch conflicts.
    EmpSlice *seen = NULL;
    size_t seen_len = 0;
    size_t seen_cap = 0;

    // Add local items first.
    //
    // The semantic pipeline (typecheck/ownership/borrows/drops/codegen) needs to see local `impl` blocks.
    // Only top-level declarations participate in the name-conflict tracking for imports.
    for (size_t i = 0; i < m->pr.program->items.len; i++) {
        EmpItem *it = (EmpItem *)m->pr.program->items.items[i];
        if (!it) continue;
        (void)emp_vec_push(&view.items, it);

        if (it->kind != EMP_ITEM_FN && it->kind != EMP_ITEM_CLASS && it->kind != EMP_ITEM_TRAIT && it->kind != EMP_ITEM_CONST && it->kind != EMP_ITEM_STRUCT && it->kind != EMP_ITEM_ENUM) continue;

        EmpSlice nm = item_decl_name(it);
        if (!nm.ptr || !nm.len) continue;
        if (seen_len + 1 > seen_cap) {
            size_t nc = seen_cap ? seen_cap * 2 : 32;
            EmpSlice *p = (EmpSlice *)realloc(seen, nc * sizeof(EmpSlice));
            if (!p) break;
            seen = p;
            seen_cap = nc;
        }
        seen[seen_len++] = nm;
    }

    // Resolve and add imported declarations.
    for (size_t i = 0; i < m->pr.program->items.len; i++) {
        EmpItem *it = (EmpItem *)m->pr.program->items.items[i];
        if (!it || it->kind != EMP_ITEM_USE) continue;

        const EmpItemUse *u = &it->as.use;

        if (trace && trace[0]) {
            fprintf(stderr,
                "[trace]  use: allow_private=%d wildcard=%d from='%.*s'\n",
                (int)u->allow_private,
                (int)u->wildcard,
                (int)u->from_path.len,
                u->from_path.ptr ? u->from_path.ptr : "");
            fflush(stderr);
        }

        // For wildcard imports, try resolving as a package directory first, else as a single module file.
        StrVec package_files;
        memset(&package_files, 0, sizeof(package_files));
        char *pkg_dir_abs = NULL;
        if (u->wildcard) {
            bool had = false;
            pkg_dir_abs = resolve_use_package_dir(&m->pr.arena, &m->pr.diags, it->span, bases2, bases2_len, u->from_path, bundled_emp_mods_abs, entry_emp_mods_abs, true, &had);
            if (had) {
                // Ambiguous package import; do not also try module-file resolution.
                strvec_free(&package_files);
                free(pkg_dir_abs);
                continue;
            }
            if (pkg_dir_abs) {
                package_files = list_em_files_in_dir(pkg_dir_abs);
            }
        }

        if (u->wildcard && pkg_dir_abs) {
            for (size_t fi = 0; fi < package_files.len; fi++) {
                EmpModule *target = modules_find(mods, package_files.items[fi]);
                if (!target || !target->pr.program) continue;
                for (size_t j = 0; j < target->pr.program->items.len; j++) {
                    EmpItem *sym = (EmpItem *)target->pr.program->items.items[j];
                    if (!sym) continue;
                    if (sym->kind != EMP_ITEM_FN && sym->kind != EMP_ITEM_CLASS && sym->kind != EMP_ITEM_TRAIT && sym->kind != EMP_ITEM_CONST && sym->kind != EMP_ITEM_STRUCT && sym->kind != EMP_ITEM_ENUM) continue;
                    if (!u->allow_private && !item_is_exported(sym)) continue;

                    EmpSlice sym_name = item_decl_name(sym);
                    if (!sym_name.ptr || !sym_name.len) continue;

                    if (name_in_list(sym_name, seen, seen_len)) {
                        diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "import: ", "name conflict for package wildcard import");
                        continue;
                    }
                    EmpItem *decl = arena_make_import_decl(&m->pr.arena, sym);
                    if (!decl) continue;
                    (void)emp_vec_push(&view.items, decl);
                    if (seen_len + 1 > seen_cap) {
                        size_t nc = seen_cap ? seen_cap * 2 : 32;
                        EmpSlice *p = (EmpSlice *)realloc(seen, nc * sizeof(EmpSlice));
                        if (!p) break;
                        seen = p;
                        seen_cap = nc;
                    }
                    seen[seen_len++] = sym_name;
                }
            }
        } else {
            bool had = false;
            char *target_abs = resolve_use_module_file(&m->pr.arena, &m->pr.diags, it->span, bases2, bases2_len, u->from_path, bundled_emp_mods_abs, entry_emp_mods_abs, true, &had);
            if (!target_abs) {
                strvec_free(&package_files);
                free(pkg_dir_abs);
                continue;
            }

            EmpModule *target = modules_find(mods, target_abs);
            free(target_abs);
            if (!target || !target->pr.program) {
                diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "module: ", "imported module failed to load");
                strvec_free(&package_files);
                free(pkg_dir_abs);
                continue;
            }

            if (u->wildcard) {
                for (size_t j = 0; j < target->pr.program->items.len; j++) {
                    EmpItem *sym = (EmpItem *)target->pr.program->items.items[j];
                    if (!sym) continue;
                    if (sym->kind != EMP_ITEM_FN && sym->kind != EMP_ITEM_CLASS && sym->kind != EMP_ITEM_TRAIT && sym->kind != EMP_ITEM_CONST && sym->kind != EMP_ITEM_STRUCT && sym->kind != EMP_ITEM_ENUM) continue;
                    if (!u->allow_private && !item_is_exported(sym)) continue;

                    EmpSlice sym_name = item_decl_name(sym);
                    if (!sym_name.ptr || !sym_name.len) continue;

                    if (name_in_list(sym_name, seen, seen_len)) {
                        diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "import: ", "name conflict for wildcard import");
                        continue;
                    }

                    EmpItem *decl = arena_make_import_decl(&m->pr.arena, sym);
                    if (!decl) continue;
                    (void)emp_vec_push(&view.items, decl);

                    if (seen_len + 1 > seen_cap) {
                        size_t nc = seen_cap ? seen_cap * 2 : 32;
                        EmpSlice *p = (EmpSlice *)realloc(seen, nc * sizeof(EmpSlice));
                        if (!p) break;
                        seen = p;
                        seen_cap = nc;
                    }
                    seen[seen_len++] = sym_name;
                }
            } else {
            for (size_t ni = 0; ni < u->names.len; ni++) {
                const EmpUseName *want = (const EmpUseName *)u->names.items[ni];
                if (!want) continue;

                bool found = false;
                for (size_t j = 0; j < target->pr.program->items.len; j++) {
                    EmpItem *sym = (EmpItem *)target->pr.program->items.items[j];
                    if (!sym) continue;
                    if (sym->kind != EMP_ITEM_FN && sym->kind != EMP_ITEM_CLASS && sym->kind != EMP_ITEM_TRAIT && sym->kind != EMP_ITEM_CONST && sym->kind != EMP_ITEM_STRUCT && sym->kind != EMP_ITEM_ENUM) continue;
                    EmpSlice sym_name = item_decl_name(sym);
                    if (!slice_eq(sym_name, want->name)) continue;
                    found = true;
                    if (!u->allow_private && !item_is_exported(sym)) {
                        diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "import: ", "symbol is not exported");
                        break;
                    }
                    EmpSlice final_name = want->alias.len ? want->alias : sym_name;
                    if (name_in_list(final_name, seen, seen_len)) {
                        diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "import: ", "name conflict for imported symbol");
                        break;
                    }
                    EmpItem *decl = arena_make_import_decl(&m->pr.arena, sym);
                    if (!decl) break;
                    if (want->alias.len) {
                        // Create a stable copy of the alias name in the module arena.
                        char *tmp = (char *)malloc(want->alias.len + 1);
                        if (tmp) {
                            memcpy(tmp, want->alias.ptr, want->alias.len);
                            tmp[want->alias.len] = '\0';
                            EmpSlice ali = slice_from_cstr_in_arena(&m->pr.arena, tmp);
                            if (decl->kind == EMP_ITEM_FN) decl->as.fn.name = ali;
                            if (decl->kind == EMP_ITEM_CLASS) decl->as.class_decl.name = ali;
                            if (decl->kind == EMP_ITEM_TRAIT) decl->as.trait_decl.name = ali;
                            if (decl->kind == EMP_ITEM_CONST) decl->as.const_decl.name = ali;
                            if (decl->kind == EMP_ITEM_STRUCT) decl->as.struct_decl.name = ali;
                            if (decl->kind == EMP_ITEM_ENUM) decl->as.enum_decl.name = ali;
                            free(tmp);
                        }
                    }
                    (void)emp_vec_push(&view.items, decl);
                    if (seen_len + 1 > seen_cap) {
                        size_t nc = seen_cap ? seen_cap * 2 : 32;
                        EmpSlice *p = (EmpSlice *)realloc(seen, nc * sizeof(EmpSlice));
                        if (!p) break;
                        seen = p;
                        seen_cap = nc;
                    }
                    seen[seen_len++] = item_decl_name(decl);
                    break;
                }
                if (!found) {
                    diagf_owned(&m->pr.arena, &m->pr.diags, it->span, "import: ", "symbol not found in target module");
                }
            }
            }
        }

        strvec_free(&package_files);
        free(pkg_dir_abs);
    }

    free(seen);
    (void)entry_emp_mods_abs;

    // Run existing semantic pipeline in module context.
    if (trace && trace[0]) {
        fprintf(stderr, "[trace]  sem: typecheck\n");
        fflush(stderr);
    }

    emp_sem_lower_defer(&m->pr.arena, &view, &m->pr.diags);
    emp_sem_typecheck(&m->pr.arena, &view, &m->pr.diags);
    if (trace && trace[0]) {
        fprintf(stderr, "[trace]  sem: ownership\n");
        fflush(stderr);
    }
    emp_sem_check_ownership(&m->pr.arena, &view, &m->pr.diags);
    if (trace && trace[0]) {
        fprintf(stderr, "[trace]  sem: borrows\n");
        fflush(stderr);
    }
    emp_sem_check_borrows_lexical(&m->pr.arena, &view, &m->pr.diags);
    if (trace && trace[0]) {
        fprintf(stderr, "[trace]  sem: drops\n");
        fflush(stderr);
    }
    emp_sem_insert_drops(&m->pr.arena, &view, &m->pr.diags);
    if (trace && trace[0]) {
        fprintf(stderr, "[trace]  sem: done\n");
        fflush(stderr);
    }

    emp_vec_free(&view.items);
}

static int ends_with(const char *s, const char *suffix) {
    size_t sl = strlen(s);
    size_t tl = strlen(suffix);
    if (sl < tl) return 0;
    return memcmp(s + (sl - tl), suffix, tl) == 0;
}

static int has_any_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    const char *slash1 = strrchr(path, '/');
    const char *slash2 = strrchr(path, '\\');
    const char *slash = slash1;
    if (!slash || (slash2 && slash2 > slash)) slash = slash2;
    if (!dot) return 0;
    if (slash && dot < slash) return 0;
    return dot[1] != '\0';
}

static char *with_em_extension(const char *path) {
    if (has_any_extension(path)) {
        return NULL;
    }
    size_t n = strlen(path);
    char *out = (char *)malloc(n + 4 + 1);
    if (!out) return NULL;
    memcpy(out, path, n);
    memcpy(out + n, ".em", 4);
    out[n + 3] = '\0';
    return out;
}

static char *read_entire_file(const char *path, size_t *out_len) {
    *out_len = 0;

    FILE *f = NULL;
#ifdef _WIN32
    if (fopen_s(&f, path, "rb") != 0) f = NULL;
#else
    f = fopen(path, "rb");
#endif
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    size_t len = (size_t)sz;
    char *buf = (char *)malloc(len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t nread = fread(buf, 1, len, f);
    fclose(f);
    if (nread != len) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    *out_len = len;
    return buf;
}

static void strip_markdown_fence_in_place(char *buf, size_t *len_io) {
    size_t len = *len_io;
    if (!buf || len < 3) return;

    // Only handle the common pattern:
    // ```lang\n<code>\n```
    if (!(buf[0] == '`' && buf[1] == '`' && buf[2] == '`')) return;

    // Find end of opening fence line.
    size_t open_line_end = 0;
    while (open_line_end < len && buf[open_line_end] != '\n' && buf[open_line_end] != '\r') {
        open_line_end++;
    }
    if (open_line_end >= len) return;

    size_t start = open_line_end;
    if (buf[start] == '\r' && start + 1 < len && buf[start + 1] == '\n') {
        start += 2;
    } else if (buf[start] == '\n' || buf[start] == '\r') {
        start += 1;
    }
    if (start >= len) return;

    // Find a closing fence at the start of a line.
    size_t end = len;
    for (size_t i = len; i-- > start;) {
        if (buf[i] != '`') continue;
        if (i + 2 >= len) continue;
        if (!(buf[i] == '`' && buf[i + 1] == '`' && buf[i + 2] == '`')) continue;
        if (i == 0 || buf[i - 1] == '\n' || buf[i - 1] == '\r') {
            end = i;
            break;
        }
    }
    if (end == len) return;

    // Trim trailing whitespace/newlines before the closing fence.
    while (end > start && (buf[end - 1] == ' ' || buf[end - 1] == '\t' || buf[end - 1] == '\n' || buf[end - 1] == '\r')) {
        end--;
    }

    if (end <= start) {
        buf[0] = '\0';
        *len_io = 0;
        return;
    }

    size_t new_len = end - start;
    memmove(buf, buf + start, new_len);
    buf[new_len] = '\0';
    *len_io = new_len;
}

static void print_token(EmpToken t) {
    printf("%4u:%-4u  %-18s  [%zu..%zu]  ",
           (unsigned)t.span.line,
           (unsigned)t.span.col,
           emp_token_kind_name(t.kind),
           t.span.start,
           t.span.end);

    if (t.kind == EMP_TOK_ERROR) {
        printf("error=%s", emp_lex_error_kind_name(t.error.kind));
        if (t.error.found) printf(" found='%c'", t.error.found);
        printf("\n");
        return;
    }

    // Print lexeme (escaped-ish for readability)
    putchar('`');
    for (size_t i = 0; i < t.lexeme.len; i++) {
        unsigned char ch = (unsigned char)t.lexeme.ptr[i];
        if (ch == '\n') {
            fputs("\\n", stdout);
        } else if (ch == '\r') {
            fputs("\\r", stdout);
        } else if (ch == '\t') {
            fputs("\\t", stdout);
        } else if (ch == '`') {
            fputs("\\`", stdout);
        } else if (ch < 32 || ch == 127) {
            printf("\\x%02X", (unsigned)ch);
        } else {
            putchar((int)ch);
        }
    }
    putchar('`');
    putchar('\n');
}

typedef enum EmpMode {
    EMP_MODE_AST,
    EMP_MODE_JSON,
    EMP_MODE_LEX,
    EMP_MODE_LL,
} EmpMode;

static void print_usage(const char *exe) {
    fprintf(stderr,
            "Usage: %s [--ast|--json|--lex|--ll] [--out file] [--nobin] [file.em]\n"
            "\n"
            "  (default) With a file input, EMP builds a .exe via LLVM\n"
            "\n"
            "  --ast   Parse and print AST\n"
            "  --json  Parse and emit AST+diags as JSON\n"
            "  --lex   Only run the lexer token dump\n"
            "  --ll    Emit LLVM IR (requires LLVM build; implies --nobin unless you set --out to .ll)\n"
            "  --nobin Do not produce a .exe; emit LLVM IR instead\n"
            "  --out   Output path: .exe by default; .ll when using --nobin\n"
            "\n"
            "Notes:\n"
            "  - EMP source files use the .em extension\n"
            "  - If you pass a path with no extension, .em is appended\n",
            exe);
}

static void print_diags(const EmpDiags *d) {
    for (size_t i = 0; i < d->len; i++) {
        const EmpDiag *x = &d->items[i];
        fprintf(stderr,
                "%u:%u  [%zu..%zu]  %s\n",
                (unsigned)x->span.line,
                (unsigned)x->span.col,
                x->span.start,
                x->span.end,
                x->message ? x->message : "<diag>");
    }
}

int main(int argc, char **argv) {
    const char *dump_args = getenv("EMP_DUMP_ARGS");
    if (dump_args && dump_args[0]) {
        fprintf(stderr, "[args] argc=%d\n", argc);
        for (int i = 0; i < argc; i++) {
            fprintf(stderr, "[args] argv[%d]=%s\n", i, argv[i] ? argv[i] : "<null>");
        }
        fflush(stderr);
    }

    if (argc >= 2 && strcmp(argv[1], "new") == 0) {
        return cmd_new(argc, argv);
    }

    const char *path = NULL;
    char *path_owned = NULL;
    const char *out_path = NULL;
    FILE *out = stdout;
    char *owned = NULL;
    const char *src = NULL;
    size_t len = 0;

    EmpMode mode = EMP_MODE_AST;
    bool mode_explicit = false;
    bool nobin = false;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--ast") == 0) {
            mode = EMP_MODE_AST;
            mode_explicit = true;
        } else if (strcmp(a, "--json") == 0) {
            mode = EMP_MODE_JSON;
            mode_explicit = true;
        } else if (strcmp(a, "--lex") == 0) {
            mode = EMP_MODE_LEX;
            mode_explicit = true;
        } else if (strcmp(a, "--ll") == 0 || strcmp(a, "--ir") == 0) {
            mode = EMP_MODE_LL;
            mode_explicit = true;
            nobin = true;
        } else if (strcmp(a, "--nobin") == 0) {
            nobin = true;
        } else if (strcmp(a, "--out") == 0 || strcmp(a, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --out\n");
                print_usage(argv[0]);
                return 2;
            }
            out_path = argv[++i];
        } else if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (a[0] == '-') {
            fprintf(stderr, "Unknown flag: %s\n", a);
            print_usage(argv[0]);
            return 2;
        } else {
            path = a;
        }
    }

    // Default behavior: if the user passed a file and didn't explicitly choose a mode,
    // compile to a native executable via LLVM.
    if (path && !mode_explicit) {
        mode = EMP_MODE_LL;
    }

    // Open output stream only for non-binary outputs.
    bool wants_binary = (path != NULL) && (mode == EMP_MODE_LL) && !nobin;
    if (out_path && !wants_binary) {
        out = NULL;
    #ifdef _WIN32
        if (fopen_s(&out, out_path, "wb") != 0) out = NULL;
    #else
        out = fopen(out_path, "wb");
    #endif
        if (!out) {
            fprintf(stderr, "Failed to open output file: %s\n", out_path);
            return 1;
        }
    }

    if (path) {
        path_owned = with_em_extension(path);
        if (path_owned) {
            path = path_owned;
        } else if (!ends_with(path, ".em")) {
            fprintf(stderr, "Warning: input file is not .em: %s\n", path);
        }
    }

    if (!path) {
        static const char sample[] =
            "fn pair(auto a, auto b) -> (int c, int d) {\n"
            "  return (a, b);\n"
            "}\n"
            "\n"
            "fn main() {\n"
            "  auto sum;\n"
            "  int32 sum32 = 0;\n"
            "  int[] data;\n"
            "\n"
            "  for _, val in data {\n"
            "    sum32 += val;\n"
            "    sum32 += data[_];\n"
            "  }\n"
            "\n"
            "  let t = pair(1, 2);\n"
            "  return;\n"
            "}\n";
        src = sample;
        len = sizeof(sample) - 1;
    } else if (mode == EMP_MODE_LEX || mode == EMP_MODE_LL) {
        owned = read_entire_file(path, &len);
        if (!owned) {
            fprintf(stderr, "Failed to read file: %s\n", path);
            return 1;
        }
        strip_markdown_fence_in_place(owned, &len);
        src = owned;
    }

    int exit_code = 0;

    if (mode == EMP_MODE_LEX) {
        EmpLexer lex = emp_lexer_new(src, len);
        int error_count = 0;
        for (;;) {
            EmpToken t = emp_lexer_next(&lex);
            print_token(t);
            if (t.kind == EMP_TOK_EOF) break;
            if (t.kind == EMP_TOK_ERROR) error_count++;
        }
        exit_code = error_count == 0 ? 0 : 1;
    } else if (!path) {
        EmpParseResult r = emp_parse(src, len);

        // Semantics (phase -1): lower `defer { ... }` to explicit scope-exit statements.
        emp_sem_lower_defer(&r.arena, r.program, &r.diags);

        // Semantics (phase 0): type checking / minimal inference.
        emp_sem_typecheck(&r.arena, r.program, &r.diags);

        // Semantics (phase 1): ownership-only checking (no borrow checking yet).
        emp_sem_check_ownership(&r.arena, r.program, &r.diags);

        // Semantics (phase 2): lexical borrow checking (shared vs mutable).
        emp_sem_check_borrows_lexical(&r.arena, r.program, &r.diags);

        // Semantics (phase 3): drop insertion (explicit drops at scope ends / returns).
        emp_sem_insert_drops(&r.arena, r.program, &r.diags);

        if (mode == EMP_MODE_LL) {
    #ifdef EMP_HAVE_LLVM
            bool ok = emp_codegen_emit_llvm_ir(&r.arena, r.program, &r.diags, "emp", out);
            (void)ok;
    #else
            fprintf(stderr, "LLVM backend not enabled in this build. Reconfigure/build with the x64 preset and LLVM available.\n");
            r.diags.len = r.diags.len ? r.diags.len : 1;
    #endif
        } else if (mode == EMP_MODE_JSON) {
            // When stdout is redirected, MSVC treats it as fully-buffered.
            // The JSON writer does many small writes; avoid CRT buffering issues by disabling buffering.
            setvbuf(out, NULL, _IONBF, 0);
            emp_program_to_json(out, r.program, &r.diags);
        } else {
            if (r.diags.len) {
                fputs("Diagnostics:\n", stderr);
                print_diags(&r.diags);
                fputs("\n", stderr);
            }
            emp_program_print(r.program);
        }

        exit_code = r.diags.len == 0 ? 0 : 1;
        emp_parse_result_free(&r);
    } else {
        // Multi-module mode: load entry file and its transitive dependencies via `use`.
        EmpModules mods;
        modules_init(&mods);

        const char *trace = getenv("EMP_TRACE");

        char *entry_abs = abs_path_dup(path);
        char *entry_dir = entry_abs ? path_dirname_dup(entry_abs) : NULL;
        char *entry_root = entry_dir ? path_parent_dir_dup(entry_dir) : NULL;
        char *entry_emp_mods = entry_root ? path_join2(entry_root, "emp_mods") : NULL;

        // Bundled stdlib lives next to emp.exe in <exe_dir>/stdlib/emp_mods.
        char *exe_dir = get_exe_dir_abs();
        char *bundled_stdlib = exe_dir ? path_join2(exe_dir, "stdlib") : NULL;
        char *bundled_emp_mods = bundled_stdlib ? path_join2(bundled_stdlib, "emp_mods") : NULL;

        // Dev/test convenience: when running from the repo root, prefer the checked-in
        // stdlib at ./stdlib/emp_mods over the copied snapshot next to emp.exe.
        // This keeps edits to stdlib modules immediately visible to the compiler.
        char *cwd_emp_mods = abs_path_dup("stdlib/emp_mods");
        if (cwd_emp_mods && dir_exists(cwd_emp_mods)) {
            free(bundled_emp_mods);
            bundled_emp_mods = cwd_emp_mods;
        } else {
            free(cwd_emp_mods);
        }

        EmpModule *entry = NULL;
        if (entry_abs) entry = load_module(&mods, entry_abs);
        if (!entry) {
            fprintf(stderr, "Failed to read/parse entry module: %s\n", path);
            free(entry_abs);
            free(entry_dir);
            modules_free(&mods);
            return 1;
        }

        // Load dependencies.
        // NOTE: `load_module()` may push into `mods.items` and trigger a `realloc`,
        // which invalidates any previously taken pointers into `mods.items`.
        for (size_t mi = 0; mi < mods.len; mi++) {
            if (!mods.items[mi].pr.program) continue;

            // Re-acquire the module pointer as needed; do not keep it across `load_module()`.
            for (size_t ii = 0; ii < mods.items[mi].pr.program->items.len; ii++) {
                EmpModule *m = &mods.items[mi];
                EmpItem *it = (EmpItem *)m->pr.program->items.items[ii];
                if (!it || it->kind != EMP_ITEM_USE) continue;
                const EmpItemUse *u = &it->as.use;

                if (trace && trace[0]) {
                    fprintf(stderr,
                        "[trace] dep: in %s use wildcard=%d from='%.*s'\n",
                        m->path_abs ? m->path_abs : "<module>",
                        (int)u->wildcard,
                        (int)u->from_path.len,
                        u->from_path.ptr ? u->from_path.ptr : "");
                    fflush(stderr);
                }

                // Package wildcard: load all modules in the folder.
                if (u->wildcard) {
                    EmpResolveBase bases[5];
                    memset(bases, 0, sizeof(bases));
                    size_t bases_len = 0;
                    if (m->dir_abs) bases[bases_len++] = (EmpResolveBase){"module_dir", m->dir_abs};
                    if (entry_dir && !bases_contains(bases, bases_len, entry_dir)) bases[bases_len++] = (EmpResolveBase){"entry_dir", entry_dir};
                    if (entry_emp_mods && !bases_contains(bases, bases_len, entry_emp_mods)) bases[bases_len++] = (EmpResolveBase){"emp_mods", entry_emp_mods};
                    if (entry_root && !bases_contains(bases, bases_len, entry_root)) bases[bases_len++] = (EmpResolveBase){"entry_root", entry_root};
                    if (bundled_emp_mods && dir_exists(bundled_emp_mods) && !bases_contains(bases, bases_len, bundled_emp_mods)) bases[bases_len++] = (EmpResolveBase){"bundled", bundled_emp_mods};

                    bool ambiguous_pkg = false;
                    char *pkg_dir_abs = resolve_use_package_dir(&m->pr.arena, &m->pr.diags, it->span, bases, bases_len, u->from_path, bundled_emp_mods, entry_emp_mods, false, &ambiguous_pkg);
                    if (ambiguous_pkg) {
                        free(pkg_dir_abs);
                        continue;
                    }
                    if (pkg_dir_abs) {
                        if (trace && trace[0]) {
                            fprintf(stderr, "[trace] dep: wildcard dir -> %s\n", pkg_dir_abs);
                            fflush(stderr);
                        }
                        StrVec files = list_em_files_in_dir(pkg_dir_abs);
                        for (size_t fi = 0; fi < files.len; fi++) {
                            if (!modules_find(&mods, files.items[fi])) {
                                if (trace && trace[0]) {
                                    fprintf(stderr, "[trace] dep: load %s\n", files.items[fi]);
                                    fflush(stderr);
                                }
                                EmpModule *dep = load_module(&mods, files.items[fi]);
                                // `mods.items` may have moved; re-acquire `m` before emitting diags.
                                m = &mods.items[mi];
                                (void)dep;
                            }
                        }
                        strvec_free(&files);
                        free(pkg_dir_abs);
                        continue;
                    }
                    free(pkg_dir_abs);
                }

                EmpResolveBase bases[5];
                memset(bases, 0, sizeof(bases));
                size_t bases_len = 0;
                if (m->dir_abs) bases[bases_len++] = (EmpResolveBase){"module_dir", m->dir_abs};
                if (entry_dir && !bases_contains(bases, bases_len, entry_dir)) bases[bases_len++] = (EmpResolveBase){"entry_dir", entry_dir};
                if (entry_emp_mods && !bases_contains(bases, bases_len, entry_emp_mods)) bases[bases_len++] = (EmpResolveBase){"emp_mods", entry_emp_mods};
                if (entry_root && !bases_contains(bases, bases_len, entry_root)) bases[bases_len++] = (EmpResolveBase){"entry_root", entry_root};
                if (bundled_emp_mods && dir_exists(bundled_emp_mods) && !bases_contains(bases, bases_len, bundled_emp_mods)) bases[bases_len++] = (EmpResolveBase){"bundled", bundled_emp_mods};

                char *target_abs = resolve_use_module_file(&m->pr.arena, &m->pr.diags, it->span, bases, bases_len, u->from_path, bundled_emp_mods, entry_emp_mods, false, NULL);
                if (!target_abs) continue;
                if (trace && trace[0]) {
                    fprintf(stderr, "[trace] dep: resolved -> %s\n", target_abs);
                    fflush(stderr);
                }
                if (!modules_find(&mods, target_abs)) {
                    if (trace && trace[0]) {
                        fprintf(stderr, "[trace] dep: load %s\n", target_abs);
                        fflush(stderr);
                    }
                    EmpModule *dep = load_module(&mods, target_abs);
                    // `mods.items` may have moved; re-acquire `m` before emitting diags.
                    m = &mods.items[mi];
                    (void)dep;
                }
                free(target_abs);
            }
        }

        // `entry` pointer may have been invalidated by dependency loads (realloc).
        // Re-acquire it by absolute path before later use.
        entry = modules_find(&mods, entry_abs);
        if (!entry) {
            fprintf(stderr, "Internal error: entry module missing after dependency load\n");
            free(entry_abs);
            free(entry_dir);
            modules_free(&mods);
            return 1;
        }

        // Run semantics in each module with imports in scope.
        for (size_t mi = 0; mi < mods.len; mi++) {
            build_module_view_and_run(&mods, &mods.items[mi], entry_dir, entry_root, bundled_emp_mods, entry_emp_mods);
        }

        free(bundled_emp_mods);
        free(bundled_stdlib);
        free(exe_dir);

        // `entry` pointer could be invalidated by the semantic loop if it triggered any
        // module pushes in the future; keep this robust.
        entry = modules_find(&mods, entry_abs);
        if (!entry) {
            fprintf(stderr, "Internal error: entry module missing before output\n");
            free(entry_abs);
            free(entry_dir);
            modules_free(&mods);
            return 1;
        }

        // Merge diags with module path prefix.
        EmpDiags merged;
        emp_diags_init(&merged);
        for (size_t mi = 0; mi < mods.len; mi++) {
            EmpModule *m = &mods.items[mi];
            char prefix[4096];
            const char *mpath = m->path_abs ? m->path_abs : "<module>";
            snprintf(prefix, sizeof(prefix), "[%s] ", mpath);
            for (size_t di = 0; di < m->pr.diags.len; di++) {
                const EmpDiag *d = &m->pr.diags.items[di];
                diagf_owned(&entry->pr.arena, &merged, d->span, prefix, d->message ? d->message : "<diag>");
            }
        }

        if (mode == EMP_MODE_LL) {
            if (merged.len) {
                fputs("Diagnostics:\n", stderr);
                print_diags(&merged);
                exit_code = 1;
            } else {
#ifdef EMP_HAVE_LLVM
                // Merge module items into one flat program for now.
                // NOTE: This is sufficient for the current std + app workflow (no namespacing yet).
                EmpProgram merged_program;
                memset(&merged_program, 0, sizeof(merged_program));
                for (size_t mi = 0; mi < mods.len; mi++) {
                    EmpModule *m = &mods.items[mi];
                    if (!m->pr.program) continue;
                    for (size_t ii = 0; ii < m->pr.program->items.len; ii++) {
                        void *it = m->pr.program->items.items[ii];
                        if (!it) continue;
                        (void)emp_vec_push(&merged_program.items, it);
                    }
                }

                if (nobin) {
                    // IR output (file or stdout)
                    bool ok_ir = emp_codegen_emit_llvm_ir(&entry->pr.arena, &merged_program, &merged, path ? path : "emp", out);
                    if (!ok_ir || merged.len) {
                        if (merged.len) {
                            fputs("Diagnostics:\n", stderr);
                            print_diags(&merged);
                        }
                        exit_code = 1;
                    }
                } else {
                    (void)ensure_dir("out");
                    (void)ensure_dir("out/bin");

                    char *base = path_basename_noext(path);
                    char *ll_path = NULL;
                    char *obj_path = NULL;
                    char *exe_path = NULL;

#ifdef _WIN32
                    if (out_path) exe_path = xstrdup(out_path);
                    else {
                        char tmp[MAX_PATH];
                        snprintf(tmp, sizeof(tmp), "out\\bin\\%s.exe", base ? base : "emp");
                        exe_path = xstrdup(tmp);
                    }
                    {
                        char tmp[MAX_PATH];
                        snprintf(tmp, sizeof(tmp), "out\\%s.ll", base ? base : "emp");
                        ll_path = xstrdup(tmp);
                        snprintf(tmp, sizeof(tmp), "out\\%s.obj", base ? base : "emp");
                        obj_path = xstrdup(tmp);
                    }
#else
                    if (out_path) exe_path = xstrdup(out_path);
                    else {
                        char tmp[PATH_MAX];
                        snprintf(tmp, sizeof(tmp), "out/bin/%s", base ? base : "emp");
                        exe_path = xstrdup(tmp);
                    }
                    {
                        char tmp[PATH_MAX];
                        snprintf(tmp, sizeof(tmp), "out/%s.ll", base ? base : "emp");
                        ll_path = xstrdup(tmp);
                        snprintf(tmp, sizeof(tmp), "out/%s.o", base ? base : "emp");
                        obj_path = xstrdup(tmp);
                    }
#endif

                    FILE *irf = NULL;
#ifdef _WIN32
                    if (ll_path) fopen_s(&irf, ll_path, "wb");
#else
                    if (ll_path) irf = fopen(ll_path, "wb");
#endif
                    if (!irf) {
                        fprintf(stderr, "Failed to open IR temp file: %s\n", ll_path ? ll_path : "<null>");
                        exit_code = 1;
                    } else {
                        bool ok_ir = emp_codegen_emit_llvm_ir(&entry->pr.arena, &merged_program, &merged, path ? path : "emp", irf);
                        fclose(irf);
                        if (!ok_ir || merged.len) {
                            if (merged.len) {
                                fputs("Diagnostics:\n", stderr);
                                print_diags(&merged);
                            }
                            exit_code = 1;
                        }
                    }

                    if (exit_code == 0) {
#ifdef _WIN32
#ifndef EMP_LLVM_ROOT
#define EMP_LLVM_ROOT "llvm-21.1.8-windows-amd64-msvc17-msvcrt"
#endif
                        char llc_exe[MAX_PATH];
                        char lld_exe[MAX_PATH];
                        snprintf(llc_exe, sizeof(llc_exe), "%s\\bin\\llc.exe", EMP_LLVM_ROOT);
                        snprintf(lld_exe, sizeof(lld_exe), "%s\\bin\\lld-link.exe", EMP_LLVM_ROOT);

                        if (!file_exists(llc_exe) || !file_exists(lld_exe)) {
                            fprintf(stderr, "LLVM tools missing (llc.exe/lld-link.exe). Check EMP_LLVM_ROOT.\n");
                            exit_code = 1;
                        } else {
                            const char *llc_args[] = {llc_exe, "-filetype=obj", "-o", obj_path, ll_path, NULL};
                            intptr_t rc1 = spawn_wait(llc_exe, llc_args);
                            if (rc1 != 0) {
                                fprintf(stderr, "llc failed with code %lld\n", (long long)rc1);
                                exit_code = 1;
                            } else {
                                char *um_x64 = find_windows_kits_um_x64();
                                if (!um_x64) {
                                    fprintf(stderr, "Failed to locate Windows SDK libs (kernel32.lib). Install Windows 10 SDK via VS Installer.\n");
                                    exit_code = 1;
                                } else {
                                    char outarg[MAX_PATH * 2];
                                    char libpatharg[MAX_PATH * 2];
                                    char entryarg[128];
                                    snprintf(outarg, sizeof(outarg), "/OUT:%s", exe_path);
                                    snprintf(libpatharg, sizeof(libpatharg), "/LIBPATH:%s", um_x64);
                                    snprintf(entryarg, sizeof(entryarg), "/ENTRY:mainCRTStartup");
                                    const char *link_args[] = {lld_exe, "/NOLOGO", "/SUBSYSTEM:CONSOLE", entryarg, outarg, libpatharg, obj_path, "kernel32.lib", NULL};
                                    intptr_t rc2 = spawn_wait(lld_exe, link_args);
                                    if (rc2 != 0) {
                                        fprintf(stderr, "lld-link failed with code %lld\n", (long long)rc2);
                                        exit_code = 1;
                                    }
                                    free(um_x64);
                                }
                            }
                        }
#else
                        char llc[PATH_MAX];
                        char lld[PATH_MAX];
                        const char *llc_exe = NULL;
                        const char *lld_exe = NULL;
                        if (find_in_path("llc", llc, sizeof(llc))) llc_exe = llc;
                        if (find_in_path("ld.lld", lld, sizeof(lld))) lld_exe = lld;
                        else if (find_in_path("lld", lld, sizeof(lld))) lld_exe = lld;

                        if (!llc_exe || !lld_exe) {
                            fprintf(stderr, "LLVM tools missing (llc and ld.lld/lld). Install llvm/lld on Ubuntu.\n");
                            exit_code = 1;
                        } else {
                            const char *llc_args[] = {llc_exe, "-filetype=obj", "-relocation-model=pic", "-o", obj_path, ll_path, NULL};
                            intptr_t rc1 = spawn_wait(llc_exe, llc_args);
                            if (rc1 != 0) {
                                fprintf(stderr, "llc failed with code %lld\n", (long long)rc1);
                                exit_code = 1;
                            } else {
                                const char *link_args[] = {lld_exe, "-pie", "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "/usr/lib/x86_64-linux-gnu/Scrt1.o", "/usr/lib/x86_64-linux-gnu/crti.o", obj_path, "-lc", "/usr/lib/x86_64-linux-gnu/crtn.o", "-o", exe_path, NULL};
                                intptr_t rc2 = spawn_wait(lld_exe, link_args);
                                if (rc2 != 0) {
                                    fprintf(stderr, "lld failed with code %lld\n", (long long)rc2);
                                    exit_code = 1;
                                }
                            }
                        }
#endif
                    }

                    free(base);
                    free(ll_path);
                    free(obj_path);
                    free(exe_path);
                }

                emp_vec_free(&merged_program.items);
#else
                fprintf(stderr, "LLVM backend not enabled in this build. Reconfigure/build with the x64 preset and LLVM available.\n");
                exit_code = 1;
#endif
            }
        } else if (mode == EMP_MODE_JSON) {
            setvbuf(out, NULL, _IONBF, 0);
            emp_program_to_json(out, entry->pr.program, &merged);
        } else {
            // Like JSON, keep debug output unbuffered to avoid losing output on crashes.
            setvbuf(out, NULL, _IONBF, 0);
            if (merged.len) {
                fputs("Diagnostics:\n", stderr);
                print_diags(&merged);
                fputs("\n", stderr);
            }
            emp_program_print(entry->pr.program);
        }

        if (mode != EMP_MODE_LL) {
            exit_code = merged.len == 0 ? 0 : 1;
        }
        emp_diags_free(&merged);
        free(entry_emp_mods);
        free(entry_abs);
        free(entry_dir);
        free(entry_root);
        modules_free(&mods);
    }

    free(owned);
    free(path_owned);
    if (out && out != stdout) fclose(out);
    return exit_code;
}

#include "moonfruit.h"

#include <termios.h>
#include <sys/ioctl.h>

#define BASE_ENTRY_POINT
#include "base.h"

//#define func2(x) ((6 * func(2*(x), 3)))
//#undef func

//#define func(a, b) (2 + 6 / (a + b) % func3(124) - 23)

#define should_not_have_params (u64)param
#define with_params(a, b, c) (a+b+c)
#define no_params()

#define func2(a) a / 7
#define func3(z) func2(z) * 10
#define func(y) (func3(y) + 3) * (3 no_params())

#define my_exponent1 4e+3
#define my_exponent2 4e3
#define my_exponent3 4p-3
#define my_exp 4p-3
#define my_exponent24 4p-3
#define my_expander 123

#define num .4e-3
#define bad_exponent 3tone3 // tokenizer doesn't throw an error while using gcc

#define MULTIPLE_LINES 0, \
                       1, \
                       2

const u8 test[] = "#define skipping this string literal too";

void func_w_va_args(...) {

}

typedef struct {
    i32  argc;
    u8 **argv;
} MainArgs;

void* parallel_main(void* main_args) {
    i32 argc = ((MainArgs*)main_args)->argc;
    u8** argv = ((MainArgs*)main_args)->argv;

    if (argc < 2) {
        ThreadExit(NULL);
    }

    MoonFruit_ChunkQueue* chunk_Q = 0L;
    MoonFruit_File* f = 0L;
    if (LaneIdx() == 0) {
        Arena* Q_arena = default_arena();
        String path = String(argv[1]);
        f = moonfruit_file_create_and_open(thread_ctx.shared_arena, path);

        chunk_Q = moonfruit_chunk_queue_create(Q_arena, LaneCount() * 2);
        mutex_assign(&chunk_Q->mutex);
        for (u64 i = 0; i < LaneCount(); i++) {
            moonfruit_file_push_next_chunk(f, chunk_Q);
        }
    }
    LaneSyncPtr(chunk_Q, 0);
    LaneSyncPtr(f, 0);

    for (; moonfruit_chunk_queue_size(chunk_Q) > 0;) {
        MoonFruit_Chunk chunk = moonfruit_chunk_queue_pop(chunk_Q);
        moonfruit_chunk_process(chunk, chunk_Q);
    }
    LaneSync();

    MoonFruit_MacroInfo* macro_info = 0L;
    if (LaneIdx() == 0) {
        macro_info = moonfruit_macro_info_build(thread_ctx.shared_arena, f);
    }
    LaneSyncPtr(macro_info, 0);

    // basic user input
    if (LaneIdx() == 0) {
        struct termios old_term;
        u64 w = 0;
        u64 h = 0;
        DeferBlock({
            tcgetattr(STDIN_FILENO, &old_term);
 
            struct termios term;
            term.c_iflag &= ~(BRKINT | ICRNL | INLCR | ISTRIP | IXON);
            term.c_oflag &= ~(OPOST);
            term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);

            struct winsize win;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0) {
                w = win.ws_col;
                h = win.ws_row;
            }
        }, {
            printf("\033[2J\033[H");
            printf("\033]112\007");
            fflush(stdout);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
        }) {
            printf("\033[H\033[2J");
            printf("\033]12;#000000\007");

            String input_str = EmptyString(LaneArena(), w);
            input_str.size = 0;

            u8 c[4] = {0};
            printf("\033[H\033[30;42m");
            for (u64 i = 0; i < w; i++) printf(" ");
            printf("\033[HInput: \033[0m");
            while (c[0] != '\033') {
                fflush(stdout);
                i32 num_read = read(STDIN_FILENO, c, 4);
                if (num_read == 1) {
                    if (c[0] == '\033') break;
                    if (c[0] == 0x7f) { // backspace
                        input_str = string_chop(input_str, 1);
                    } else {
                        if (!(char_is_alpha(*c) || char_is_digit(*c, 10) || *c == '_')) continue;
                        Assert(input_str.size < w);
                        input_str.str[input_str.size++] = c[0];
                    }

                    printf("\033[2J\033[H");

                    MoonFruit_MacroArray macros = moonfruit_macro_match(LaneArena(), macro_info, input_str);

                    // for testing
                    if (macros.count > 0) {
                        MoonFruit_Macro* macro = &macros.data[0];
                        MoonFruit_ExprTree expr_tree = moonfruit_macro_build_expr_tree_no_args(LaneArena(), macro_info, macro);
                        String macro_eval = moonfruit_expr_tree_format(LaneArena(), expr_tree);
                        String macro_name = moonfruit_macro_format(LaneArena(), macro_info, *macro, MF_FORMAT_DEFINITION);
                        printf("\r\n%.*s\r\n => %.*s", macro_name.size, macro_name.str, macro_eval.size, macro_eval.str);
                    }
                    // end testing

                    /*
                    for EachElement(macro, MoonFruit_Macro, macros) {
                        String macro_eval = moonfruit_macro_eval(LaneArena(), macro_info, *macro, (MoonFruit_ArgValArray){0});
                        String macro_name = moonfruit_macro_format(LaneArena(), macro_info, *macro, MF_FORMAT_DEFINITION);
                        printf("\r\n%.*s = %.*s", macro_name.size, macro_name.str, macro_eval.size, macro_eval.str);
                    }
                    */

                    printf("\033[H\033[30;42m");
                    for (u64 i = 0; i < w; i++) printf(" ");
                    printf("\033[HInput: ");

                    printf("%.*s", input_str.size, input_str.str);
                    printf("\033[0m");
                } else if (num_read > 1) {
                    break;
                }
            }
        }
    }
    LaneSync();
}

String parent_dir(String path, u32 num_dirs) {
    if (num_dirs == 0)
        return path;
    i64 idx = path.size - 1;
    for (; idx >= 0 && path.str[idx] != '/'; idx--);
    path.size = idx;
    return parent_dir(path, num_dirs - 1);
}

i32 main(i32 argc, u8 **argv) {
    u64 num_threads = 1;

    Arena *arena = arena_alloc(1024, 1024);
    String dir   = parent_dir(String(argv[0]), 2);

    u8 *test_files[] = {"MoonFruitMacros/moonfruit_main.c"};

    for (u64 i = 0; i < sizeof(test_files) / sizeof(*test_files); i++) {
        u8 *test_file_name = test_files[i];

        TempArena temp_arena = temp_arena_begin(arena);


        String test_path = string_format(temp_arena.arena, "%.*s/%s", dir.size,
                                         dir.str, test_file_name);
        printf("test path: %.*s\n", test_path.size, test_path.str);

        u8 *argv_test[] = {(u8 *)argv[0], test_path.str};
        MainArgs main_args   = (MainArgs){
            .argc = sizeof(argv_test) / sizeof(*argv_test),
            .argv = argv_test,
        };

        create_parallel_entry_point(num_threads, 1 + MOONFRUIT_DEFINITION_TREE_INIT_SIZE, parallel_main, &main_args);
        temp_arena_end(temp_arena);
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include "completion.h"
#include "sstring.h"
#include "executor.h"
#include "slot.h"
#include "command.h"

static char **filename_list = NULL;
static size_t filename_list_len = 0;

static char *
filepath_generator(const char *text, int state) {
    static size_t list_index, len;
    char *name;

    /* If this is a new word to complete, initialize now.  This includes
       saving the length of TEXT for efficiency, and initializing the index
       variable to 0. */
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    /* Return the next name which partially matches from the command list. */
    if (list_index < filename_list_len) {
        while ((name = filename_list[list_index]) != NULL) {
            list_index++;
            if (strncmp(name, text, len) == 0) {
                if (name[strlen(name) - 1] == '/') {
                    /* default space will not be appended if the matched path is directory */
                    rl_completion_suppress_append = 1;
                } else {
                    rl_completion_suppress_append = 0;
                }
                return name;
            }
        }
    }

    /* If no names matched, then return NULL. */
    return NULL;
}

char **
remote_filepath_completion_func(const char *text, int start, int end) {
    char cmd[1024];
    char **matches = NULL;
    sstring out;

    if (start != end && (text[0] == '/' || text[0] == '~' || text[0] == '.')) {
        snprintf(cmd, 1024, "command ls -aF1d %s*", text);
    } else {
        strcpy(cmd, "command echo $PATH | command tr ':' '\n' | while read path; do command ls -1a $path; done");
    }

    filename_list_len = 0;
    filename_list = NULL;
    out = new_emptystring();

    sync_exec_remote_cmd(slots, cmd, &out, NULL);
    if (string_length(out)) {
        filename_list = string_split(out, '\n', &filename_list_len);
    }

    if (filename_list_len) {
        matches = rl_completion_matches(text, filepath_generator);
    }

    /* explicit declare not perform default filename completion
       even if the application's completion function returns no matches */
    rl_attempted_completion_over = 1;

    free(filename_list);
    string_free(out);
    return matches;
}

static char *
inner_command_generator(const char *text, int state) {
    static struct command *pcmd;
    static size_t len;
    char *name;

    if (!state) {
        pcmd = inner_commands;
        len = strlen(text);
    }

    while ((pcmd = pcmd->next) != NULL) {
        name = pcmd->name;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

char **
inner_completion_func(const char *text, int start, int end) {
    char **matches = NULL;
    //printf("\n[%s], [%s], %d, %d\n", text, rl_line_buffer, start, end);

    if (start == 0) {
        matches = rl_completion_matches(text, inner_command_generator);
    }

    rl_attempted_completion_over = 1;

    return matches;
}

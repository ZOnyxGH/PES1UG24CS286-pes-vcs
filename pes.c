#include "pes.h"
#include "index.h"
#include "commit.h"
#include "tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void cmd_init(void) {
    mkdir(PES_DIR, 0755);
    mkdir(OBJECTS_DIR, 0755);
    mkdir(".pes/refs", 0755);
    mkdir(REFS_DIR, 0755);
    FILE *f = fopen(HEAD_FILE, "w");
    if (f) { fprintf(f, "ref: refs/heads/main\n"); fclose(f); }
    printf("Initialized empty PES repository in .pes/\n");
}

void cmd_add(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "Usage: pes add <file>...\n"); return; }
    static Index index;
    if (index_load(&index) != 0) { fprintf(stderr, "error: failed to load index\n"); return; }
    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) != 0)
            fprintf(stderr, "error: failed to add '%s'\n", argv[i]);
        else
            printf("staged: %s\n", argv[i]);
    }
}

void cmd_status(void) {
    static Index index;
    if (index_load(&index) != 0) { fprintf(stderr, "error: failed to load index\n"); return; }
    index_status(&index);
}

void cmd_commit(int argc, char *argv[]) {
    const char *message = NULL;
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0) { message = argv[i + 1]; break; }
    }
    if (!message) { fprintf(stderr, "error: commit requires a message (-m \"message\")\n"); return; }
    ObjectID commit_id;
    if (commit_create(message, &commit_id) != 0) { fprintf(stderr, "error: commit failed\n"); return; }
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, hex);
    printf("Committed: %.12s... %s\n", hex, message);
}

static void print_commit(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    printf("commit %s\nAuthor: %s\nDate:   %llu\n\n    %s\n\n",
           hex, commit->author, (unsigned long long)commit->timestamp, commit->message);
}

void cmd_log(void) {
    if (commit_walk(print_commit, NULL) != 0)
        fprintf(stderr, "No commits yet.\n");
}

void cmd_branch(int argc, char *argv[]) {
    if (argc == 2) { branch_list(); }
    else if (argc == 3) {
        if (branch_create(argv[2]) == 0) printf("Created branch '%s'\n", argv[2]);
        else fprintf(stderr, "error: failed to create branch '%s'\n", argv[2]);
    } else if (argc == 4 && strcmp(argv[2], "-d") == 0) {
        if (branch_delete(argv[3]) == 0) printf("Deleted branch '%s'\n", argv[3]);
        else fprintf(stderr, "error: failed to delete branch '%s'\n", argv[3]);
    } else {
        fprintf(stderr, "Usage:\n pes branch\n pes branch <name>\n pes branch -d <name>\n");
    }
}

void cmd_checkout(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "Usage: pes checkout <branch_or_commit>\n"); return; }
    if (checkout(argv[2]) == 0) printf("Switched to '%s'\n", argv[2]);
    else fprintf(stderr, "error: checkout failed.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: pes <command> [args]\n"); return 1; }
    const char *cmd = argv[1];
    if      (strcmp(cmd, "init")     == 0) cmd_init();
    else if (strcmp(cmd, "add")      == 0) cmd_add(argc, argv);
    else if (strcmp(cmd, "status")   == 0) cmd_status();
    else if (strcmp(cmd, "commit")   == 0) cmd_commit(argc, argv);
    else if (strcmp(cmd, "log")      == 0) cmd_log();
    else if (strcmp(cmd, "branch")   == 0) cmd_branch(argc, argv);
    else if (strcmp(cmd, "checkout") == 0) cmd_checkout(argc, argv);
    else { fprintf(stderr, "Unknown command: %s\n", cmd); return 1; }
    return 0;
}

// Phase 5 stubs — not yet implemented
void branch_list(void) { printf("(branch list not implemented)\n"); }
int branch_create(const char *name) { (void)name; return -1; }
int branch_delete(const char *name) { (void)name; return -1; }
int checkout(const char *target) { (void)target; return -1; }

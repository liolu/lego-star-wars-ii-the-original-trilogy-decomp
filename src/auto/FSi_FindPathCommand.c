typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef int s32;
typedef s32 BOOL;

typedef enum {
    FS_RESULT_SUCCESS = 0,
    FS_RESULT_FAILURE = 1,
} FSResult;

enum {
    FS_COMMAND_SEEKDIR = 2,
    FS_COMMAND_READDIR = 3,
};

enum {
    FS_FILE_NAME_MAX = 127,
};

typedef struct FSArchive FSArchive;

typedef struct FSFileID {
    FSArchive *arc;
    u32 file_id;
} FSFileID;

typedef struct FSDirPos {
    FSArchive *arc;
    u16 own_id;
    u16 index;
    u32 pos;
} FSDirPos;

typedef struct FSDirEntry {
    union {
        FSFileID file_id;
        FSDirPos dir_id;
    };
    u32 is_directory;
    u32 name_len;
    char name[FS_FILE_NAME_MAX + 1];
} FSDirEntry;

typedef struct FSFile {
    struct FSFile *prev;
    struct FSFile *next;
    FSArchive *arc;
    u32 stat;
    u32 command;
    FSResult error;
    void *queue[2];
    union {
        struct {
            u32 own_id;
            u32 top;
            u32 bottom;
            u32 pos;
        } file;
        struct {
            FSDirPos pos;
            u32 parent;
        } dir;
    } prop;
    union {
        struct {
            FSDirPos pos;
        } seekdir;
        struct {
            FSDirEntry *p_entry;
            BOOL skip_string;
        } readdir;
        struct {
            FSDirPos pos;
            const char *path;
            BOOL find_directory;
            union {
                FSFileID *file;
                FSDirPos *dir;
            } result;
        } findpath;
    } arg;
} FSFile;

extern FSResult FSi_TranslateCommand(FSFile *p_file, s32 command);
extern int FSi_StrNICmp(const char *a, const char *b, u32 len);
extern void FSi_SeekDirDirect(FSFile *p_dir, u32 dir_id);

static inline BOOL FSi_IsSlash(u32 c) {
    return (c == '/') || (c == '\\');
}

/* Default "find path" command handler (entry in fsi_default_command).
 * Starts at the directory in arg.findpath.pos (same storage as
 * arg.seekdir.pos, so the first SEEKDIR seeks there) and walks
 * arg.findpath.path one '/'- or '\\'-separated component at a time:
 *   "."  is skipped; ".." seeks to the parent unless at the root (own_id 0);
 *   any other name is compared (FSi_StrNICmp) with each READDIR entry of the
 *   same kind and length; a matching directory is entered with SEEKDIR.
 * A non-final component always counts as a directory. On success the final
 * file entry is written to *result.file, or, when find_directory is set, the
 * reached directory position to *result.dir. Empty or over-long (> 127 byte)
 * components, a missing entry or a kind mismatch give FS_RESULT_FAILURE. */
FSResult FSi_FindPathCommand(FSFile *p_dir) {
    const u8 *path    = (const u8 *) p_dir->arg.findpath.path;
    const BOOL is_dir = p_dir->arg.findpath.find_directory;

    (void) FSi_TranslateCommand(p_dir, FS_COMMAND_SEEKDIR);
    for (; *path; path += (*path ? 1 : 0)) {
        int name_len     = 0;
        u32 is_directory = 0;

        for (; (is_directory = (u32) path[name_len]), (is_directory && !FSi_IsSlash(is_directory)); ++name_len) {
        }
        if (is_directory || is_dir) {
            is_directory = 1;
        }
        if (name_len == 0) {
            return FS_RESULT_FAILURE;
        } else if (*path == '.') {
            if (name_len == 1) {
                path += 1;
                continue;
            } else if ((name_len == 2) & (path[1] == '.')) {
                if (p_dir->prop.dir.pos.own_id != 0) {
                    FSi_SeekDirDirect(p_dir, p_dir->prop.dir.parent);
                }
                path += 2;
                continue;
            }
        }
        if (name_len > FS_FILE_NAME_MAX) {
            return FS_RESULT_FAILURE;
        } else {
            FSDirEntry etr;
            p_dir->arg.readdir.p_entry     = &etr;
            p_dir->arg.readdir.skip_string = 0;
            for (;;) {
                if (FSi_TranslateCommand(p_dir, FS_COMMAND_READDIR) != FS_RESULT_SUCCESS) {
                    return FS_RESULT_FAILURE;
                }
                if ((is_directory != etr.is_directory) || (name_len != etr.name_len) ||
                    FSi_StrNICmp((const char *) path, etr.name, (u32) name_len)) {
                    continue;
                }
                if (is_directory) {
                    path += name_len;
                    p_dir->arg.seekdir.pos = etr.dir_id;
                    (void) FSi_TranslateCommand(p_dir, FS_COMMAND_SEEKDIR);
                    break;
                } else if (is_dir) {
                    return FS_RESULT_FAILURE;
                } else {
                    *p_dir->arg.findpath.result.file = etr.file_id;
                    return FS_RESULT_SUCCESS;
                }
            }
        }
    }
    if (!is_dir) {
        return FS_RESULT_FAILURE;
    }
    *p_dir->arg.findpath.result.dir = p_dir->prop.dir.pos;
    return FS_RESULT_SUCCESS;
}

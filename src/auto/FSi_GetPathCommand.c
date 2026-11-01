/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
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
    FALSE = 0,
    TRUE  = 1,
};

enum {
    FS_COMMAND_READDIR = 3,
};

enum {
    FS_FILE_STATUS_IS_DIR = 0x20,
};

enum {
    FS_INVALID_ID = 0x10000,
};

/* Archive names are packed into one u32; these bounds give the printed length. */
enum {
    ARC_NAME_LEN1_MAX = 0xFF,
    ARC_NAME_LEN2_MAX = 0xFF00,
    ARC_SEPARATOR_LEN = 2, /* ":/" */
};

typedef struct FSArchive {
    union {
        char ptr[4];
        u32 pack;
    } name;
} FSArchive;

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
    char name[128];
} FSDirEntry;

typedef struct FSFileLink {
    struct FSFile *prev;
    struct FSFile *next;
} FSFileLink;

typedef struct FSFile {
    FSFileLink link;
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
            FSDirEntry *p_entry;
            BOOL skip_string;
        } readdir;
        struct {
            u8 *buf;
            u32 buf_len;
            u16 total_len;
            u16 dir_id;
        } getpath;
        struct {
            FSDirPos pos;
            const char *path;
            BOOL find_directory;
            void *result;
        } findpath;
    } arg;
} FSFile;

extern void func_020d2898(FSFile *p_file);
extern void FSi_SeekDirDirect(FSFile *p_dir, u32 id);
extern FSResult FSi_TranslateCommand(FSFile *p_file, u32 command);
extern void MI_CpuCopy8(const void *src, void *dest, u32 size);
extern char data_02101554[]; /* ":/" separator after the archive name */

static inline u32 FSi_GetArchiveNameLength(const FSArchive *p_arc) {
    const u32 name = p_arc->name.pack;
    if (name <= ARC_NAME_LEN1_MAX) {
        return 1;
    } else if (name <= ARC_NAME_LEN2_MAX) {
        return 2;
    } else {
        return 3;
    }
}

/*
 * Default FS command handler for "get path" (entry in the fsi_default_command
 * table). Builds "<archive>:/<dir>/.../<file>" for p_file into
 * p_file->arg.getpath.buf, walking the directory table with a temporary handle.
 *  1. Find the parent directory: own id for a directory, the cached dir_id, or
 *     a scan of every directory for the entry holding file_id.
 *  2. If total_len is 0, measure the path (archive name, ":/", file name, each
 *     parent directory name plus '/', terminator) and cache total_len/dir_id.
 *  3. With no buffer, return SUCCESS (length query only); fail if buf_len is
 *     shorter than total_len.
 *  4. Copy the archive name and ":/", then fill the rest from the end: file name
 *     with its NUL (or a lone NUL for a directory), then '/' + each parent name.
 * Returns FS_RESULT_FAILURE when the directory is not found or the buffer is too small.
 */
FSResult FSi_GetPathCommand(FSFile *p_file) {
    FSArchive *const p_arc = p_file->arc;
    FSDirEntry entry;
    FSFile tmp;
    u32 file_id;
    u32 pos;
    u32 num_dir;
    u32 dir_id;

    func_020d2898(&tmp); /* reset the temporary handle */
    tmp.arc = p_file->arc;

    if ((p_file->stat & FS_FILE_STATUS_IS_DIR) ? TRUE : FALSE) {
        dir_id  = p_file->prop.dir.pos.own_id;
        file_id = FS_INVALID_ID;
    } else {
        file_id = p_file->prop.file.own_id;
        if (p_file->arg.getpath.total_len != 0) {
            dir_id = p_file->arg.getpath.dir_id;
        } else {
            pos     = 0;
            num_dir = 0;
            dir_id  = FS_INVALID_ID;
            do {
                FSi_SeekDirDirect(&tmp, pos);
                if (!pos) {
                    num_dir = tmp.prop.dir.parent;
                }
                tmp.arg.readdir.p_entry     = &entry;
                tmp.arg.readdir.skip_string = TRUE;
                while (FSi_TranslateCommand(&tmp, FS_COMMAND_READDIR) == FS_RESULT_SUCCESS) {
                    if (!entry.is_directory && (entry.file_id.file_id == file_id)) {
                        dir_id = tmp.prop.dir.pos.own_id;
                        break;
                    }
                }
            } while ((dir_id == FS_INVALID_ID) && (++pos < num_dir));
        }
    }

    if (dir_id == FS_INVALID_ID) {
        p_file->arg.getpath.total_len = 0;
        return FS_RESULT_FAILURE;
    }

    if (p_file->arg.getpath.total_len == 0) {
        u32 id;
        u32 total = 0;
        {
            const u32 name = p_arc->name.pack;
            if (name <= ARC_NAME_LEN1_MAX) {
                total += 1;
            } else if (name <= ARC_NAME_LEN2_MAX) {
                total += 2;
            } else {
                total += 3;
            }
        }
        total += ARC_SEPARATOR_LEN;
        if (file_id != FS_INVALID_ID) {
            total += entry.name_len;
        }
        id = dir_id;
        if (id != 0) {
            FSi_SeekDirDirect(&tmp, id);
            do {
                FSi_SeekDirDirect(&tmp, tmp.prop.dir.parent);
                tmp.arg.readdir.p_entry     = &entry;
                tmp.arg.readdir.skip_string = TRUE;
                while (FSi_TranslateCommand(&tmp, FS_COMMAND_READDIR) == FS_RESULT_SUCCESS) {
                    if (entry.is_directory && (entry.dir_id.own_id == id)) {
                        total += entry.name_len + 1;
                        break;
                    }
                }
                id = tmp.prop.dir.pos.own_id;
            } while (id != 0);
        }
        p_file->arg.getpath.total_len = (u16) (total + 1);
        p_file->arg.getpath.dir_id    = (u16) dir_id;
    }

    {
        u32 total;
        u8 *const buf = p_file->arg.getpath.buf;
        if (!buf) {
            return FS_RESULT_SUCCESS;
        }
        total = p_file->arg.getpath.total_len;
        if (p_file->arg.getpath.buf_len < total) {
            return FS_RESULT_FAILURE;
        }
        {
            u32 pos       = 0;
            const u32 len = FSi_GetArchiveNameLength(p_arc);
            MI_CpuCopy8(p_arc->name.ptr, buf + pos, len);
            pos += len;
            MI_CpuCopy8(data_02101554, buf + pos, ARC_SEPARATOR_LEN);
        }
        FSi_SeekDirDirect(&tmp, dir_id);
        if (file_id != FS_INVALID_ID) {
            tmp.arg.readdir.p_entry     = &entry;
            tmp.arg.readdir.skip_string = FALSE;
            while (FSi_TranslateCommand(&tmp, FS_COMMAND_READDIR) == FS_RESULT_SUCCESS) {
                if (!entry.is_directory && (entry.file_id.file_id == file_id)) {
                    break;
                }
            }
            {
                const u32 len = entry.name_len + 1;
                MI_CpuCopy8(entry.name, buf + total - len, len);
                total -= len;
            }
        } else {
            (buf + total)[-1] = '\0';
            total -= 1;
        }
        while (dir_id != 0) {
            FSi_SeekDirDirect(&tmp, tmp.prop.dir.parent);
            tmp.arg.readdir.p_entry     = &entry;
            tmp.arg.readdir.skip_string = FALSE;
            buf[total - 1]              = '/';
            total -= 1;
            while (FSi_TranslateCommand(&tmp, FS_COMMAND_READDIR) == FS_RESULT_SUCCESS) {
                if (entry.is_directory && (entry.dir_id.own_id == dir_id)) {
                    const u32 len = entry.name_len;
                    MI_CpuCopy8(entry.name, buf + total - len, len);
                    total -= len;
                    break;
                }
            }
            dir_id = tmp.prop.dir.pos.own_id;
        }
    }
    return FS_RESULT_SUCCESS;
}

#include "miniz.c"
#include <string.h>
#include <dirent.h>

#ifndef _WIN32
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <unistd.h>
#endif

#define MAX_FILE_SIZE    8192 * 1024 * 2

void DoDirectory(char *filename) {
    char DIR_BUFFER[8192];

    DIR_BUFFER[0] = 0;
    int len = strlen(filename);

    int dir_index = 0;
    for (int i = 0; i < len; i++) {
        char c = filename[i];
        if ((c == '/') || (c == '\\')) {
            if (DIR_BUFFER[0]) {
#ifdef _WIN32
                mkdir(DIR_BUFFER);
#else
                mkdir(DIR_BUFFER, 0777L);
#endif
            }
        }
        DIR_BUFFER[dir_index++] = c;
        DIR_BUFFER[dir_index]   = 0;
    }
}

int UnZip(char *filename) {
    int err_no = 0;

    mz_zip_archive zip_archive;

    memset(&zip_archive, 0, sizeof(zip_archive));
    mz_bool status = mz_zip_reader_init_file(&zip_archive, filename, 0);

    if (status) {
        int num_files = mz_zip_reader_get_num_files(&zip_archive);
        for (int i = 0; i < num_files; i++) {
            mz_zip_archive_file_stat file_stat;
            if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
                char *name = file_stat.m_filename;
                if ((name) && (!strstr(name, ".."))) {
                    DoDirectory(name);
                    mz_zip_reader_extract_to_file(&zip_archive, i, name, 0);
                }
            }
        }
        mz_zip_reader_end(&zip_archive);
    } else
        return -1;
    return err_no;
}

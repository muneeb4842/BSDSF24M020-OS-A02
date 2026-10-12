#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

void do_ls(const char *dir, int long_format, int horizontal_display);
void print_in_columns(char entries[][256], int count);
void print_in_horizontal_columns(char entries[][256], int count);
void print_long_entry(const char *path, const char *name);
int compare_entries(const void *a, const void *b);

int main(int argc, char const *argv[])
{
    int long_format = 0;
    int horizontal_display = 0;
    int target_argc = 0;
    const char *target_dirs[10];

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0)
        {
            long_format = 1;
        }
        else if (strcmp(argv[i], "-x") == 0)
        {
            horizontal_display = 1;
        }
        else
        {
            target_dirs[target_argc++] = argv[i];
        }
    }

    if (target_argc == 0)
    {
        do_ls(".", long_format, horizontal_display);
    }
    else
    {
        for (int i = 0; i < target_argc; i++)
        {
            if (target_argc > 1)
            {
                printf("Directory listing of %s : \n", target_dirs[i]);
            }
            do_ls(target_dirs[i], long_format, horizontal_display);
            if (i < target_argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

int compare_entries(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

void do_ls(const char *dir, int long_format, int horizontal_display)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char entries[1024][256];
    int count = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        
        if (count < 1024)
        {
            snprintf(entries[count], sizeof(entries[count]), "%s", entry->d_name);
            count++;
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    if (count > 0)
    {
        qsort(entries, count, sizeof(entries[0]), compare_entries);

        if (long_format)
        {
            for (int i = 0; i < count; i++)
            {
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entries[i]);
                print_long_entry(fullpath, entries[i]);
            }
        }
        else
        {
            if (horizontal_display)
            {
                print_in_horizontal_columns(entries, count);
            }
            else
            {
                print_in_columns(entries, count);
            }
        }
    }
}

void print_long_entry(const char *path, const char *name)
{
    struct stat file_stat;
    if (lstat(path, &file_stat) < 0)
    {
        perror("lstat error");
        return;
    }

    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    printf(" %3ld", (long)file_stat.st_nlink);

    struct passwd *pwd = getpwuid(file_stat.st_uid);
    if (pwd != NULL)
        printf(" %s", pwd->pw_name);
    else
        printf(" %d", file_stat.st_uid);

    struct group *grp = getgrgid(file_stat.st_gid);
    if (grp != NULL)
        printf(" %s", grp->gr_name);
    else
        printf(" %d", file_stat.st_gid);

    printf(" %8ld", (long)file_stat.st_size);

    char time_buf[80];
    struct tm *timeinfo = localtime(&file_stat.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", timeinfo);
    printf(" %s", time_buf);

    printf(" %s\n", name);
}

void print_in_columns(char entries[][256], int count)
{
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(entries[i]);
        if (len > max_len)
            max_len = len;
    }
    max_len += 2;

    int cols = term_width / max_len;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r;
            if (idx < count)
            {
                printf("%-*s", max_len, entries[idx]);
            }
        }
        printf("\n");
    }
}

void print_in_horizontal_columns(char entries[][256], int count)
{
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(entries[i]);
        if (len > max_len)
            max_len = len;
    }
    max_len += 2;

    int cols = term_width / max_len;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r * cols + c;
            if (idx < count)
            {
                printf("%-*s", max_len, entries[idx]);
            }
        }
        printf("\n");
    }
}

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

void do_ls(const char *dir, int long_format, int horizontal_display, int show_hidden, int reverse_sort, int recursive, int human_readable, int color_output);
void print_in_columns(char entries[][256], int count, const char *dir, int color_output);
void print_in_horizontal_columns(char entries[][256], int count, const char *dir, int color_output);
void print_long_entry(const char *path, const char *name, int human_readable, int color_output);
void format_size(long bytes, char *buf);
int compare_entries(const void *a, const void *b);
void print_colored_name(const char *dir, const char *name, int color_output);

int main(int argc, char const *argv[])
{
    int long_format = 0;
    int horizontal_display = 0;
    int show_hidden = 0;
    int reverse_sort = 0;
    int recursive = 0;
    int human_readable = 0;
    int color_output = 0;
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
        else if (strcmp(argv[i], "-a") == 0)
        {
            show_hidden = 1;
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            reverse_sort = 1;
        }
        else if (strcmp(argv[i], "-R") == 0)
        {
            recursive = 1;
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            human_readable = 1;
        }
        else if (strcmp(argv[i], "--color") == 0)
        {
            color_output = 1;
        }
        else
        {
            target_dirs[target_argc++] = argv[i];
        }
    }

    if (target_argc == 0)
    {
        do_ls(".", long_format, horizontal_display, show_hidden, reverse_sort, recursive, human_readable, color_output);
    }
    else
    {
        for (int i = 0; i < target_argc; i++)
        {
            if (target_argc > 1)
            {
                printf("Directory listing of %s : \n", target_dirs[i]);
            }
            do_ls(target_dirs[i], long_format, horizontal_display, show_hidden, reverse_sort, recursive, human_readable, color_output);
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

void format_size(long bytes, char *buf)
{
    double size = (double)bytes;
    int i = 0;
    const char *units[] = {"", "K", "M", "G", "T"};
    while (size >= 1024 && i < 4)
    {
        size /= 1024;
        i++;
    }
    if (i == 0)
    {
        sprintf(buf, "%ld", bytes);
    }
    else
    {
        sprintf(buf, "%.1f%s", size, units[i]);
    }
}

void print_colored_name(const char *dir, const char *name, int color_output)
{
    if (!color_output)
    {
        printf("%s", name);
        return;
    }

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) < 0)
    {
        printf("%s", name);
        return;
    }

    if (S_ISDIR(st.st_mode))
    {
        // Bold Blue for Directories
        printf("\033[1;34m%s\033[0m", name);
    }
    else if (st.st_mode & S_IXUSR)
    {
        // Bold Green for Executables
        printf("\033[1;32m%s\033[0m", name);
    }
    else
    {
        printf("%s", name);
    }
}

void do_ls(const char *dir, int long_format, int horizontal_display, int show_hidden, int reverse_sort, int recursive, int human_readable, int color_output)
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
        if (!show_hidden && entry->d_name[0] == '.')
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

        if (reverse_sort)
        {
            for (int i = 0; i < count / 2; i++)
            {
                char temp[256];
                strcpy(temp, entries[i]);
                strcpy(entries[i], entries[count - 1 - i]);
                strcpy(entries[count - 1 - i], temp);
            }
        }

        if (long_format)
        {
            for (int i = 0; i < count; i++)
            {
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entries[i]);
                print_long_entry(fullpath, entries[i], human_readable, color_output);
            }
        }
        else
        {
            if (horizontal_display)
            {
                print_in_horizontal_columns(entries, count, dir, color_output);
            }
            else
            {
                print_in_columns(entries, count, dir, color_output);
            }
        }

        if (recursive)
        {
            for (int i = 0; i < count; i++)
            {
                if (strcmp(entries[i], ".") == 0 || strcmp(entries[i], "..") == 0)
                    continue;

                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entries[i]);

                struct stat st;
                if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
                {
                    printf("\n%s:\n", fullpath);
                    do_ls(fullpath, long_format, horizontal_display, show_hidden, reverse_sort, recursive, human_readable, color_output);
                }
            }
        }
    }
}

void print_long_entry(const char *path, const char *name, int human_readable, int color_output)
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

    if (human_readable)
    {
        char size_buf[16];
        format_size((long)file_stat.st_size, size_buf);
        printf(" %8s", size_buf);
    }
    else
    {
        printf(" %8ld", (long)file_stat.st_size);
    }

    char time_buf[80];
    struct tm *timeinfo = localtime(&file_stat.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", timeinfo);
    printf(" %s ", time_buf);

    // Extract base directory for color check
    char parent_dir[1024];
    strcpy(parent_dir, path);
    char *last_slash = strrchr(parent_dir, '/');
    if (last_slash)
    {
        *last_slash = '\0';
    }
    else
    {
        strcpy(parent_dir, ".");
    }

    print_colored_name(parent_dir, name, color_output);
    printf("\n");
}

void print_in_columns(char entries[][256], int count, const char *dir, int color_output)
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
                int printed_len = strlen(entries[idx]);
                print_colored_name(dir, entries[idx], color_output);
                int padding = max_len - printed_len;
                for (int p = 0; p < padding; p++) printf(" ");
            }
        }
        printf("\n");
    }
}

void print_in_horizontal_columns(char entries[][256], int count, const char *dir, int color_output)
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
                int printed_len = strlen(entries[idx]);
                print_colored_name(dir, entries[idx], color_output);
                int padding = max_len - printed_len;
                for (int p = 0; p < padding; p++) printf(" ");
            }
        }
        printf("\n");
    }
}

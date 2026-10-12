#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

extern int errno;

void do_ls(const char *dir, int long_format);
void print_long_format(const char *filepath, const char *filename);
void print_in_columns(char entries[][256], int count);

int main(int argc, char const *argv[])
{
    int long_format = 0;
    int target_argc = 0;
    const char *target_dirs[10];

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0)
        {
            long_format = 1;
        }
        else
        {
            target_dirs[target_argc++] = argv[i];
        }
    }

    if (target_argc == 0)
    {
        do_ls(".", long_format);
    }
    else
    {
        for (int i = 0; i < target_argc; i++)
        {
            if (target_argc > 1)
            {
                printf("Directory listing of %s : \n", target_dirs[i]);
            }
            do_ls(target_dirs[i], long_format);
            if (i < target_argc - 1)
                puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_format)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char entries[1024][256];
    int count = 0;
    struct dirent *entry;
    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
        {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);
            print_long_format(filepath, entry->d_name);
        }
        else
        {
            if (count < 1024)
            {
                strncpy(entries[count], entry->d_name, sizeof(entries[count]) - 1);
                entries[count][sizeof(entries[count]) - 1] = '\0';
                count++;
            }
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    if (!long_format && count > 0)
    {
        print_in_columns(entries, count);
    }
}

void print_in_columns(char entries[][256], int count)
{
    struct winsize w;
    int term_width = 80; // default fallback
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
    max_len += 2; // padding space

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

void print_long_format(const char *filepath, const char *filename)
{
    struct stat file_stat;
    if (lstat(filepath, &file_stat) < 0)
    {
        perror("lstat failed");
        return;
    }

    char perms[11] = "----------";
    if (S_ISDIR(file_stat.st_mode))  perms[0] = 'd';
    else if (S_ISLNK(file_stat.st_mode)) perms[0] = 'l';
    else if (S_ISCHR(file_stat.st_mode)) perms[0] = 'c';
    else if (S_ISBLK(file_stat.st_mode)) perms[0] = 'b';
    else if (S_ISSOCK(file_stat.st_mode)) perms[0] = 's';
    else if (S_ISFIFO(file_stat.st_mode)) perms[0] = 'p';

    if (file_stat.st_mode & S_IRUSR) perms[1] = 'r';
    if (file_stat.st_mode & S_IWUSR) perms[2] = 'w';
    if (file_stat.st_mode & S_IXUSR) perms[3] = 'x';
    if (file_stat.st_mode & S_IRGRP) perms[4] = 'r';
    if (file_stat.st_mode & S_IWGRP) perms[5] = 'w';
    if (file_stat.st_mode & S_IXGRP) perms[6] = 'x';
    if (file_stat.st_mode & S_IROTH) perms[7] = 'r';
    if (file_stat.st_mode & S_IWOTH) perms[8] = 'w';
    if (file_stat.st_mode & S_IXOTH) perms[9] = 'x';

    struct passwd *pwd = getpwuid(file_stat.st_uid);
    struct group *grp = getgrgid(file_stat.st_gid);

    char time_str[80];
    struct tm *timeinfo = localtime(&file_stat.st_mtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", timeinfo);

    printf("%s %3lu %-8s %-8s %8ld %s %s\n",
           perms,
           (unsigned long)file_stat.st_nlink,
           pwd ? pwd->pw_name : "unknown",
           grp ? grp->gr_name : "unknown",
           (long)file_stat.st_size,
           time_str,
           filename);
}

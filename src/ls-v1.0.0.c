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

extern int errno;

void do_ls(const char *dir, int long_format);
void print_long_format(const char *filepath, const char *filename);

int main(int argc, char const *argv[])
{
    int long_format = 0;
    int target_argc = 0;
    const char *target_dirs[10];

    // Simple argument parsing for -l flag
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
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
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
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void print_long_format(const char *filepath, const char *filename)
{
    struct stat file_stat;
    if (lstat(filepath, &file_stat) < 0)
    {
        perror("lstat failed");
        return;
    }

    // File type and permissions string
    char perms[11];
    strcpy(perms, "----------");

    // File Type
    if (S_ISDIR(file_stat.st_mode))  perms[0] = 'd';
    else if (S_ISLNK(file_stat.st_mode)) perms[0] = 'l';
    else if (S_ISCHR(file_stat.st_mode)) perms[0] = 'c';
    else if (S_ISBLK(file_stat.st_mode)) perms[0] = 'b';
    else if (S_ISSOCK(file_stat.st_mode)) perms[0] = 's';
    else if (S_ISFIFO(file_stat.st_mode)) perms[0] = 'p';

    // Owner permissions
    if (file_stat.st_mode & S_IRUSR) perms[1] = 'r';
    if (file_stat.st_mode & S_IWUSR) perms[2] = 'w';
    if (file_stat.st_mode & S_IXUSR) perms[3] = 'x';

    // Group permissions
    if (file_stat.st_mode & S_IRGRP) perms[4] = 'r';
    if (file_stat.st_mode & S_IWGRP) perms[5] = 'w';
    if (file_stat.st_mode & S_IXGRP) perms[6] = 'x';

    // Other permissions
    if (file_stat.st_mode & S_IROTH) perms[7] = 'r';
    if (file_stat.st_mode & S_IWOTH) perms[8] = 'w';
    if (file_stat.st_mode & S_IXOTH) perms[9] = 'x';

    // Resolve User and Group names
    struct passwd *pwd = getpwuid(file_stat.st_uid);
    struct group *grp = getgrgid(file_stat.st_gid);

    // Format modification time
    char time_str[80];
    struct tm *timeinfo = localtime(&file_stat.st_mtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", timeinfo);

    // Print formatted long output line
    printf("%s %3lu %-8s %-88s %8ld %s %s\n",
           perms,
           (unsigned long)file_stat.st_nlink,
           pwd ? pwd->pw_name : "unknown",
           grp ? grp->gr_name : "unknown",
           (long)file_stat.st_size,
           time_str,
           filename);
}

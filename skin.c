#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<wait.h>
#include<dirent.h>

#define PATHMAX 4048

char buf[PATHMAX];
char fpath[PATHMAX];
char spath[PATHMAX];

void skinning_dir(DIR *);
void skinning_file(char *, char *);
void making_dir(char *, char *);

int main(int argc, char *argv[]){
    struct stat statbuf;
    DIR * dir;
    struct dirent target;

    if(argc < 2){
        fprintf(stderr, "usage : %s <directory path>\n", argv[0]);
        exit(1);
    }

    if(stat(argv[1], &statbuf) < 0){
        fprintf(stderr, "stat error for %s\n", argv[1]);
        exit(1);
    }

    if(!S_ISDIR(statbuf.st_mode)){
        fprintf(stderr, "%s is not directory\n", argv[1]);
        exit(1);
    }

    if(argv[1][0] != '/'){
        getcwd(buf, PATHMAX);
        sprintf(buf, "%s/%s", buf, argv[1]);
    }
    else strcpy(buf, argv[1]);
    realpath(buf, fpath);
    sprintf(spath, "%s__skin", fpath);
    // printf("%s\n", fpath);

    dir = opendir(argv[1]);
    chdir(argv[1]);
    mkdir(spath, 0777);
    skinning_dir(dir);
    chdir("..");
    closedir(dir);

}

void skinning_dir(DIR *cur){
    DIR *dirbuf;
    struct dirent *dp = readdir(cur);
    struct stat statbuf;

    while(dp != NULL){
        stat(dp->d_name, &statbuf);
        getcwd(buf, PATHMAX);

        //directory part
        if(S_ISDIR(statbuf.st_mode)){
            if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                dp = readdir(cur);
                continue;
            }
            
            making_dir(buf + strlen(fpath), dp->d_name);
            //cwd change to child directory
            dirbuf = opendir(dp->d_name);
            chdir(dp->d_name);


            //recursive
            skinning_dir(dirbuf);

            
            //return to parent 
            chdir("..");
            closedir(dirbuf);
            // printf("exit dir..\n");
        }
        else{
        //else part
            // printf("%s/%s\n", buf, dp->d_name);
            // printf("%s/%s\n", buf + strlen(fpath), dp->d_name);
            skinning_file(buf + strlen(fpath), dp->d_name);

        }

        dp = readdir(cur);
    }
}

void skinning_file(char *child_path, char* name){
    char tbuf[FILENAME_MAX];
    struct stat statbuf;
    int fd;
    sprintf(tbuf, "%s%s/%s", spath, child_path, name);
    printf("%s\n", tbuf);  
    if((fd = open(tbuf, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){
        fprintf(stderr, "open error for %s\n", tbuf);
        exit(1);
    }

    sprintf(tbuf, "%s%s/%s", fpath, child_path, name);
    stat(tbuf, &statbuf);
    write(fd, &statbuf.st_mtime, sizeof(statbuf.st_mtime));
    write(fd, tbuf, strlen(tbuf)+1);
    close(fd);
}

void making_dir(char *child_path, char * name){
    char tbuf[FILENAME_MAX];

    sprintf(tbuf, "%s%s/%s", spath, child_path, name);
    printf("making dir : %s\n", tbuf);
    mkdir(tbuf, 0777);
}
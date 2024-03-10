#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<wait.h>
#include<dirent.h>
#include<errno.h>
#include<time.h>
#include<getopt.h>

#define PATHMAX 4096
#define NAMEMAX 256

const int HASHLEN = sizeof(time_t);
char buf[PATHMAX];
char buf2[PATHMAX];
char npath[PATHMAX];
char opath[PATHMAX];

int cOption = 0;
int rOption = 0;

void validate_argv(int , char *[], char *);
void option_check(int, char *[]);
void molt_dir(char *, char *);
void molt_file(int, int);
void do_diff_in_command(const char *, const char *);

int main(int argc, char *argv[]){
    struct stat statbuf;
    char *argvptr, c;

    //first directory
    validate_argv(argc, argv, opath);

    //second directory
    validate_argv(argc, argv, npath);

    //option checking (-r, -c)
    option_check(argc, argv);
    
    //molting dir
    strcpy(buf, opath);
    strcpy(buf2, npath);
    molt_dir(buf + strlen(buf), buf2 + strlen(buf2));
}

void validate_argv(int argc, char *argv[], char *target_path){
    struct stat statbuf; 
    static int current_arg_num = 1;
    for(; current_arg_num < argc; current_arg_num++){
        if(argv[current_arg_num][0] != '-') break;
    }

    if(current_arg_num >= argc){
        fprintf(stderr, "usage : %s <old version> <new version>\n", argv[0]);
        exit(1);
    }

    if(argv[current_arg_num][0] == '/'){
        strcpy(target_path, argv[current_arg_num]);
    }
    else{
        getcwd(buf, PATHMAX);
        sprintf(buf, "%s/%s", buf, argv[current_arg_num]);
        if(realpath(buf, target_path) < 0){
            fprintf(stderr, "realpath error for %s\n", buf);
            exit(1);
        }
    }

    if(stat(target_path, &statbuf) < 0){
        fprintf(stderr, "stat error for %s\n", target_path);
        exit(1);
    }

    if(!S_ISDIR(statbuf.st_mode)){
        fprintf(stderr, "%s is not directory file\n", target_path);
        exit(1);
    }

    current_arg_num++;
}

void option_check(int argc, char *argv[]){
    char c;
    while((c = getopt(argc, argv, "cr")) != -1){
        switch (c)
        {
        case 'c':
            cOption = 1;
            break;
        case 'r':
            rOption = 1;
            break;
        default:
            // printf("Unkown option %c\n", optopt);
            break;
        }
    }
}

void molt_dir(char *o_parent, char *n_parent){
    struct dirent **old_ptr, **new_ptr;
    static struct stat o_statbuf, n_statbuf;
    int oldcnt, newcnt, i = 0, j = 0, cmpresult, ofd, nfd;

    *o_parent = '/'; o_parent++; *o_parent = '\0';
    *n_parent = '/'; n_parent++; *n_parent = '\0';


    if((oldcnt = scandir(buf, &old_ptr, NULL, alphasort)) == -1){
        fprintf(stderr, "scandir error for %s\n", buf);
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }
    
    if((newcnt = scandir(buf2, &new_ptr, NULL, alphasort)) == -1){
        fprintf(stderr, "scandir error for %s\n", buf2);
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    while(i < oldcnt && j < newcnt){
        if(!strcmp(old_ptr[i]->d_name, ".") || !strcmp(old_ptr[i]->d_name, "..")){
            i++;
            continue;
        }

        if(!strcmp(new_ptr[j]->d_name, ".") || !strcmp(new_ptr[j]->d_name, "..")){
            j++;
            continue;
        }

        if((cmpresult = strcmp(old_ptr[i]->d_name, new_ptr[j]->d_name)) == 0){
            strcpy(o_parent, old_ptr[i]->d_name);
            strcpy(n_parent, new_ptr[j]->d_name);

            if(stat(buf, &o_statbuf) < 0){
                fprintf(stderr, "stat error for %s\n", buf);
                exit(1);
            }

            if(stat(buf2, &n_statbuf) < 0){
                fprintf(stderr, "stat error for %s\n", buf2);
                exit(1);
            }

            //recursiive debug
            // printf("%s, %s\n", buf, buf2);

            if(S_ISDIR(o_statbuf.st_mode) && S_ISDIR(n_statbuf.st_mode)){
                molt_dir(buf + strlen(buf), buf2 + strlen(buf2));
            }
            else if(S_ISREG(o_statbuf.st_mode) && S_ISREG(n_statbuf.st_mode)){
                if((ofd = open(buf, O_RDONLY)) < 0){
                    fprintf(stderr, "open error for %s\n", buf);
                    exit(1);
                }

                if((nfd = open(buf2, O_RDWR)) < 0){
                    fprintf(stderr, "open error for %s\n", buf2);
                    exit(1);
                }

                molt_file(ofd, nfd);

                close(ofd);
                close(nfd);
            }

            i++, j++;
        }
        else if(cmpresult < 0) {
            if(cOption){
                strcpy(o_parent, old_ptr[i]->d_name);
                printf("%s file is deleted in new version\n", buf);
            }
            i++;
        }
        else {
            if(cOption){
                strcpy(n_parent, new_ptr[j]->d_name);
                printf("%s file is created in new version\n", buf2);
            }
            j++;
        }
    } 

    for(i = 0; i < oldcnt; i++) free(old_ptr[i]);
    for(j = 0; j < newcnt; j++) free(new_ptr[j]);

    free(old_ptr);
    free(new_ptr);
}

void molt_file(int ofd, int nfd){
    char obuf[HASHLEN + 1];
    char nbuf[HASHLEN + 1];
    time_t omt, nmt;
    char tbuf1[PATHMAX];
    char tbuf2[PATHMAX];

    read(ofd, obuf, HASHLEN);
    read(ofd, tbuf1, PATHMAX);
    read(nfd, nbuf, HASHLEN);
    read(nfd, tbuf2, PATHMAX);

    // //time debug
    // printf("%s", ctime((time_t *)obuf));
    // printf("%s", ctime((time_t *)nbuf));
    // printf("%ld, %ld\n", (time_t)*obuf, (time_t)*nbuf);

    if(memcmp((void *)obuf, (void *)nbuf, HASHLEN) == 0){
        // //file change debug
        // printf("file not changed!\n");
        // printf("%s, %s\n", tbuf1, tbuf2);

        //delete code
        if(rOption){
            lseek(nfd, HASHLEN, SEEK_SET);
            write(nfd, tbuf1, strlen(tbuf1));
            write(nfd, tbuf1 + strlen(tbuf1), sizeof(char));
            if(strcmp(tbuf1, tbuf2) != 0){
                remove(tbuf2);
                printf("%s removed.\n", tbuf2);
            }
            else {
                printf("%s aleady reomved\n", tbuf2);
            }
        }
    }
    else {
        // //file change debug
        // printf("file changed\n");
        // printf("%s, %s\n", tbuf1, tbuf2);

        if(cOption){
            printf("%s, %s\n", tbuf1, tbuf2);
            do_diff_in_command(tbuf1, tbuf2);
        }
    }     
}
    
void do_diff_in_command(const char *o_path, const char *n_path){
    char cbuf[PATHMAX];
    sprintf(cbuf, "diff %s %s", o_path, n_path);
    system(cbuf);
}

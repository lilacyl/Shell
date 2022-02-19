/**
 * shell
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector * history = NULL;
static char * next_cmd = NULL;
static vector * process_running = NULL;


/**
 * Process Creator & Destructor
*/
struct process * pro_creator(char * command, pid_t pid) {
    char * cmd = calloc(strlen(command) + 1, 1);
    strcpy(cmd, command);
    struct process * this = calloc(sizeof(struct process), 1);
    this->command = cmd;
    this->pid = pid;
    return this;
}

void pro_destructor(pid_t pid) {
    /**Find the process*/
    for(size_t i = 0; i < vector_size(process_running); i++) {
        struct process *this = ((process*)vector_get(process_running, i));
        if ( this->pid == pid ) {
            free(this->command);
            free(this);
            vector_erase(process_running, i);
            break;
        }
    }
}
/**
 * The build in commmand of cd
 * Changes the current working directory of the shell to <path>
 * Need to first delete the '\n' char
*/
int cd_builtIn(char* dest) {
    char temp[strlen(dest)+1];
    strcpy(temp, dest); //be careful with the strncpy
    //temp[strlen(dest)] = '\0';
    //strtok(temp, " ");
    int change = chdir(temp);
    if(change == -1) {
        print_no_directory(temp);
        return -1;
    }
    return 1;
}
/**
 * Find the nth command
*/
int nth_buildIn(char * command){
    if (strlen(command) < 2)
    {
        print_invalid_index();
        return -1;
    }
    vector_pop_back(history);
    //FIXED: temp value is wrong;
    char temp[strlen(command)];
    for (size_t i = 0; i < strlen(command) - 1; i++)
    {
        if (!isdigit(command[i+1])) {
            print_invalid_index();
            return -1;
        } 
    }
    strcpy(temp, command + 1);
    temp[strlen(command) - 1] = '\0';
    size_t n = atoi(temp);
    if(n >= vector_size(history)) {
        print_invalid_index();
        return -1;
    }
    char * cmd = vector_get(history, n);
    print_command(cmd);
    next_cmd = malloc(strlen(cmd) + 1);
    next_cmd = strcpy(next_cmd, cmd);
    return 1;
}
/** 
 * !<prefix> Function
*/
int prefix_buildIn(char * command) {
    vector_pop_back(history);
    for (size_t i = 0; i < vector_size(history); i++) {
        char* v = (char*) vector_get(history, (vector_size(history)- i - 1));
        if(!strncmp(command+1, v, strlen(command) - 1)) {
            next_cmd = malloc(strlen(v) + 1);
            next_cmd = strcpy(next_cmd, v);
            print_command(next_cmd);
            return 1;
        }
    }
    print_no_history_match();
    return -1;
}
/**
 * print the process info
*/
int ps_buildIn(char* file, char* file_access, int io) {
    int fd1 = 0;
    int stdout_copy = 0;
    if (file) {
        stdout_copy = dup(io);
        close(io);
        FILE * f1 = fopen(file, file_access);
        int fd1 = fileno(f1);
        if (!f1) {
            print_script_file_error();
            exit(1);
        }
        if (fd1 != 1) {
            exit(1);
        }
    }
    print_process_info_header();
    for(size_t i = 0; i < vector_size(process_running); i++) {
        struct process * pro = vector_get(process_running,i);
        pid_t pid = pro->pid;
        char* cmd = pro->command;
        /**creating a pro_info struct and putting in PID and COMMAND*/
        struct process_info * pro_info = calloc(sizeof(struct process_info), 1);
        pro_info->pid = pid;
        pro_info->command = calloc(strlen(cmd) + 1, 1);
        strcpy(pro_info->command, cmd);
        /**
         * use int sprintf(char *str, const char *format, ...);
         * to get the /proc/[pid]/status
        */
         char kernel_file[100];
        char * info = NULL;
        size_t i_size = 0;
        sprintf(kernel_file, "/proc/%d/status", pid);
        FILE * status = fopen(kernel_file, "r");
        if(status == NULL) {
            print_script_file_error();
            exit(1);
        } else {
            while (1) {
                ssize_t getline_return = getline(&info, &i_size, status);
                if(getline_return == -1) { break;}
                if (!strncmp(info, "Threads", 7)) {
                    pro_info->nthreads = atol(info + 9);
                } else if (!strncmp(info, "VmSize", 6)) {
                    pro_info->vsize = atol(info + 8);
                } else if (!strncmp(info, "State", 5)) {
                    pro_info->state = *(info + 7);
                }
                free(info);
                info = NULL;
                i_size = 0;
            }
            fclose(status);
        }
        /**
         * first
         * /proc/[pid]/stat
         * (14) utime  %lu
         * (15) stime  %lu
         * (22) starttime  %llu
        */

        sprintf(kernel_file, "/proc/%d/stat", pid);
        FILE * stat = fopen(kernel_file, "r");
        if (!stat) {
            print_script_file_error();
            exit(1);
        }
        getline(&info, &i_size, stat);
        char * p = strtok(info, " ");
        unsigned long ut;
        unsigned long st;
        unsigned long long starttime;
        for(size_t i = 0; i < 23;) {
            i++;
            if(i == 14) {
                ut = atol(p);
            } else if(i == 15) {
                st = atol(p);
            } else if(i == 22) {
                starttime = atol(p);
            }
            p = strtok(NULL, " ");
        }
        fclose(stat);
        free(info);
        info = NULL;
        i_size = 0;

        sprintf(kernel_file,"/proc/stat");
        FILE *statfsys = fopen(kernel_file, "r");
        if (!statfsys) {
            print_script_file_error();
            exit(1);
        }
        unsigned long bt = 0;
        while(getline(&info, &i_size, statfsys) != -1) {
            if(!strncmp(info, "btime", 5)) {
                bt = atol(info + 6);
                break;
            }
        }
        fclose(statfsys);
        free(info);
        info = NULL;
        i_size = 0; 

        char time_str[100];
        char time_str2[100];
        unsigned long total_s = (ut + st) / sysconf(_SC_CLK_TCK);
        time_t total_time = starttime / sysconf(_SC_CLK_TCK) + bt;
        struct tm * tm_info = localtime(&total_time);
        execution_time_to_string(time_str, 100, total_s/60, total_s % 60);


        time_struct_to_string(time_str2, 100, tm_info);
        pro_info->start_str = calloc(strlen(time_str2)+1, 1);
        strcpy(pro_info->start_str, time_str2);

        pro_info->time_str = calloc(strlen(time_str)+1, 1);
        strcpy(pro_info->time_str, time_str);

        
        print_process_info(pro_info);
        free(pro_info->command);
        free(pro_info->start_str);
        free(pro_info->time_str);
        free(pro_info);

    }
    
    if (file) {
        close(fd1);
        dup2(stdout_copy, io);
        close(stdout_copy);
    }
    return 1;
}
/**
 * Kill, stop, continue can all use the same function
*/
int ksc_buildIn(pid_t pid, int sig){
  for (size_t i = 0; i < vector_size(process_running); i++) {
    struct process * pro = (process *)vector_get(process_running, i);
    if ( pro->pid == pid ) { //found
      kill(pro->pid, sig);
      if (sig == SIGKILL) { 
        print_killed_process(pro->pid, pro->command);
        pro_destructor(pro->pid);
      } else if (sig == SIGSTOP){
        print_stopped_process(pro->pid, pro->command);
      }
      return 1;
    }
  }
  
  print_no_process_found(pid);
  return -1;
}
/**
 * external command
*/
int external(char * command, char* file, char* file_access, int io) {
    fflush(stdout);
    size_t i = 0;
    i = vector_size(process_running);
    pid_t fork_pid = fork();
    if (fork_pid < 0) { 
        print_fork_failed();
        exit(1);
    } else if (fork_pid == 0){ //CHILD
        if (command[strlen(command)-1] == '&') {
            command[strlen(command)-1] = '\0';
            if (command[strlen(command)-1] == ' ') {
                command[strlen(command)-1] = '\0';
            }
        }
        fflush(stdin);
        /**if we need to write to a file*/
        if (file) {
            // printf("file:%s\n",file);
            // printf("%d\n", io);
            close(io);
            FILE * f1 = fopen(file, file_access);
            int fd1 = fileno(f1);
            if (!f1) {
                print_script_file_error();
                exit(1);
            }
            if (fd1 != io) {
                exit(1);
            }
            
        }
        sstring * ss_cmd = cstr_to_sstring(command);
        vector * v_cmd = sstring_split(ss_cmd, ' ');
        char * argv_exec[vector_size(v_cmd)+1];
        for(size_t i = 0; i < vector_size(v_cmd); i++) {
            argv_exec[i] = (char*) vector_get(v_cmd,i);
        }
        argv_exec[vector_size(v_cmd)] = NULL;
        sstring_destroy(ss_cmd);
        ss_cmd = NULL;
        execvp(argv_exec[0], argv_exec);
        print_exec_failed(command);
        vector_destroy(v_cmd);
        v_cmd = NULL;
        exit(1);
    } else {
        struct process * pro = pro_creator(command, fork_pid);
        vector_push_back(process_running, pro);
        print_command_executed(fork_pid);
        /*only wait for forground*/
        if (command[strlen(command)-1] == '&') {
            if (setpgid(fork_pid,fork_pid) == -1) {
                print_setpgid_failed();
                return -1;
            }
        } else {
            if (setpgid(fork_pid,getpid()) == -1) {
                print_setpgid_failed();
                return -1;
            }
            int status;
            pid_t result_pid = waitpid(fork_pid, &status, 0);
            pro_destructor(fork_pid);
            if (result_pid == -1) {
                print_wait_failed();
                exit(1);
            }
            if (WIFEXITED(status) && WEXITSTATUS(status)) {
                return -1;
            }
            fflush(stdout);
        }
    }
    return 1;
}
/**
 * single command, where there are no logical ops
*/
int single_command(char * command) {
    if(!strncmp(command, "cd ",3)) {
        return cd_builtIn(command + 3);
    } else if (!strcmp(command, "!history")) {
        vector_pop_back(history); //history should not be stored
        for(size_t i = 0; i < vector_size(history); i++) {
            print_history_line(i, (char*) vector_get(history,i));
        }
        return 1;
    } else if (command[0] == '#') {
        return nth_buildIn(command);
    } else if (command[0] == '!') {
        return prefix_buildIn(command);
    } else if (!strcmp(command, "exit")) {
        vector_pop_back(history);
        return 0;
    } else if (!strcmp(command, "ps")) {
        return ps_buildIn(NULL, NULL, -1);
    } else if (!strncmp(command, "kill", 4)) {
        if(strlen(command)< 6) {
            print_invalid_command(command);
            return -1;
        }
        return ksc_buildIn(atoi(command + 5), SIGKILL);
    } else if (!strncmp(command, "stop", 4)) {
        if(strlen(command)< 6) {
            print_invalid_command(command);
            return -1;
        }
        return ksc_buildIn(atoi(command + 5), SIGSTOP);
    } else if (!strncmp(command, "cont", 4)) {
        if(strlen(command)< 6) {
            print_invalid_command(command);
            return -1;
        }
        return ksc_buildIn(atoi(command + 5), SIGCONT);
    } else {
        return external(command, NULL, NULL, -1);
    }
}
/**
 * handle Command
*/
int handle_command(char * command) {
    /**
     * 1 - &&
     * 2 - ||
     * 3 - ;
     * 4 - >>
     * 5 - >
     * 6 - <
    */
    int logical_op = 0;
    int op_idx = 0;
    for (size_t i = 0; i < strlen(command) - 1; i++) {
        if (command[i] == command[i+1] && command[i] == '&') {
            logical_op = 1;
            op_idx = i;
            break;
        } else if (command[i] == command[i+1] && command[i] == '|') {
            logical_op = 2;
            op_idx = i;
            break;
        } else if (command[i] == ';') {
            logical_op = 3;
            op_idx = i;
            break;
        } else if (command[i] == '>' && command[i] == command[i+1]) {
            logical_op = 4;
            op_idx = i;
            break;
        } else if (command[i] == '>') {
            logical_op = 5;
            op_idx = i;
            break;
        } else if (command[i] == '<') {
            logical_op = 6;
            op_idx = i;
            break;
        }
        
    }
    if (!logical_op) {
        return single_command(command);
    } else {
        if (logical_op == 1) { // AND &&
            char cmd1[op_idx];
            strncpy(cmd1, command, op_idx - 1);
            cmd1[op_idx - 1] = '\0';
            int status = single_command(cmd1);
            if (status < 0) { return -1;}
            char cmd2[strlen(command)- op_idx - 1];
            strncpy(cmd2, command + op_idx + 3, strlen(command)-op_idx -2);
            cmd2[strlen(command)-op_idx - 2] = '\0';
            return single_command(cmd2);
        } else if(logical_op == 2) { // OR ||
            char cmd1[op_idx];
            strncpy(cmd1, command, op_idx - 1);
            cmd1[op_idx - 1] = '\0';
            int status = single_command(cmd1);
            if (status > 0) { return 1;}
            char cmd2[strlen(command)- op_idx - 1];
            strncpy(cmd2, command + op_idx + 3, strlen(command)-op_idx -2);
            cmd2[strlen(command)-op_idx - 2] = '\0';
            return single_command(cmd2);
        } else if (logical_op == 3) { // SEPARATOR ;
            char cmd1[op_idx + 1];
            strncpy(cmd1, command, op_idx);
            cmd1[op_idx] = '\0';
            single_command(cmd1);
            char cmd2[strlen(command)- op_idx - 1];
            strncpy(cmd2, command + op_idx + 2, strlen(command)-op_idx -2);
            cmd2[strlen(command)-op_idx - 2] = '\0';
            return single_command(cmd2);
        } else if (logical_op == 5) { // OUTPUT >
            char file[strlen(command)- op_idx - 1];
            strncpy(file, command + op_idx + 2, strlen(command)-op_idx -2);
            char cmd1[op_idx + 1];
            strncpy(cmd1, command, op_idx);
            cmd1[op_idx] = '\0';
            if(!strcmp(cmd1, "ps")) {
                return ps_buildIn(file, "w+", 1);
            } else {
                return external(cmd1, file, "w+", 1);
            }
            
        } else if (logical_op == 4) { //APPEND >>
            char file[strlen(command)- op_idx - 1];
            strncpy(file, command + op_idx + 3, strlen(command)-op_idx -2);
            file[strlen(command)-op_idx - 2] = '\0';
            char cmd1[op_idx + 1];
            strncpy(cmd1, command, op_idx);
            cmd1[op_idx] = '\0';
            if(!strcmp(cmd1, "ps")) {
                return ps_buildIn(file, "a+", 1);
            } else {
                return external(cmd1, file, "a+", 1);
            }
            return 1;
        } else { // INPUT <
            char file[strlen(command)- op_idx - 1];
            strncpy(file, command + op_idx + 2, strlen(command)-op_idx -2);
            file[strlen(command)-op_idx - 2] = '\0';
            char cmd1[op_idx + 1];
            strncpy(cmd1, command, op_idx);
            cmd1[op_idx] = '\0';
            if(!strcmp(cmd1, "ps")) {
                return ps_buildIn(file, "r", 0);
            } else {
                return external(cmd1, file, "r", 0);
            }
            return 1;
        }
        
        
    }
}
/**
 * history file handler
*/
void hist_handler(FILE * hist_file) {
    if (hist_file == NULL) {return;}
    // ssize_t write(int fd, const void *buf, size_t count);
    for (size_t i = 0; i < vector_size(history); i++) {
        char * line = vector_get(history, i);
        //write(hist_file, line, strlen(line) + 1);
        fprintf(hist_file,"%s\n",line);
    }
    fclose(hist_file);
}
/**
 * SIGINT & SIGCHLD HANDLER
 * 
*/
void sigHandler() {
    for(size_t i = 0; i < vector_size(process_running); i++) {
        struct process * pro = (process *)vector_get(process_running, i);
        if(pro->pid != getpgid(pro->pid)){
            kill(pro->pid, SIGKILL);
            pro_destructor(pro->pid);
        }
    }
    return;
}
/**
 * shell main function
 * INPUT: ./shell -h <filename>
*/
int shell(int argc, char *argv[]) {
    signal(SIGINT, sigHandler);
    process_running = shallow_vector_create();
    history = string_vector_create();
    /*INPUT FLAG HANDLER*/
    FILE*  his_file = NULL;
    FILE*  cmd_file = NULL;
    /**
     * If flag in argv is -h (history) then this variable is 1;
     * Else the flag is -f (file) then this variable is -1;
    */
    if (argc > 1) {
        int file_type = 0;
        if (!strcmp(argv[1], "-h")) {
            file_type = 1;
            his_file = fopen(argv[2], "a+");
            if(!his_file) print_script_file_error();
            char *his_cmd = NULL;
            size_t his_cmd_size = 0;
            while(1){
                ssize_t getline_return = getline(&his_cmd, &his_cmd_size, his_file);
                if(getline_return == -1) {
                    free(his_cmd);
                    his_cmd = NULL;
                    break;
                }
                if (his_cmd[strlen(his_cmd) - 1] == '\n') {
                    his_cmd[strlen(his_cmd) - 1] = '\0';
                }
                if(his_cmd[0] == '\n'){
                free(his_cmd);
                his_cmd = NULL;
                continue;
                }
                vector_push_back(history, his_cmd);
                free(his_cmd);
                his_cmd = NULL;
            }
            if (argc > 3) {
                if (!strcmp(argv[3], "-f")) {
                    cmd_file = fopen(argv[4], "r");
                    if(!cmd_file) print_script_file_error();
                } else {
                    print_usage();
                    exit(1);
                }
            }
        } else if (!strcmp(argv[1], "-f")) {
            file_type = -1; 
            cmd_file = fopen(argv[2], "r");
            if(!cmd_file) print_script_file_error();
            if (argc > 3) {
                if (!strcmp(argv[3], "-f")) {
                    his_file = fopen(argv[4], "a");
                    if(!his_file) print_script_file_error();
                    char *his_cmd = NULL;
                    size_t his_cmd_size = 0;
                    while(1){
                        ssize_t getline_return = getline(&his_cmd, &his_cmd_size, his_file);
                        if(getline_return == -1) {
                            free(his_cmd);
                            his_cmd = NULL;
                            break;
                        }
                        if (his_cmd[strlen(his_cmd) - 1] == '\n') {
                            his_cmd[strlen(his_cmd) - 1] = '\0';
                        }
                        
                        if(his_cmd[0] == '\n'){
                        free(his_cmd);
                        his_cmd = NULL;
                        continue;
                        }
                        vector_push_back(history, his_cmd);
                        free(his_cmd);
                        his_cmd = NULL;
                    }
                } else {
                    print_usage();
                    exit(1);
                }
            }
        } else {
            print_usage();
            exit(1);
        }
        
    }
    /*INITIALIZING THE SHELL*/
    char* directory = calloc(50,1);
    directory = getcwd(directory, 50);
    pid_t shell_pid = getpid();
    next_cmd = NULL;
    int dont_print_prompt = 0;
    /*TAKING & RUNNING COMMAND*/
    while (1) {
        //printf("next command is : %s, and dont_print_prompt is :%d\n", next_cmd, dont_print_prompt);
        if (!next_cmd && !dont_print_prompt) {print_prompt(directory, shell_pid);}
        char *cmd = NULL;
        size_t cmd_size = 0;
        if (next_cmd) {
            cmd = calloc(strlen(next_cmd) + 1, 1);
            cmd_size = strlen(next_cmd);
            cmd = strcpy(cmd, next_cmd);
            free(next_cmd);
            next_cmd = NULL;
        } else if(cmd_file) {
            ssize_t getline_return = getline(&cmd, &cmd_size, cmd_file);
            if(getline_return == -1) {
                fclose(cmd_file);
                cmd_file = NULL;
                free(cmd);
                cmd = NULL;
                dont_print_prompt = 1;
                continue;
            }
            if (cmd[strlen(cmd) - 1] == '\n') {
                cmd[strlen(cmd) - 1] = '\0';
            }
            if(cmd[0] == '\n'){
            free(cmd);
            cmd = NULL;
            continue;
            }
            cmd[cmd_size - 2] = '\0';
            print_command(cmd);
        } else {
            //printf("we do perform a normal getline\n");
            ssize_t getline_return = getline(&cmd, &cmd_size, stdin);
            if(getline_return == -1) {exit(1);}
            if (cmd[strlen(cmd) - 1] == '\n') {
                cmd[strlen(cmd) - 1] = '\0';
            }
            if(cmd[0] == '\n'){
            free(cmd);
            cmd = NULL;
            continue;
            }
            cmd[cmd_size - 2] = '\0';
            dont_print_prompt = 0;
        }
        if (strlen(cmd) < 1) { //Handle new line
            free(cmd);
            cmd = NULL;
            continue;
        }
        vector_push_back(history, cmd);
        int exit = handle_command(cmd);
        free(cmd);
        cmd = NULL;
        if (!exit) {break;}
    }
    /**
     * NEED TO FREE
     * 1. directory
     * 2. getline malloc space
     * 3. history file
     * 4. command file
     * 5. proccess in process_running
     * 6. process_running
    */
    hist_handler(his_file);
    if(cmd_file) fclose(cmd_file);
    vector_destroy(history);
    free(directory);
    for(size_t i = 0; i < vector_size(process_running); i++) {
        struct process * pro = (process *)vector_get(process_running, i);
        kill(pro->pid, SIGKILL);
        pro_destructor(pro->pid);
    }
    vector_destroy(process_running);
    return 0;
}
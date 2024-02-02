#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/param.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<sys/wait.h>

struct bg_pro
{
    pid_t pid;
    char* command;
    struct bg_pro *next;
};

struct bg_pro *head = NULL;

//insert into the linked list
void insert(pid_t pid, char* command)
{
    if(head == NULL){
        head = (struct bg_pro*)malloc(sizeof(struct bg_pro));
        head->pid = pid;
        head->command = strdup(command); //change to strncpy
        head->next = NULL;
        return;
    }else{
        struct bg_pro *temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = (struct bg_pro*)malloc(sizeof(struct bg_pro));
        temp->next->pid = pid;
        temp->next->command = strdup(command);
        temp->next->next = NULL;
    }
}

//delete from the linked list
void delete(pid_t pid)
{
    if(head == NULL){
        return;
    }else if(head->pid == pid){
        struct bg_pro *temp = head;
        head = head->next;
        free(temp);
        return;
    }else{
        struct bg_pro *temp = head;
        while(temp->next != NULL){
            if(temp->next->pid == pid){
                struct bg_pro *temp2 = temp->next;
                temp->next = temp->next->next;
                free(temp2);
                return;
            }
            temp = temp->next;
        }
    }
}



//TODO: Fix
void dealloc_arr(char **arr, int num_args){
    int i = 0;
    for(int i = 0; i < num_args; i++){
        free(arr[i]);
    }
    free(arr);
}

int count_args(char *arg_str){
    char *str_cpy = malloc(sizeof(char)* strlen(arg_str) + 1);
    strncpy(str_cpy, arg_str, strlen(arg_str) + 1);

    int ret_val = 0;
    char *cur_word = strtok(str_cpy, " \n");
    while(cur_word != NULL){
        cur_word = strtok(NULL, " \n");
        ret_val++;
    }
    free(str_cpy);
    return ret_val;
}

void print_bg_list(){
    struct bg_pro *temp = head;
    int bg_count = 0;
    while(temp != NULL){
        printf("%d: %s %d\n", temp->pid, temp->command, bg_count);
        bg_count++;
        temp = temp->next;
    }
    printf("Total background jobs: %d\n", bg_count);
}

void check_child(){
    if(head != NULL){
        pid_t ter= waitpid(0, NULL, WNOHANG); 
        while(ter > 0){
            if(ter > 0){
                if(head->pid == ter){
                    printf("%d: %s has terminated\n", head->pid, head->command);
                    head = head->next;
                    delete(ter);
                }else{
                    struct bg_pro *temp = head;
                    while(temp->next != NULL){
                        if(temp->next->pid == ter){
                            printf("%d: %s has terminated\n", temp->next->pid, temp->next->command);
                            temp->next = temp->next->next;
                            delete(ter);
                        }
                        temp = temp->next;
                    }
                }
            }
            ter = waitpid(0, NULL, WNOHANG);
        }
    }
}

//Function to read arguments from cmd line and execute using execvp
int read_exc_args(char *arg_str, char ** argv_ret){
    int num_args = count_args(arg_str);

    //parsing given args

    //extra space for null terminoator
    argv_ret = malloc(sizeof(char*) * (num_args+1));
    char *cur_word = strtok(arg_str, " \n");
    if(cur_word == NULL) {
        argv_ret[0] = arg_str;
    }
    int i = 0;
    int total_cmd_char = 0;
    while(cur_word != NULL){
        total_cmd_char += strlen(cur_word);
        argv_ret[i] = malloc(sizeof(char) * strlen(cur_word) + 1);
        strncpy(argv_ret[i], cur_word, strlen(cur_word) + 1);

        cur_word = strtok(NULL, " \n");
        i++;
    }
    argv_ret[num_args+2]=NULL;
    


    if(strncmp(argv_ret[0], "cd", 2) == 0){
        if(argv_ret[1] == NULL || strncmp(argv_ret[1], "~", 1) == 0){
            chdir(getenv("HOME"));
        }else{
            if(argv_ret[1][0] != '/' && argv_ret[1][0] != '.'){
                int fpath_len = strlen(argv_ret[1]) + 3;
                char *file_path = malloc(sizeof(char) * fpath_len);
                snprintf(file_path, fpath_len, "./%s", argv_ret[0]);
            }
            chdir(argv_ret[1]);
        }
        return 1;
    }else if(strncmp(argv_ret[0], "bglist", 6) == 0){
        print_bg_list();
    }else if(strncmp(argv_ret[0], "bg", 2) == 0){
        //We need to fork and run the remaining elements in argv (cut out bg) 
        //Concatenate remaing strings in commmand for ll
            
        char *full_cmd_str = malloc(sizeof(char) * total_cmd_char);
        int l = 0;
        while(argv_ret[l] != NULL){

           strncat(full_cmd_str, argv_ret[l], strlen(argv_ret[l]) + strlen(full_cmd_str) + 1);
           strncat(full_cmd_str, " ", 1);
           argv_ret[l-1] = argv_ret[l];
           l++;
        }
         
        //then do fork, in parent process add child process to bglist
        pid_t bg_pid = fork();
        if(bg_pid == 0){
            execvp(argv_ret[0], argv_ret);
        }else{
            insert(bg_pid, full_cmd_str);
        }
    }else{
        //Executing given arguments
        pid_t p = fork();
        if(p < 0){
            printf("Fork failed :(\n");
        }else if(p == 0){
            //running child process
            execvp(argv_ret[0], argv_ret);
        }else{
            //parent waits for child process to complete
            wait(NULL);
        }
        dealloc_arr(argv_ret, num_args);
        return 0;
    }
    return 3;
}

int len_cur_dir(){
    char *cur_user = getlogin();
    char *hostname = malloc(sizeof(char) * 255); 
    int hostflag = gethostname(hostname, 255); //max hostname len
    
    char *cwd = malloc(sizeof(char) * MAXPATHLEN);
    getcwd(cwd, MAXPATHLEN);
    
    int ovr_len = strlen(cur_user) + strlen(hostname) + strlen(cwd) + 8; //account for nulls and new chars
    return ovr_len;
}

void set_cur_dir(char *input){

    char *cur_user = getlogin();
    char *hostname = malloc(sizeof(char) * 255); 
    int hostflag = gethostname(hostname, 255); //max hostname len
    
    char *cwd = malloc(sizeof(char) * MAXPATHLEN);
    getcwd(cwd, MAXPATHLEN);
    
    int ovr_len = len_cur_dir();
    snprintf(input, ovr_len, "%s@%s: %s> ", cur_user, hostname, cwd);
    free(hostname);
    free(cwd);
}

int main(){
    int ovr_len = len_cur_dir();
    char *prompt = malloc(sizeof(char) * ovr_len);
    set_cur_dir(prompt);
    int bailout = 0;
    while(!bailout){

        char* reply = readline(prompt);

        if(strncmp(reply, "exit", strlen(reply)) == 0){
            bailout = 1;
            exit(1);
        }else{
            check_child();
            char** args;
            int event_type = read_exc_args(reply, args);
            if(event_type == 1){
                set_cur_dir(prompt);
            }
        }
        free(reply);
    }

    free(prompt);

}


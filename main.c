#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define PIPE_LIMIT 2
#define PER_COMMAND_TOKEN_LIMIT 5

char* builtin_commands[] = {
    "cd",
    "help",
};

int start_shell();

void print_ascii_art(){
        puts(
        "__   __        _           _       _____ _          _ _ \n"
        "\\ \\ / /       | |         ( )     /  ___| |        | | |\n"
        " \\ V /__ _ ___| |__  _   _|/ ___  \\ `--.| |__   ___| | |\n"
        "  \\ // _` / __| '_ \\| | | | / __|  `--. \\ '_ \\ / _ \\ | |\n"
        "  | | (_| \\__ \\ | | | |_| | \\__ \\ /\\__/ / | | |  __/ | |\n"
        "  \\_/\\__,_|___/_| |_|\\__,_| |___/ \\____/|_| |_|\\___|_|_|\n"
        "\n"
        "                 Use at your own risk! \n"

    );
}

void print_prompt(){
    char* username = getenv("USER");
    char* dirname[1024];
    getcwd(dirname,sizeof(dirname));
    printf("%s@%s$ ",username,dirname);
}

char* read_input(){
    int INPUT_CMD_LIMIT = 1024;
    char* input = malloc(INPUT_CMD_LIMIT*sizeof(char));
    int i = 0;
    while(1){
        char c = getchar();
        if(c == '\n' || c == EOF){
            input[i]='\0';
            // printf("read input as : %s \n",input);
            return input;
        }
        input[i]=c;
        i++;
        if(i>=INPUT_CMD_LIMIT){
            char* temp = realloc(input,INPUT_CMD_LIMIT*2);
            if (temp == NULL) {
                printf("Memory reallocation failed\n");
                free(input);
                exit(EXIT_FAILURE);
            }
            input = temp;
        }
    }
    
    return input;
}

bool convert_args(char* input, char*** args){
    int i = 0;
    int j=0;
    bool ispipe =false;
    char* token = strtok(input," ");
    while(1){
        
        if(strcmp(token,"|")==0){
            if(i==0 && j==0){
                printf("Invalid command : | at the beginning\n");
                exit(EXIT_FAILURE);
            }
            ispipe = true;
            args[i][j]= NULL;
            i++;        
            if(i>=PIPE_LIMIT){
                printf("Too many pipes\n");
                exit(EXIT_FAILURE);
            }
            j=0;
            token = strtok(NULL," ");
            continue;
        }
        if(args[i]==NULL){
            args[i] = malloc(PER_COMMAND_TOKEN_LIMIT*sizeof(char*));
            if(args[i]==NULL){
                printf("Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
        }
        if(j>=PER_COMMAND_TOKEN_LIMIT){
            printf("Too many arguments\n");
            exit(EXIT_FAILURE);
        }
        args[i][j] = token;
        j++;
        token = strtok(NULL," ");
        if(token == NULL){
            break;
        }
    }
    args[i][j]=NULL;
    return ispipe;
}

int execute_command(char** args){

    for(int i=0;i< sizeof(builtin_commands)/sizeof(builtin_commands[0]);i++){
        if(strcmp(args[0],builtin_commands[i])==0){
            switch(i){
                case 0 :  
                        if(chdir(args[1]) != 0){
                            printf("error executing chdir");
                        }
                        break;
                case 1 : 
                        puts("this shell is designed for educational purpose\n"
                            "can handle pipe with two commands! enjoy ;)\n"
                            );
                        break;
            }
            return 1;
        }
    }

    pid_t pid = fork();
    pid_t wpid;
    int status;
    if(pid==0){
      //child  
      if(execvp(args[0],args)==-1)
        exit(EXIT_FAILURE);
      exit(EXIT_SUCCESS);     
    }
    else{
        // parent
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 0;
}

int execute_pipe_command(char*** args){
    int pipefd[2];
    if(pipe(pipefd) < 0){
        printf("error while creating pipe");
        exit(EXIT_FAILURE);
    }
    pid_t cpid1 = fork();
    if(cpid1 < 0){
        printf("Error creating child process");
        exit(EXIT_FAILURE);
    }
        
    
    if(cpid1 == 0){
        //child1
        close(pipefd[0]);
        close(1);
        dup2(pipefd[1],STDOUT_FILENO);
        close(pipefd[1]);
        if(execute_command(args[0])==1){
            printf("error executing command 1");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else{
        //parent
        pid_t cpid2 = fork();
        if(cpid2 < 0){
            printf("error while creating pipe");
            exit(EXIT_FAILURE);
        }
        if(cpid2 == 0){
            //child 2
            close(pipefd[1]);
            close(0);
            dup2(pipefd[0],STDIN_FILENO);
            close(pipefd[0]);
            if(execute_command(args[1])==1){
                printf("error executing command 2");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        else{
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(cpid1, NULL,0);
            waitpid(cpid2,NULL,0);
        }
    }
    return 0;
}

int start_shell(){
    char*** args = malloc(PIPE_LIMIT*sizeof(char**));
    while(1){
        print_prompt();
        char* input = read_input();
        if(input[0] == '\0') continue;
        
        if(strcmp(input,"exit")==0){
            free(input);
            break;
        }

        bool ispipe = convert_args(input,args);

        if(ispipe){
            //pipe
            execute_pipe_command(args);
        }
        else{
            execute_command(args[0]);
        }

        free(input);
    }
    int i = 0;
    int j=0;
    while(args[i] != NULL){
        while(args[i][j]!=NULL){
            free(args[i][j]);
            j++;
        }
        free(args[i]);
        i++;
    }
    
    return 0;
}



int main(){
    print_ascii_art();
    start_shell();
    return 0;
}

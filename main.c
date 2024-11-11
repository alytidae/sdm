#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

enum TokenType{
    TEXT,         // unmodified text
    START_BLOCK,  // @@sdm-block
    STOP_BLOCK,   // @@
    DEVICE_PAREN, // ~~
    DEVICE_NAME,  // ~~device~~ between ~~
    CONFIG,       // 
    END_OF_FILE,  // \0
};

struct Token{
    enum TokenType type;
    char* lexeme;
};

void show_help(){
    printf("Usage:\n");
    printf("    sdm [OPTIONS] [file]\n\n");
    printf("Options:\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -s, --devices            Show available devices in the config file (requires a file to be provided)\n");
    printf("  -d, --device             Get the device config (requires a file to be provided)\n");
    printf("\n");
}

void show_devices(struct Token* tokens){
    char names[100][1000];
    int n_i = 0;
    for (int i = 0; tokens[i].type != END_OF_FILE; i++){
        if (tokens[i].type == DEVICE_NAME){
            bool exists = false;
            for (int j = 0; j < n_i; j++){
                if (strcmp(names[j], tokens[i].lexeme) == 0){
                    exists = true;
                }
            }
            if (!exists){
                strcpy(names[n_i], tokens[i].lexeme);
                names[n_i++][strlen(tokens[i].lexeme)] = '\0';
            }
        }
    }

    printf("Avalible devices for this config file: \n");
    for (int i = 0; i < n_i; i++){
        printf("%d. %s\n", i+1, names[i]);
    }
}

void show_device_config(char* device, struct Token* tokens){
    for (int i = 0; tokens[i].type != END_OF_FILE; i++){
        if (tokens[i-2].type == DEVICE_NAME && strcmp(device, tokens[i-2].lexeme) == 0){
            printf("%s", tokens[i].lexeme);
        }else if(tokens[i].type == TEXT){
            printf("%s", tokens[i].lexeme);
        }
    }
}


void add_token(enum TokenType type, char* lexeme, struct Token* tokens, int* token_index){
    struct Token t;
    t.type = type;
    t.lexeme = malloc(strlen(lexeme) + 1);
    if (t.lexeme != NULL) { 
        strcpy(t.lexeme, lexeme); 
        t.lexeme[strlen(lexeme)] = '\0';
    }
    
    tokens[(*token_index)++] = t;
}

void scan_tokens(char* text, int size, struct Token* tokens){
    int i = 0;
    int token_i = 0;

    bool block_start = false;
    bool device_start = false;

    char stream[size];
    int s_i = 0;

    while (text[i] != '\0'){
        stream[s_i] = text[i];
        stream[s_i+1] = '\0';
       
        s_i++;
        i++;

        if(block_start){
            if (strstr(stream, "~~") != NULL && !device_start){
                device_start = true;
                
                char tmp_config[s_i - 2 + 1];
                snprintf(tmp_config, s_i - 2 + 1, "%.*s", s_i - 2, stream);
                add_token(CONFIG, tmp_config, tokens, &token_i);
                add_token(DEVICE_PAREN, "~~", tokens, &token_i);

                s_i = 0;

            }else if(strstr(stream, "~~") != NULL && device_start){
                device_start = false;
                                
                char tmp_config[s_i - 2 + 1];
                snprintf(tmp_config, s_i - 2 + 1, "%.*s", s_i - 2, stream);
                add_token(DEVICE_NAME, tmp_config, tokens, &token_i);
                add_token(DEVICE_PAREN, "~~", tokens, &token_i);

                s_i = 0;
            }

        }

        if (strstr(stream, "@@sdm-block") != NULL && !block_start){
            block_start = true;

            char tmp_config[s_i - 11 + 1];
            snprintf(tmp_config, s_i - 11 + 1, "%.*s", s_i - 11, stream);
            add_token(TEXT, tmp_config, tokens, &token_i);
            add_token(START_BLOCK, "@@sdm-block", tokens, &token_i);

            s_i = 0;

        }else if(strstr(stream, "@@") != NULL && block_start){
            block_start  = false;

            char tmp_config[s_i - 2 + 1];
            snprintf(tmp_config, s_i - 2 + 1, "%.*s", s_i - 2, stream);
            add_token(CONFIG, tmp_config, tokens, &token_i);
            add_token(STOP_BLOCK, "@@", tokens, &token_i);

            s_i = 0;
        }
        
    }

    if (strlen(stream) > 1){
        add_token(TEXT, stream, tokens, &token_i);
    }

    add_token(END_OF_FILE, "\0", tokens, &token_i);
}

int main(int argc, char* argv[]) {

    int option_index = 0;
    int opt;

    bool config_provided = false;
    char* config = false;
    bool device_provided = false;
    char* device;
    bool devices_asked = false;

    static struct option long_options[] = {
        {"device",  required_argument, 0, 'd'},
        {"devices", no_argument,       0, 's'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "d:sh", long_options, &option_index)) != -1){
        switch(opt){
            case 'd':
                device_provided = true;
                device = optarg;
                break;
            case 'o':
                devices_asked = true;
                break;
            case 'h':
                show_help();
                break;
        }
    };

    if (optind < argc) 
    {
        config_provided = true;
        config = argv[optind];
    }

    if (argc == 1){
        show_help();
    }

    if (config_provided){
        FILE* fptr = fopen(config, "r");
        char* buffer;
        long buffer_size;
    
        if (!fptr){
            printf("ERROR: the file does not exists\n");
            return -1;
        }

        fseek(fptr, 0, SEEK_END);
        buffer_size = ftell(fptr);
        fseek (fptr, 0, SEEK_SET);
        buffer = malloc(buffer_size+1);

        if (!buffer){
            printf("ERROR: Not possible to allocate buffer\n");
            return -1;
        }
        fread(buffer, 1, buffer_size+1, fptr);
        buffer[buffer_size] = '\0';


        struct Token tokens[10000];
        scan_tokens(buffer, buffer_size, tokens);

        if(!device_provided || devices_asked){
            show_devices(tokens);
        }else{
            show_device_config(device, tokens);
        }

        for (int i = 0; tokens[i].type != END_OF_FILE; i++){
            free(tokens[i].lexeme);
        }
        fclose(fptr);
        free(buffer);
    }
}

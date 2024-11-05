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
    int line;
};

void show_help(){
    printf("Usage:\n");
    printf("    sdm [OPTIONS] [file]\n\n");
    printf("Options:\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -s, --devices PATH       Show available devices in the config file\n");
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

//TODO: Rewrite later
void scan_tokens(char* text, int size, struct Token* tokens){
    int i = 0;
    int token_i = 0;
    int line = 0;

    bool block_start = false;
    bool device_start = false;

    char stream[size];
    int s_i = 0;

    while (text[i] != '\0'){
        if (text[i] == '\n'){
            line++;
        }

        stream[s_i] = text[i];
        stream[s_i+1] = '\0';
       
        s_i++;
        i++;

        if(block_start){
            if (strstr(stream, "~~") != NULL && !device_start){
                device_start = true;
                
                char tmp_config[s_i - 2 + 1];

                strncpy(tmp_config, stream, s_i - 2 + 1);
                tmp_config[s_i - 2] = '\0';

                struct Token t;
                t.type = CONFIG;
                t.lexeme = malloc(strlen(tmp_config) + 1);
                if (t.lexeme != NULL) { 
                    strcpy(t.lexeme, tmp_config); 
                }
                t.line = line;

                tokens[token_i++] = t;

                t.type = DEVICE_PAREN;
                t.lexeme = malloc(3);
                if (t.lexeme != NULL) { 
                    strcpy(t.lexeme, "~~\0"); 
                }
                t.line = line;

                tokens[token_i++] = t;
                s_i = 0;
            }else if(strstr(stream, "~~") != NULL && device_start){
                device_start = false;
                                
                char tmp_device[s_i - 2 + 1];

                strncpy(tmp_device, stream, s_i - 2 + 1);
                tmp_device[s_i - 2] = '\0';

                struct Token t;
                t.type = DEVICE_NAME;
                t.lexeme = malloc(strlen(tmp_device) + 1);
                if (t.lexeme != NULL) { 
                    strcpy(t.lexeme, tmp_device); 
                }
                t.line = line;

                tokens[token_i++] = t;

                t.type = DEVICE_PAREN;
                t.lexeme = malloc(3);
                if (t.lexeme != NULL) { 
                    strcpy(t.lexeme, "~~\0"); 
                }
                t.line = line;

                tokens[token_i++] = t;
                s_i = 0;
            }

        }

        if (strstr(stream, "@@sdm-block") != NULL && !block_start){
            block_start = true;
            char tmp_text[s_i- 11 + 1];

            strncpy(tmp_text, stream, s_i - 11 +1);
            tmp_text[s_i - 11] = '\0';

            struct Token t;
            t.type = TEXT;
            t.lexeme = malloc(strlen(tmp_text) + 1);
            if (t.lexeme != NULL) { 
                strcpy(t.lexeme, tmp_text); 
            }
            t.line = line;
                   
            tokens[token_i++] = t;

            t.type = START_BLOCK;
            t.lexeme = malloc(11+1);
            if (t.lexeme != NULL) { 
                strcpy(t.lexeme, "@@sdm-block"); 
            }
            t.line = line;
            
            tokens[token_i++] = t;

            s_i = 0;

        }else if(strstr(stream, "@@") != NULL && block_start){
            block_start  = false;

            char tmp_config[s_i - 2 + 1];

            strncpy(tmp_config, stream, s_i - 2 + 1);
            tmp_config[s_i - 2] = '\0';

            struct Token t;
            t.type = CONFIG;
            t.lexeme = malloc(strlen(tmp_config) + 1);
            if (t.lexeme != NULL) { 
                strcpy(t.lexeme, tmp_config); 
            }
            t.line = line;

            tokens[token_i++] = t;

            t.type = STOP_BLOCK;
            t.lexeme = malloc(2 + 1);
            if (t.lexeme != NULL) { 
                strcpy(t.lexeme, "@@"); 
            }
            t.line = line;

            tokens[token_i++] = t;

            s_i = 0;
        }
        
    }

    if (strlen(stream) > 1){
        struct Token t;
        t.type = TEXT;
        t.lexeme = malloc(strlen(stream) + 1);
        if (t.lexeme != NULL) { 
            strcpy(t.lexeme, stream); 
        }
        t.line = line;

        tokens[token_i++] = t;
    }

    struct Token te = {END_OF_FILE, "\0", line};
    tokens[token_i++] = te;
}

int main(int argc, char* argv[]) {

    int option_index = 0;
    int opt;

    bool config_provided = false;
    char* config = false;
    bool device_provided = false;
    char* device;
    bool output_provided = false;
    char* output;

    static struct option long_options[] = {
        {"device",  required_argument, 0, 'd'},
        {"output",  required_argument, 0, 'o'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    //TODO: Add custom erros
    while ((opt = getopt_long(argc, argv, "d:o:h", long_options, &option_index)) != -1){
        switch(opt){
            case 'd':
                device_provided = true;
                device = optarg;
                break;
            case 'o':
                output_provided = true;
                output = optarg;
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

        if(!device_provided){
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

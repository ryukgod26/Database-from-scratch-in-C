// #ifdef _WIN32
// typedef long ssize_t;
// #endif

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct , Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100

typedef struct
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];

} Row;


const uint32_t ID_SIZE = size_of_attribute(Row,id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row,username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row,email);

//fields offsets
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct{
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;

typedef struct{
char* buffer;
size_t buffer_length;
ssize_t input_length;
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNISED_COMMAND
} MetaCommandResult;

typedef enum{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNISED_COMMAND,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum {
    STATEMENT_SELECT,
    STATEMENT_INSERT
} StatementType;




typedef struct{
    StatementType type;
    Row row_to_insert;
} Statement;

void close_input_buffer(InputBuffer* inputBuffer){
    free(inputBuffer->buffer);
    free(inputBuffer);
}

InputBuffer* get_input_buffer(){
InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
inputBuffer->buffer=NULL;
inputBuffer->buffer_length= 0;
inputBuffer->input_length=0;

return inputBuffer;
}



void print_prompt(){

printf("db > ");

}

MetaCommandResult do_meta_command(InputBuffer* inputBuffer){
    if(strcmp(inputBuffer->buffer,".exit") == 0){
        printf("Existing The Database Shell.");
        close_input_buffer(inputBuffer);
        exit(EXIT_SUCCESS);
    }
    else{
        return META_COMMAND_UNRECOGNISED_COMMAND;
    }
}

void read_input(InputBuffer* inputBuffer){

    ssize_t bytes_read = getline(&(inputBuffer->buffer),&(inputBuffer->buffer_length),stdin);

    if(bytes_read <= 0){
        printf("Error Reading Input.");
        exit(EXIT_FAILURE);
    }

    //Removing the New Line Character(\n)
    inputBuffer->input_length = bytes_read -1;
    //Replacing the New Line Character with null Terminator (\0)
    inputBuffer->buffer[bytes_read - 1] = 0;

}

PrepareResult prepare_statement(InputBuffer* inputBuffer , Statement* statement){

    if(strncmp(inputBuffer->buffer,"insert",6) == 0){
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(inputBuffer->buffer,"insert %d %s %s",
            &(statement->row_to_insert.id),
            &(statement->row_to_insert.username),
            &(statement->row_to_insert.email)
    );
    if(args_assigned <3 ){
        return PREPARE_SYNTAX_ERROR;
    }
        return PREPARE_SUCCESS;
    }

    if(strncmp(inputBuffer->buffer,"select",6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNISED_COMMAND;

}

void execute_statement(Statement* statement){
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        printf("This is an Insert Statement\n");
        break;
    
    case (STATEMENT_SELECT):
        printf("This is an Select Statement.\n");
        break;
    }
}

void serialize_row(Row* source, void* destination){
    //copying the value of id at the location dest+IDOffset of id size to store th buffer in memoery as a struct
    memcpy(destination + ID_OFFSET, &(source->id),ID_SIZE);
    memcmp(destination + USERNAME_OFFSET,&(source->username),USERNAME_SIZE);
    memcmp(destination + EMAIL_OFFSET,&(source->email),EMAIL_OFFSET);
}

void deserialize_row(void* source, Row* destination){

    memcpy(&(destination->id),source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET , USERNAME_SIZE);
    memcpy(&(destination->email),source + EMAIL_OFFSET,EMAIL_SIZE);

}

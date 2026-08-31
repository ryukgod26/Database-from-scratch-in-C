// #ifdef _WIN32
// typedef long ssize_t;
// #endif

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct , Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100

typedef struct
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE+1];
    char email[COLUMN_EMAIL_SIZE+1];

} Row;




#define ID_SIZE  size_of_attribute(Row,id)
#define USERNAME_SIZE  size_of_attribute(Row,username)
#define EMAIL_SIZE  size_of_attribute(Row,email)
//fields offsets
#define ID_OFFSET  0
#define USERNAME_OFFSET  (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET  (USERNAME_OFFSET + USERNAME_SIZE)

#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

#define PAGE_SIZE 4096
#define ROWS_PER_PAGE  (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)


typedef struct 
{
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct{
    uint32_t num_rows;
    Pager pager;
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
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
} PrepareResult;

typedef enum {
    STATEMENT_SELECT,
    STATEMENT_INSERT
} StatementType;

typedef enum{
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    EXECUTE_TABLE_FULL
} ExecuteResult;



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
        printf("Existing The Database Shell.\n");
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
        printf("Error Reading Input.\n");
        exit(EXIT_FAILURE);
    }

    //Removing the New Line Character(\n)
    inputBuffer->input_length = bytes_read -1;
    //Replacing the New Line Character with null Terminator (\0)
    inputBuffer->buffer[bytes_read - 1] = 0;

}

PrepareResult prepare_insert(InputBuffer* inputBuffer, Statement* statement){
    statement->type = STATEMENT_INSERT;

    char* keyword = strtok(inputBuffer->buffer," ");
    char* id_str = strtok(NULL," ");
    char* username = strtok(NULL," ");
    char* email = strtok(NULL," ");

    if(NULL == id_str || NULL == username || NULL == email ){
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_str);

    if(id < 0){
        return PREPARE_NEGATIVE_ID;
    }

    if(strlen(username) > COLUMN_USERNAME_SIZE-1){
        return PREPARE_STRING_TOO_LONG;
    }
    
    if(strlen(email) > COLUMN_EMAIL_SIZE -1){
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username , username);
    strcpy(statement->row_to_insert.email , email);

    return PREPARE_SUCCESS;

}

PrepareResult prepare_statement(InputBuffer* inputBuffer , Statement* statement){

    if(strncmp(inputBuffer->buffer,"insert",6) == 0){
        return prepare_insert(inputBuffer,statement);
    }

    if(strncmp(inputBuffer->buffer,"select",6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNISED_COMMAND;

}


void print_row(Row* row){
printf("id:%d ,Username:%s, Email:%s \n",row->id,row->username,row->email);
}


void serialize_row(Row* source, void* destination){
    //copying the value of id at the location dest+IDOffset of id size to store th buffer in memoery as a struct
    memcpy(destination + ID_OFFSET, &(source->id),ID_SIZE);
    memcpy(destination + USERNAME_OFFSET,&(source->username),USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET,&(source->email),EMAIL_OFFSET);
}

void deserialize_row(void* source, Row* destination){

    memcpy(&(destination->id),source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET , USERNAME_SIZE);
    memcpy(&(destination->email),source + EMAIL_OFFSET,EMAIL_SIZE);
}

void* get_page(Pager pager, uint32_t page_num){
    
}

void* row_slot(Table* table,uint32_t row_num){
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = get_page(table->pager,page_num);
    
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}


ExecuteResult execute_insert(Statement* statement, Table* table){
    if(table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row* row_to_insert = &(statement->row_to_insert);
    print_row(row_to_insert);
    serialize_row(row_to_insert,row_slot(table,table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}


ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    for(uint32_t i=0; i<table->num_rows;i++){
        deserialize_row(row_slot(table,i),&row);
        printf("test");
        print_row(&row);
    }

    return EXECUTE_SUCCESS;

}

ExecuteResult execute_statement(Statement* statement,Table* table){
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement,table);
    
    case (STATEMENT_SELECT):
        return execute_select(statement,table);
    }
}

Pager* pager_open(const char* filename){
    int fd = open(filename,
            O_RDWR | O_CREAT, //Read write or Create a new file
            S_IWUSR | S_IRUSR //User Write permission or User Read Permission
    );

    if(fd == -1){
        printf("Unable to Open File.\n");
        exit(EXIT_FAILURE);
    }

    off_t file_size = lseek(fd,0,SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_size;
    for(int i =0; i<TABLE_MAX_PAGES;i++){
        pager->pages[i] = NULL;
    }

    return pager;

}

Table* db_open(const char* filename){
    Pager* pager = pager_open(filename);
    Table* table = (Table*) malloc(sizeof(Table));
    return table;
}

void free_table(Table* table){
    for(int i =0; table->pages[i]; i++){
        free(table->pages[i]);
    }
    free(table);
}

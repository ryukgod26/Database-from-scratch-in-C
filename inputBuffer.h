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
#include<errno.h>

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
// #define ROWS_PER_PAGE  (PAGE_SIZE / ROW_SIZE)
// #define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)

//Common Node Header Layout

//size of a page
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
//offset of a node
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
//size of the parent node will be used to find its siblings
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

//LEAF NODE HEADER LAYOUT

const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;


//LEAF NODE BODY LAYOUT

const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE ;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;


//Functions to access Leaf Nodes Fields
uint32_t* leaf_node_num_cells(void* node){
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num){
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num){
    return leaf_node_cell(node,cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num){
    return leaf_node_cell(node,cell_num) + LEAF_NODE_KEY_SIZE;
}

void intialize_leaf_node(void* node){
    *leaf_node_num_cells(node) = 0;
}



typedef struct 
{
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
 } Pager;

typedef struct{
    // uint32_t num_rows;
    uint32_t root_page_num;
    Pager* pager;
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

typedef enum { NodeLeaf , NodeInternal } NodeType;

typedef struct{
Table* table;
uint32_t page_num;
uint32_t cell_num;
bool end_of_table;
} Cursor;

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

Cursor* table_start(Table* table){
Cursor* cursor = malloc(sizeof(Cursor));

cursor->table = table;
cursor->page_num = table->root_page_num;
cursor->cell_num = 0;

void* root_node = get_page(table->pager,table->root_page_num;
uint32_t num_cells = *leaf_node_num_cells(root_node);
cursor->end_of_table = (num_cells==0);



return cursor;

}

Cursor* table_end(Table* table){
Cursor* cursor = malloc(sizeof(Cursor));

cursor->table = table;
cursor->row_num = table->num_rows;
cursor->end_of_table = true;

return cursor;
// Add this line


}

void cursor_advance(Cursor* cursor){

//cursor->row_num += 1;
//if(cursor->row_num >= cursor->table->num_rows){
//cursor->end_of_table = true;
//}

uint32_t page_num = cursor->page_num;
void* node = get_page(cursor->table->pager,page_num);
cursor->cell_num +=1;

if(cursor->cell_num >= (*leaf_node_num_cells(node))){
cursor->end_of_table = true;

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
    strncpy(destination + USERNAME_OFFSET,source->username,USERNAME_SIZE);
    strncpy(destination + EMAIL_OFFSET,source->email,EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination){

    memcpy(&(destination->id),source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET , USERNAME_SIZE);
    memcpy(&(destination->email),source + EMAIL_OFFSET,EMAIL_SIZE);
}

void* get_page(Pager* pager, uint32_t page_num){


	if (page_num >= pager->num_pages){
			pager->num_pages = page_num +1;
			
			}
	return pager->pages[page_num];
//    pager->pages[page_num] = page;
    // if(page_num > TABLE_MAX_PAGES){
    //     printf("Requested is Out of Bounds.");
    //     exit(EXIT_FAILURE);
    // }

    // if(pager->pages[page_num] == NULL){
    //     //Page is not available in cache. Loading Page from the Disk.
    //     void* page = malloc(PAGE_SIZE);
    //     uint32_t num_pages = pager->file_length / PAGE_SIZE;
    //     pager->pages[page_num] = page;

    //     //checking if a page is partially stored in the database. 
    //     if(pager->file_length % PAGE_SIZE)
    //     {
    //     //Incrementing the number of pages to include the Partial Page.
    //     num_pages += 1;
    //     }
        
    //     //checking if requested page is in the total pages
    //     if(page_num <= num_pages){
    //         //Loading the Page into Memory
    //         lseek(pager->file_descriptor,page_num * PAGE_SIZE,SEEK_SET);
    //         ssize_t bytes_read = read(pager->file_descriptor,page,PAGE_SIZE);
    //         if(bytes_read == -1){
    //             printf("Error While Reading FIle %d \n",errno);
    //             exit(EXIT_FAILURE);
    //         }

    //     }

    // }

    // return pager->pages[page_num];
    
}

//Writing the data into the disk
void pager_flush(Pager* pager,uint32_t page_num){
    if (pager->pages[page_num] == NULL)
    {
        printf("Tried to flush NULL Page.\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor,page_num * PAGE_SIZE,SEEK_SET);

    if(offset == -1){
        printf("Error Seeking :d \n",errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor,pager->pages[page_num],PAGE_SIZE);

    if(bytes_written == -1){
        printf("Error While Writing %d \n",errno);
        exit(EXIT_FAILURE);
    }

}

void db_close(Table* table){
    Pager* pager = table->pager;

    for(uint32_t i=0; i< pager->pages; i++){
        if(pager->pages[i] == NULL){
            continue;
        }
        pager_flush(pager,i);
        free(pager->pages[i]);
        pager->pages[i] =NULL;
    }

    // //If there is a Partial Page. I am ggoing to remove it later when i switch to B-Tree.
    // uint32_t partial_page = table->num_rows % ROWS_PER_PAGE;
    // if(partial_page > 0){
    //     uint32_t page_num = num_full_pages;
    //     if(pager->pages[page_num] != NULL){
    //         pager_flush(pager,page_num,partial_page * ROW_SIZE);
    //         free(pager->pages[page_num]);
    //         pager->pages[page_num] = NULL;
    //     }
    // }

    int result = close(pager->file_descriptor);
    if(result == -1){
        printf("Error Closing Database File.\n");
        exit(EXIT_FAILURE);
    }

    //Freeing unused or partial pages.
    for(uint32_t i =0; i<TABLE_MAX_PAGES;i++){
        void* page = pager->pages[i];
        if(page){
            free(page);
            pager->pages[i] =NULL;
        }
    }

    free(pager);
    free(table);

    printf("Database Closed Successfully.\n");

}

void* cursor_value(Cursor* cursor){
//    uint32_t row_num = cursor->row_num;
  //  uint32_t page_num = row_num / ROWS_PER_PAGE;
  uint32_t page_num = cursor->page_num;
  
    void* page = get_page(cursor->table->pager,page_num);
  //  uint32_t row_offset = row_num % ROWS_PER_PAGE;
    //uint32_t byte_offset = row_offset * ROW_SIZE;
    return leaf_node_value(page,cursor->cell_num);
}


ExecuteResult execute_insert(Statement* statement, Table* table){
    if(table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row* row_to_insert = &(statement->row_to_insert);
    print_row(row_to_insert);
    Cursor* cursor = table_end(table);

//    serialize_row(row_to_insert,row_slot(table,table->num_rows));
    serialize_row(row_to_insert,cursor_value(cursor));
    table->num_rows += 1;

    free(cursor);
    return EXECUTE_SUCCESS;
}

MetaCommandResult do_meta_command(InputBuffer* inputBuffer,Table* table){
    if(strcmp(inputBuffer->buffer,".exit") == 0){
        printf("Existing The Database Shell.\n");
        db_close(table);
        close_input_buffer(inputBuffer);
        exit(EXIT_SUCCESS);
    }
    else{
        return META_COMMAND_UNRECOGNISED_COMMAND;
    }
}

ExecuteResult execute_select(Statement* statement, Table* table){

    Cursor* cursor = table_start(table);
    Row row;
//    for(uint32_t i=0; i<table->num_rows;i++){
  //      deserialize_row(row_slot(table,i),&row);
    //    printf("test");
      //  print_row(&row);
//    }
	while(!(cursor->end_of_table)){
	deserialize_row(cursor_value(cursor),&row);
	print_row(&row);
	cursor_advance(cursor);
	}

	free(cursor);
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
    pager->num_pages = (file_size/ PAGE_SIZE);

    if( file_size % PAGE_SIZE != 0){
    printf("DB File is not full of Pages. Corrupt File.\n");
    exit(EXIT_FAILURE);
    }

    for(int i =0; i<TABLE_MAX_PAGES;i++){
        pager->pages[i] = NULL;
    }

    return pager;

}

Table* db_open(const char* filename){
    Pager* pager = pager_open(filename);
//    uint32_t num_rows = pager->file_length / ROW_SIZE;

    Table* table = malloc(sizeof(Table));

    table->pager = pager;
//    table->num_rows = num_rows;
	table->root_page_num = 0;
	
	if(pager->num_pages == 0){
	//It means this is a new database file. So we intialize the db page 0 as leaf node.
	void* root_node = get_page(pager,0);
	intialize_leaf_node(root_node);
	}
    return table;
}

// void free_table(Table* table){
//     for(int i =0; table->pages[i]; i++){
//         free(table->pages[i]);
//     }
//     free(table);
// }
void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
void* page = get_page(cursor->table->pager,cursor->page_num);

uint32_t num_cells = *(leaf_node_num_cells(node));
if(num_cells >= LEAF_NODE_MAX_CELLS){
printf("Leaf Node is Full. Need to Implement Splitting a Node.");
exit(EXIT_FAILURE);
}
if(cursor->cell_num < num_cells){
//Making Room for new Cell
for(uint32_t i = num_cells; i > cursor->cell_num; i--){
memcpy(leaf_node_cell(node,i),leaf_node_cell(node,i-1),LEAF_NODE_CELL_SIZE);
}
}

*(leaf_node_num_cells(node)) += 1;
*(leaf_node_key(node,cursor->cell_num)) ,= key;
serialize_row(value,leaf_node_value(node,cursor->cell_num));
}
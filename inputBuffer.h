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
#define NODE_TYPE_SIZE  sizeof(uint8_t)
//offset of a node
#define NODE_TYPE_OFFSET  0
#define IS_ROOT_SIZE  sizeof(uint8_t)
#define IS_ROOT_OFFSET  NODE_TYPE_SIZE
//size of the parent node will be used to find its siblings
#define PARENT_POINTER_SIZE  sizeof(uint32_t)
#define PARENT_POINTER_OFFSET  (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE  (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

//LEAF NODE HEADER LAYOUT

#define LEAF_NODE_NUM_CELLS_SIZE  sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET  COMMON_NODE_HEADER_SIZE
#define LEAF_MODE_NEXT_LEAF_SIZE sizeof(uint32_t)
#define LEAF_NODE_NEXT_LEAF_OFFSET (LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE)
#define LEAF_NODE_HEADER_SIZE  (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_MODE_NEXT_LEAF_SIZE)


//LEAF NODE BODY LAYOUT

#define LEAF_NODE_KEY_SIZE  sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET  0
#define LEAF_NODE_VALUE_SIZE  ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET  (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE  (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define SPACE_FOR_CELLS  (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS  (SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)


//Internal Node Header Layout

#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)

//Internal Node Body Layout
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTENAL_NODE_CELL_SIZE (INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_CHILD_SIZE)

//Constants for Splitting the node
#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS+1) / 2)
#define LEAF_NODE_LEFT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS +1 ) - LEAF_NODE_RIGHT_SPLIT_COUNT)

//Internal Node Max Cells
#define PAGE_FOR_INTERNAL_NODE_CELLS (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE)
//#define INTERNAL_NODE_MAX_CELLS ( SPACE_FOR_CELLS / INTENAL_NODE_CELL_SIZE)
#define INTERNAL_NODE_MAX_CELLS 3


#define INVALID_PAGE_NUM UINT32_MAX




typedef struct 
{
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
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
    EXECUTE_TABLE_FULL,
    DUPLICATE_KEY
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


uint32_t* internal_node_right_child(void* node);
void create_new_root(Table* table,uint32_t right_child_page_num);
void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);

//Functions to Get and Set Node Type.,
NodeType get_node_type(void* node){
uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
return (NodeType)value;
}

void set_node_type(void* node,  NodeType type){
uint8_t value = type;
*((uint8_t*)(node + NODE_TYPE_OFFSET)) = value;
}




bool is_node_root(void* node){
uint8_t value = *((uint8_t*)(node + IS_ROOT_OFFSET));
return (bool)value;
}

void set_node_root(void* node, bool is_root){
    uint8_t value = is_root;
    *((uint8_t*)(node + IS_ROOT_OFFSET )) = value;
}




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

uint32_t* leaf_node_next_leaf(void* node){
return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

void initialize_leaf_node(void* node){
  set_node_type(node, NodeLeaf);
  set_node_root(node, false);
  *leaf_node_num_cells(node) = 0;
  *leaf_node_next_leaf(node) = 0; 
}


void close_input_buffer(InputBuffer* inputBuffer){
    free(inputBuffer->buffer);
    free(inputBuffer);
}


//Functions for Internal Node


uint32_t* internal_node_num_keys(void* node){
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

void intialize_internal_node(void* node){
    set_node_type(node,NodeInternal);
    set_node_root(node,false);
    *internal_node_num_keys(node) = 0;
    *internal_node_right_child(node) = INVALID_PAGE_NUM;
}


uint32_t* internal_node_right_child(void* node){
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num){
    return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTENAL_NODE_CELL_SIZE;
}

uint32_t* internal_node_child(void* node, uint32_t child_num){
    uint32_t num_keys = *internal_node_num_keys(node);
    if(child_num > num_keys){
        printf("Trying to access Child Number %d of a Internal Node.", child_num);
        exit(EXIT_FAILURE);
    }
    else if(child_num == num_keys){
       uint32_t* right_child = internal_node_right_child(node);
       if(*right_child == INVALID_PAGE_NUM){
        printf("Trying to access Right Child of a Internal Node but it is a Invalid Page.");
        exit(EXIT_FAILURE);
       }
       return right_child;

    }
    else{
        uint32_t* child = internal_node_cell(node,child_num);
        if(*child == INVALID_PAGE_NUM){
            printf("Trying to access a child of Internal Node but It was a Invalid Page ");
            exit(EXIT_FAILURE);
        }
        return child;
    }
}
uint32_t* internal_node_key(void* node, uint32_t key_num) {
  return (uint32_t*)((char*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE);
}

void* get_page(Pager* pager, uint32_t page_num) {
  if (page_num > TABLE_MAX_PAGES) {
    printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
           TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }

  if (pager->pages[page_num] == NULL) {
    // Cache miss. Allocate memory and load from file.
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->file_length / PAGE_SIZE;

    // We might save a partial page at the end of the file
    if (pager->file_length % PAGE_SIZE) {
      num_pages += 1;
    }

    if (page_num <= num_pages) {
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) {
        printf("Error reading file: %d\n", errno);
        exit(EXIT_FAILURE);
      }
    }

    pager->pages[page_num] = page;

    if (page_num >= pager->num_pages) {
      pager->num_pages = page_num + 1;
    }
  }

  return pager->pages[page_num];
}


// void* get_page(Pager* pager, uint32_t page_num){
//   if (page_num > TABLE_MAX_PAGES) {
//     printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
//            TABLE_MAX_PAGES);
//     exit(EXIT_FAILURE);
//   }

// 	if (page_num >= pager->num_pages){
// 			pager->num_pages = page_num +1;
			
// 			}
//     void* page = pager->pages[page_num];
//     if(page == NULL){
//         page = malloc(PAGE_SIZE);
//         pager->pages[page_num] = page;
//         if(page_num >= pager->num_pages){
//             pager->num_pages = page_num + 1;
//         }
//     }
// 	return pager->pages[page_num];
// //    pager->pages[page_num] = page;
//     // if(page_num > TABLE_MAX_PAGES){
//     //     printf("Requested is Out of Bounds.");
//     //     exit(EXIT_FAILURE);
//     // }

//     // if(pager->pages[page_num] == NULL){
//     //     //Page is not available in cache. Loading Page from the Disk.
//     //     void* page = malloc(PAGE_SIZE);
//     //     uint32_t num_pages = pager->file_length / PAGE_SIZE;
//     //     pager->pages[page_num] = page;

//     //     //checking if a page is partially stored in the database. 
//     //     if(pager->file_length % PAGE_SIZE)
//     //     {
//     //     //Incrementing the number of pages to include the Partial Page.
//     //     num_pages += 1;
//     //     }
        
//     //     //checking if requested page is in the total pages
//     //     if(page_num <= num_pages){
//     //         //Loading the Page into Memory
//     //         lseek(pager->file_descriptor,page_num * PAGE_SIZE,SEEK_SET);
//     //         ssize_t bytes_read = read(pager->file_descriptor,page,PAGE_SIZE);
//     //         if(bytes_read == -1){
//     //             printf("Error While Reading FIle %d \n",errno);
//     //             exit(EXIT_FAILURE);
//     //         }

//     //     }

//     // }

//     // return pager->pages[page_num];
    

// }



//Function to get the max key of a node
uint32_t get_node_max_key(Pager* pager,void* node){
  if (get_node_type(node) == NodeLeaf) {
    return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
  }
  void* right_child = get_page(pager,*internal_node_right_child(node));
  return get_node_max_key(pager, right_child);
}


//Funxtion to get the reference parent node of a node from the parent node offset
uint32_t* node_parent(void* node){
return node + PARENT_POINTER_OFFSET;
}

uint32_t get_unused_page_num(Pager* pager){
return pager->num_pages;
}


Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key){
    void* node = get_page(table->pager,page_num);
    uint32_t num_cells = *(leaf_node_num_cells(node));

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;

    //Binary Search
    uint32_t min_index = 0;
    uint32_t one_past_min_index = num_cells;
    while ((one_past_min_index != min_index))
    {
        uint32_t index = (min_index + one_past_min_index) /2;
        uint32_t key_at_index = *(leaf_node_key(node,index));
        if(key == key_at_index){
            cursor->cell_num = index;
            return cursor;
        }
        if(key < key_at_index){
            one_past_min_index = index;
        }
        else{
            min_index = index +1;
        }
    }
    cursor ->cell_num = min_index;
    return cursor;
}



uint32_t internal_node_find_child(void* node, uint32_t key){
    // return the index of the child which should return the given key

    uint32_t num_keys = *internal_node_num_keys(node);

    //Binary Search
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;
    if (num_keys == 0) return 0;  // Handle empty nodes
    while (min_index != max_index){
uint32_t  index = (min_index + max_index)/2;	    
uint32_t key_to_right = *internal_node_key(node,index);
if (key_to_right >= key){

max_index = index;

}
else{
min_index = index +1 ;

}
    }
    return min_index;

}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key){
    void* node = get_page(table->pager,page_num);

    uint32_t child_index = internal_node_find_child(node,key);
    uint32_t child_num = *internal_node_child(node,child_index);
    void* child = get_page(table->pager,child_num);
    
    switch (get_node_type(child))
    {
    case NodeLeaf:
        return leaf_node_find(table,child_num,key);
        break;
    
    case NodeInternal:
        return internal_node_find(table,child_num,key);
        break;
    }
}

//Function to Split and Insert the Internal Node
void internal_node_split_and_insert(Table* table,  uint32_t parent_page_num, uint32_t child_page_num){
uint32_t old_page_num = parent_page_num;
void* old_node = get_page(table->pager,old_page_num);
uint32_t old_max = get_node_max_key(table->pager,old_node);

void* child_node = get_page(table->pager,child_page_num);
uint32_t child_max = get_node_max_key(table->pager,child_node);

uint32_t new_page_num = get_unused_page_num(table->pager);

uint32_t splitting_node = is_node_root(old_node);
void* parent;
void* new_node;
if(splitting_node){
    create_new_root(table,new_page_num);
    parent =get_page(table->pager,table->root_page_num);
    old_page_num = *internal_node_child(parent,0);
    old_node = get_page(table->pager,old_page_num);
}
else{
    parent = get_page(table->pager,*node_parent(old_node));
    new_node = get_page(table->pager,new_page_num);
    intialize_internal_node(new_node);
}

uint32_t* old_num_keys = internal_node_num_keys(old_node);

uint32_t cur_page_num = *internal_node_right_child(old_node);
void* cur = get_page(table->pager,cur_page_num);


internal_node_insert(table,new_page_num,cur_page_num);
*node_parent(cur) = new_page_num;
*internal_node_right_child(old_node) = INVALID_PAGE_NUM;

for(int i = INTERNAL_NODE_MAX_CELLS -1; i > INTERNAL_NODE_MAX_CELLS/2; i--){
cur_page_num = *internal_node_child(old_node,i);
cur =  get_page(table->pager,cur_page_num);

internal_node_insert(table,new_page_num,cur_page_num);
*node_parent(cur) = new_page_num;
(*old_num_keys)--;
}
*internal_node_right_child(old_node) = *internal_node_child(old_node,*old_num_keys-1);
(*old_num_keys)--;

uint32_t max_after_split = get_node_max_key(table->pager,old_node);
uint32_t destination_page_num = child_max < max_after_split ? old_page_num : new_page_num;

internal_node_insert(table,destination_page_num,child_page_num);
*node_parent(child_node) = destination_page_num;
update_internal_node_key(parent,old_max,get_node_max_key(table->pager,old_node));

if(!splitting_node){
    internal_node_insert(table,*node_parent(old_node),new_page_num);
    *node_parent(new_node) = *node_parent(old_node);
}

}


//Function for Inserting in Internal Node
void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num){

    void* parent = get_page(table->pager,parent_page_num);
    void* child = get_page(table->pager,child_page_num);
    // uint32_t child_max_keys = get_node_max_key(child);
    uint32_t child_max_keys = get_node_max_key(table->pager,child);
    uint32_t index = internal_node_find_child(parent,child_max_keys);

    uint32_t original_num_keys = *internal_node_num_keys(parent);
    // *internal_node_num_keys(parent) = original_num_keys +1;

    if(original_num_keys >= INTERNAL_NODE_MAX_CELLS){
       internal_node_split_and_insert(table,parent_page_num,child_page_num);
       return ;
    }

    uint32_t right_child_page_num = *internal_node_right_child(parent);
      if (right_child_page_num == INVALID_PAGE_NUM) {
    *internal_node_right_child(parent) = child_page_num;
    return;
  }
    void* right_child = get_page(table->pager,right_child_page_num);
*internal_node_num_keys(parent) = original_num_keys + 1;
    if(child_max_keys > get_node_max_key(table->pager, right_child)){
        //Replacing the right Child
        *internal_node_child(parent,original_num_keys) = right_child_page_num;
        *internal_node_key(parent,original_num_keys) = get_node_max_key(table->pager, right_child);
        *internal_node_right_child(parent) = child_page_num;
    }
    else{
        //Making Room For the New Cell
        for(uint32_t i = original_num_keys; i > index; i--){
            void* destination =  internal_node_cell(parent,i);
            void* source = internal_node_cell(parent,i -1);
            memcpy(destination,source,INTENAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent,index) = child_page_num;
        *internal_node_key(parent,index) = child_max_keys;
    }
}


//Function to update internal node key
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key){
uint32_t old_child_index = internal_node_find_child(node, old_key);
*internal_node_key(node,old_child_index) = new_key;

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




//It is commnetwd out becasue It only works when our root node is a Leaf Node 
//when splitting happens an da Internal Node becomes the root node.
//This function return the cell 0 of root node (Internal ) whixh does not contain any rows and it displays wrobg Info 
/*
Cursor* table_start(Table* table){
Cursor* cursor = malloc(sizeof(Cursor));

cursor->table = table;
cursor->page_num = table->root_page_num;
cursor->cell_num = 0;

void* root_node = get_page(table->pager,table->root_page_num);
uint32_t num_cells = *leaf_node_num_cells(root_node);
cursor->end_of_table = (num_cells==0);



return cursor;

}
*/



// Cursor* internal_node_find(Table* table,uint32_t page_num,uint32_t key){
// void* node  = get_page(table->pager,page_num);
// uint32_t num_keys = *internal_node_num_keys(node);

// //Using Binary Search to find the Index of yhe Child to Search
// uint32_t min_index = 0;
// uint32_t max_index = num_keys; //Because Yhere is One more Child Then Key

// while(min_index != max_index){
// uint32_t index = (min_index+max_index)/2;
// uint32_t key_to_right = *internal_node_key(node,index);
// if(key_to_right >= key){
// max_index = index;
// }
// else{
// min_index = index+1;
// }
// }
// uint32_t child_num = *internal_node_child(node,min_index);
// void* child = get_page(table->pager,child_num);
// switch(get_node_type(child)){
// 	case NodeLeaf:
// 		return leaf_node_find(table,child_num,key);
// 		break;
// 	case NodeInternal:
// 		return internal_node_find(table,child_num,key);
// 		break;
// }


// }


//return the Position of a given Key.
//If the Key is not Present then return the location where key should be inserted
Cursor* table_find(Table* table,uint32_t key){
    uint32_t root_page_num = table->root_page_num;
    void* root_node = get_page(table->pager,root_page_num);

    if(get_node_type(root_node) == NodeLeaf){
        return leaf_node_find(table,root_page_num,key);
    }
    else{
//        printf("Searching for Internal Node is not Implemented Yet.");
  //      exit(EXIT_FAILURE);
	  return internal_node_find(table,root_page_num,key);
    }

}


Cursor* table_start(Table* table){
Cursor* cursor = table_find(table,0);

void* node = get_page(table->pager,cursor->page_num);
uint32_t num_cells = *leaf_node_num_cells(node);
cursor->end_of_table = (num_cells == 0);

return cursor;

}








// Cursor* table_end(Table* table){
// Cursor* cursor = malloc(sizeof(Cursor));

// cursor->table = table;
// // cursor->row_num = table->num_rows;
// cursor->page_num = table->root_page_num;

// void* root_node = get_page(table->pager,table->root_page_num);
// uint32_t num_cells = *(leaf_node_num_cells(root_node));
// cursor->cell_num = num_cells;
// cursor->end_of_table = true;

// return cursor;

// }

void cursor_advance(Cursor* cursor){

//cursor->row_num += 1;
//if(cursor->row_num >= cursor->table->num_rows){
//cursor->end_of_table = true;
//}

uint32_t page_num = cursor->page_num;
void* node = get_page(cursor->table->pager,page_num);
cursor->cell_num +=1;

if(cursor->cell_num >= (*leaf_node_num_cells(node))){
//cursor->end_of_table = true;
//Advance to the next leaf node 

uint32_t next_page_num = *leaf_node_next_leaf(node);
if(next_page_num == 0){
//This is the last leaf node.
cursor->end_of_table = true;
}
else{
cursor->page_num = next_page_num;
cursor->cell_num = 0;
}

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
    memcpy(destination + EMAIL_OFFSET,&(source->email),EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination){

    memcpy(&(destination->id),source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET , USERNAME_SIZE);
    memcpy(&(destination->email),source + EMAIL_OFFSET,EMAIL_SIZE);
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

    for(uint32_t i=0; i< pager->num_pages; i++){
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





void create_new_root(Table* table,uint32_t right_child_page_num){

    void* root = get_page(table->pager,table->root_page_num);
    void* right_child = get_page(table->pager,right_child_page_num);
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    void* left_child = get_page(table->pager,left_child_page_num);


    if(get_node_type(root) == NodeInternal){
        intialize_internal_node(right_child);
        intialize_internal_node(left_child);

    }

    //Copying the old root into left child
    memcpy(left_child,root,PAGE_SIZE);
    set_node_root(left_child,false);
        // printf("t2");
    if(get_node_type(left_child) == NodeInternal){
        void* child;
        for(int i =0; i< *internal_node_num_keys(left_child);i++){
            child = get_page(table->pager,*internal_node_child(left_child,i));
            *node_parent(child) = left_child_page_num;
        }
        child = get_page(table->pager,*internal_node_right_child(left_child));
        *node_parent(child) = left_child_page_num;
    }

    //Root Node is now a new Internal Node with one key and two children
    intialize_internal_node(root);
    set_node_root(root,true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root,0) = left_child_page_num;
    uint32_t left_child_max_keys = get_node_max_key(table->pager, left_child);
    *internal_node_key(root,0) = left_child_max_keys;
    *internal_node_right_child(root) = right_child_page_num;

    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
        // printf("t1");
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



void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value){
//Creating a new node and moving half the cells over. Insert the new values in one of two nodes. Updating Parent or creating a new Parent.

void* old_node = get_page(cursor->table->pager,cursor->page_num);
uint32_t old_max = get_node_max_key(cursor->table->pager, old_node);
uint32_t new_page_num = get_unused_page_num(cursor->table->pager);


void* new_node = get_page(cursor->table->pager,new_page_num);
initialize_leaf_node(new_node);
*node_parent(new_node) = *node_parent(old_node);
*leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
*leaf_node_next_leaf(old_node) = new_page_num;
//Dividing all the old keys and the new key evenly between the old node and new node.
//old node is left and new node is right.
//Starting from the left moving each key to the right position.
// printf("test");
 for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
    void* destination_node;
    if (i >= LEAF_NODE_LEFT_SPLIT_COUNT)
    {
        destination_node = new_node;
    }
    else{
        destination_node = old_node;
    }

    uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
    void* destination = leaf_node_cell(destination_node,index_within_node);

    if(i == cursor->cell_num){
        serialize_row(value,leaf_node_value(destination_node,index_within_node));
        *leaf_node_key(destination_node,index_within_node) = key;
    }
    else if(i > cursor->cell_num){
        memcpy(destination,leaf_node_cell(old_node,i-1),LEAF_NODE_CELL_SIZE);
    }
    else{
        memcpy(destination,leaf_node_cell(old_node,i),LEAF_NODE_CELL_SIZE);
    }
    
 }

*(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
*(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;


if(is_node_root(old_node)){
   return create_new_root(cursor->table,new_page_num);
}
else{
//    printf("YET to Implement Updating Parent AFter Splitting");
  //  exit(EXIT_FAILURE);
  uint32_t parent_page_num = *node_parent(old_node);
  uint32_t new_max = get_node_max_key(cursor->table->pager, old_node);
  void* parent = get_page(cursor->table->pager,parent_page_num);

  update_internal_node_key(parent,old_max,new_max);
  internal_node_insert(cursor->table,parent_page_num,new_page_num);
  return ;
}


}


void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
void* node = get_page(cursor->table->pager,cursor->page_num);

uint32_t num_cells = *(leaf_node_num_cells(node));
if(num_cells >= LEAF_NODE_MAX_CELLS){
leaf_node_split_and_insert(cursor,key,value);
return;
}
if(cursor->cell_num < num_cells){
//Making Room for new Cell
for(uint32_t i = num_cells; i > cursor->cell_num; i--){
memcpy(leaf_node_cell(node,i),leaf_node_cell(node,i-1),LEAF_NODE_CELL_SIZE);
}
}

*(leaf_node_num_cells(node)) += 1;
*(leaf_node_key(node,cursor->cell_num)) = key;
serialize_row(value,leaf_node_value(node,cursor->cell_num));
}


void print_constants(){
    printf("ROW SIZE: %d\n.",ROW_SIZE);
    printf("COMMON NODE HEADER SIZE: %d\n.",COMMON_NODE_HEADER_SIZE);
    printf("LEAF NODE HEADER SIZE: %d\n.",LEAF_NODE_HEADER_SIZE);
    printf("LEAF NODE CELL SIZE: %d\n.",LEAF_NODE_CELL_SIZE);
    printf("LEAF NODE MAX CELLS: %d\n.",LEAF_NODE_MAX_CELLS);
    printf("LEAF NODE KEY SIZE: %d\n.",LEAF_NODE_KEY_SIZE);
    printf("LEAF NODE VALUE SIZE: %d\n.",LEAF_NODE_VALUE_SIZE);
}

void print_leaf_node(void* node){
    uint32_t num_cells = *(leaf_node_num_cells(node));
    printf("leaf (size %d)\n.",num_cells);
    for(uint32_t i = 0; i< num_cells; i++){
        uint32_t key = *leaf_node_key(node,i);
        printf("  -%d : %d\n",i, key);
    }
}

ExecuteResult execute_insert(Statement* statement, Table* table){
    // if(table->num_rows >= TABLE_MAX_ROWS){
    //     return EXECUTE_TABLE_FULL;
    // }


    // if(num_cells >= LEAF_NODE_MAX_CELLS){
    //     return EXECUTE_TABLE_FULL;
    // }

    Row* row_to_insert = &(statement->row_to_insert);
    // print_row(row_to_insert);
    // Cursor* cursor = table_end(table);

    uint32_t key_to_insert = row_to_insert->id;
    Cursor* cursor = table_find(table,key_to_insert);
    
    void* node = get_page(table->pager,cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if(cursor->cell_num < num_cells){
        uint32_t key_at_index = *(leaf_node_key(node,cursor->cell_num));
        if(key_at_index == key_to_insert){
            return DUPLICATE_KEY;
        }
    }


//    serialize_row(row_to_insert,row_slot(table,table->num_rows));
    // serialize_row(row_to_insert,cursor_value(cursor));
    // table->num_rows += 1;
    leaf_node_insert(cursor,row_to_insert->id,row_to_insert);
    free(cursor);
    return EXECUTE_SUCCESS;
}

//helper to visualize the btree
void indent(uint32_t level){
    for(uint32_t i =0; i< level; i++){
        printf(" ");
    }
}

void print_tree(Pager* pager, uint32_t page_num , uint32_t indentation_level){
    
    void* node = get_page(pager,page_num);

    uint32_t num_keys,child;

    switch(get_node_type(node)){
        case NodeLeaf:
            num_keys = *leaf_node_num_cells(node);
            indent(indentation_level);
            printf("- leaf (size %d)\n ", num_keys);
            for(uint32_t i =0; i < num_keys;i++){
                indent(indentation_level+1);
                printf("- %d\n",*leaf_node_key(node,i));
            };
            break;


        case NodeInternal:
            num_keys = *internal_node_num_keys(node);
            indent(indentation_level);
            printf("- Internal (size %d)\n",num_keys);
            if (num_keys>0){
                for(uint32_t i =0; i< num_keys; i++){
                    child = *internal_node_child(node,i);
                    print_tree(pager,child,indentation_level+1);

                    indent(indentation_level +1);
                    printf("- key %d\n",*internal_node_key(node,i));
                }
                child = *internal_node_right_child(node);
            print_tree(pager,child,indentation_level+1);
            }
            
            break;
    }
}

MetaCommandResult do_meta_command(InputBuffer* inputBuffer,Table* table){
    if(strcmp(inputBuffer->buffer,".exit") == 0){
        printf("Existing The Database Shell.\n");
         close_input_buffer(inputBuffer);
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else if(strcmp(inputBuffer->buffer,".constants") == 0){
        printf("CONSTANTS: ");
        print_constants();
        return META_COMMAND_SUCCESS;
    }
    else if(strcmp(inputBuffer->buffer,".visualize")==0){
        printf("Tree: ");
        print_tree(table->pager,0,0);
        return META_COMMAND_SUCCESS;
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

    // if( file_size % PAGE_SIZE != 0){
    // printf("DB File is not full of Pages. Corrupt File.\n");
    // exit(EXIT_FAILURE);
    // }

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
    // printf("test");
	initialize_leaf_node(root_node);
    set_node_root(root_node,true);
	}
    return table;
}

// void free_table(Table* table){
//     for(int i =0; table->pages[i]; i++){
//         free(table->pages[i]);
//     }
//     free(table);
// }
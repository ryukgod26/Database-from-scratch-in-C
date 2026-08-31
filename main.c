#include "inputBuffer.h"

int main(int argc, char* argv[]){

if(argc < 2){
    printf("Please Provide a database file name.\n");
    printf("Format to run the Program is : ./a.exe database_file_name \n");
    exit(EXIT_FAILURE);
}

char* filename = argv[1];

InputBuffer* input = get_input_buffer();
Table* table = db_open(filename);
while(true){

print_prompt();
read_input(input);

if(input->buffer[0] == '.'){
    switch(do_meta_command(input,table)){
        case META_COMMAND_SUCCESS:
            continue;
        case META_COMMAND_UNRECOGNISED_COMMAND:
            printf("Uncrecognised Command %s \n" , input->buffer);
            continue;
    }

}

Statement statement;
switch (prepare_statement(input,&statement))
{
case PREPARE_SUCCESS:
    
    break;
case PREPARE_SYNTAX_ERROR:
    printf("Syntax error. Could Not Parse the Statement.\n");
    continue;

case PREPARE_UNRECOGNISED_COMMAND:
    printf("Unrecognised Keyword at the start  %s \n",input->buffer);
    continue;

case PREPARE_STRING_TOO_LONG:
    printf("String is too long!!!\n");
    continue;

case PREPARE_NEGATIVE_ID:
    printf("ID Cannot Be Negative.\n");
    continue;
    
}

switch (execute_statement(&statement,table))
{
case EXECUTE_SUCCESS:
    printf("\nEXECUTED SUCCEFULLY.\n");fflush(stdout);
    break;

case DUPLICATE_KEY:
    printf("\n You cannot Use Same key for Two VAlues.\n");
    break;
    
case EXECUTE_TABLE_FULL:
    printf("\nTABLE IS FULL\n");
    break;
}

}

// if(strcmp(input->buffer,".exit")==0){

// close_input_buffer(input);
// return 0;

// }
// else{
// printf("Unrecognised Command: %s\n",input->buffer);
// }



return 0;
}
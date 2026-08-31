#include "inputBuffer.h"

int main(char* argv , int argc){

InputBuffer* input = get_input_buffer();
while(true){

print_prompt();
read_input(input);

if(input->buffer[0] == '.'){
    switch(do_meta_command(input)){
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

case PREPARE_UNRECOGNISED_COMMAND:
    printf("Unrecognised Keyword at the start  %s \n",input->buffer);
    continue;
}

execute_statement(&statement);

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


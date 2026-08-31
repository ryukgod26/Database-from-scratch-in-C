#include "inputBuffer.h"

int main(char* argv , int argc){

InputBuffer* input = get_input_buffer();
while(true){

print_prompt();
read_prompt(input);


if(strcmp(input->buffer,".exit")==0){

close_input_buffer();
return 0;

}
else{
printf("Unrecognised Command: %s.\n",input->buffer);
}

}


return 0;
}

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>


typedef struct{
char* buffer;
size_t buffer_length;
ssize_t input_length;
} InputBuffer;


InputBuffer* get_input_buffer(){
InputBuffer* inputBuffer = (InputBuffer*) malloc(sizeof(InputBuffer));
inputBuffer->buffer=NULL;
inputBuffer->buffer_length= 0;
inputBuffer->input_length=0;

return inputBuffer;
}

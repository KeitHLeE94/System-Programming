#define MAXSIZE 20

typedef struct node{
  char* command;
  struct node* next;
}node;

typedef struct stack{
  int top;
  node* header;
}stack;

node* create_node(char* command){
  int command_size = strlen(command) + 1;
  node* new_node = (node*) calloc(1, sizeof(node));
	new_node->next = NULL;
	new_node->command = (char*)calloc(1, command_size);
	strcpy(new_node->command, command);
	return new_node;
}

void push(stack* s, node* new_node){
  if(s->header == NULL){
    new_node->next = NULL;
    s->header = new_node;
    s->top++;
  }
  else if(s->top == 20){
    node* temp = s->header;
    while(temp != NULL){
      temp = temp->next;
    }
    free(temp);
    s->top--;
    new_node->next = s->header;
    s->header = new_node;
    s->top++;
  }
  else{
    new_node->next = s->header;
    s->header = new_node;
    s->top++;
  }
}

char* search(stack* s, char* search_command){
  node* temp;
  if(s->header == NULL){
    printf("No history\n");
    return NULL;
  }
  else if(search_command[0] == '!'){
    temp = s->header;
    if(search_command[1] == '!'){
      temp = s->header;
    }
    else{
      int i;
      int iter = atoi(&search_command[1]);
      for(i=1; i<iter; i++){
        temp = temp->next;
      }
    }
  }
  return temp->command;
}

void print_stack(stack* s){
  node* temp = s->header;
  int i;
  for(i=0; i<s->top; i++){
    printf("%d: %s\n", (i+1), temp->command);
    temp = temp->next;
  }
}

#include "commands.h"
#include "error.h"
#include "inttypes.h"
#include "ctype.h"
#include <stdio.h>
#include <stdlib.h>
#include "addr_mng.h"


int program_read(const char* filename, program_t* program){

    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(program);

    //Initialize the program
    int init = program_init(program);
    M_REQUIRE(init == ERR_NONE, ERR_BAD_PARAMETER, "%s",ERR_MESSAGE[ERR_BAD_PARAMETER]);
    
    //Open the file
    FILE* open = fopen(filename, "r");
    //Check if the file is correctly open
    M_REQUIRE_NON_NULL_CUSTOM_ERR(open, ERR_IO);

    //Scan the first character before starting the while loop that analyse the program 
    char currentChar;
    fscanf(open, "%c", &currentChar);

    //initialize a command that we will add to the program
    command_t currentCommand;

    // While loop that analyse the program
    while(!feof(open) && !ferror(open)){
            
        //Read First Char 
        while(isspace(currentChar)){
            fscanf(open,"%c",&currentChar);
        }

//============================CHECK IF WE ARE IN A READ OR WRITE=================================
        if(currentChar != 'W' && currentChar != 'R') return ERR_BAD_PARAMETER;
        else currentCommand.order = (currentChar == 'W') ? WRITE : READ;

        //While there is space, scann next char
        fscanf(open, "%c", &currentChar);
        while(isspace(currentChar)){
            fscanf(open,"%c",&currentChar);
        }

//===========================GET THE INSTRUCTION AND THE DATA SIZE IF NECESSARY==================
        //If char is I, type is instrcution and there is no data so size = 0
        

        if(currentChar == 'I') {
            currentCommand.type = INSTRUCTION;
            currentCommand.data_size = 0;
        } else {
            //If char is D, type is DATA, add it to the command
            if(currentChar == 'D'){
                currentCommand.type = DATA;

                //Check if type of data is Word(size 4) or Byte and add it to the command
                fscanf(open, "%c", &currentChar);
                if(currentChar == 'W' || currentChar == 'B') {
                    currentCommand.data_size = (currentChar == 'B') ? 1 : sizeof(word_t);
                }
                else return ERR_BAD_PARAMETER;
            }
            else return ERR_BAD_PARAMETER;
        }

        //Scann until next char != space
        fscanf(open, "%c", &currentChar);
        while(isspace(currentChar)){
            fscanf(open,"%c",&currentChar);
        }

//=============================GET THE DATA TO WRITE IF WE NEED TO=============================
        if(currentCommand.order == WRITE){

            // Read the 0 and x
            //fscanf(open, "%c", &currentChar);
            if(currentChar != '0') return ERR_BAD_PARAMETER;
            fscanf(open, "%c", &currentChar);
            if(currentChar != 'x') return ERR_BAD_PARAMETER;
                
            //Read data depending to the size and add it ot the command
            if(currentCommand.data_size == 1){
                word_t data = 0;
                fscanf(open, "%"SCNx32, &data); //CHECK IF WE CAN SCANF WORDS
                currentCommand.write_data = data;
            }                
            else{
                word_t data = 0;
                fscanf(open, "%"SCNx32, &data);
                currentCommand.write_data = data;                
            }

            // Scan until next char is != space
            fscanf(open, "%c", &currentChar);
            while(isspace(currentChar)){
                fscanf(open,"%c",&currentChar);
            }
    } else  {
        currentCommand.write_data = 0;
    }

//===========================READ THE ADDRESS==============================================
        //Scan the @ 0 and x that preceeds the address value
        if(currentChar != '@') return ERR_BAD_PARAMETER;
        fscanf(open,"%c", &currentChar);
        if(currentChar != '0') return ERR_BAD_PARAMETER;
        fscanf(open,"%c", &currentChar);
        if(currentChar != 'x') return ERR_BAD_PARAMETER;

        //Scane the address and add it to the command
        uint64_t addr = 0;
        fscanf(open,"%"SCNx64, &addr); // CHECK IF WE CAN SCANF UINT64
        init_virt_addr64(&currentCommand.vaddr, addr);
        
//========================ADD THE COMMAND TO THE PROGRAMM ==================================
        //ADD COMMAND
        program_add_command(program, &currentCommand); 

        //Scan next char
        fscanf(open, "%c", &currentChar);

        //Read First Char 
        fscanf(open,"%c",&currentChar);
    }

    //Close the file correctly
    fclose(open);
    return ERR_NONE;

}


int program_init(program_t* program){

    // Check that the programm is non NULL
	M_REQUIRE_NON_NULL(program);
    
    // Initialize the programm with 0 lines, memory is allocated depending of listing, listing is calloc with 10 space
    program->nb_lines = 0;
    program->allocated = sizeof(program->listing);
    program->listing = calloc(10, sizeof(command_t));
    M_EXIT_IF_NULL(program->listing, 10);

    return ERR_NONE;
}


int program_print(FILE* file, const program_t* program){

    //check that the file and program are non nul
    M_REQUIRE_NON_NULL_CUSTOM_ERR(program, ERR_BAD_PARAMETER);
	M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_BAD_PARAMETER);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(program->listing, ERR_BAD_PARAMETER);

    
    for_all_lines(line, program) {
        //ORDER
        char order = (line->order == READ) ? 'R' : 'W';
        
        //TYPE
        char type = (line->type == INSTRUCTION) ? 'I' : 'D';
        
        //DATA_SIZE
        size_t size = line->data_size;
        char data_size = (size == 1) ? 'B' : 'W';

        //VADDR
        uint64_t vaddr = virt_addr_t_to_uint64_t(&line->vaddr);

        //WRITE_DATA
        int charErr = 0;

        //Depending of the type of command (read instrcution, write data etc...) print the appropriate thing
        if(type == 'I'){ 
            charErr = fprintf(file, "%c " "%c " "@0x%016"PRIX64 "\n",order, type, vaddr);
        }
        else if (order == 'R'){
            charErr = fprintf(file, "%c " "%c%c " "@0x%016"PRIX64 "\n",order, type, data_size, vaddr);
        }
        else if (order == 'W' && data_size == 'B'){
            charErr = fprintf(file, "%c " "%c" "%c " "0x%02"PRIX32 " @0x%016"PRIX64 "\n",order, type ,data_size,line->write_data, vaddr);
        } else charErr = fprintf(file, "%c " "%c" "%c " "0x%08"PRIX32 " @0x%016"PRIX64 "\n",order, type ,data_size,line->write_data, vaddr); 

        //Check that prnting has been corretly executed
        if(charErr < 0){
            return _IO_ERR_SEEN;
        } 
    }
	return ERR_NONE;
}

//We do some check directly in program_read
int program_add_command(program_t* program, const command_t* command){

    M_REQUIRE_NON_NULL(program);
    M_REQUIRE_NON_NULL(command);


    
    // Un test pour vérifier que order est bien READ ou WRITE
    if(command->order != WRITE && command->order != READ) {
        return ERR_BAD_PARAMETER;
    }

    // Un test pour vérifier que type est bien DATA ou INSTRUCTION
    if(command->type != INSTRUCTION && command->type != DATA) {
        return ERR_BAD_PARAMETER;
    }
    // Un test pour vérifier que data_size est bien 1 ou sizeof(word_t)
    if(command->data_size != 0 && command->data_size != 1 && (command->data_size % sizeof(word_t) != 0)){
        return ERR_BAD_PARAMETER;
    }
    // Un test pour vérifier qu'en cas de READ, write_data est bien 0
    if(command->order == READ && command->write_data != 0) {
        return ERR_BAD_PARAMETER;
    }

    //INSTRUCTIONS: We can only read an address (No write, no data <==> data size == 0)
    if (command->type == INSTRUCTION) {
        if(command->order == WRITE || command->data_size != 0){
                return ERR_BAD_PARAMETER;

        } 
    } 
    
    //DATA: check data size if valid for write and write
    else {
        if(command->order == WRITE || command->order == READ){
            if (command->data_size == 0) {
                return ERR_BAD_PARAMETER;
            }

            // Un test pour vérifier que write_data ne dépasse pas la taille maximale (0xff) lors d'un WRITE
            if (command->order == WRITE) {
                if(command->write_data > 0xff) {
                    return ERR_BAD_PARAMETER;
                }
            }
        }     
    }
    
    //Update progamm argument
    if (program->nb_lines == program->allocated) {
        program->allocated = 2*program->allocated;
        program->listing = realloc(program->listing, sizeof(command_t)*program->allocated);
        M_REQUIRE_NON_NULL(program->listing);
    }

    program->listing[program->nb_lines] = *command;
    program->nb_lines += 1; 

    return ERR_NONE;    
}

int program_shrink(program_t* program){
	
    M_REQUIRE_NON_NULL(program);

    if(program->nb_lines > 0) {
        // We reduce the space allocated to the space needed only <=> the number of line use
        program->allocated = program->nb_lines;
        program->listing = realloc(program->listing, program->nb_lines*sizeof(command_t));

        M_EXIT_IF_NULL(program->listing, program->lines*sizeof(command_t));
    } else {
        //If there was no line, initialize the program with basics value
        program->allocated = 10;
        program->listing = realloc(program->listing, program->allocated*sizeof(command_t));
        
        M_EXIT_IF_NULL(program->listing,10);
    }
    return ERR_NONE;
}


int program_free(program_t* program){
    // Nothing to comment.. Just free all argument of e programm
    if(program != NULL){
        free(program->listing);
        program->allocated=0;
        program->nb_lines = 0;
    }
    return 1;
}
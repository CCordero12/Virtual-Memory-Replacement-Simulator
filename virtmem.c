
#include <stdio.h>
#include <fcntl.h> 
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

//Initializing this outside a method so it can be used by the fault handling without it resetting each time.
//Alternate approach is having it as an input for fault handling and keeping track of it in main (done for lruMem, since theres no other option for that one)
int firstIn = 0;

//Method to print out the memory. Used for debugging
void memoryPrint(int memory[], int nframes){
    int i = 0;
    //Prints a message saying what it is and then opens brackets
    printf("Memory:\n[");
    //Iterates through the memory printing it out
    while(i < nframes){
        if(i == nframes -1){
            //once it gets to the last item, it prints it out without the comma and space following it
            printf("%d", memory[i]);
            break;
        }
        printf("%d, ", memory[i]);
        i++;
    }
    printf("]\n");
    
}

//Where the page replacement takes place. 
//Takes in the memory, how many frames are in the memory, the replacement Algorithm to be used, the page that needs to be added, and lruMem
//lruMem is only used if the replaceAlg is lru
//lruMem keeps track of when the page in each frame was last used, not in time but in terms of numbers of references since it was last used
int faultHandling(int memory[], int nframes, char* replaceAlg, int page, int lruMem[]){
    //Initialize frameUsed, a variable used to return which frame had its page replaced back to main
    int frameUsed = 0;

    if(strcmp(replaceAlg, "rand") == 0){
        //If the replacementAlg is rand, the frame is replaced randomly.
        frameUsed = rand()%nframes; 
        //sets the chosen frame to the value of the page
        memory[frameUsed] = page;
    }
    if(strcmp(replaceAlg, "fifo") == 0){
        //If its fifo, we just replace the next frame circularly
        frameUsed = firstIn;
        memory[frameUsed] = page;
        //Iterate to the next frame for next time
        firstIn ++;
        //If that puts it past the index of the final frame in memory, then reset it to 0
        if(firstIn >= nframes){
            firstIn = 0;
        }
    }
    if(strcmp(replaceAlg, "lru") == 0){    
        //If the replacement alg is lru, we replace the least recently used page
        //i is to iterate through the memory, max is to keep track of the highest # of references since a certain frame was used
        int i = 0;
        int max = 0;
        while(i < nframes){
            //Checks lruMem for each frame to see how long ago (in # of references) the page in that frame in memory was used
            if(lruMem[i] > max){
                //If its larger than the current max, that becomes the max and we set the frameUsed to that frame
                max = lruMem[i];
                frameUsed = i;
            }
            i++;
        }
        //After iterating through all of lruMem, we know the least recently used page, so we replace that one with the new page
        memory[frameUsed] = page;
    }

    //returns which frame we used (relevant to LRU)
    return frameUsed;
}

//Checks if each argument variable is a valid value
void isValid(int npages, int nframes, char* replaceAlg, int nrefs, char* locality){

    bool invalid = false;
    //If any of the arguments are invalid, itll print a message for each one then exit the program
    if(npages < 1){
        printf("virtmem: npages cannot be less than 1\n");
        invalid = true;
    }
    if(nframes < 1){
        printf("virtmem: nframes cannot be less than 1\n");
        invalid = true;
    }
    if(nrefs < 1){
        printf("virtmem: nrefs cannot be less than 1\n");
        invalid = true;
    }
    
    
    if(strcmp(replaceAlg, "rand") != 0 && strcmp(replaceAlg, "fifo") != 0 && strcmp(replaceAlg, "lru") != 0){
        printf("virtmem: replacement algorithm must be rand, fifo, or lru\n");
        invalid = true;
    }
    if(strcmp(locality, "ll") != 0 && strcmp(locality, "ml") != 0 && strcmp(locality, "hl") != 0){
        printf("virtmem: replacement algorithm must be rand, fifo, or lru\n");
        invalid = true;
    }
    
    
    if(npages > 1000){
        printf("virtmem: npages cannot be more than 1000");
        invalid = true;
    }
    if(nframes > 500){
        printf("virtmem: nframes cannot be more than 500");
        invalid = true;
    }
    if(nrefs > 5000){
        printf("virtmem: nrefs cannot be more than 5000");
        invalid = true;
    }

    //If anything is detected as invalid, it will exit.
    if(invalid){
        exit(1);
    }


    return;
}

//Argc is number of arguments, argv[] is an array of the different arguments as characters.
int main(int argc, char *argv[]){

    if(argc != 6){
        if(argc > 6){
            //More than 5 is too many
            printf("virtmem: too many arguments!\n");
        }
        if(argc < 6){
            //Less than 5 is too few
            printf("virtmem: too few arguments!\n");
        }
        //Give the user an example of proper usage, then exit with a failure
        printf("usage: virtmem <npages> <nframes> <rand|fifo|lru> <nrefs> <ll|ml|hl>\n");
        exit(1);
    }

    //Initializing all of the argument variables
    int npages = atoi(argv[1]);
    int nframes = atoi(argv[2]);
    char *replaceAlg = argv[3];
    int nrefs = atoi(argv[4]);
    char *locality = argv[5];
    //randRange is the range (above and below) the previous reference that the next reference can vary from
    float randrange = npages;
    int pageFaults = 0;
    int framesFull = 0;
    
    //Checks if all the arguments are valid
    isValid(npages, nframes, replaceAlg, nrefs, locality);

    //Creates the memory and lruMem variables, and initializes the prevPage variable to -1 so we know it hasnt been used yet, since thats an invalid page
    int memory[nframes];
    int lruMem[nframes];
    int prevpage = -1;

    //Uses i to iterate through the memory and lruMem and set each value to a default
    int i = 0;
    while(i < nframes){
        //Memory is set to -1 so we know that theres no pages in those frames by default since -1 is an invalid page
        memory[i] = -1;
        //lruMem starts at 1 instead of 0 so the first page call doesn't have to iterate through and increment all frames for the first page reference atleast.
        //Didn't think I would need this and i could leave it without setting a default, but without one the first couple lru replacements come out very wrong, with numbers in the thousands when there arent even 20 references for nrefs.
        lruMem[i] = 1;
        i++;
    }

    //Sets the random range for medium and high locality, 5% and 3% of npages respectively
    if(strcmp(locality, "ml") == 0){
        randrange = 0.05 * npages;
    }
    if(strcmp(locality, "hl") == 0){
        randrange = 0.03 * npages;
    }

    //If the random range is less than 1 (in cases where npages is small) round it up to 1, since we cant have decimals of pages
    if(randrange < 1){
        randrange = 1;
    }

    //Initializes the random seed for the program, since this is the first place it is used.
    //Randomly selects a starting page and sets the first frame to that page. lruMem updates its counter to 0 for the first frame.
    srand(time(NULL));
    prevpage = rand()%npages;
    memory[0] = prevpage;
    lruMem[0] = 0;
    //Increments the # of frames full and reduces the amount of remaining references 
    framesFull++;
    nrefs--;
    //memoryPrint(memory, nframes);

    //If theres more than 1 ref, we continue here until we're out
    while(nrefs > 0) {
        //minimum is the lower bound of the range for the next page that will be referenced, maximum is the upper bound
        //They are gotten by using where the previous page was minus - or plus + randRange respectively.
        int min = prevpage - randrange;
        int max = prevpage + randrange;
        //Keeps track of the frame used. Used for LRU
        int frameUsed = 0;

        //printf("min: %d, max: %d\n", min, max);

        //If the max is greater than the index for the last frame, set it to that number instead so it doesnt go out of bounds.
        //If the min is less than 0, set it to 0 so it doesn't go out of bounds
        if(max > (npages-1)){
            max = npages-1;
        }
        if(min < 0){
            min = 0;
        }
        
        //Gets the next page within the bounds of the minimum and maximum
        //Gets the remainder of the rand divided by the max - min + 1, then adds the minimum to it so its always within that range we want.
        prevpage = (rand() % (max - min + 1)) + min;
        //memoryPrint(memory, nframes);
        //printf("Page used: %d\n", prevpage);

        //Checks for a fault by iterating through memory
        //Defaults to true, and is made false if the page is found in memory
        i = 0;
        bool fault = true;
        while(i < nframes){
            if(memory[i] == prevpage){
                //If the page is found, we don't have a fault, we set the frame used to that frame, and then break so we don't keep looking
                fault = false;
                frameUsed = i;
                break;
            }
            i++;
        } 
        
        //If we have a fault
        if(fault){
            //If we still have empty frames, we use those and increment the # of full frames up
            if(framesFull < nframes){
                memory[framesFull] = prevpage;
                frameUsed = framesFull;
                framesFull++;
            //Otherwise we use our page replacement strategy in faultHandling
            }else{
                frameUsed = faultHandling(memory, nframes, replaceAlg, prevpage, lruMem);
            }
            //Then we add to the page fault count
            pageFaults++;
        }

        //Finally, iterates through lruMem and increments the counter on every frame by 1
        i = 0;
        while(i < nframes){
            lruMem[i]++;
            i++;
        }
        //Sets the recently used frame to 0 since it was used 0 references ago
        lruMem[frameUsed] = 0;

        //Increments nrefs down
        nrefs--;
    }

    

    printf("Total number of page faults: %d\n", pageFaults);
    printf("Number of empty frames: %d\n", (nframes - framesFull));

    exit(0);
}
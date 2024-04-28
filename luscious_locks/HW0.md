# Welcome to Homework 0!

For these questions you'll need the mini course and  "Linux-In-TheBrowser" virtual machine (yes it really does run in a web page on your machine using javascript!) at -

https://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away equal to 1?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions. For now just save them in a text document - we'll explain how to hand them later. 
They will be due in the second week of class.

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

https://cs-education.github.io/sys/

The coursebook:

https://cs341.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Ed: (you'll need to accept the sign up link I sent you)
https://edstem.org/

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document (which the course staff will grade later) because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".
```C
#include <unistd.h>
int main() {
	write(1, "Hi! My name is Dan Chen", 23);
	return 0;
}
```
### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```
   ```C
   void write_triangle(int n) {
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < i; j++) {
				write(2,"*", 1);
			}
			write(2,"\n", 1);
		}
	}
   ```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).
   ```C
    #include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	int main() {
		mode_t mode = S_IRUSR | S_IWUSR; //do we need this?
		int fd = open("message.txt", O_TRUNC | O_RDWR | O_CREAT, mode);
		write(fd, "Hello, World!", 13);
		close(fd);
		return 0;
	}
   ```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
   ```C
   	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	int main() {
		mode_t mode = S_IRUSR | S_IWUSR; 
		close(1);
		int fd = open("message.txt", O_TRUNC | O_RDWR | O_CREAT, mode); // fd right now is 1 due to the open() will take the lowest available number, which is 1 now after closing the 1.
		printf("Hello, World!\n", 13);  //therefore, the previous STDOUT becomes fd so that's why printf writes into the file, message.txt. 
		close(fd);
		return 0;
	}

   ```
5. What are some differences between `write()` and `printf()`?
	1. write() can be used with different file descriptor, but printf() can onlt be used with STDOUT.
	2. write() is unbuffered, so the data can be showed immediately. However, printf() is buffered, so we either have a "\n" after the string or the buffer is full.
	3. write() is a system call, but printf() is part of C library. 

## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte? 
- at least 8 bits.
2. How many bytes are there in a `char`?
- 1 byte 
3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`
   int - 4B
   double - 8B
   float - 4B
   long - 8B
   long long - 8B
### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?

0x7fbd9d48

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?
   *(data + 3) 

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
-The reason is "hello" is in text-segemnt which is read-only. Therefore, when we try to change it, we encounter segmentation fault.

7. What does `sizeof("Hello\0World")` return?
12
8. What does `strlen("Hello\0World")` return?
5
9. Give an example of X such that `sizeof(X)` is 3.
X = "ab"
10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.
Y = int

## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?
`1.argc    2. iterating through the argv as it is an array until we encounter NULL pointer.` 
2. What does `argv[0]` represent?
`argv[0] represents the first element in argv which is generally we execute the program.` 
### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?
`-It is stored with the command-line, above the stack.`
`-It is in process's own memory (in the shell);` 

### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C 
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?
`sizeof(ptr)` -> 4
`sizeof(array)` -> 6
### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?
`stack data structure`

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?
`I should allocate it in a space where the data persisted after the function call`
2. What are the differences between heap and stack memory? 
`Stack: 1. when a function is called, a block of memory (stack frame) is reserved on the stack. When we return from that function, the memory will automatically free. `
`Heap: 1. It is generally controled by the programmer. When we try to allocate on the heap, we use malloc() (or new), and we use free() (or delete) to deallocate that piece of memory`
3. Are there other kinds of memory in a process?
`text-segement, data memory, register memory`
4. Fill in the blank: "In a good C program, for every malloc, there is a ___".
`free`
### Heap allocation gotchas
5. What is one reason `malloc` can fail?
`when there is no enough memory space to allocate the bytes we want. (Insufficient memory)`
6. What are some differences between `time()` and `ctime()`? 
`time() can give the current time that system considers. ctime() is used to convert the time() to human-readable string. Also, ctime() has static storage, which means everytime we call ctime(), it will overrite the buffer, but time() does not have`
7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```
`double-freed`
8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```
`Dangling pointer. We should not use it as we just freed that memory.`
9. How can one avoid the previous two mistakes? 
`So every time we free a pointer, we set it to NULL (i.e ptr = NULL). The reason is when we try to free a NULL ptr, it is a harmless operation, and setting it to NULL prevents dangling pointer.`
### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).
```C

struct Person {
	char* name;
	int age;
	struct Person** friends;
}

typedef struct Person person_t
```
11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.
```C
int main() {
	person_t* as = (person_t*) malloc(sizeof(person_t));
	person_t* sm = (person_t*) malloc(sizeof(person_t));
	as->name = "Agent Smith";
	as->age = 128;
	as->friends = (person_t**) malloc(sizeof(person_t*));
	as->friends[0] = sm;
	sm->name = "Sonny Moore";
	sm->age = 256;
	sm->friends = (person_t**) malloc(sizeof(person_t*));
	sm->friends[0] = as;

	free(as->friends);
	free(sm->friends);
	free(as);
	free(sm);
	return 0;
}
```

### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).
12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).
```C
person_t* create(char* aname, int aage) {
	person_t* person = (person_t*) malloc(sizeof(person_t));
	if (person == NULL) {
		return NULL;
	}
	person->name = strdup(aname);
	person->age = age;
	person->friends = (person_t**) malloc(sizeof(person_t*) * 10);
	if (person->friends == NULL) {
		free(person->name);
		free(person);
		return NULL;
	}
	for (int i = 0; i < 10; i++) {
		person->friends[i] = NULL;
	}
	return person;
}
```
`If we do not initialize these fields in the array, it can cause dangling pointers or other undefined behaviors.`
13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.
```C
void destroy(person_t* p) {
	if (p == NULL) {
		return;
	}
	free(p->name);
	free(p->friends);
	memset(p, 0, sizeof(person_t));
	free(p);

}
```


## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?
`getchar(), putchar()`
2. Name one issue with `gets()`.
`it can cause buffer overflow`
### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".
```C
char h[6];
int five;
char w[6];
sscanf("Hello 5 World", "%s %d, %s", h, &five, w);
```
### `getline` is useful
4. What does one need to define before including `getline()`?
`#define _GNU_SOURCE_`
5. Write a C program to print out the content of a file line-by-line using `getline()`.
```C
int main() {
	FILE *file = fopen("myfile.txt", "r");
	char* line[] = NULL;
	size_t capacity = 0;
	ssize_t result; //number of characters that getline retrieved.
	while ((result = getline(&line, &capacity, stdin)) != -1) {
		printf("%s", buffer);
	}
	free(line);
	fclose(file);
	return 0;
}
```


## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?
`-g`
2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.
`The reason is make does not recompile the files unless it detects that their corresponding source files have been modified. It would be better to first `make clean` then make it again`
3. Are tabs or spaces used to indent the commands after the rule in a Makefile?
`tabs`
4. What does `git commit` do? What's a `sha` in the context of git?
`git commit is to save changes in local repository and creats a new snapshot of the current state of the project with the staged modifications. `
`A "sha" in Git is a hash (unique identifier (a SHA-1 hash)) for commits and other objects in the repository, used to track and reference them.`
5. What does `git log` show you?
`It shows a chronological list of commits in a Git repository. It displays information about each commit.`
6. What does `git status` tell you and how would the contents of `.gitignore` change its output?
` `git status` tells me the state of the working directory and the staging area. It also tells you untracked files, modified files, and branch information, etc`
`.gitignore can affect the outout of "git status" by excluding certain files or directroies from being displayed as untracked`
7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?
` "git push" uploads changes from your local repository to a remote repository. It sends the commits in the local branch to the corresponding branch on the remote repository, keeping them in sync.`
`The reason that commit message is insufficient is because it does not include what bugs that you fixed or any other useful information.`
8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?
`the "non-fast-forward error" is that the local branch has diverged from remote branch, so GIT can not make a simple fast-forward merge. This occurs when others have made conflicting changes to the remote branch. `
`The way to resolve it is to fetch the latest changes from the remote repository using `git fetch` then merge or rebase your local changes on top of the updated remote branch`

## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Ed.
- Find, in your opinion, the best and worst C code on the web and post the link to Ed.
- Write a short C program with a deliberate subtle C bug and post it on Ed to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on Ed.

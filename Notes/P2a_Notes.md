# P2a Notes

#### segmentation fault with `strcpy`

```C
char * child_argv[2];
char * token;

getline(&line, &line_size, stdin);
token = strtok(line, " \n");
strcpy(child_argv[0], token);
```

This piece of code will cause segmentation fault. This is because `*child_argv` hasn't been initialized, should first allocate memory for it. Therefore, either use `malloc` to allocate memory first or use `strdup`. Either case `free()` is required.

#### print a pointer

Use `%p` to print a pointer. 

>  **p** The argument shall be a pointer to **void**. The value of the pointer is converted to a sequence of printing characters, in an implementation-defined manner.

Don't forget the cast, otherwise is undefined behavior.

```c
printf("%p\n",(void*)&a);
```

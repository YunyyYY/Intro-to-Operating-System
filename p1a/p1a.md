# p1a

This file is project 1a for CS537 Spring 2019. It contains 3 source codes for a set of simplified linux utilities called my-cat, my-sed and my-uniq.

#### my-cat

The source file for this command is my-cat.c. It accepts files that it will read and print to standard output. If no file specified, it exits automatically. To use it, input: 

```shell
> ./my-cat [file1] [file2] ...
```



#### my-sed

The source file for this command is my-sed.c. It searches a string in every input files and repalces the first target in each line with another given string.  If no file provided, it reads from the standard input. To use it, input: 

```shell
> ./my-sed <find-string> <replace-string> [file1] ...
> ./my-sed <find-string> <replace-string>
```

#### my-uniq

The source file for this command is my-uniq.c. It finds out adjacent duplicate lines in every file, ignore the duplicate ones and prints the rest to the standard output. To use it, input:

```shell
> ./my-uniq [file1] ...
```


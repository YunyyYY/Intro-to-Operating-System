# P1a 

#### meaning of `./excutable`

`./config` means you're calling something in the current working directory. In this case config is an executable. You have to specify the path for executables if they're **outside your $PATH variable** and that's why `config` isn't enough.

`../config` would be used if the `config` executable were in the parent of the current working directory.

In UNIX systems, it is traditional to return 0 

upon success, and non-zero upon failure. Here, we will use 1 to indicate failure.

#### strcpy（）

cannot directly copy a c-string to another c-string, which will copy address.

```C
//file_name = argv[file_num];
strcpy(file_name, argv[file_num]);
```

#### cat 

<code>cat /usr/share/dict/words | less</code>

#### vim

<code>set nu</code> 显示行号

#### <code>fclose()</code>

cannot passing <code>NULL</code> to <code>fclose()</code>.

#### <code>getline</code> 

In C, the function is defined as:

<code>ssize_t getline(char ** restrict linep, size_t * restrict linecapp, FILE * restrict stream);</code>

the second parameter is <code>size_t *</code>, so cannot pass <code>const size_t *</code> to it.

#### <code>strcmp()</code>

<code>strcmp(foo, NULL)</code> will result in undefined behavior. As states in C11, 

> If an argument to a function has an invalid value (such as [...] a null pointer [...]) [...], the behavior is **undefined**.

#### <code>fgets()</code> and <code>getline()</code>

```C
ssize_t getline(char ** restrict linep, size_t * restrict linecapp, 
                FILE * restrict stream);
char * fgets(char * restrict str, int size, FILE * restrict stream)
```

<code>getline()</code> accepts a <code>size_t</code>, which is essentially unsigned long, but <code>fgets()</code> accepts <code>int</code>. Therefore the maximum they can hold is different.